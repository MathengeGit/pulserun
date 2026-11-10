# CLAUDE.md — PulseRun Handoff Guide

> **For Claude Code only.** Read this entire file before writing a single line of code.
> Local path: `C:\dev\pulserun`
> GitHub repo: `https://github.com/MathengeGit/pulserun`

---

## 1. What this project is

PulseRun is a **2D level-based obstacle platformer** (Geometry Dash style) written in **C++17 with no external graphics library**. Every pixel on screen is produced by graphics algorithms the team implemented from scratch: DDA/Bresenham lines, midpoint circles/ellipses, polygon scanline fill, Cohen-Sutherland clipping, cubic Bézier and Catmull-Rom curves.

This is a **Computer Graphics university semester project** (Strathmore University). The graders care not just that the game works, but that the graphics pipeline is hand-rolled.

---

## 2. Current state — what is done

**Checkpoint 1 is complete.** Do not re-implement anything already done.

| Area | Status | Location |
|------|--------|----------|
| Framebuffer (pixel array + BMP/PNG export) | ✅ Done | `src/core/framebuffer.cpp` |
| DDA, Bresenham lines (all 8 octants), dashed, thick | ✅ Done | `src/raster/line.cpp` |
| Midpoint circle, ellipse, arcs, filled variants | ✅ Done | `src/raster/circle.cpp` |
| Polygon fill (edge table, AET, even-odd), point-in-polygon | ✅ Done | `src/raster/polygon.cpp` |
| Cohen-Sutherland + Sutherland-Hodgman clipping | ✅ Done | `src/raster/clip.cpp` |
| Cubic Bézier (De Casteljau), Catmull-Rom spline | ✅ Done | `src/raster/curve.cpp` |
| Alpha blending, gradients, glow layers | ✅ Done | `src/core/` |
| 5×7 bitmap font | ✅ Done | `src/raster/font.cpp` |
| Game sprites: cube, spikes, blocks, saws, orbs, portals | ✅ Done | `src/game/sprites.cpp` |
| UI: HUD, buttons, level cards, logo | ✅ Done | `src/ui/ui.cpp` |
| Demo scenes: gameplay, main menu, level select, death | ✅ Done | `demos/` |
| 84 automated algorithm tests | ✅ Done | `tests/` |
| `vec2` math type | ✅ Done | `src/math/vec2.h` |

**Output is currently offline**: demos render to PNG files. There is no window, no keyboard input, no game loop, no physics, and no level loading yet.

---

## 3. Repository layout

```
src/
  core/        colour, framebuffer, BMP/PNG writers
  math/        vec2.h  ← mat3 goes here next
  raster/      line, circle, polygon, clip, curve, font
  game/        sprites (cube, spikes, saws, orbs, portals, backdrop)
  ui/          HUD, buttons, level cards, logo
demos/         one .cpp per technique + four game-style scenes
tests/         test_algorithms.cpp (84 checks)
levels/        level1.txt (to be created)
docs/screenshots/  rendered PNGs used as evidence
CMakeLists.txt
```

---

## 4. Build system

```bash
# Windows (MSYS2 UCRT64 — the team's primary environment)
pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja

cmake -S . -B build -G Ninja
cmake --build build

# Run tests
./build/test_algorithms          # Windows: .\build\test_algorithms.exe

# Render all demo screenshots
cmake --build build --target screenshots

# Run a single demo
./build/demo_scenes docs/screenshots
```

After every change, run `test_algorithms` and confirm all 84 checks pass before committing.

---

## 5. Team — who owns what

| Member | GitHub username | Responsibility |
|--------|----------------|----------------|
| **Wachira Njoroge** (Dior) | DiorWachira | Graphics engine, mat3 transforms |
| **Mathenge Gitahi** | MathengeGit | C++ foundation, animation, visual FX |
| **Stacy Akinyi** | — | Gameplay physics & collision |
| **Marie Hoppe Solini** | — | Level design, file format & loader |
| **Diana Mucheke** | — | UI engine, pause/level-complete screens |
| **Anise Wambui Kimani** | — | Testing & documentation |

**Commit identity (overrides anything else in this file):** teammates commit and push from their own machines under their own names, which is expected. Claude must never appear as a contributor. Every commit and PR from this machine is authored as **DiorWachira** `<njoroge.wachira@strathmore.edu>`, the existing global git identity, whoever the task is assigned to. Do **not** run `git config user.name` / `user.email` to impersonate other team members, and never add "Co-Authored-By: Claude", "Generated with Claude Code" or any other AI attribution to commits, PR titles or PR bodies. The assignee column above is about ownership; credit the assignee in the issue title (e.g. `[Stacy] ...`), not in the git author.

---

## 6. GitHub workflow

Milestones already exist on the repo:
- **Milestone 1** — Checkpoint 2 — Transforms, Window & Game Loop
- **Milestone 2** — Checkpoint 3 — Physics, Collision & Level Loading
- **Milestone 3** — Checkpoint 4 — Animation, Effects & Final Polish

**Issue already created:**
- #1 — `[Wachira] Implement mat3 2D transform class` (Milestone 1)

### For every piece of work:

1. **Create a GitHub issue** if one doesn't exist yet:
   ```bash
   gh issue create --title "[Name] Short description" \
     --body "Task, acceptance criteria, branch name" \
     --milestone 1   # 1=transforms, 2=physics, 3=animation
   ```

2. **Create a feature branch** off `main`:
   ```bash
   git checkout -b feature/short-name
   ```

3. **Work in small commits** — commit after each logical chunk, not at the end.

4. **Open a pull request** that closes the issue:
   ```bash
   gh pr create \
     --title "feat: short description" \
     --body "Closes #N\n\n## Changes\n- ...\n\n## Test results\nAll 84 tests pass."
   ```
   Do **not** include any AI attribution lines in the PR body.

5. **Merge to `main`** after the PR is reviewed (or self-merge for now).

### Branch naming convention
```
feature/<short-name>     # new functionality
fix/<short-name>         # bug fixes
docs/<short-name>        # documentation only
test/<short-name>        # tests only
```

---

## 7. What to build next — ordered priority

Work through these in order. Each item maps to a Milestone.

### MILESTONE 1 — Checkpoint 2: Transforms, Window & Game Loop

#### Task 1 (Issue #1): mat3 2D transform class  `[Wachira]`
**Branch:** `feature/mat3-transform`

Create `src/math/mat3.h` and `src/math/mat3.cpp`:

```cpp
// mat3 — column-major 3x3 for 2D homogeneous transforms
struct mat3 {
    float m[3][3];

    static mat3 identity();
    static mat3 translate(float dx, float dy);
    static mat3 rotate(float radians);
    static mat3 scale(float sx, float sy);

    mat3 operator*(const mat3& other) const;
    vec2 operator*(const vec2& v) const;   // promote v to (x,y,1)
    mat3 inverse() const;
};
```

Add at least 6 tests in `tests/test_algorithms.cpp`:
- `identity() * vec2` leaves the vector unchanged
- `translate(5, -3) * vec2(0,0)` → `(5,-3)`
- `rotate(π/2) * vec2(1,0)` ≈ `(0,1)`
- Chained `translate * rotate * scale` matches expected output
- `m * m.inverse()` ≈ identity (within 1e-5)
- Transform applied to a sprite vertex matches a hand-computed value

PR title: `feat: add mat3 2D homogeneous transform class`  
Closes: #1

---

#### Task 2: Real-time window using SDL2  `[Mathenge]`
**Branch:** `feature/sdl2-window`

Create a new GitHub issue first:
```bash
gh issue create --title "[Mathenge] Add real-time window and render loop (SDL2)" \
  --body "Opens a 1280x720 window, blits Framebuffer pixels each frame, closes on X button. Gate behind PULSERUN_HEADLESS flag for headless CI." \
  --milestone 1
```

Add `find_package(SDL2)` to `CMakeLists.txt`. Create `src/core/window.h` / `window.cpp`:

```cpp
class Window {
public:
    Window(int w, int h, const char* title);
    ~Window();
    bool poll_events();          // returns false if window closed
    void present(const Framebuffer& fb);
};
```

The main entry point (`demos/game_main.cpp` or similar) becomes:
```cpp
Window win(1280, 720, "PulseRun");
while (win.poll_events()) {
    // draw frame into fb
    win.present(fb);
}
```

If SDL2 is unavailable in CI, `#ifdef PULSERUN_HEADLESS` exports PNG instead.

---

#### Task 3: Fixed-timestep game loop  `[Mathenge]`
**Branch:** `feature/game-loop`  
Issue: Create via `gh issue create`

Add `src/game/game_loop.h`:
```cpp
class GameLoop {
    static constexpr double FIXED_DT = 1.0 / 60.0;   // 60 Hz physics
public:
    void run(Window& win, GameState& state);
    // internally: accumulates real time, runs fixed update steps,
    // renders once per display frame with interpolation alpha
};
```

---

#### Task 4: Camera scrolling  `[Wachira]`
**Branch:** `feature/camera-scroll`  
Issue: Create via `gh issue create`

Add a `Camera` struct that holds a world-to-screen `mat3`. Pass it to all draw calls:
```cpp
struct Camera {
    float scroll_x = 0.0f;
    mat3  world_to_screen() const;  // returns translate(-scroll_x, 0)
};
```

The player's x position advances the scroll. Apply `camera.world_to_screen() * vertex` before every rasterisation call.

---

### MILESTONE 2 — Checkpoint 3: Physics, Collision & Level Loading

#### Task 5: Player physics  `[Stacy]`
**Branch:** `feature/player-physics`

```cpp
struct Player {
    vec2 pos;
    vec2 vel;
    bool on_ground = false;

    void update(float dt);   // gravity = 1200 px/s², terminal vel = 900 px/s
    void jump();             // vel.y = -600 px/s if on_ground
};
```
Hold Space = hold jump (variable height: cap at 350 ms).

---

#### Task 6: Collision detection  `[Stacy]`
**Branch:** `feature/collision`

Extend `src/raster/polygon.cpp`'s existing `pointInPolygon` (ray-casting, already implemented) into a full hitbox system:

```cpp
bool aabb_overlap(const AABB& a, const AABB& b);             // broad phase
bool polygon_overlap(const Polygon& player, const Polygon& obstacle);  // narrow phase
CollisionResult resolve(Player& player, const std::vector<Obstacle>& obs);
```

A player touching any spike polygon → death. A player touching the top face of a block polygon → `on_ground = true`.

---

#### Task 7: Level file format & loader  `[Marie]`
**Branch:** `feature/level-loader`

Extend `src/game/level.h`. Level files in `levels/` use this plain-text format:
```
# PulseRun Level 1
width 10000
ground_y 500
bg_color 0x0d0d1a

BLOCK   200  450  60  50
SPIKE   400  450
SAW     700  430  radius=20  speed=1.5
ORB     900  350
PORTAL  9800 400  target=2
```

Loader parses the file, constructs `std::vector<Obstacle>` with correct sprite types.

---

#### Task 8: Author Level 1  `[Marie]`
**Branch:** `feature/level1`  

Create `levels/level1.txt` — a ~10 000-pixel-wide level with a gradual difficulty curve:
- Spikes at 400, 800, 1200 (single, then double, then triple gaps)
- Rotating saws at 2000, 3500
- Jump orbs at 1600, 2800 to introduce the mechanic
- Portal at 9800 to reach Level 2 (which can be a stub for now)

---

### MILESTONE 3 — Checkpoint 4: Animation, Effects & Final Polish

#### Task 9: Sprite animation system  `[Mathenge]`
**Branch:** `feature/animation`

Add `src/game/animation.h`:
```cpp
struct Animation {
    std::vector<std::function<void(Framebuffer&, vec2)>> frames;
    float fps;
    float elapsed = 0;
    int   current = 0;
    void update(float dt);
    void draw(Framebuffer& fb, vec2 pos);
};
```
Animate:
- Cube face rotation (already drawn statically — drive it with the physics elapsed time)
- Death burst (5-frame shockwave expanding from player position)
- Jump pad spring compression

---

#### Task 10: Pause menu & level-complete screen  `[Diana]`
**Branch:** `feature/pause-and-complete`

Reuse `src/ui/ui.cpp`. On ESC keypress → overlay a pause menu with:
- "Resume" button
- "Restart" button
- "Main Menu" button

On reaching the portal → level-complete overlay with:
- "LEVEL COMPLETE" text (5×7 bitmap font, big scale)
- Attempt count, best-time display
- "Next Level" and "Main Menu" buttons

---

#### Task 11: Death & retry screen  `[Diana]`
**Branch:** `feature/death-screen`

The death scene PNG already exists (`docs/screenshots/04_death_scene.png`). Wire it into the game loop:
- On death → play shockwave animation (5 frames at 15fps)
- Then display "X% complete" text and a "RETRY" button
- Retry resets player position and scroll, restarts the fixed-timestep loop

---

#### Task 12: Extend test suite  `[Anise]`
**Branch:** `test/extended-coverage`

Add tests for every new system in `tests/test_algorithms.cpp`:
- mat3: 6+ tests (above)
- Collision: AABB broad phase, narrow phase polygon overlap, death detection
- Level loader: parse a 3-line test file, assert obstacle positions
- Physics: gravity integration, variable jump timing
- Target: ≥ 120 total tests

---

#### Task 13: Final documentation  `[Anise]`
**Branch:** `docs/final-readme`

Update the README:
- Add a "How to play" section (Space = jump, hold for higher jump)
- Document the new level file format
- Update the screenshots section with the final gameplay video/GIFs
- Add a "What we learned" section per grader requirements

---

## 8. Code style — match existing patterns

- **No external graphics library** — all shapes go through `Framebuffer` pixel-by-pixel
- C++17, STL types allowed (`std::vector`, `std::string`, `std::function`, etc.)
- Header-only structs for small POD types (`vec2`, `mat3`, `AABB`)
- Implementation in matching `.cpp` file for anything with a non-trivial body
- No exceptions (`-fno-exceptions` is fine), use return codes / booleans
- `Colour` uses `uint8_t r, g, b, a`; use `alpha_blend()` from `src/core/`
- Keep file sizes small — split into logical units rather than one monster file
- `const` on everything that doesn't mutate

---

## 9. Commit message format

```
type: short imperative description (≤72 chars)

Body: what changed and why (optional for small commits)

Closes #N   ← always link to the GitHub issue
```

Types: `feat`, `fix`, `test`, `docs`, `refactor`, `chore`

Example:
```
feat: add mat3 2D homogeneous transform class

Implements translate, rotate, scale factories plus matrix×vec2 multiply.
Adds 6 tests in test_algorithms.cpp; all 84+6 checks pass.

Closes #1
```

Do **not** include any tool or AI attribution. Commits made from this machine appear under DiorWachira; teammates' commits carry their own names (see §5).

---

## 10. Quick-start for Claude Code

```bash
# 1. Clone
git clone https://github.com/MathengeGit/pulserun.git C:\dev\pulserun
cd C:\dev\pulserun

# 2. Build and verify everything passes
cmake -S . -B build -G Ninja
cmake --build build
./build/test_algorithms      # expect: 84 tests PASSED

# 3. Confirm the author is DiorWachira (global identity; do not override per task)
git config user.name     # expect: DiorWachira
git config user.email    # expect: njoroge.wachira@strathmore.edu

# 4. Create the branch for the first task
git checkout -b feature/mat3-transform

# 5. Implement, then run tests again, then commit
cmake --build build && ./build/test_algorithms

# 6. Open a PR that closes the issue
gh auth login       # authenticate once
gh pr create \
  --title "feat: add mat3 2D homogeneous transform class" \
  --body "$(cat <<'EOF'
## Changes
- Added `src/math/mat3.h` with static factories and operators
- Added `src/math/mat3.cpp` with implementation
- Added 6 tests to `tests/test_algorithms.cpp`

## Test results
All 90 checks pass.

Closes #1
EOF
)"
```

---

## 11. Important constraints for every AI-assisted session

- Never disable or skip tests to make a build pass
- Never delete existing algorithm implementations — build on top of them
- Always run `./build/test_algorithms` before opening a PR
- Do not add any "Co-Authored-By: Claude" or other AI attribution lines
- Output stays in the `Framebuffer` — do not add OpenGL, DirectX, or Vulkan
- Keep the headless PNG path working at all times (CI depends on it)
- Use GitHub issues and PRs for every piece of work, even small ones
- Link every PR to its issue with `Closes #N`
