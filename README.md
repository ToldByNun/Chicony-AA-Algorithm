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

You move your mouse (**orange**). The program also computes an **assisted cursor position** (**blue**). Blue is not a hard snap to the center — it is a **filtered offset** applied on top of your real cursor.

The core model is:

```
output position = real mouse position + assist offset
```

The offset is **persistent**: it can build up while you move, freeze when you stop, and decay only while you are actively moving (above a minimum speed). This avoids the “smoothness keeps playing out after you stop” feeling.

This project is a **visualizer only**. It runs a fullscreen black overlay, reads your real cursor, runs the algorithm, and draws the result. It does **not** hook games or inject input.

---

## Quick Start

1. Open `Algorithm/Algorithm/Algorithm.sln` in **Visual Studio 2022**
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
| Light red circle | Light red       | **circleSize** — assist area radius (not a center point) |
| Top-right panel  | Gray / red text | Live **CircleGuard snapping** analysis                  |

When orange and blue overlap completely, assist offset is effectively zero.

---

## Core Concepts

### Offset model (not center snap)

The target is treated as a **circle** with radius `circleSize`, not a single center point. Assist pulls toward the **nearest point on that circle’s edge**, along the radial line from the screen center through your cursor.

```
assist target on radius = center + normalize(mouse − center) × circleSize
correction vector       = assist target on radius − mouse position
```

The blue cursor is always:

```
blue = orange + offset
```

When orange moves, blue moves with it. The **offset** (blue − orange) is what assist controls.

### Activation ring

Between `circleSize` and `FOV` lies the **activation ring**. Assist is strongest near the inner edge (`circleSize`) and ramps down toward the outer edge (`FOV`). Inside the `circleSize` disk, distance-based pull is off — offset only decays.

### Movement gating

Assist only **updates** the offset while the cursor moves fast enough (`minMovementSpeed`). If you stop:

- The offset **freezes** in place (no further smoothing, no decay)
- Blue stays at the same relative distance from orange

The same rule applies when leaving FOV: offset **decays back to zero only while you are moving**. If you stop outside FOV, the offset freezes.

### Friction (sticky swipe)

When you swipe **tangentially** across the assist area, offset updates are slowed. Orange keeps moving but blue can lag briefly — a wider effective window during fast crosses.

### Prediction

The algorithm looks at **where the mouse is going**, not just where it is:

```
predicted position = current position + velocity × lookahead
```

Fast flicks **toward** the assist area add extra correction strength. Smooth natural movement reduces it — unless you are clearly flicking toward the assist target.

### Strength vs pixel caps

These are separate knobs:

| Knob                 | What it controls                                              |
| -------------------- | ------------------------------------------------------------- |
| `assistStrength`     | How hard assist pulls (scales correction strength directly)   |
| `maxOffsetFraction`  | Optional cap: max offset as a fraction of distance to edge    |
| `maxOffsetPixels`    | Optional hard cap: max offset length in pixels (0 = disabled) |

`assistStrength` changes how strong the pull **feels**. Pixel caps limit how far blue can drift from orange **without** changing the strength calculation itself.

---

## FOV vs circleSize

| Term           | Role                          | Visual                         |
| -------------- | ----------------------------- | ------------------------------ |
| **FOV**        | Hard outer boundary           | Large red circle               |
| **circleSize** | Assist area radius            | Light red circle               |

```
        ┌──────────── FOV ────────────┐
        │   activation ring         │
        │  ┌── circleSize ──┐       │
        │  │  (no distance  │       │
        │  │   pull inside) │       │
        │  └────────────────┘       │
        └───────────────────────────┘
```

- **Outside FOV** — assist inactive; offset decays only while moving
- **Activation ring** (`circleSize` < distance ≤ `FOV`) — distance-weighted pull toward circle edge
- **Inside circleSize disk** — no distance pull; offset decays toward zero while moving

---

## How the Assist Works Each Frame

### Step 0 — Velocity

```
velocity     = (mousePosition − previousMousePosition) / deltaTime
mouseSpeed   = |velocity|
distance     = |mousePosition − targetCenter|
insideFOV    = distance ≤ fov
insideDisk   = distance ≤ circleSize
```

### Step 1 — Outside FOV

```
if mouseSpeed < minMovementSpeed:
    offset = previousOffset          // freeze
else:
    offset = lerp(previousOffset, 0, offsetDecay × dt)

output = mousePosition + offset
fovEntryBlend = 0
```

### Step 2 — Inside FOV, no movement

```
if mouseSpeed < minMovementSpeed:
    output = mousePosition + previousOffset    // freeze, no smoothing continuation
```

### Step 3 — FOV entry ramp

While moving inside FOV, assist strength ramps in gradually:

```
inwardSpeed   = dot(velocity, directionTowardCenter)   // clamped ≥ 0
velocityBoost = clamp(inwardSpeed / fovEntryVelocityReference, 0, fovEntryVelocityBoost)
entryRate     = fovEntryRamp × (1 + velocityBoost) × dt

if mouseSpeed ≥ minMovementSpeed:
    fovEntryBlend = clamp(fovEntryBlend + entryRate, 0, 1)
```

When stationary, `fovEntryBlend` is left unchanged (frozen).

### Step 4 — Compute desired offset

**Assist target on radius:**

```
if insideDisk:
    assistTarget = mousePosition
else:
    assistTarget = targetCenter + normalize(mouse − center) × circleSize

correctionVector   = assistTarget − mousePosition
correctionDistance = |correctionVector|
```

**Distance weight** (stronger toward inner edge of activation ring):

```
ringWidth  = fov − circleSize
depth      = 1 − clamp((distance − circleSize) / ringWidth, 0, 1)
distanceWeight = smoothstep(depth)        // 0 at FOV edge, 1 at circleSize edge
```

**Prediction weight** (flicks toward assist area):

```
if mouseSpeed > minSwipeSpeed:
    movementToward = dot(normalize(velocity), normalize(correctionVector))
    flickIntensity = clamp(movementToward × (speed / minSwipeSpeed), 0, 1)
    predictionWeight = flickIntensity × flickBoost

    predictedPos = mouse + velocity × lookahead
    if distance(predictedPos, center) < distance:
        predictionWeight += flickBoost × 0.25
```

**Correction strength:**

```
correctionStrength = distanceWeight × assistStrength
                   + predictionWeight × predictionInfluence

naturalDampen = naturalMovementPreserve × naturalMovementInfluence
flickBlend    = clamp(predictionWeight / flickBoost, 0, 1)
correctionStrength ×= 1 − naturalDampen × (1 − flickBlend)
correctionStrength ×= fovEntryBlend
```

**Desired offset and caps:**

```
desiredOffset = normalize(correctionVector) × correctionDistance × correctionStrength

if maxOffsetFraction > 0:
    desiredOffset = clamp length to correctionDistance × maxOffsetFraction

if maxOffsetPixels > 0:
    desiredOffset = clamp length to maxOffsetPixels

if moving radially away from center:
    desiredOffset = lerp(desiredOffset, 0, releaseAmount)
```

### Step 5 — Blend offset over time

```
updateRate = offsetSmoothing × dt
updateRate ×= radiusBrakeFactor
updateRate ×= fovEntryBlend

if mouseSpeed > minSwipeSpeed:
    updateRate ×= clamp(speed / minSwipeSpeed, 1, 3)    // faster movement = faster catch-up

if tangential swipe:
    updateRate ×= 1 − tangentialAmount × friction

blendedOffset = lerp(previousOffset, desiredOffset, updateRate)
output        = mousePosition + blendedOffset
```

---

## CircleGuard Snapping (Live Analysis)

[CircleGuard](https://github.com/circleguard/circleguard) analyzes osu! replays for suspicious snap movement (aim correction). This project ports the **core detection idea** for live visualization.

### Plain explanation

Take **3 consecutive mouse positions** A → B → C. Look at the **angle at point B**:

- Straight motion → large angle (~180°)
- Sharp kink → very small angle

A snap is flagged when:

1. Angle at B is very acute (< **10°** by default)
2. Both segments |AB| and |BC| are long enough (> **8 px** by default)

### Math (law of cosines)

For triangle A-B-C with angle β at B:

```
cos(β) = (|AB|² + |BC|² − |AC|²) / (2 × |AB| × |BC|)
β (degrees) = arccos(cos(β)) × 180/π
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

All values live in `Assist::Settings` (`Modules/Assist/Assist.hpp`).

### Boundaries

| Setting      | What it does                                      |
| ------------ | ------------------------------------------------- |
| `fov`        | Outer assist boundary (large red ring)            |
| `circleSize` | Assist area radius — pull target is this circle   |

### Strength and caps

| Setting             | What it does                                                       |
| ------------------- | ------------------------------------------------------------------ |
| `assistStrength`    | Global pull scale (1.0 = full pull toward circle edge at max weight) |
| `maxOffsetFraction` | Max offset as fraction of distance to circle edge (0 = disabled)   |
| `maxOffsetPixels`   | Hard pixel cap on offset length (0 = disabled)                     |

### Prediction and natural movement

| Setting                    | What it does                                           |
| -------------------------- | ------------------------------------------------------ |
| `predictionInfluence`      | How much flick / prediction adds to strength           |
| `naturalMovementInfluence` | How much smooth hand motion reduces strength           |
| `lookahead`                | Prediction horizon in seconds                          |
| `flickBoost`               | Base flick-toward-assist boost                         |

### FOV entry

| Setting                      | What it does                                    |
| ---------------------------- | ----------------------------------------------- |
| `fovEntryRamp`               | How fast assist ramps in after entering FOV     |
| `fovEntryVelocityBoost`      | Extra ramp from inward velocity (0–0.35 typical) |
| `fovEntryVelocityReference`  | Inward speed (px/s) that gives full velocity boost |

### Movement thresholds

| Setting            | What it does                                              |
| ------------------ | --------------------------------------------------------- |
| `minMovementSpeed` | Below this speed, offset freezes (no update, no decay)    |
| `minSwipeSpeed`    | Below this speed, flick / friction / release logic is off |

### Offset dynamics

| Setting             | What it does                                              |
| ------------------- | --------------------------------------------------------- |
| `offsetSmoothing`   | How fast offset blends toward desired (higher = snappier)   |
| `offsetDecay`       | Decay rate outside FOV or when assist inactive (while moving) |
| `insideOffsetDecay` | Decay rate inside the circleSize disk (while moving)      |
| `offsetRelease`     | How strongly radial away movement releases offset         |
| `friction`          | Tangential swipe stickiness (slows offset updates)        |
| `radiusBrakeStrength` | Slows offset updates near the circleSize edge           |

Example tweak in `main.cpp` after `classManager.init()`:

```cpp
auto& s = globals.assist->settings();
s.fov = 200.f;
s.circleSize = 69.f;
s.assistStrength = 1.5f;
s.maxOffsetPixels = 40.f;   // hard cap at 40 px
s.offsetSmoothing = 6.f;
s.minMovementSpeed = 25.f;
```

---

## How to Test

1. **Far from target** — orange and blue should overlap (outside FOV or zero offset)
2. **Slow approach in activation ring** — blue separates toward circle edge; strength grows as you move inward
3. **Stop moving** — blue freezes at current offset; no further smoothing or decay
4. **Change `assistStrength`** — visible difference in how far blue drifts from orange
5. **Set `maxOffsetPixels`** — blue never exceeds that pixel distance from orange
6. **Fast flick toward center** — blue catches up faster (speed-boosted smoothing + prediction)
7. **Fast tangential swipe** — blue lags briefly near the assist area (friction)
8. **Move radially away while inside FOV** — offset releases early via `offsetRelease`
9. **Leave FOV while moving** — offset decays toward zero
10. **Leave FOV and stop** — offset freezes (same as inside FOV)
11. **Sharp kink in movement** — log may show `status: SNAP`

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
| Entry     | `Algorithm/Algorithm/Algorithm.sln`             |

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
| `Assist`              | `process(mouse, target, dt)` → assisted cursor position         |
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
float     fovEntryBlend_;      // 0..1 temporal ramp on FOV entry
bool      hasFrameHistory_;
```

### Offset model

```cpp
previousOffset = previousAssistPosition − previousMousePosition;
desiredOffset  = computeDesiredOffset(context);
updateRate     = computeOffsetUpdateRate(context, correctionStrength);
blendedOffset  = mix(previousOffset, desiredOffset, updateRate);
output         = mousePosition + blendedOffset;
```

Movement gating applies before blending:

```cpp
if (mouseSpeed < minMovementSpeed)
    output = mousePosition + previousOffset;   // freeze, skip blend
```

### Process flow

```
process(mouse, target, dt)
│
├─ first frame → output = mouse, init history
│
├─ outside FOV
│    ├─ moving  → offset = decay(previousOffset, offsetDecay)
│    └─ stopped → offset = previousOffset (frozen)
│
└─ inside FOV
     ├─ ramp fovEntryBlend while moving
     ├─ stopped → output = mouse + previousOffset (frozen)
     ├─ !shouldApplyAssist → decay offset while moving
     └─ active assist
          ├─ desiredOffset = computeDesiredOffset()
          ├─ updateRate    = computeOffsetUpdateRate()
          └─ output = mouse + mix(previousOffset, desiredOffset, updateRate)
```

### Assist target on radius

```cpp
glm::vec2 computeAssistTargetOnRadius(context) {
    if (insideAssistDisk)
        return mousePosition;

    direction = normalize(mousePosition − targetCenter);
    return targetCenter + direction × circleSize;
}
```

### Distance weight

Inverted falloff — strongest at `circleSize`, weakest at `FOV`:

```cpp
distanceBeyondDisk = distanceToCenter − circleSize;
activationRingWidth = fov − circleSize;
depthIntoRing = 1 − clamp(distanceBeyondDisk / activationRingWidth, 0, 1);
distanceWeight = smoothstep(depthIntoRing);
// smoothstep(t) = t² × (3 − 2t)
```

### Prediction weight

```cpp
directionToAssist = assistTargetOnRadius − mousePosition;
movementToward    = dot(normalize(velocity), normalize(directionToAssist));

if (speed ≤ minSwipeSpeed || movementToward ≤ 0) → 0

flickIntensity    = clamp(movementToward × (speed / minSwipeSpeed), 0, 1);
predictionWeight  = flickIntensity × flickBoost;

predictedPos = mouse + velocity × lookahead;
if (distance(predictedPos, center) < distanceToCenter)
    predictionWeight += flickBoost × 0.25;
```

### Natural movement preserve

```cpp
velocityContinuity  = dot(normalize(velocity), normalize(previousVelocity));
smoothMovementBlend = clamp((velocityContinuity + 1) × 0.5, 0, 1);
speedBlend          = clamp(speed / 220, 0, 1);
naturalPreserve     = smoothMovementBlend × speedBlend;
```

Applied with flick exception:

```cpp
naturalDampen  = naturalPreserve × naturalMovementInfluence;
flickBlend     = clamp(predictionWeight / flickBoost, 0, 1);
correctionStrength ×= 1 − naturalDampen × (1 − flickBlend);
```

### Correction strength and desired offset

```cpp
correctionStrength = distanceWeight × assistStrength
                   + predictionWeight × predictionInfluence;
correctionStrength ×= fovEntryBlend;

desiredOffset = normalize(correctionVector) × correctionDistance × correctionStrength;

// Optional caps (applied to offset magnitude, not strength):
desiredOffset = capOffsetByFractionOfDistance(desiredOffset, correctionDistance);
desiredOffset = clampOffsetToMaxPixels(desiredOffset);
```

### Radial release

```cpp
radialAway = max(dot(normalize(velocity), awayDirection), 0);
if (radialAway > 0.1 && speed > minSwipeSpeed)
    releaseAmount = radialAway × (distance / fov) × offsetRelease;
    desiredOffset = mix(desiredOffset, vec2(0), releaseAmount);
```

### Offset update rate

```cpp
updateRate = clamp(offsetSmoothing × dt, 0, 1);
updateRate ×= radiusBrakeFactor;     // slower near circleSize edge
updateRate ×= fovEntryBlend;

if (speed > minSwipeSpeed)
    updateRate ×= clamp(speed / minSwipeSpeed, 1, 3);

// Friction on tangential swipes:
if (radialAway < 0.3)
    updateRate ×= 1 − (1 − radialAway) × friction;
```

### Radius brake factor

```cpp
if (insideAssistDisk) → radiusBrakeStrength;

depthInRing = clamp((distance − circleSize) / (fov − circleSize), 0, 1);
return radiusBrakeStrength + depthInRing × (1 − radiusBrakeStrength);
```

### Decay helpers

```cpp
decayOffset(context, rate):
    return mix(previousOffset, vec2(0), clamp(rate × dt, 0, 1))

capOffsetByFractionOfDistance(offset, correctionDistance):
    cap = correctionDistance × maxOffsetFraction
    clamp |offset| to cap

clampOffsetToMaxPixels(offset):
    clamp |offset| to maxOffsetPixels
```

---

## CircleGuardSnapping Module

Live port of [circlecore `Investigations.snaps`](https://github.com/circleguard/circlecore/blob/master/circleguard/investigations.py), without beatmap / hitobject filtering.

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
cosBeta = -(AC² − AB² − BC²) / (2 × AB × BC);
cosBeta = clamp(cosBeta, −1, 1);
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

| Field                       | Default | Description                                              |
| --------------------------- | ------- | -------------------------------------------------------- |
| `fov`                       | `200.f` | Hard assist boundary (px)                                |
| `circleSize`                | `69.f`  | Assist area radius (px)                                  |
| `assistStrength`            | `1.5f`  | Global correction strength scale                         |
| `maxOffsetFraction`         | `0.f`   | Max offset as fraction of distance to edge (0 = off)     |
| `maxOffsetPixels`           | `0.f`   | Hard offset cap in pixels (0 = off)                      |
| `predictionInfluence`       | `0.6f`  | Prediction contribution to strength                      |
| `naturalMovementInfluence`  | `0.55f` | Natural movement damping                                 |
| `radiusBrakeStrength`       | `0.65f` | Offset update brake near circleSize edge                 |
| `fovEntryRamp`              | `3.f`   | FOV entry ramp speed                                     |
| `fovEntryVelocityBoost`     | `0.35f` | Max extra entry ramp from inward velocity                |
| `fovEntryVelocityReference` | `300.f` | Inward speed for full velocity boost (px/s)              |
| `lookahead`                 | `0.1f`  | Prediction horizon (seconds)                             |
| `flickBoost`                | `0.25f` | Flick-toward-assist boost                                |
| `minMovementSpeed`          | `25.f`  | Min speed to update/decay offset (px/s)                  |
| `minSwipeSpeed`             | `80.f`  | Min speed for flick/friction/release (px/s)              |
| `offsetSmoothing`           | `6.f`   | Offset blend rate toward desired                         |
| `offsetDecay`               | `5.f`   | Decay rate outside FOV / inactive (while moving)         |
| `insideOffsetDecay`         | `4.f`   | Decay rate inside circleSize disk (while moving)         |
| `offsetRelease`             | `0.85f` | Radial away release factor                               |
| `friction`                  | `0.45f` | Tangential swipe stickiness                              |

Access at runtime:

```cpp
globals.assist->settings().assistStrength = 1.2f;
globals.assist->settings().maxOffsetPixels = 35.f;
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
