#include <Wire.h>
#include <VL53L1X.h>

#define TCA_ADDR 0x70

// Sensor objects (named by role for clarity)
VL53L1X sR;   // Right  -> CH 2
VL53L1X sFR;  // Front Right -> CH 3
VL53L1X sF;   // Front -> CH 4
VL53L1X sFL;  // Front Left -> CH 6
VL53L1X sL;   // Left -> CH 7

// channels
const uint8_t CH_R  = 2;
const uint8_t CH_FR = 3;
const uint8_t CH_F  = 4;
const uint8_t CH_FL = 6;
const uint8_t CH_L  = 7;

// cached/latest distances (mm)
volatile uint16_t lastR  = 0;
volatile uint16_t lastFR = 0;
volatile uint16_t lastF  = 0;
volatile uint16_t lastFL = 0;
volatile uint16_t lastL  = 0;

void tcaSelect(uint8_t channel) {
  if (channel > 7) return;
  Wire.beginTransmission(TCA_ADDR);
  Wire.write(1 << channel);
  Wire.endTransmission();
}

// sanitize/limit distance (optional)
static inline uint16_t sanitize(uint16_t d) {
  return d;
}

// initialize each sensor on its TCA channel and start continuous mode
void startSensors() {
  // Serial and Wire should already be started by main setup, but kept here for completeness if needed
  Serial.begin(230400);
  Wire.begin();
  Wire.setClock(1000000);

  // Right (CH 2)
  tcaSelect(CH_R); delay(2);
  if (!sR.init()) { Serial.println("sR init fail"); while (1); }
  sR.setDistanceMode(VL53L1X::Long);
  sR.setTimeout(200);
  sR.startContinuous(25); // 25 ms period

  // Front Right (CH 3)
  tcaSelect(CH_FR); delay(2);
  if (!sFR.init()) { Serial.println("sFR init fail"); while (1); }
  sFR.setDistanceMode(VL53L1X::Long);
  sFR.setTimeout(200);
  sFR.startContinuous(25);

  // Front (CH 4)
  tcaSelect(CH_F); delay(2);
  if (!sF.init()) { Serial.println("sF init fail"); while (1); }
  sF.setDistanceMode(VL53L1X::Long);
  sF.setTimeout(200);
  sF.startContinuous(25);

  // Front Left (CH 6)
  tcaSelect(CH_FL); delay(2);
  if (!sFL.init()) { Serial.println("sFL init fail"); while (1); }
  sFL.setDistanceMode(VL53L1X::Long);
  sFL.setTimeout(200);
  sFL.startContinuous(25);

  // Left (CH 7)
  tcaSelect(CH_L); delay(2);
  if (!sL.init()) { Serial.println("sL init fail"); while (1); }
  sL.setDistanceMode(VL53L1X::Long);
  sL.setTimeout(200);
  sL.startContinuous(25);

  Serial.println("Sensors started (Short mode, continuous).");
}

// read all sensors quickly and update the cached values
// call this regularly in your main loop at the rate you want sensor updates
void updateSensors() {
  // Right (CH 2)
  tcaSelect(CH_R);
  uint16_t d = sR.read();
  if (sR.timeoutOccurred()) d = 0;
  lastR = sanitize(d);

  // Front Right (CH 3)
  tcaSelect(CH_FR);
  d = sFR.read();
  if (sFR.timeoutOccurred()) d = 0;
  lastFR = sanitize(d);

  // Front (CH 4)
  tcaSelect(CH_F);
  d = sF.read();
  if (sF.timeoutOccurred()) d = 0;
  lastF = sanitize(d);

  // Front Left (CH 6)
  tcaSelect(CH_FL);
  d = sFL.read();
  if (sFL.timeoutOccurred()) d = 0;
  lastFL = sanitize(d);

  // Left (CH 7)
  tcaSelect(CH_L);
  d = sL.read();
  if (sL.timeoutOccurred()) d = 0;
  lastL = sanitize(d);
}

// getters for your main.ino (non-blocking — return last read value)
uint16_t rightDistance()     { return lastR;  } // call right() if you prefer that name
uint16_t frontRightDistance(){ return lastFR; }
uint16_t frontDistance()     { return lastF;  }
uint16_t frontLeftDistance() { return lastFL; }
uint16_t leftDistance()      { return lastL;  }

// alias names you asked for (left())
uint16_t left()  { return leftDistance(); }
uint16_t front() { return frontDistance(); }
uint16_t fr()    { return frontRightDistance(); }
uint16_t fl()    { return frontLeftDistance(); }
uint16_t right() { return rightDistance(); }
