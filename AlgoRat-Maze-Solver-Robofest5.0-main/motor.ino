// motor.ino
// TB6612FNG motor control for Teensy 4.1
// Uses pins defined by you:
// Motor A: PWMA=28, AIN1=27, AIN2=26
// Motor B: PWMB=29, BIN1=31, BIN2=32
// STBY = 30

const int PWMA = 28;
const int AIN1 = 27;
const int AIN2 = 26;

const int PWMB = 29;
const int BIN1 = 31;
const int BIN2 = 32;

const int STBY = 30;

// Safe default speed (0-255)
const int SAFE_SPEED = 150;

// Straight-drive PID-ish gains (P-only). Tune these.
float Kp_center = 0.9;   // proportional gain for centering correction
int maxTrim = 60;       // maximum speed adjustment applied to each motor

// ------------------ low-level helpers ------------------
void motorDriverEnable() {
  digitalWrite(STBY, HIGH);
}

void motorDriverDisable() {
  digitalWrite(STBY, LOW);
}

void setupMotors() {
  pinMode(PWMA, OUTPUT);
  pinMode(AIN1, OUTPUT);
  pinMode(AIN2, OUTPUT);

  pinMode(PWMB, OUTPUT);
  pinMode(BIN1, OUTPUT);
  pinMode(BIN2, OUTPUT);

  pinMode(STBY, OUTPUT);
  motorDriverEnable();

  // stop at startup
  analogWrite(PWMA, 0);
  analogWrite(PWMB, 0);
}

// ------------------ motor primitives ------------------
// Motor A (left motor / whichever physical mapping you have)
void motorA_forward(int pwm) {
  digitalWrite(AIN1, LOW);
  digitalWrite(AIN2, HIGH);
  analogWrite(PWMA, constrain(pwm, 0, 255));
}

void motorA_backward(int pwm) {
  digitalWrite(AIN1, HIGH);
  digitalWrite(AIN2, LOW);
  analogWrite(PWMA, constrain(pwm, 0, 255));
}

// Motor B (right motor)
void motorB_forward(int pwm) {
  digitalWrite(BIN1, HIGH);
  digitalWrite(BIN2, LOW);
  analogWrite(PWMB, constrain(pwm, 0, 255));
}

void motorB_backward(int pwm) {
  digitalWrite(BIN1, LOW);
  digitalWrite(BIN2, HIGH);
  analogWrite(PWMB, constrain(pwm, 0, 255));
}

void motors_stop() {
  analogWrite(PWMA, 0);
  analogWrite(PWMB, 0);
}

// Set independent motor speeds and directions
// pwmA, pwmB: -255 .. 255 (negative = backward)
void setMotorSpeeds(int pwmA, int pwmB) {
  if (pwmA >= 0) motorA_forward(pwmA);
  else motorA_backward(-pwmA);

  if (pwmB >= 0) motorB_forward(pwmB);
  else motorB_backward(-pwmB);
}

// ------------------ higher-level motions ------------------
void driveForward(int speed) {
  speed = constrain(speed, 0, 255);
  motorA_forward(speed);
  motorB_forward(speed);
}

void driveBackward(int speed) {
  speed = constrain(speed, 0, 255);
  motorA_backward(speed);
  motorB_backward(speed);
}

void turnLeftInPlace(int speed) {
  // left backward, right forward
  speed = constrain(speed, 0, 255);
  motorA_backward(speed);
  motorB_forward(speed);
}

void turnRightInPlace(int speed) {
  // left forward, right backward
  speed = constrain(speed, 0, 255);
  motorA_forward(speed);
  motorB_backward(speed);
}

// ------------------ straight drive using sensors ------------------
// Uses sensor getters: left(), right(), front(), etc. (from your sensor file).
// Keeps robot centered between walls by trimming motor speeds.
// Call updateSensors() frequently from main loop so these getters remain fresh.

void straightDrive(int baseSpeed) {
  // Read cached sensor values
  uint16_t L = left();   // left distance in mm (0 if none/timeout)
  uint16_t R = right();  // right distance in mm

  // If both sensors are invalid, just drive straight
  if (L == 0 && R == 0) {
    setMotorSpeeds(baseSpeed, baseSpeed);
    return;
  }

  // If one side is invalid, try to use the other side as reference
  // If left only available, assume we want to keep left at similar distance to last known or a target.
  float error = 0.0;
  if (L == 0) {
    // only right valid => try to keep equal distance by mirroring (small heuristic)
    // If robot is too close to right, steer left (decrease right motor)
    error = -(float)R;
  } else if (R == 0) {
    error = (float)L;
  } else {
    // Prefer centering: error = left - right
    error = (float)L - (float)R;
  }

  // Apply proportional controller
  float trim = Kp_center * error / 10.0; // scale down (units -> adjust)
  // Constrain trim to avoid over-correction
  trim = constrain((int)round(trim), -maxTrim, maxTrim);

  int leftSpeed  = baseSpeed - (int)trim;
  int rightSpeed = baseSpeed + (int)trim;

  // Ensure speeds are within PWM bounds
  leftSpeed  = constrain(leftSpeed, 0, 255);
  rightSpeed = constrain(rightSpeed, 0, 255);

  setMotorSpeeds(leftSpeed, rightSpeed);
}

// ------------------ debug helpers ------------------
void printMotorState() {
  Serial.print("MotorA PWM: "); Serial.print(analogRead(PWMA)); // note: analogRead won't give PWM value; for debug we just show a marker
  Serial.print(" | MotorB PWM: "); Serial.println(analogRead(PWMB));
}
