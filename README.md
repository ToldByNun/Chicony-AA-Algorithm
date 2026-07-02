# Chicony AA Algorithm

Educational recreation and visualization of an aim-assist-style cursor filter, inspired by documented aim-assist behavior and analysis tooling such as [CircleGuard](https://github.com/circleguard/circleguard).

> **Educational project only.** This repository is for study and reference. Do not copy, reuse, modify, or integrate this code elsewhere without explicit permission from the copyright holder.

Copyright (c) 2026 ToldByNun. All rights reserved.

---

## Table of Contents

### User Guide (no code required)

1. [Overview](#overview)
2. [Quick Start](#quick-start)
3. [On-Screen Elements](#on-screen-elements)
4. [Core Concepts](#core-concepts)
5. [FOV vs circleSize](#fov-vs-circlesize)
6. [How the Assist Works Each Frame](#how-the-assist-works-each-frame)
7. [CircleGuard Snapping (Live Analysis)](#circleguard-snapping-live-analysis)
8. [Settings Explained](#settings-explained)
9. [How to Test](#how-to-test)

### Developer Guide

1. [Tech Stack](#tech-stack)
2. [Project Structure](#project-structure)
3. [Architecture](#architecture)
4. [Render Pipeline](#render-pipeline)
5. [Assist Algorithm](#assist-algorithm)
6. [CircleGuardSnapping Module](#circleguardsnapping-module)
7. [Settings Reference](#settings-reference)
8. [Build Instructions](#build-instructions)
9. [License & Attribution](#license--attribution)

---

# User Guide

## Overview

You move your mouse (**orange**). The program also computes an **assisted cursor position** (**blue**). Blue is not a perfect snap to target - it is a **filtered, softened suggestion** that tries to look natural while subtly helping toward the goal.

This project is a **visualizer only**. It runs a fullscreen black overlay, reads your real cursor, runs the algorithm, and draws the result. It does **not** hook games or inject input.

---

## Quick Start

1. Open `Algorithm/Algorithm.sln` in **Visual Studio 2022**
2. Select **Debug | x64**
3. Build and run `Algorithm.exe`
4. A black fullscreen overlay appears — move your mouse toward the center of the screen

---

## On-Screen Elements


| Element          | Color           | Meaning                                                 |
| ---------------- | --------------- | ------------------------------------------------------- |
| Orange circle    | Orange          | Your **real** mouse position                            |
| Blue circle      | Blue            | The **assisted** position (algorithm output)            |
| Large red circle | Red             | **FOV** — outer boundary; assist is off outside this    |
| Light red circle | Light red       | **circleSize** — suggested target size (soft hint only) |
| Top-right panel  | Gray / red text | Live **CircleGuard snapping** analysis                  |


When orange and blue overlap completely, assist is effectively inactive (1:1 pass-through).

---

## Core Concepts

### Magnetism

When you are close enough to the target, the algorithm gently pulls the blue cursor toward the center. Pull strength is capped so it does not hard-snap like a bot.

```
offset direction = toward target
pull distance    = strength × remaining distance (capped by maxPull)
```

### Friction (sticky swipe)

When you swipe **across** the target, orange keeps moving fast but blue can lag briefly near the center — giving a wider effective hit window. This is done by slowing offset updates during tangential movement.

### Prediction

The algorithm looks at **where the mouse is going**, not just where it is:

```
predicted position = current position + velocity * lookahead time
```

If you flick toward the target, assist increases briefly. If movement is smooth and natural, assist is reduced so motion looks human.

---

## FOV vs circleSize

These are two different circles with different roles:


| Term           | Role                 | Analogy                                                         |
| -------------- | -------------------- | --------------------------------------------------------------- |
| **FOV**        | Hard outer boundary  | Assist is fully off outside this                                |
| **circleSize** | Soft size suggestion | Approximate hit-circle radius - shapes weights, not a hard clip |


Main assist weight comes from **prediction** and **natural movement**, not from a hard center snap.

---

## How the Assist Works Each Frame

### Phase A - Outside FOV

```
distance to target > FOV  ->  blue follows orange 1:1
```

When leaving the zone, blue **smoothly resyncs** to orange via offset decay (no instant jump).

### Phase B - Inside FOV

Three influences are combined:

1. **Circle suggestion** — soft weight from `circleSize` / `fov` relationship
2. **Prediction** — flick detection + lookahead
3. **Natural movement** — reduces assist when hand motion is smooth

```
combined pull =
  (circleSuggestion * circleSuggestionInfluence
 + prediction      * predictionInfluence)
 * magnetStrength
 * (1 − naturalPreserve * naturalMovementInfluence)

combined pull = clamp(combined pull, 0, maxPull)
```

Blue moves only a **fraction** of the remaining distance toward target — never a full snap unless prediction strongly supports it.

### Phase C - Resync when pulling away

Assist uses an **offset model**:

```
offset       = blue − orange
blue position = orange + offset
```

- Orange moves -> blue moves with it (same base)
- Offset decays toward zero -> blue glides back to orange
- Radial movement away from target releases offset early via `releaseStrength`

---

## CircleGuard Snapping (Live Analysis)

[CircleGuard](https://github.com/circleguard/circleguard) analyzes osu! replays for suspicious snap movement (aim correction). This project ports the **core detection idea** for live visualization.

### Plain explanation

Take **3 consecutive mouse positions** A -> B -> C. Look at the **angle at point B**:

- Straight motion -> large angle (~180°)
- Sharp kink -> very small angle

A snap is flagged when:

1. Angle at B is very acute (< **10°** by default)
2. Both segments |AB| and |BC| are long enough (> **8 px** by default)

### Math (law of cosines)

For triangle A-B-C with angle β at B:

```
cos(β) = (|AB|² + |BC|² - |AC|²) / (2 * |AB| * |BC|)
β (degrees) = arccos(cos(β)) * 180/π
```

Snap when: `β < maxAngle` **and** `min(|AB|, |BC|) > minDistance`

### Log panel


| Line           | Meaning                              |
| -------------- | ------------------------------------ |
| `live angle`   | Current angle from last 3 samples    |
| `live min leg` | Shorter of segments AB / BC          |
| `thresholds`   | Detection limits (default 10°, 8 px) |
| `status: SNAP` | Current frame qualifies as a snap    |
| `[snap] t=...` | Past detected snap events            |


> **Note:** Full CircleGuard also filters snaps against beatmap hitobjects. This visualizer analyzes raw mouse movement only — useful for learning, but may produce more false positives.

---

## Settings Explained

All values live in `Assist::Settings`. Plain-language summary:


| Setting                     | What it does                                                             |
| --------------------------- | ------------------------------------------------------------------------ |
| `fov`                       | How far assist can still affect the cursor (large red ring)              |
| `circleSize`                | Suggested target radius (small light-red ring)                           |
| `magnetStrength`            | Overall pull strength toward target                                      |
| `maxPull`                   | Max fraction of remaining distance pulled per frame - prevents hard snap |
| `circleSuggestionInfluence` | How much the circle hint contributes                                     |
| `predictionInfluence`       | How much flick / prediction contributes                                  |
| `naturalMovementInfluence`  | How much smooth hand motion reduces assist                               |
| `friction`                  | How long blue sticks near target during tangential swipes                |
| `lookahead`                 | How far ahead prediction looks (seconds)                                 |
| `flickBoost`                | Extra pull when flicking toward target                                   |
| `minSwipeSpeed`             | Minimum speed before swipe/flick logic activates                         |
| `smoothing`                 | How fast blue follows computed offset inside zone (lower = softer)       |
| `syncSpeed`                 | How fast blue resyncs to orange outside FOV                              |
| `releaseStrength`           | How early assist releases when moving radially away                      |


Example tweak in `main.cpp` after `classManager.init()`:

```cpp
auto& s = globals.assist->settings();
s.fov = 200.f;
s.circleSize = 69.f;
s.maxPull = 0.3f;
s.smoothing = 6.f;
```

---

## How to Test

1. **Far from target** - orange and blue should overlap
2. **Slow approach** - blue separates gently; no hard lock to center
3. **Fast swipe through target** - blue lingers near center briefly (friction)
4. **Pull radially away** - blue smoothly resyncs to orange
5. **Sharp kink in movement** - log may show `status: SNAP`

---

# Developer Guide

## Tech Stack


| Component | Detail                                          |
| --------- | ----------------------------------------------- |
| Language  | C++20                                           |
| Toolchain | Visual Studio 2022, MSVC v143                   |
| Platform  | Windows x64                                     |
| Rendering | DirectX 11 + Dear ImGui (Win32 / DX11 backends) |
| Math      | GLM                                             |
| Entry     | `Algorithm/Algorithm.sln`                       |


---

## Project Structure

```
Chicony AA Algorithm/
├── README.md
└── Algorithm/
    └── Algorithm/
        ├── Algorithm.sln
        ├── Algorithm.vcxproj
        └── Algorithm/
            ├── main.cpp
            ├── Manager/
            │   ├── Classmanager/Classmanager.hpp
            │   └── Globals/Globals.hpp
            ├── Modules/
            │   ├── Overlay/          # DX11 window + ImGui loop
            │   ├── Assist/           # Cursor assist filter
            │   └── Visuals/          # Drawing + snap log panel
            └── Engine/
                ├── CircleGuardSnapping/
                ├── ImGui/            # Vendored Dear ImGui
                └── glm/              # Vendored GLM
```

---

## Architecture

### Bootstrap (`main.cpp`)

```cpp
ClassManager classManager;
globals.classManager = &classManager;

globals.overlay = classManager.addClass<Overlay>("Overlay");
globals.assist  = classManager.addClass<Assist>("Assist");
globals.visuals = classManager.addClass<Visuals>("Visuals");

if (!classManager.init()) return 1;

while (globals.overlay->isRunning())
    globals.overlay->update();

classManager.deinit();
```

### Module responsibilities


| Module                | Role                                                            |
| --------------------- | --------------------------------------------------------------- |
| `Overlay`             | Borderless topmost Win32 window, D3D11 device, ImGui frame loop |
| `Assist`              | `process(mouse, target, dt)` -> assisted cursor position        |
| `Visuals`             | Draws FOV rings, cursors, calls Assist, feeds snap detection    |
| `CircleGuardSnapping` | 3-point angle snap analysis + log strings                       |


### Globals

```cpp
struct Globals {
    ClassManager* classManager;
    Overlay*      overlay;
    Assist*       assist;
    Visuals*      visuals;
};
extern Globals globals;
```

All modules inherit `IManagedClass` (`init`, `deinit`, `update`) and are registered via `ClassManager`.

---

## Render Pipeline

```
Overlay::update()
  └─ renderFrame()
       ├─ ImGui::NewFrame()
       ├─ BackgroundDrawList → black fullscreen fill
       ├─ Overlay::onDraw()
       │    └─ Visuals::draw()
       │         ├─ GetCursorPos()           → orange cursor
       │         ├─ snapDetection.addSample()
       │         ├─ draw FOV + circleSize rings
       │         ├─ Assist::process()        → blue cursor
       │         └─ drawSnapDetectionLog()   → top-right panel
       ├─ ImGui::Render()
       └─ SwapChain::Present()
```

Drawing uses `ImDrawList` only — no ImGui widget GUI.

---

## Assist Algorithm

### Entry point

```cpp
glm::vec2 Assist::process(
    const glm::vec2& mousePosition,
    const glm::vec2& targetCenter,
    float deltaTime);
```

### Persistent state

```cpp
glm::vec2 previousMousePosition_;
glm::vec2 previousAssistPosition_;
glm::vec2 previousMouseVelocity_;
bool hasFrameHistory_;
```

### Offset model

```cpp
previousOffset = previousAssistPosition - previousMousePosition;
desiredOffset  = computeDesiredOffsetInZone(...);
blendedOffset  = mix(previousOffset, desiredOffset, offsetBlendRate);
output         = mousePosition + blendedOffset;
```

Outside FOV: `desiredOffset = vec2(0)`, blended via `syncSpeed`.

### Weight functions

**Circle suggestion** (`computeCircleSuggestionWeight`):

```cpp
outsideDistance = distanceToTarget - circleRadius;
outsideRange    = fovRadius - circleRadius;
outsideFalloff  = 1 - clamp(outsideDistance / outsideRange, 0, 1);
return outsideFalloff * 0.45f;
```

**Prediction** (`computePredictionWeight`):

```cpp
movementTowardTarget = dot(normalize(velocity), normalize(target - mouse));
flickIntensity       = clamp(movementTowardTarget * (speed / minSwipeSpeed), 0, 1);
predictionWeight     = flickIntensity * flickBoost;

predictedPos = mouse + velocity * lookahead;
if (distance(predictedPos, target) < distanceToTarget)
    predictionWeight += flickBoost * 0.25;
```

**Natural movement preserve** (`computeNaturalMovementPreserve`):

```cpp
velocityContinuity  = dot(normalize(velocity), normalize(previousVelocity));
smoothMovementBlend = clamp((velocityContinuity + 1) * 0.5, 0, 1);
speedBlend          = clamp(speed / 220, 0, 1);
return smoothMovementBlend * speedBlend;
```

**Combined pull**:

```cpp
combinedPull = (circleSuggestionWeight * circleSuggestionInfluence
              + predictionWeight       * predictionInfluence)
              * magnetStrength;
combinedPull *= 1 - naturalPreserve * naturalMovementInfluence;
combinedPull  = clamp(combinedPull, 0, maxPull);

desiredOffset = normalize(target - mouse) * combinedPull * distanceToTarget;
```

**Radial release** (moving away from target inside zone):

```cpp
releaseAmount   = radialAwayMovement * (distance / fov) * releaseStrength;
desiredOffset   = mix(desiredOffset, vec2(0), releaseAmount);
```

**Friction** (tangential swipe — reduces offset blend rate):

```cpp
if (radialAwayMovement < 0.3)
    offsetBlendRate *= 1 - (1 - radialAwayMovement) * friction;
```

---

## CircleGuardSnapping Module

Live port of [circlecore `Investigations.snaps](https://github.com/circleguard/circlecore/blob/master/circleguard/investigations.py)`, without beatmap / hitobject filtering.

### Location

`Engine/CircleGuardSnapping/CircleGuardSnapping.hpp`  
`Engine/CircleGuardSnapping/CircleGuardSnapping.cpp`

### Sample buffer

```cpp
std::deque<SnapSample> samples_;  // rolling window of 3 positions
void addSample(const glm::vec2& position, float timeSeconds);
```

### Angle at B for triple (A, B, C)

```cpp
cosBeta = -(AC² - AB² - BC²) / (2 * AB * BC);
cosBeta = clamp(cosBeta, -1, 1);
angleDegrees = degrees(acos(cosBeta));
```

### Snap candidate

```cpp
minLeg = min(AB, BC);
isSnap = (minLeg > minDistancePixels) && (angleDegrees < maxAngleDegrees);
```

Defaults match CircleGuard: **10°** max angle, **8 px** min leg distance.

### Log output

`rebuildLogLines()` populates `logLines_`, rendered by `Visuals::drawSnapDetectionLog()` via `ImDrawList::AddText`.

---

## Settings Reference

### `Assist::Settings` — `Modules/Assist/Assist.hpp`


| Field                       | Default | Description                      |
| --------------------------- | ------- | -------------------------------- |
| `fov`                       | `200.f` | Hard assist boundary (px)        |
| `circleSize`                | `69.f`  | Soft target size suggestion (px) |
| `magnetStrength`            | `2.45f` | Global pull scale                |
| `maxPull`                   | `0.35f` | Max pull fraction per frame      |
| `circleSuggestionInfluence` | `0.4f`  | Circle hint weight               |
| `predictionInfluence`       | `0.65f` | Prediction weight                |
| `naturalMovementInfluence`  | `0.55f` | Natural movement damping         |
| `friction`                  | `0.45f` | Tangential swipe stickiness      |
| `lookahead`                 | `0.1f`  | Prediction horizon (seconds)     |
| `flickBoost`                | `0.22f` | Flick toward target boost        |
| `minSwipeSpeed`             | `80.f`  | Min speed for swipe logic (px/s) |
| `smoothing`                 | `8.f`   | In-zone offset follow rate       |
| `syncSpeed`                 | `5.f`   | Out-of-zone resync rate          |
| `releaseStrength`           | `0.85f` | Radial away release factor       |


Access at runtime:

```cpp
globals.assist->settings().fov = 150.f;
```

### `CircleGuardSnapSettings` — `Engine/CircleGuardSnapping/CircleGuardSnapping.hpp`


| Field               | Default | Description                    |
| ------------------- | ------- | ------------------------------ |
| `maxAngleDegrees`   | `10.f`  | Snap angle threshold           |
| `minDistancePixels` | `8.f`   | Minimum leg length             |
| `maxLogEntries`     | `14`    | Snap events shown in log panel |


---

## Build Instructions

**Requirements**

- Windows 10/11
- Visual Studio 2022 with "Desktop development with C++"
- Windows 10 SDK

**Steps**

1. Open `Algorithm/Algorithm/Algorithm.sln`
2. Configuration: **Debug** or **Release**
3. Platform: **x64**
4. Build → Run

**Output:** `Algorithm/Algorithm/x64/Debug/Algorithm.exe`

**Linked libraries:** `d3d11.lib`, `dxgi.lib`, `d3dcompiler.lib`

---

## License & Attribution

Copyright (c) 2026 ToldByNun. All rights reserved.

This source code is provided for **educational and reference purposes only**. You may view and study the code, but you may not copy, modify, redistribute, or use any part of it in other projects without prior written permission.

**Third-party references**

- [CircleGuard / circlecore](https://github.com/circleguard/circlecore) — snap detection algorithm
- [Dear ImGui](https://github.com/ocornut/imgui)
- [GLM](https://github.com/g-truc/glm)

