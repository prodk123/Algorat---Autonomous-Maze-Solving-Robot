// maze.ino
// Maintains the in-memory representation of the maze and exposes helpers for
// flood-fill exploration + fastest-path generation.

#include <Arduino.h>
#include "algo_types.h"

namespace {

const uint8_t WALL_N = 1 << DIR_NORTH;
const uint8_t WALL_E = 1 << DIR_EAST;
const uint8_t WALL_S = 1 << DIR_SOUTH;
const uint8_t WALL_W = 1 << DIR_WEST;

const int8_t DX[4] = { 0,  1,  0, -1};
const int8_t DY[4] = { 1,  0, -1,  0};

// Sensor tuning (mm)
const uint16_t WALL_DISTANCE_MM = 110;

uint8_t g_walls[MAZE_SIZE][MAZE_SIZE];
uint8_t g_known[MAZE_SIZE][MAZE_SIZE];
bool    g_visited[MAZE_SIZE][MAZE_SIZE];

inline uint8_t dirBit(Direction dir) {
  return (1U << dir);
}

bool wallPresent(uint8_t x, uint8_t y, Direction dir) {
  return (g_walls[x][y] & dirBit(dir)) != 0;
}

bool wallKnown(uint8_t x, uint8_t y, Direction dir) {
  return (g_known[x][y] & dirBit(dir)) != 0;
}

void setWall(uint8_t x, uint8_t y, Direction dir, bool present) {
  uint8_t bit = dirBit(dir);
  g_known[x][y] |= bit;
  if (present) g_walls[x][y] |= bit;
  else         g_walls[x][y] &= ~bit;

  int nx = x + DX[dir];
  int ny = y + DY[dir];
  if (inBounds(nx, ny)) {
    Direction opposite = turnBack(dir);
    uint8_t obit = dirBit(opposite);
    g_known[nx][ny] |= obit;
    if (present) g_walls[nx][ny] |= obit;
    else         g_walls[nx][ny] &= ~obit;
  }
}

bool passageOpen(uint8_t x, uint8_t y, Direction dir, bool requireKnown) {
  if (!inBounds(x, y)) return false;
  if (requireKnown && !wallKnown(x, y, dir)) return false;
  if (wallPresent(x, y, dir)) return false;
  int nx = x + DX[dir];
  int ny = y + DY[dir];
  if (!inBounds(nx, ny)) return false;
  return true;
}

void computeFlood(uint8_t distances[MAZE_SIZE][MAZE_SIZE], bool requireKnown) {
  const uint8_t INF = 0xFF;
  for (uint8_t x = 0; x < MAZE_SIZE; ++x) {
    for (uint8_t y = 0; y < MAZE_SIZE; ++y) {
      distances[x][y] = INF;
    }
  }

  struct Cell { uint8_t x; uint8_t y; };
  Cell queue[MAZE_SIZE * MAZE_SIZE];
  uint16_t head = 0;
  uint16_t tail = 0;

  for (uint8_t x = CENTER_MIN; x <= CENTER_MAX; ++x) {
    for (uint8_t y = CENTER_MIN; y <= CENTER_MAX; ++y) {
      distances[x][y] = 0;
      queue[tail++] = {x, y};
    }
  }

  while (head < tail) {
    Cell cur = queue[head++];
    uint8_t base = distances[cur.x][cur.y];
    for (uint8_t d = 0; d < 4; ++d) {
      Direction dir = static_cast<Direction>(d);
      if (!passageOpen(cur.x, cur.y, dir, requireKnown)) continue;

      uint8_t nx = cur.x + DX[d];
      uint8_t ny = cur.y + DY[d];
      if (!inBounds(nx, ny)) continue;

      if (distances[nx][ny] > base + 1) {
        distances[nx][ny] = base + 1;
        queue[tail++] = {nx, ny};
      }
    }
  }
}

Direction bestDirectionFromDistances(uint8_t x, uint8_t y, Direction heading,
                                     bool requireKnownWalls,
                                     const Direction preferredOrder[4]) {
  uint8_t distances[MAZE_SIZE][MAZE_SIZE];
  computeFlood(distances, requireKnownWalls);

  const uint8_t INF = 0xFF;
  Direction fallback = turnBack(heading);
  Direction bestDir = fallback;
  uint8_t bestVal = INF;
  bool haveBest = false;

  for (int i = 0; i < 4; ++i) {
    Direction dir = preferredOrder[i];
    if (!passageOpen(x, y, dir, requireKnownWalls)) continue;
    uint8_t nx = x + DX[dir];
    uint8_t ny = y + DY[dir];
    if (!inBounds(nx, ny)) continue;
    uint8_t val = distances[nx][ny];
    if (val == INF) continue;

    if (!haveBest || val < bestVal) {
      bestVal = val;
      bestDir = dir;
      haveBest = true;
    } else if (val == bestVal) {
      bool currentVisited = cellVisited(x + DX[bestDir], y + DY[bestDir]);
      bool candidateVisited = cellVisited(nx, ny);
      if (currentVisited && !candidateVisited) {
        bestDir = dir;
      }
    }
  }

  return haveBest ? bestDir : fallback;
}

bool nextDirectionForRun(uint8_t x, uint8_t y, Direction &dirOut) {
  uint8_t distances[MAZE_SIZE][MAZE_SIZE];
  computeFlood(distances, true);
  const uint8_t INF = 0xFF;

  uint8_t bestVal = INF;
  bool haveBest = false;
  Direction bestDir = DIR_NORTH;

  for (uint8_t d = 0; d < 4; ++d) {
    Direction dir = static_cast<Direction>(d);
    if (!passageOpen(x, y, dir, true)) continue;
    uint8_t nx = x + DX[d];
    uint8_t ny = y + DY[d];
    if (!inBounds(nx, ny)) continue;
    uint8_t val = distances[nx][ny];
    if (val == INF) continue;
    if (!haveBest || val < bestVal) {
      bestVal = val;
      bestDir = dir;
      haveBest = true;
    }
  }

  if (!haveBest) return false;
  dirOut = bestDir;
  return true;
}

} // namespace

RunPlan g_runPlan;
bool    g_runPlanReady = false;

void mazeReset() {
  memset(g_walls, 0, sizeof(g_walls));
  memset(g_known, 0, sizeof(g_known));
  memset(g_visited, 0, sizeof(g_visited));
  g_runPlan.length = 0;
  g_runPlanReady = false;

  for (uint8_t x = 0; x < MAZE_SIZE; ++x) {
    setWall(x, 0, DIR_SOUTH, true);
    setWall(x, MAZE_SIZE - 1, DIR_NORTH, true);
  }
  for (uint8_t y = 0; y < MAZE_SIZE; ++y) {
    setWall(0, y, DIR_WEST, true);
    setWall(MAZE_SIZE - 1, y, DIR_EAST, true);
  }
}

bool inBounds(int x, int y) {
  return x >= 0 && y >= 0 && x < MAZE_SIZE && y < MAZE_SIZE;
}

bool isCenter(uint8_t x, uint8_t y) {
  return x >= CENTER_MIN && x <= CENTER_MAX &&
         y >= CENTER_MIN && y <= CENTER_MAX;
}

void markVisited(uint8_t x, uint8_t y) {
  if (!inBounds(x, y)) return;
  g_visited[x][y] = true;
}

bool cellVisited(uint8_t x, uint8_t y) {
  if (!inBounds(x, y)) return false;
  return g_visited[x][y];
}

void applySensorWalls(uint8_t x, uint8_t y, Direction heading,
                      uint16_t leftDist, uint16_t frontDist, uint16_t rightDist) {
  if (!inBounds(x, y)) return;

  auto observe = [&](Direction dir, uint16_t measurement) {
    if (measurement == 0) return;
    bool wall = measurement <= WALL_DISTANCE_MM;
    setWall(x, y, dir, wall);
  };

  observe(turnLeft(heading), leftDist);
  observe(heading, frontDist);
  observe(turnRight(heading), rightDist);
}

Direction chooseExplorationDirection(uint8_t x, uint8_t y, Direction heading) {
  Direction order[4] = {
    heading,
    turnLeft(heading),
    turnRight(heading),
    turnBack(heading)
  };
  return bestDirectionFromDistances(x, y, heading, false, order);
}

Direction chooseRunDirection(uint8_t x, uint8_t y) {
  Direction dummyPref[4] = {DIR_NORTH, DIR_EAST, DIR_SOUTH, DIR_WEST};
  return bestDirectionFromDistances(x, y, DIR_NORTH, true, dummyPref);
}

bool buildShortestRun(uint8_t startX, uint8_t startY, Direction startHeading,
                      RunPlan &plan) {
  plan.length = 0;
  uint8_t x = startX;
  uint8_t y = startY;
  Direction heading = startHeading;

  uint8_t safety = 0;
  const uint8_t safetyLimit = RunPlan::MAX_LENGTH - 4;

  while (!isCenter(x, y) && safety < safetyLimit) {
    Direction nextDir;
    if (!nextDirectionForRun(x, y, nextDir)) {
      return false;
    }

    uint8_t diff = (static_cast<uint8_t>(nextDir) + 4 - static_cast<uint8_t>(heading)) & 0x03;
    auto pushCommand = [&](RunCommand cmd) -> bool {
      if (plan.length >= RunPlan::MAX_LENGTH) return false;
      plan.commands[plan.length++] = static_cast<uint8_t>(cmd);
      return true;
    };

    if (diff == 1) {
      if (!pushCommand(RUN_CMD_TURN_RIGHT)) return false;
      heading = turnRight(heading);
    } else if (diff == 3) {
      if (!pushCommand(RUN_CMD_TURN_LEFT)) return false;
      heading = turnLeft(heading);
    } else if (diff == 2) {
      if (!pushCommand(RUN_CMD_TURN_AROUND)) return false;
      heading = turnBack(heading);
    }

    if (!pushCommand(RUN_CMD_FORWARD)) return false;
    x += DX[heading];
    y += DY[heading];
    ++safety;
  }

  return isCenter(x, y);
}
