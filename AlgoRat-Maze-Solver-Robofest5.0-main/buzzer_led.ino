// merged_buzzer_led.ino
// Merged buzzer + LED breathing. Buzzer plays melodies asynchronously (non-blocking).
// Provides wrappers start_bl(), finish_bl(), explored_bl() so main.ino can just call one of them.
// On Teensy boards this uses IntervalTimer to run buzzer+LED in the background. On other boards
// you must call buzzer_led_loop() inside loop() (fallback).

#include <Arduino.h>

// ------------------ NOTE DEFINITIONS (unchanged) ------------------
#define NOTE_B0  31
#define NOTE_C1  33
#define NOTE_CS1 35
#define NOTE_D1  37
#define NOTE_DS1 39
#define NOTE_E1  41
#define NOTE_F1  44
#define NOTE_FS1 46
#define NOTE_G1  49
#define NOTE_GS1 52
#define NOTE_A1  55
#define NOTE_AS1 58
#define NOTE_B1  62
#define NOTE_C2  65
#define NOTE_CS2 69
#define NOTE_D2  73
#define NOTE_DS2 78
#define NOTE_E2  82
#define NOTE_F2  87
#define NOTE_FS2 93
#define NOTE_G2  98
#define NOTE_GS2 104
#define NOTE_A2  110
#define NOTE_AS2 117
#define NOTE_B2  123
#define NOTE_C3  131
#define NOTE_CS3 139
#define NOTE_D3  147
#define NOTE_DS3 156
#define NOTE_E3  165
#define NOTE_F3  175
#define NOTE_FS3 185
#define NOTE_G3  196
#define NOTE_GS3 208
#define NOTE_A3  220
#define NOTE_AS3 233
#define NOTE_B3  247
#define NOTE_C4  262
#define NOTE_CS4 277
#define NOTE_D4  294
#define NOTE_DS4 311
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_FS4 370
#define NOTE_G4  392
#define NOTE_GS4 415
#define NOTE_A4  440
#define NOTE_AS4 466
#define NOTE_B4  494
#define NOTE_C5  523
#define NOTE_CS5 554
#define NOTE_D5  587
#define NOTE_DS5 622
#define NOTE_E5  659
#define NOTE_F5  698
#define NOTE_FS5 740
#define NOTE_G5  784
#define NOTE_GS5 831
#define NOTE_A5  880
#define NOTE_AS5 932
#define NOTE_B5  988
#define NOTE_C6  1047
#define NOTE_CS6 1109
#define NOTE_D6  1175
#define NOTE_DS6 1245
#define NOTE_E6  1319
#define NOTE_F6  1397
#define NOTE_FS6 1480
#define NOTE_G6  1568
#define NOTE_GS6 1661
#define NOTE_A6  1760
#define NOTE_AS6 1865
#define NOTE_B6  1976
#define NOTE_C7  2093
#define NOTE_CS7 2217
#define NOTE_D7  2349
#define NOTE_DS7 2489
#define NOTE_E7  2637
#define NOTE_F7  2794
#define NOTE_FS7 2960
#define NOTE_G7  3136
#define NOTE_GS7 3322
#define NOTE_A7  3520
#define NOTE_AS7 3729
#define NOTE_B7  3951
#define NOTE_C8  4186
#define NOTE_CS8 4435
#define NOTE_D8  4699
#define NOTE_DS8 4978
#define REST      0

// ------------------ BUZZER (non-blocking) ------------------
// default buzzer pin (can be overridden with initBuzzer)
static int _buzzerPin = 33;

// buzzer state machine
static const int* _currentMelody = nullptr;
static int _currentMelodyLen = 0;
static int _currentTempo = 120;
static int _buzzerNotes = 0;
static int _buzzerIndex = 0; // index into melody array (0,2,4,...)
static unsigned long _noteStartMillis = 0;
static unsigned long _noteDurationMs = 0;
static unsigned long _noteEndMillis = 0;
static bool _buzzerPlaying = false;
static unsigned long _buzzerWholenote = 0;

// helper to start any melody asynchronously
void _buzzerStartMelody(const int *melody, int length, int tempo) {
  _currentMelody = melody;
  _currentMelodyLen = length;
  _currentTempo = tempo;
  _buzzerNotes = length / 2;
  _buzzerIndex = 0;
  _noteStartMillis = 0;
  _noteDurationMs = 0;
  _noteEndMillis = 0;
  _buzzerPlaying = true;
  _buzzerWholenote = (60000UL * 4UL) / max(1, _currentTempo);
}

// call from loop() (or ISR) to progress buzzer playback
void buzzer_update() {
  if (!_buzzerPlaying || _currentMelody == nullptr) return;

  unsigned long now = millis();

  // starting first note
  if (_noteStartMillis == 0) {
    // initialize first note
    _noteStartMillis = now;
    int note = _currentMelody[_buzzerIndex];
    int divider = _currentMelody[_buzzerIndex + 1];

    if (divider > 0) {
      _noteDurationMs = _buzzerWholenote / divider;
    } else if (divider < 0) {
      _noteDurationMs = _buzzerWholenote / abs(divider);
      _noteDurationMs = (_noteDurationMs * 3UL) / 2UL; // dotted
    } else {
      _noteDurationMs = _buzzerWholenote / 4; // fallback quarter
    }

    // play or rest
    if (note == REST) {
      // no tone; just wait
    } else {
      // play tone for 90% of note duration (like previous blocking behavior)
      unsigned long toneDur = (_noteDurationMs * 9UL) / 10UL;
      tone(_buzzerPin, note, (unsigned int)toneDur);
    }

    _noteEndMillis = _noteStartMillis + _noteDurationMs;
    return;
  }

  // check if current note finished
  if (now >= _noteEndMillis) {
    // ensure tone stopped (in case duration was slightly different)
    noTone(_buzzerPin);

    // advance to next note
    _buzzerIndex += 2;
    if ((_buzzerIndex / 2) >= _buzzerNotes) {
      // finished melody
      _buzzerPlaying = false;
      _currentMelody = nullptr;
      _currentMelodyLen = 0;
      _noteStartMillis = 0;
      _noteDurationMs = 0;
      _noteEndMillis = 0;
      return;
    }

    // start next note immediately
    _noteStartMillis = now;
    int note = _currentMelody[_buzzerIndex];
    int divider = _currentMelody[_buzzerIndex + 1];

    if (divider > 0) {
      _noteDurationMs = _buzzerWholenote / divider;
    } else if (divider < 0) {
      _noteDurationMs = _buzzerWholenote / abs(divider);
      _noteDurationMs = (_noteDurationMs * 3UL) / 2UL; // dotted
    } else {
      _noteDurationMs = _buzzerWholenote / 4; // fallback quarter
    }

    if (note == REST) {
      // nothing to call
    } else {
      unsigned long toneDur = (_noteDurationMs * 9UL) / 10UL;
      tone(_buzzerPin, note, (unsigned int)toneDur);
    }

    _noteEndMillis = _noteStartMillis + _noteDurationMs;
  }
}

// stop playback
void stop_buzzer() {
  _buzzerPlaying = false;
  _currentMelody = nullptr;
  noTone(_buzzerPin);
  _noteStartMillis = 0;
  _noteDurationMs = 0;
  _noteEndMillis = 0;
}

// init buzzer pin (keeps compatibility)
void initBuzzer(int pin) {
  _buzzerPin = pin;
  pinMode(_buzzerPin, OUTPUT);
}

// ---- Mario melody (exactly as provided) ----
const int _marioTempo = 200;
const int _marioMelody[] = {
  // full Mario melody preserved
  NOTE_E5,8, NOTE_E5,8, REST,8, NOTE_E5,8, REST,8, NOTE_C5,8, NOTE_E5,8,
  NOTE_G5,4, REST,4, NOTE_G4,8, REST,4,
  NOTE_C5,-4, NOTE_G4,8, REST,4, NOTE_E4,-4,
  NOTE_A4,4, NOTE_B4,4, NOTE_AS4,8, NOTE_A4,4,
  NOTE_G4,-8, NOTE_E5,-8, NOTE_G5,-8, NOTE_A5,4, NOTE_F5,8, NOTE_G5,8,
  REST,8, NOTE_E5,4,NOTE_C5,8, NOTE_D5,8, NOTE_B4,-4,
  NOTE_C5,-4, NOTE_G4,8, REST,4, NOTE_E4,-4,
  NOTE_A4,4, NOTE_B4,4, NOTE_AS4,8, NOTE_A4,4,
  NOTE_G4,-8, NOTE_E5,-8, NOTE_G5,-8, NOTE_A5,4, NOTE_F5,8, NOTE_G5,8,
  REST,8, NOTE_E5,4,NOTE_C5,8, NOTE_D5,8, NOTE_B4,-4,
  REST,4, NOTE_G5,8, NOTE_FS5,8, NOTE_F5,8, NOTE_DS5,4, NOTE_E5,8,
  REST,8, NOTE_GS4,8, NOTE_A4,8, NOTE_C4,8, REST,8, NOTE_A4,8, NOTE_C5,8, NOTE_D5,8,
  REST,4, NOTE_DS5,4, REST,8, NOTE_D5,-4,
  NOTE_C5,2, REST,2,
  REST,4, NOTE_G5,8, NOTE_FS5,8, NOTE_F5,8, NOTE_DS5,4, NOTE_E5,8,
  REST,8, NOTE_GS4,8, NOTE_A4,8, NOTE_C4,8, REST,8, NOTE_A4,8, NOTE_C5,8, NOTE_D5,8,
  REST,4, NOTE_DS5,4, REST,8, NOTE_D5,-4,
  NOTE_C5,2, REST,2,
  NOTE_C5,8, NOTE_C5,4, NOTE_C5,8, REST,8, NOTE_C5,8, NOTE_D5,4,
  NOTE_E5,8, NOTE_C5,4, NOTE_A4,8, NOTE_G4,2,
  NOTE_C5,8, NOTE_C5,4, NOTE_C5,8, REST,8, NOTE_C5,8, NOTE_D5,8, NOTE_E5,8,
  REST,1,
  NOTE_C5,8, NOTE_C5,4, NOTE_C5,8, REST,8, NOTE_C5,8, NOTE_D5,4,
  NOTE_E5,8, NOTE_C5,4, NOTE_A4,8, NOTE_G4,2,
  NOTE_E5,8, NOTE_E5,8, REST,8, NOTE_E5,8, REST,8, NOTE_C5,8, NOTE_E5,4,
  NOTE_G5,4, REST,4, NOTE_G4,4, REST,4,
  NOTE_C5,-4, NOTE_G4,8, REST,4, NOTE_E4,-4,
  NOTE_A4,4, NOTE_B4,4, NOTE_AS4,8, NOTE_A4,4,
  NOTE_G4,-8, NOTE_E5,-8, NOTE_G5,-8, NOTE_A5,4, NOTE_F5,8, NOTE_G5,8,
  REST,8, NOTE_E5,4, NOTE_C5,8, NOTE_D5,8, NOTE_B4,-4,
  NOTE_C5,-4, NOTE_G4,8, REST,4, NOTE_E4,-4,
  NOTE_A4,4, NOTE_B4,4, NOTE_AS4,8, NOTE_A4,4,
  NOTE_G4,-8, NOTE_E5,-8, NOTE_G5,-8, NOTE_A5,4, NOTE_F5,8, NOTE_G5,8,
  REST,8, NOTE_E5,4, NOTE_C5,8, NOTE_D5,8, NOTE_B4,-4,
  NOTE_E5,8, NOTE_C5,4, NOTE_G4,8, REST,4, NOTE_GS4,4,
  NOTE_A4,8, NOTE_F5,4, NOTE_F5,8, NOTE_A4,2,
  NOTE_D5,-8, NOTE_A5,-8, NOTE_A5,-8, NOTE_A5,-8, NOTE_G5,-8, NOTE_F5,-8,
  NOTE_E5,8, NOTE_C5,4, NOTE_A4,8, NOTE_G4,2,
  NOTE_E5,8, NOTE_C5,4, NOTE_G4,8, REST,4, NOTE_GS4,4,
  NOTE_A4,8, NOTE_F5,4, NOTE_F5,8, NOTE_A4,2,
  NOTE_B4,8, NOTE_F5,4, NOTE_F5,8, NOTE_F5,-8, NOTE_E5,-8, NOTE_D5,-8,
  NOTE_C5,8, NOTE_E4,4, NOTE_E4,8, NOTE_C4,2,
  NOTE_E5,8, NOTE_C5,4, NOTE_G4,8, REST,4, NOTE_GS4,4,
  NOTE_A4,8, NOTE_F5,4, NOTE_F5,8, NOTE_A4,2,
  NOTE_D5,-8, NOTE_A5,-8, NOTE_A5,-8, NOTE_A5,-8, NOTE_G5,-8, NOTE_F5,-8,
  NOTE_E5,8, NOTE_C5,4, NOTE_A4,8, NOTE_G4,2,
  NOTE_E5,8, NOTE_C5,4, NOTE_G4,8, REST,4, NOTE_GS4,4,
  NOTE_A4,8, NOTE_F5,4, NOTE_F5,8, NOTE_A4,2,
  NOTE_B4,8, NOTE_F5,4, NOTE_F5,8, NOTE_F5,-8, NOTE_E5,-8, NOTE_D5,-8,
  NOTE_C5,8, NOTE_E4,4, NOTE_E4,8, NOTE_C4,2,
};

// wrapper to start Mario asynchronously
void start_buzzer() {
  _buzzerStartMelody(_marioMelody, sizeof(_marioMelody) / sizeof(_marioMelody[0]), _marioTempo);
}

// ---- Star Wars melody ----
const int _starWarsTempo = 108;
const int _starWarsMelody[] = {
  NOTE_AS4,8, NOTE_AS4,8, NOTE_AS4,8,
  NOTE_F5,2, NOTE_C6,2,
  NOTE_AS5,8, NOTE_A5,8, NOTE_G5,8, NOTE_F6,2, NOTE_C6,4,
  NOTE_AS5,8, NOTE_A5,8, NOTE_G5,8, NOTE_F6,2, NOTE_C6,4,
  NOTE_AS5,8, NOTE_A5,8, NOTE_AS5,8, NOTE_G5,2, NOTE_C5,8, NOTE_C5,8, NOTE_C5,8,
  NOTE_F5,2, NOTE_C6,2,
  NOTE_AS5,8, NOTE_A5,8, NOTE_G5,8, NOTE_F6,2, NOTE_C6,4,
  NOTE_AS5,8, NOTE_A5,8, NOTE_G5,8, NOTE_F6,2, NOTE_C6,4,
  NOTE_AS5,8, NOTE_A5,8, NOTE_AS5,8, NOTE_G5,2, NOTE_C5,-8, NOTE_C5,16,
  NOTE_D5,-4, NOTE_D5,8, NOTE_AS5,8, NOTE_A5,8, NOTE_G5,8, NOTE_F5,8,
  NOTE_F5,8, NOTE_G5,8, NOTE_A5,8, NOTE_G5,4, NOTE_D5,8, NOTE_E5,4,NOTE_C5,-8, NOTE_C5,16,
  NOTE_D5,-4, NOTE_D5,8, NOTE_AS5,8, NOTE_A5,8, NOTE_G5,8, NOTE_F5,8,
  NOTE_C6,-8, NOTE_G5,16, NOTE_G5,2, REST,8, NOTE_C5,8,
  NOTE_D5,-4, NOTE_D5,8, NOTE_AS5,8, NOTE_A5,8, NOTE_G5,8, NOTE_F5,8,
  NOTE_F5,8, NOTE_G5,8, NOTE_A5,8, NOTE_G5,4, NOTE_D5,8, NOTE_E5,4,NOTE_C6,-8, NOTE_C6,16,
  NOTE_F6,4, NOTE_DS6,8, NOTE_CS6,4, NOTE_C6,8, NOTE_AS5,4, NOTE_GS5,8, NOTE_G5,4, NOTE_F5,8,
  NOTE_C6,1
};
void finish_buzzer() {
  _buzzerStartMelody(_starWarsMelody, sizeof(_starWarsMelody) / sizeof(_starWarsMelody[0]), _starWarsTempo);
}

// ---- Rickroll melody ----
const int _rickTempo = 114;
const int _rickMelody[] = {
  // full original array preserved
  NOTE_C5,-4, NOTE_G4,-4, NOTE_E4,4,
  NOTE_A4,-8, NOTE_B4,-8, NOTE_A4,-8, NOTE_GS4,-8, NOTE_AS4,-8, NOTE_GS4,-8,
  NOTE_G4,8, NOTE_D4,8, NOTE_E4,-2
};
void explored_buzzer() {
  _buzzerStartMelody(_rickMelody, sizeof(_rickMelody) / sizeof(_rickMelody[0]), _rickTempo);
}

// ------------------ LED BREATHING CODE ------------------
// Non-blocking breathing + smoother color cycling + never fully off between breaths.
// Call start_led() each loop to run breathing; stop_finish_led() requests soft mode.

int redPin   = 14;
int greenPin = 13;
int bluePin  = 15;

void setColor(int r, int g, int b) {
  if (r < 0) r = 0; if (r > 255) r = 255;
  if (g < 0) g = 0; if (g > 255) g = 255;
  if (b < 0) b = 0; if (b > 255) b = 255;
  analogWrite(redPin,   r);
  analogWrite(greenPin, g);
  analogWrite(bluePin,  b);
}

unsigned long breathPeriod = 2000;    // ms for full in+out cycle
float gammaCorrection = 2.2;

volatile uint8_t baseR = 255;
volatile uint8_t baseG = 80;
volatile uint8_t baseB = 40;

bool cycleColors = true;
unsigned int cyclesPerColor = 2;

const uint8_t palette[][3] = {
  {255,  30,  30},
  {255,  60,  10},
  {255, 180,   0},
  {255, 255,   0},
  {180, 255,   0},
  {  0, 255,  60},
  {  0, 255, 180},
  {  0, 220, 255},
  { 20, 120, 255},
  { 70,  80, 255},
  {140,  60, 255},
  {255,  40, 255},
  {255,  80, 180},
};
const int paletteCount = sizeof(palette) / sizeof(palette[0]);

uint8_t minLevel = 20;

static bool _ledInitialized = false;
static unsigned long _startMillis = 0;
static bool _active = false;
static unsigned int _breathCycleCount = 0;
static int _paletteIndex = 0;
static bool _stopRequested = false;
static bool _inSoftDown = false;
static bool _softMode = false;
static unsigned long _softStartMillis = 0;
const float MY_PI = 3.14159265358979323846;

void _ledInitIfNeeded() {
  if (_ledInitialized) return;
  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(bluePin, OUTPUT);
  setColor(0,0,0);
  _ledInitialized = true;
}

void set_finish_color(uint8_t r, uint8_t g, uint8_t b) {
  baseR = r; baseG = g; baseB = b;
  cycleColors = false;
}

void enable_palette_cycle(bool enable) {
  cycleColors = enable;
  if (enable) {
    baseR = palette[_paletteIndex][0];
    baseG = palette[_paletteIndex][1];
    baseB = palette[_paletteIndex][2];
  }
}

void set_breath_period(unsigned long ms) {
  if (ms < 200) ms = 200;
  breathPeriod = ms;
}

void set_min_level(uint8_t lvl) {
  if (lvl > 200) lvl = 200;
  minLevel = lvl;
}

void start_led() {
  _ledInitIfNeeded();

  if (_stopRequested && !_inSoftDown && _softMode) {
    return;
  }

  if (!_active) {
    _active = true;
    _startMillis = millis();
    _breathCycleCount = 0;
    _paletteIndex = 0;
    _stopRequested = false;
    _inSoftDown = false;
    _softMode = false;
    if (cycleColors) {
      baseR = palette[_paletteIndex][0];
      baseG = palette[_paletteIndex][1];
      baseB = palette[_paletteIndex][2];
    }
  }

  if (_inSoftDown) {
    unsigned long now = millis();
    unsigned long softElapsed = now - _softStartMillis;
    if (softElapsed >= breathPeriod) {
      _inSoftDown = false;
      _softMode = true;
      _startMillis = now;
      return;
    }
    unsigned long elapsed = softElapsed % breathPeriod;
    float phase = (float)elapsed / (float)breathPeriod;
    float raw = sin(phase * 2.0 * MY_PI - (MY_PI / 2.0));
    float norm = (raw + 1.0) * 0.5;
    float corrected = pow(norm, gammaCorrection);
    float progress = (float)softElapsed / (float)breathPeriod;
    float minFrac = (float)minLevel / 255.0;
    float amplitudeScale = 1.0 - (progress * (1.0 - minFrac));
    float scaled = minFrac + (corrected * amplitudeScale * (1.0 - minFrac));
    uint8_t r = (uint8_t)(baseR * scaled + 0.5);
    uint8_t g = (uint8_t)(baseG * scaled + 0.5);
    uint8_t b = (uint8_t)(baseB * scaled + 0.5);
    setColor(r,g,b);
    return;
  }

  unsigned long now = millis();
  unsigned long elapsedSinceStart = now - _startMillis;
  unsigned long elapsed = elapsedSinceStart % breathPeriod;
  float phase = (float)elapsed / (float)breathPeriod;
  float raw = sin(phase * 2.0 * MY_PI - (MY_PI / 2.0));
  float norm = (raw + 1.0) * 0.5;
  float corrected = pow(norm, gammaCorrection);
  float minFrac = (float)minLevel / 255.0;

  if (_softMode) {
    float softScale = 0.5;
    float scaled = minFrac + (corrected * softScale * (1.0 - minFrac));
    uint8_t r = (uint8_t)(baseR * scaled + 0.5);
    uint8_t g = (uint8_t)(baseG * scaled + 0.5);
    uint8_t b = (uint8_t)(baseB * scaled + 0.5);
    setColor(r,g,b);
  } else {
    float scaled = minFrac + (corrected * (1.0 - minFrac));
    uint8_t r = (uint8_t)(baseR * scaled + 0.5);
    uint8_t g = (uint8_t)(baseG * scaled + 0.5);
    uint8_t b = (uint8_t)(baseB * scaled + 0.5);
    setColor(r,g,b);
  }

  static int lastCyclePhase = -1;
  int cyclePhase = (elapsed == 0);
  if (cyclePhase && !lastCyclePhase) {
    _breathCycleCount++;
    if (cycleColors && _breathCycleCount >= cyclesPerColor) {
      _breathCycleCount = 0;
      _paletteIndex = (_paletteIndex + 1) % paletteCount;
      baseR = palette[_paletteIndex][0];
      baseG = palette[_paletteIndex][1];
      baseB = palette[_paletteIndex][2];
    }
  }
  lastCyclePhase = cyclePhase;
}

void stop_finish_led() {
  if (!_active) return;
  if (_inSoftDown || _softMode) return;
  _stopRequested = true;
  _inSoftDown = true;
  _softStartMillis = millis();
  cycleColors = false;
}

void resume_finish_led() {
  if (!_active) {
    start_led();
    return;
  }
  _softMode = false;
  _inSoftDown = false;
  _stopRequested = false;
  _startMillis = millis();
}

void toggle_finish_led() {
  if (!_active) start_led();
  else if (_softMode || _inSoftDown) resume_finish_led();
  else stop_finish_led();
}

// Optional compatibility helpers (you can still call these)
void buzzer_led_setup() {
  initBuzzer(_buzzerPin);
  _ledInitIfNeeded();
  _active = false;
}

// Should be called every loop if no background runner present (non-Teensy)
void buzzer_led_loop() {
  start_led();
  buzzer_update();
  delay(1);
}

// ----------------- Background runner (Teensy IntervalTimer) -----------------
#if defined(__IMXRT1062__) || defined(KINETISK) || defined(__MK20DX256__)
  // Teensy cores: include IntervalTimer (comes with Teensyduino)
  #include <IntervalTimer.h>
  static IntervalTimer _buzzerLedTimer;
  static bool _timerRunning = false;
#endif

const unsigned long _buzzer_led_tick_us = 2000UL; // 2 ms tick (tune as needed)

// ISR tick (runs in interrupt context when timer active)
static void _buzzer_led_tick_isr() {
  // Keep this fairly small. On Teensy, this is usually OK at a few ms interval.
  buzzer_update();
  start_led();
}

static void _start_background_runner() {
#if defined(__IMXRT1062__) || defined(KINETISK) || defined(__MK20DX256__)
  if (!_timerRunning) {
    _buzzerLedTimer.begin(_buzzer_led_tick_isr, _buzzer_led_tick_us);
    _timerRunning = true;
  }
#else
  // Non-Teensy: no IntervalTimer. You must call buzzer_led_loop() in loop().
#endif
}

static void _stop_background_runner() {
#if defined(__IMXRT1062__) || defined(KINETISK) || defined(__MK20DX256__)
  if (_timerRunning) {
    _buzzerLedTimer.end();
    _timerRunning = false;
  }
#endif
}

// --- Convenience API for main.ino ---
// These are the functions you asked for: call only start_bl() in main to start Mario + LED.
// finish_bl() and explored_bl() do Star Wars and Rickroll respectively.

void start_bl() {
  initBuzzer(_buzzerPin);
  _ledInitIfNeeded();
  _active = true;
  _startMillis = millis();
  _start_background_runner();
  start_buzzer();
}

void finish_bl() {
  initBuzzer(_buzzerPin);
  _ledInitIfNeeded();
  _active = true;
  _startMillis = millis();
  _start_background_runner();
  finish_buzzer();
}

void explored_bl() {
  initBuzzer(_buzzerPin);
  _ledInitIfNeeded();
  _active = true;
  _startMillis = millis();
  _start_background_runner();
  explored_buzzer();
}

// optional: stop both buzzer and background runner
void stop_bl() {
  stop_buzzer();
  _stop_background_runner();
  // Set LEDs to min floor (approx)
  float minFrac = (float)minLevel / 255.0;
  setColor((int)(baseR * minFrac + 0.5), (int)(baseG * minFrac + 0.5), (int)(baseB * minFrac + 0.5));
}

