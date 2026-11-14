# CMPUT 350 Project 2

A C++20/SFML platformer using an entity-component-system engine.

## Team and responsibilities

- **Person A — Utsha Samanta:** player input, movement, jumping, wall sliding,
  wall jumping, dash, collision handling, spike respawning, star collection and
  bouncing, gameplay sound triggers, and camera-shake triggers.
- **Person B — Raiyana Rahman:** initial entities and map setup, star spawning,
  score display, camera following and shake application, sound registration,
  window-manager features, and documentation.

## Build and run

Use CMake 3.28 or newer and a C++20 compiler. On the lab's Linux x86_64
machines with GCC 9.4, CMake uses the bundled SFML libraries. Other systems
fetch SFML 3.1 and its dependencies and need internet access on the first build.

The supplied `assets/sky.map` and `assets/world.map` define the background and
level. Keep their layout unchanged.

From the project root:

```sh
cmake --preset debug
cmake --build build
./build/platformer/platformer -path assets
```

The default resource path is `assets/`, relative to the working directory.
When running from another directory, use `-path /absolute/path/to/assets`.
CMake also copies assets into the build directory at configure time.

Controls specified for Person A: arrow keys to move, Z to jump, C to dash,
and Down to fall faster. Close the window to quit. The current InputSystem
is still a stub, so interactive traversal requires Person A's implementation.

Run the automated spawn and score checks with `./build/engine/ECSTest`.

## Person B features

- Each map `S` creates a star spawner: one star every 120 frames continuously.
  A negative `totalSpawns` means continuous spawning; zero stops spawning, and
  a positive value limits the remaining spawns.
- Stars launch upward with a random horizontal velocity. Each has location,
  movement, gravity, collision, and sprite components, with entity type `star`.
- One extra star starts outside the left wall for wall-jump testing.
- One ScoreComponent owns the score and three screen-space digit entities.
  The display starts at 000 and clamps to 999 without changing the stored score.
- The camera follows horizontally, keeps its base height fixed, tracks slowly
  outside the middle 40%, and immediately enforces the outer 10% margins.
- CameraShake applies alternating offsets for `frameLimit` frames. Its offset
  resets when the timer ends; shake never changes the camera's base position.
- Simulation is capped at 60 frames per second; timing and velocities use frames.

## Integration contracts

Person A owns controls, player physics, all star collision/bounce behavior,
collection, and sound/shake triggers. Increment the single ScoreComponent's
`score` on collection. Write the camera entity's `CameraShake.frameLimit` and
`offset` on impact. Obtain sounds through `GetSoundManager().GetSound(name)`
and call `play()`. The registered names are `jump`, `land`, `collect`, and `wall`.

CameraFollower belongs to the player being followed and stores the player's ID.
CameraComponent and CameraShake belong to the camera entity.

System order is collision snapshot, input, gravity, movement, collision, score,
camera, sprites, then spawning. SpawnSystem finishes its iteration before adding
entities, because insertion can invalidate iterators and component references.

## Resource ownership

Managers belong to the engine and live until the engine is destroyed. Systems
access them through EngineView getters; returned manager references are borrowed.

EntityID is an index; check IsValid before using an ID that may have been removed.
GetComponent returns a borrowed reference. Adding a component of the same type
may reallocate storage, and removing entities/components invalidates the removed
data. Keep IDs across frames and reacquire component references when needed.
Creating entities invalidates entity iterators if the entity vector reallocates.

SpriteManager owns textures and sprites. SpriteID remains an index into its
sprite vector. RegisterTexture may invalidate borrowed sprite references; get
sprites again by ID after registering more. Texture references remain stable in
the texture map. Do not retain a sprite reference after engine destruction.
SpriteSystem sets position and scale before each draw because entities can share
a SpriteID.

SoundManager owns buffers and sounds. RegisterSound may invalidate borrowed
sound references when its vector grows. Register all sounds during setup, then
retrieve them by name when playing. Do not retain them past engine destruction.

WindowManager owns the render window through shared_ptr. GetWindow returns a
shared owner, but normal use should keep its lifetime within the engine's.
SetWorldRect centers and fits the requested world rectangle while preserving
aspect ratio. DebugDrawRect and DebugFrameRect accept world coordinates.
The float WindowToWorld/WorldToWindow overloads convert distances using scale
only; the Point2D overloads convert positions using scale and camera translation.
CollisionComponent stores its current collisionBox relative to the entity, but
previousCollisionBox is a snapshot in world coordinates.

SystemManager owns systems through unique_ptr and runs them in insertion order.
ClearSystems destroys those systems and invalidates references to them.

## Submission checks

Merge Person A first, then integrate Person B with that branch. Confirm
collection and all four sounds, test both obstacle sections,
and tune movement/dash and star bounce with Person A. Build and run on a lab
machine. Fill in team names and contributions in ABOUT.TXT. Build and run again
from the exact extracted submission archive before submitting. Do not include
local build output in the archive.
