#pragma once

#include <Arduino.h>

// Global maze/grid configuration ------------------------------------------------
static const uint8_t MAZE_SIZE = 16;          // 16x16 classic micromouse maze
static const uint8_t CENTER_MIN = 7;          // first index that belongs to the goal square
static const uint8_t CENTER_MAX = 8;          // last index that belongs to the goal square

// Exploration + navigation helpers ---------------------------------------------
enum Direction : uint8_t {
  DIR_NORTH = 0,
  DIR_EAST  = 1,
  DIR_SOUTH = 2,
  DIR_WEST  = 3
};

inline Direction turnLeft(Direction d)  { return static_cast<Direction>((d + 3) & 0x03); }
inline Direction turnRight(Direction d) { return static_cast<Direction>((d + 1) & 0x03); }
inline Direction turnBack(Direction d)  { return static_cast<Direction>((d + 2) & 0x03); }

enum RunCommand : uint8_t {
  RUN_CMD_FORWARD = 0,
  RUN_CMD_TURN_LEFT,
  RUN_CMD_TURN_RIGHT,
  RUN_CMD_TURN_AROUND
};

struct RunPlan {
  enum { MAX_LENGTH = 256 };
  uint8_t commands[MAX_LENGTH];
  uint16_t length = 0;
};

extern RunPlan g_runPlan;
extern bool g_runPlanReady;

// Maze APIs implemented in maze.ino --------------------------------------------
void mazeReset();
bool inBounds(int x, int y);
bool isCenter(uint8_t x, uint8_t y);

void markVisited(uint8_t x, uint8_t y);
bool cellVisited(uint8_t x, uint8_t y);

void applySensorWalls(uint8_t x, uint8_t y, Direction heading,
                      uint16_t leftDist, uint16_t frontDist, uint16_t rightDist);

Direction chooseExplorationDirection(uint8_t x, uint8_t y, Direction heading);
Direction chooseRunDirection(uint8_t x, uint8_t y);

bool buildShortestRun(uint8_t startX, uint8_t startY, Direction startHeading,
                      RunPlan &plan);
