// button.ino
// Teensy 4.1 button on pin 0 (active LOW recommended)

const int BUTTON_PIN = 0;

unsigned long lastChange = 0;
unsigned long pressStart = 0;

bool lastState = HIGH;     // HIGH = released (if pulled-up)
bool waitingRelease = false;

int clickCount = 0;

const unsigned long debounceTime = 30;          // ms
const unsigned long clickGap = 300;             // max gap between clicks
const unsigned long longPressTime = 1000;       // 1 sec
const unsigned long veryLongPressTime = 5000;   // 5 sec

void setupButton() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
}

// Returns:
//  -1 = no event
//   0 = long press (1 sec)
//   1 = single click
//   2 = double click
//   3 = triple click
//   4 = very long press (5 sec)
int button() {
  int state = digitalRead(BUTTON_PIN);

  unsigned long now = millis();

  // Debounce
  if (state != lastState) {
    lastChange = now;
    lastState = state;
  }
  if (now - lastChange < debounceTime) return -1;

  // Button pressed (active LOW)
  if (state == LOW && !waitingRelease) {
    pressStart = now;
    waitingRelease = true;
  }

  // Still holding → check long / very long press
  if (waitingRelease && state == LOW) {

    // Very long press (5 sec)
    if (now - pressStart >= veryLongPressTime) {
      waitingRelease = false;
      clickCount = 0;
      return 4;
    }

    // Long press (1 sec)
    if (now - pressStart >= longPressTime) {
      waitingRelease = false;
      clickCount = 0;
      return 0;
    }
  }

  // Button released → handle clicks
  if (waitingRelease && state == HIGH) {
    waitingRelease = false;
    clickCount++;

    // Wait for double/triple click window
    static unsigned long releaseTime = 0;
    releaseTime = now;

    // Check if the click series is complete later
    while (millis() - releaseTime < clickGap) {
      if (digitalRead(BUTTON_PIN) == LOW) {
        // another click incoming
        return -1;
      }
    }

    // End of click sequence
    int result = clickCount;
    clickCount = 0;
    return result;  // 1 = single, 2 = double, 3 = triple
  }

  return -1;
}
