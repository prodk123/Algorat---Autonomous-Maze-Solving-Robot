// in main.ino
#include "algo_types.h"

enum RobotMode : uint8_t {
  MODE_IDLE = 0,
  MODE_EXPLORING,
  MODE_WAITING_RUN,
  MODE_RUNNING,
  MODE_DONE
};

struct Pose {
  int8_t x;
  int8_t y;
  Direction heading;
};

// Timing + speed tuning (adjust on real robot)
const uint16_t CELL_TRAVEL_TIME_MS = 650;  // duration to move one cell
const uint16_t TURN_TIME_MS = 240;         // duration for 90-degree turn
const uint16_t BASE_DRIVE_SPEED = 180;
const uint16_t TURN_SPEED = 170;
const uint16_t FRONT_STOP_DISTANCE = 70;  // mm, fail-safe stop

static RobotMode g_mode = MODE_IDLE;
static Pose g_pose = { 0, 0, DIR_NORTH };
static bool g_explorationComplete = false;

void resetPose();
void handleButtonEvent(int evt);
void startExploration();
void performExplorationStep();
void onExplorationComplete();
void rotateTo(Direction target);
void turnLeft90();
void turnRight90();
void turnAround();
void driveOneCellForward();
void startFastRun();
void executeRunPlan();
void printPose(const char *label);

void setup() {
  Serial.begin(230400);
  setupButton();
  setupMotors();
  startSensors();
  mazeReset();
  resetPose();
  start_bl();  // Mario theme + breathing LEDs at power up
  Serial.println("Micromouse ready. Single-click = explore, double-click = speed run.");
}

void loop() {
  updateSensors();

  int evt = button();
  if (evt != -1) {
    handleButtonEvent(evt);
  }

  if (g_mode == MODE_EXPLORING) {
    performExplorationStep();
  }

  delay(5);  // keep loop responsive; sensor update period is ~25 ms
}

void handleButtonEvent(int evt) {
  switch (evt) {
    case 1:  // single click -> exploration
      if (g_mode == MODE_IDLE || g_mode == MODE_DONE) {
        startExploration();
      }
      break;
    case 2:  // double click -> fast run (if plan ready)
      if (g_runPlanReady && (g_mode == MODE_WAITING_RUN || g_mode == MODE_DONE)) {
        startFastRun();
      } else {
        Serial.println("Run plan not ready yet.");
      }
      break;
    default:
      break;
  }
}

void startExploration() {
  Serial.println("\n[Micromouse] Starting exploration.");
  mazeReset();
  resetPose();
  g_mode = MODE_EXPLORING;
  g_explorationComplete = false;
  g_runPlanReady = false;
  explored_bl();  // audible cue that exploration mode engaged (gentle tune)
}

void resetPose() {
  g_pose.x = 0;
  g_pose.y = 0;
  g_pose.heading = DIR_NORTH;
  markVisited(g_pose.x, g_pose.y);
}

void performExplorationStep() {
  if (g_mode != MODE_EXPLORING) return;

  applySensorWalls(g_pose.x, g_pose.y, g_pose.heading,
                   left(), front(), right());
  markVisited(g_pose.x, g_pose.y);

  if (isCenter(g_pose.x, g_pose.y)) {
    onExplorationComplete();
    return;
  }

  Direction nextDir = chooseExplorationDirection(g_pose.x, g_pose.y, g_pose.heading);
  rotateTo(nextDir);
  driveOneCellForward();

  g_pose.x += (int8_t)(nextDir == DIR_EAST) - (int8_t)(nextDir == DIR_WEST);
  g_pose.y += (int8_t)(nextDir == DIR_NORTH) - (int8_t)(nextDir == DIR_SOUTH);
  printPose("Exploration step ->");
}

void onExplorationComplete() {
  Serial.println("[Micromouse] Goal reached. Building shortest run...");
  motors_stop();
  bool ok = buildShortestRun(0, 0, DIR_NORTH, g_runPlan);
  g_runPlanReady = ok;
  g_explorationComplete = true;
  g_mode = MODE_WAITING_RUN;
  if (ok) {
    Serial.print("Shortest plan length: ");
    Serial.println(g_runPlan.length);
    finish_bl();  // celebratory Star Wars jingle
    Serial.println("Double-click button to execute speed run. Reposition robot at start!");
  } else {
    Serial.println("Failed to compute run plan. Please re-explore.");
  }
}

void rotateTo(Direction target) {
  uint8_t current = g_pose.heading;
  uint8_t desired = target;
  uint8_t diff = (desired + 4 - current) & 0x03;

  if (diff == 0) return;
  if (diff == 1) {
    turnRight90();
  } else if (diff == 3) {
    turnLeft90();
  } else {
    turnAround();
  }
  g_pose.heading = target;
}

void driveOneCellForward() {
  unsigned long start = millis();
  while (millis() - start < CELL_TRAVEL_TIME_MS) {
    updateSensors();
    if (front() > 0 && front() < FRONT_STOP_DISTANCE) {
      Serial.println("[WARN] Front wall too close! stopping.");
      break;
    }
    straightDrive(BASE_DRIVE_SPEED);
    delay(5);
  }
  motors_stop();
  delay(30);
}

void turnLeft90() {
  unsigned long start = millis();
  while (millis() - start < TURN_TIME_MS) {
    updateSensors();
    turnLeftInPlace(TURN_SPEED);
    delay(2);
  }
  motors_stop();
  delay(20);
}

void turnRight90() {
  unsigned long start = millis();
  while (millis() - start < TURN_TIME_MS) {
    updateSensors();
    turnRightInPlace(TURN_SPEED);
    delay(2);
  }
  motors_stop();
  delay(20);
}

void turnAround() {
  turnRight90();
  turnRight90();
}

void startFastRun() {
  if (!g_runPlanReady) {
    Serial.println("Cannot start run: plan missing.");
    return;
  }

  Serial.println("\n[Micromouse] Starting fast run. Ensure robot is at start cell.");
  finish_bl();  // audible cue for speed run start
  resetPose();
  g_mode = MODE_RUNNING;
  executeRunPlan();
  motors_stop();
  g_mode = MODE_DONE;
  explored_bl();  // softer tune to indicate completion
}

void executeRunPlan() {
  for (uint16_t i = 0; i < g_runPlan.length; ++i) {
    RunCommand cmd = static_cast<RunCommand>(g_runPlan.commands[i]);
    switch (cmd) {
      case RUN_CMD_FORWARD:
        driveOneCellForward();
        g_pose.x += (int8_t)(g_pose.heading == DIR_EAST) - (int8_t)(g_pose.heading == DIR_WEST);
        g_pose.y += (int8_t)(g_pose.heading == DIR_NORTH) - (int8_t)(g_pose.heading == DIR_SOUTH);
        break;
      case RUN_CMD_TURN_LEFT:
        turnLeft90();
        g_pose.heading = turnLeft(g_pose.heading);
        break;
      case RUN_CMD_TURN_RIGHT:
        turnRight90();
        g_pose.heading = turnRight(g_pose.heading);
        break;
      case RUN_CMD_TURN_AROUND:
        turnAround();
        g_pose.heading = turnBack(g_pose.heading);
        break;
      default:
        break;
    }
    printPose("Run cmd ->");
  }
  Serial.println("[Micromouse] Fast run finished.");
}

void printPose(const char *label) {
  Serial.print(label);
  Serial.print(" (");
  Serial.print(g_pose.x);
  Serial.print(", ");
  Serial.print(g_pose.y);
  Serial.print(") heading ");
  Serial.println(g_pose.heading);
}