# PulseRun

**A 2D level-based obstacle runner whose graphics are built from scratch in C++.**

**Play the live build: https://diorwachira.github.io/pulserun/** (rebuilt from `main` automatically)

PulseRun is our Computer Graphics semester project. The goal is not just a game that looks good, but one where every
shape on screen is produced by graphics algorithms we implemented ourselves: lines, circles, polygons, clipping and
curves. The same C++ renderer draws into a plain array of pixels on the desktop (SDL2 window), in the browser
(WebAssembly, copied onto a `<canvas>`), and in headless mode (PNG files for CI and evidence).

## Screenshots

Rendered by CI from the latest `main` (the game plays itself using the level solver, see below):

| Main menu | Level select |
|---|---|
| ![Menu](https://diorwachira.github.io/pulserun/shots/10_game_menu.webp) | ![Level select](https://diorwachira.github.io/pulserun/shots/11_game_level_select.webp) |
| **Mid-run, Level 1** | **Hitbox overlay (H)** |
| ![Gameplay](https://diorwachira.github.io/pulserun/shots/13_game_play.webp) | ![Hitboxes](https://diorwachira.github.io/pulserun/shots/14_game_hitboxes.webp) |
| **Crashed** | **Level complete** |
| ![Crashed](https://diorwachira.github.io/pulserun/shots/17_game_crashed.webp) | ![Complete](https://diorwachira.github.io/pulserun/shots/18_game_complete.webp) |

Checkpoint 1 stills (committed): [gameplay](docs/screenshots/06_scene_gameplay.png), [menu](docs/screenshots/07_scene_menu.png),
[level select](docs/screenshots/08_scene_level_select.png), [death](docs/screenshots/09_scene_death.png).
Technique demos: [lines](docs/screenshots/01_lines.png), [circles & ellipses](docs/screenshots/02_circles.png),
[polygons](docs/screenshots/03_polygons.png), [clipping](docs/screenshots/04_clipping.png),
[curves](docs/screenshots/05_curves.png).

## How to play

The cube runs on its own. You only decide when to jump.

| Input | Action |
|---|---|
| **Space**, **↑**, **W**, left click, or tap | Jump. **Hold** for a higher jump (the extension lasts up to 350 ms). Keep holding to jump again on landing. |
| Press while touching a yellow **orb** | Mid-air jump |
| Yellow **pad** on the ground | Launches you automatically |
| **Esc** / **P** / the ‖ button | Pause (Resume, Restart, Main Menu) |
| **R** | Restart the level |
| **H** | Show the collision polygons (dashed = AABB broad phase, solid = exact hull) |
| **← →**, **Enter** | Move between buttons and choose |

Spikes and saws kill. You can land on top of blocks, but hitting their side or underside kills. Reach the portal to
finish the level. Best %, best time and attempts are saved (browser `localStorage`, or `pulserun_save.txt` on desktop).

| Level | Name | Length | Speed | Teaches |
|---|---|---|---|---|
| 1 | PRIMER | 10 000 px | 420 px/s | single, double, triple spikes; orbs; saws; blocks; a pad |
| 2 | ESCALATION | 12 000 px | 460 px/s | orb chains, low ceilings, pads onto platforms, riding a roof |
| 3 | ADVANCED | 14 000 px | 520 px/s | a tunnel (tap, don't hold), saw gauntlet, rising platforms |

## Group members and responsibilities

| Member | Initial responsibility | Work in the repository |
|---|---|---|
| Wachira Njoroge | Graphics engine & algorithms | `mat3` transforms (#1), camera (#13) |
| Stacy Akinyi | Gameplay & physics | Player physics (#15), collision detection (#17) |
| Okeyo Marie Hoppe Solini | Level design & architecture | Level format and loader (#19), Levels 1–3 (#21) |
| Mathenge Gitahi | Animation & visual effects | C++ foundation, all Checkpoint 1 algorithms, sprites, UI, demos, tests; window (#5), game loop (#11), animation (#23) |
| Diana Mucheke | UI engine & menus | Pause and level-complete screens (#26), death and retry screen (#25) |
| Kimani Anise Wambui | Testing & documentation | CI and live deploy (#7), extended tests (#28), this README (#34) |

Issue numbers link each task to its pull request. See the Issues, pull requests and commit history for each member's
actual work.

## Technologies and tools

- **Language:** C++17, no graphics library. All drawing goes through our own `Framebuffer` (an array of pixels).
- **Window:** SDL2 opens the window and reads input; it never draws. We scale the framebuffer ourselves (nearest
  neighbour) and hand SDL finished pixels. In the browser, Emscripten compiles the same code to WebAssembly and the
  pixels are copied onto a canvas with `putImageData`.
- **Build:** CMake + Ninja. GCC (MSYS2 UCRT64) on Windows, GCC on Linux, Emscripten 3.1.64 for the web.
- **CI/CD:** GitHub Actions builds, runs the tests, renders screenshots and deploys the web build to GitHub Pages on
  every push to `main`.

## Graphics techniques and algorithms

| Topic | What is implemented | Source | Where PulseRun uses it |
|---|---|---|---|
| Lines | DDA; Bresenham (all 8 octants, integer only); dashed lines; thick lines (filled quad, round caps) | `src/raster/line.cpp` | Ground ticks, saw spokes, UI frames, hitbox overlay (dashed AABBs) |
| Circles and ellipses | Midpoint circle; midpoint ellipse (2 regions); filled versions; arcs; thick arcs | `src/raster/circle.cpp` | Orbs, pads, difficulty faces, death shock-wave rings, orb pop |
| Polygons | Edge-table + active-edge-table scanline fill (even-odd, concave safe); point-in-polygon | `src/raster/polygon.cpp` | Cube, spikes, blocks, saws, skyline; collision narrow phase |
| Clipping | Cohen-Sutherland lines; Sutherland-Hodgman polygons | `src/raster/clip.cpp` | Death shards clipped to the viewport |
| Curves | Cubic Bezier (De Casteljau); Catmull-Rom spline | `src/raster/curve.cpp` | Motion trail behind the cube, hills, portal edges, logo pulse line |
| 2D transforms | `Mat3` homogeneous matrices: translate, rotate, scale, about a pivot, inverse | `src/math/mat3.cpp` | Camera world-to-screen, screen shake, hitbox overlay |
| Colour | Alpha blending, colour interpolation, gradients, layered glow | `src/core/` | Glow on every neon shape, sky and ground, overlays |
| Text | 5×7 bitmap font as scaled rectangles | `src/raster/font.cpp` | HUD, menus, buttons (and the website's headlines) |

## Game systems

| System | Source | Notes |
|---|---|---|
| Window + input | `src/core/window*.cpp` | SDL2, browser and headless backends behind one `Window` class |
| Fixed-timestep loop | `src/game/game_loop.cpp` | 60 Hz physics; render interpolation; stalls clamped to 250 ms |
| Camera | `src/game/camera.h` | `world_to_screen()` is a `Mat3`; follows the player, eases up for high jumps |
| Physics | `src/game/player.cpp` | Gravity 1200 px/s², terminal 900 px/s, jump −600 px/s, hold extension up to 350 ms |
| Collision | `src/game/collision.cpp` | AABB broad phase, exact polygon overlap (edges + containment), landing rules |
| Levels | `src/game/level.cpp`, `levels/*.txt` | Plain-text format (below) |
| Solver | `src/game/solver.cpp` | Depth-first search over jump timings; proves every level is beatable |
| Animation | `src/game/animation.cpp` | Flip-book `Animation`: 5-frame death burst, pad spring, orb pop |
| Screens | `src/game/game.cpp`, `game_screens.cpp` | Menu, level select, settings, play, pause, death, complete |

## Level file format

Levels are plain text in `levels/` (one item per line, `#` starts a comment). World y grows downward and the ground
surface is at `ground_y`.

```
# PulseRun Level 1
name PRIMER
number 1
difficulty 0          # 0 easy, 1 normal, 2 hard (level-select face)
width 10000
ground_y 500
speed 420             # run speed, px/s
bg_color 0x10164a     # sky colour

BLOCK   200  450  60  50          # top-left x y, width, height (tiled with square blocks)
SPIKE   400  450                  # top-left of a 50 x 50 spike (optional w h)
SAW     700  430  radius=20  speed=1.5   # centre, radius, spin in turns/s
ORB     900  350                  # centre (optional radius=)
PAD     1200 488                  # top-left of a 50 x 12 jump pad
PORTAL  9800 440  target=2        # centre of the exit, next level number
```

Numbers can also be written as `key=value` (`w=`, `h=`, `radius=`, `speed=`, `target=`). Errors report the line, for
example `line 7: unknown object 'SPIKES'`. Every shipped level is checked by the test suite: idling must die early, and
the solver must find a run that reaches the portal.

## How to compile and run

You need a C++17 compiler and CMake. On Windows we use MSYS2 UCRT64:
`pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja mingw-w64-ucrt-x86_64-SDL2`.

```bash
git clone https://github.com/MathengeGit/pulserun.git
cd pulserun
cmake -S . -B build -G Ninja               # finds SDL2 if installed; otherwise builds headless
cmake --build build
./build/pulserun                           # play (Windows: .\build\pulserun.exe)
./build/test_algorithms                    # 231 checks
./build/pulserun --showcase out            # render every game screen to PNG (no window needed)
cmake --build build --target screenshots   # Checkpoint 1 technique demos into docs/screenshots/
```

Without SDL2, or with `-DPULSERUN_HEADLESS=ON`, the game has no window and renders the showcase PNGs instead.

Web build (needs the [Emscripten SDK](https://emscripten.org)):

```bash
emcmake cmake -S . -B build-web -DCMAKE_BUILD_TYPE=Release
cmake --build build-web --target pulserun   # build-web/pulserun.js + pulserun.wasm
```

Serve `web/` with `pulserun.js`/`pulserun.wasm` copied into `web/game/` (for example `python -m http.server`).

## Live deployment

`.github/workflows/ci.yml` runs on every push and pull request:

1. **native**: headless Linux build, `test_algorithms`, the technique demos and `pulserun --showcase`.
2. **web**: the game compiled to WebAssembly.
3. **site**: `web/` + the game + WebP screenshots + `build.json` (commit, test result, run link).
4. **deploy**: GitHub Pages, on `main` only.

The team repo deploys once an admin enables Pages (**Settings → Pages → Source: GitHub Actions**). Until then the deploy
step is skipped and the run stays green. The live link above is a fork running the same workflow: every 20 minutes it
checks whether the team's `main` has moved, and if so rebuilds and redeploys it.

## Repository layout

```
app/          main.cpp: the game entry point (play, or --showcase)
src/core/     colour, framebuffer, PNG/BMP writers, window backends, input, save storage
src/math/     vec2, mat3
src/raster/   line, circle, polygon, clip, curve, font      <- the graphics algorithms
src/game/     sprites, player, collision, level, world, solver, camera, loop, animation, screens
src/ui/       HUD, buttons, level cards, logo
levels/       level1.txt, level2.txt, level3.txt
demos/        one demo per technique + the Checkpoint 1 game-style stills
tests/        test_algorithms.cpp (231 checks)
web/          the website around the live build
docs/         screenshots and checkpoint evidence
```

## What we learned

Engineering lessons from building this (each member should add their own reflections for the grader):

- **A fixed timestep makes physics reproducible.** Advancing physics in exact 1/60 s steps made every run deterministic.
  That made a level solver possible, and it lets the tests assert that each level is beatable.
- **Automated playtesting finds impossible levels.** The solver showed that a ceiling 120 px above the ground can never
  be passed (a minimum jump rises 150 px), and that 60 px stair steps are too short at 520 px/s. Both were fixed before
  anyone hand-tested them.
- **Hitboxes should be forgiving.** The cube's hitbox is smaller than its sprite and spikes use an inner triangle, so a
  crash always looks deserved. The H overlay makes the choice visible.
- **Separating "draw" from "present" pays off.** Because the renderer only writes into a `Framebuffer`, the same code
  outputs to a PNG, an SDL window and a browser canvas with no changes to any algorithm.
- **Input edges need care.** A key press that lands between two physics steps must be kept until one step uses it, or
  quick taps are lost. The key that starts a level must not also count as a jump.

## Known limitations

- Lines and shapes are not anti-aliased (edges are pixel-exact, which is intentional).
- Sutherland-Hodgman can leave thin joining edges along the window border when it clips a concave polygon.
- The bitmap font is uppercase only with a limited symbol set.
- Our PNG encoder stores pixels uncompressed, so each screenshot is about 1.5 MB (CI converts them to WebP for the site).
- No sound.
- Tested with GCC and Emscripten (Clang). MSVC has not been tested.
