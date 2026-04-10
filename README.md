# 🐀 AlgoRat: Autonomous Maze Solver Robot
![C++](https://img.shields.io/badge/C++-00599C?style=for-the-badge&logo=c%2B%2B&logoColor=white)
![Arduino](https://img.shields.io/badge/Arduino-00979D?style=for-the-badge&logo=Arduino&logoColor=white)
![Robofest 5.0](https://img.shields.io/badge/Robofest-5.0-FF0000?style=for-the-badge&logo=codeforces&logoColor=white)

**AlgoRat** is an autonomous micromouse robot developed for the Robofest 5.0 maze-solving competition. Engineered to map, analyze, and traverse complex mazes efficiently, AlgoRat leverages advanced search algorithms and real-time sensory feedback for optimal path calculation.

---

## 🎯 Overview

AlgoRat employs a dual-phase solving strategy:
1. **Exploration Phase:** Systematically maps the maze from start to a predefined goal (the center) using **Flood Fill**.
2. **Speed Run Phase:** Calculates the shortest, optimal path using shortest-path techniques akin to **A*** and executes a high-speed traversal to the goal.

The robot is completely autonomous, drawing real-time data from distance sensors (ToF) and interacting via its internal state machine, button inputs, and audio-visual feedback mechanisms (LEDs + Buzzer tracks).

---

## 🧠 Core Navigation Algorithms

AlgoRat's intelligence is defined by the rigorous pathfinding implementation found in `maze.ino`:

- **Flood Fill Explorer:** Re-evaluates distance paths on-the-fly dynamically. As the robot traverses unknown cells, it observes walls using Time-of-Flight (ToF) sensors and updates the known maze topology. It prioritizes unvisited cells and adjusts its gradient map recursively.
- **A* / Shortest-Path Generator:** Once the center is reached, the solver computes an ultra-minimized command queue (`RunPlan`) from start to finish. Redundant backtracking is ignored, yielding a swift `Speed Run` sequence.

---

## ⚙️ Hardware Components

- **Microcontroller:** Arduino-based controller (using `C++/.ino` structure).
- **Time-of-Flight Sensors (ToF):** Mounted on the front, left, and right, delivering millimeter-accurate obstacle detection up to `110mm`.
- **Motor Control System:** Dual continuous locomotion modules with encoders (`motor.ino`, `encoder.ino`) to manage straight drives, 90-degree snap turns, and perfect 180-degree turnarounds over uniform grid cells.
- **Button Interface:** A reliable central tactile switch used for state cycling.
- **Audiovisual Indicators:** Features a buzzer (`buzzer_led.ino`) that plays distinct melodic cues (e.g., Mario melody on startup, Star Wars theme on goal resolution) and breathing LEDs based on the solver state.

---

## 📁 System Architecture

| File | Description |
| ---- | ----------- |
| `main.ino` | Central execution loop; houses the state machine (`IDLE`, `EXPLORING`, `WAITING_RUN`, `RUNNING`). Coordinates sensors, movement, and algorithms. |
| `maze.ino` | Algorithmic powerhouse; implements grid bounds, **Flood Fill** distances, and **A***-like path compilation. |
| `ToF.ino` | Distance interpretation; handles array setups and telemetry. |
| `motor.ino` / `encoder.ino` | Kinematics and closed-loop speed regulation (`BASE_DRIVE_SPEED`, `TURN_SPEED`). |
| `buzzer_led.ino` | Audio-visual outputs corresponding to the active mode. |
| `button.ino` | Debounced multi-click action parser (e.g., single-click vs double-click). |

---

## 🚀 Quick Setup & Usage

### 1. Uploading the Code
1. Open up `main.ino` in the **Arduino IDE**.
2. Make sure all hardware libraries dependencies (ToF libraries, Encoder limits, etc.) are installed.
3. Select your microcontroller board.
4. Compile and upload to the robot.

### 2. Operating the Robot
AlgoRat relies on an intuitive click-based UI:
1. **Power On:** You will hear the Mario theme queue, and the LEDs will start breathing. The robot is now in `MODE_IDLE`.
2. **Single Click (Explore):** Place AlgoRat in the starting cell. Click once. The robot will begin **mapping** the maze automatically dynamically calculating paths via **Flood Fill**.
3. **Goal Reached:** The robot safely halts at the destination, calculates the shortest speed run queue via **A***, and plays a celebratory Star Wars jingle.
4. **Double Click (Speed Run):** Move the robot back to the starting cell. Double-click the button. The robot will blast through the maze taking only the optimal high-speed track.

---

## 🛠️ Calibration & Tuning

Hardware drift? Sensor misalignment? Modify the constants in `main.ino` and `maze.ino`:

- `CELL_TRAVEL_TIME_MS` (Default `650`): Calibrate specific encoder/speed ratios for moving one cell block.
- `TURN_TIME_MS` (Default `240`): Fine-tune for perfect 90° swivels.
- `WALL_DISTANCE_MM` (Default `110`): Adjust sensor noise threshold.

---

## 📜 License
Developed for Robofest 5.0. 