# CMPUT 350 Project 2

This project is a small platformer built in C++ using SFML and an
entity-component-system engine. The game loads its world from map files and
includes player movement, variable-height jumping, wall jumping, dashing,
collisions, star collection, star spawning, sounds, a score display, and a
camera that follows the player.

## Team Contributions

### Utsha Samanta — Player Controls, Physics, and Collision

- Implemented keyboard input handling, including current and previous key states.
- Developed player movement with acceleration, deceleration, maximum velocity,
  and air control.
- Implemented variable-height jumping, fast falling, wall sliding, wall jumping,
  and dashing.
- Implemented gravity and movement systems.
- Developed collision detection and resolution for the player, platforms,
  walls, spikes, and stars.
- Implemented player respawning after touching spikes and manual respawning.
- Implemented star collection, score updates, star collisions, and bouncing.
- Added sound triggers for jumping, landing, wall contact, dashing, and
  collecting stars.
- Implemented camera-shake triggers and collision-box debug drawing.

### Raiyana Rahman — Game Setup, Rendering, Spawning, and Camera

- Set up the platformer, entity initialization, resource registration, and
  system execution order.
- Loaded the background and gameplay maps and created the map entities.
- Registered textures, sprites, and sound effects.
- Implemented star spawning with timers and randomized velocities.
- Created the score display and implemented numeric sprite updates.
- Implemented horizontal camera following, tracking boundaries, smooth camera
  movement, and camera-shake application.
- Completed window management, including world-to-screen and screen-to-world
  coordinate conversion.
- Prepared the project documentation and build instructions.

### Shared Work

- Designed and integrated the ECS architecture, components, managers, and systems.
- Tested the complete game and resolved integration issues.
- Tuned movement, gravity, jumping, collisions, dashing, and camera behavior.

## Build and Run

The project uses CMake and includes the required assets in the `assets` folder.
From the project root, run:

```sh
cmake --preset debug
cmake --build build
./build/platformer/platformer
```

To provide a different resource directory:

```sh
./build/platformer/platformer -path /path/to/assets
```

## Controls

- Left and Right arrows: move
- Down arrow: fall faster
- Z: jump or wall jump
- C: dash
- R: respawn at the starting position
- F1: toggle collision-box debug drawing

## Testing

Run the existing engine checks with:

```sh
./build/engine/ECSTest
```

The game has also been manually tested to confirm that:

- The player can move, perform short and long jumps, wall jump, and dash.
- The first obstacle section can be crossed using wall jumps.
- The second obstacle section requires dashing.
- Stars spawn, bounce, collide, and increase the score when collected.
- Spikes and the R key return the player to the starting position.
- Jump, landing, wall, dash, and collection sounds play correctly.
- The camera follows horizontally and collision impacts cause screen shake.
- F1 displays the correct collision boxes.
- Closing the window exits the game.

## AI Usage Disclosure

### Areas Where AI Was Used

AI was used in a supporting role during this project for the following work:

- Explaining the high-level ECS architecture and how entities, components,
  component storage, managers, and systems work together during a game loop.
- Aiding the EntityManager implementation, particularly the code that maps a
  component type to its index and the lookup from an EntityID to a component's
  location in component storage.
- Explaining how SpriteManager selects a rectangular region from a sprite sheet
  and how SpriteComponent controls the sprite's position and displayed size.
- Explaining, reviewing, and debugging collision detection and resolution,
  including bounding boxes, wall contact, wall jumping, star collisions, and
  collision state from the previous frame.
- Assisting with parts of the input, gravity, collision, respawn, debug drawing,
  sound-trigger, and gameplay-tuning code.
- Reviewing compiler output and finding integration problems.
- Helping organize and revise the README and project documentation.

AI suggestions were reviewed, adjusted to match the project requirements, and
compiled and manually tested before being retained in the final project. Both
team members are responsible for understanding the submitted implementation.

### Example AI Prompts Used

Examples of prompts used during development include:

- "How do I go from an EntityID to the index at which its component is located?"
- "Explain the implementation for getting the component type index."
- "Is EntityManager basically a list of all entities currently in the game, with
  components attached that systems use for calculations?"
- "When I jump off a wall, it jumps in the opposite direction and I cannot
  control it to move the other way. Why?"
- "How do I select a sprite from the sprite sheet?"

Some conversations also involved asking AI to inspect small sections of code,
explain unexpected behavior, and interpret compiler or runtime errors.

### Reflection on AI Usage

AI was most useful as a learning and debugging tool. It helped make the ECS
control flow easier to understand by connecting the responsibilities of the
EntityManager, component storage, systems, and resource managers. The
explanations of component type indices and EntityID lookups were particularly
helpful because those ideas were confusing at the beginning of the project.

AI was also useful for tracing collision and input problems that depended on
state across several frames. Examples included unreliable wall jumps, collision
box sizing, selecting the correct part of a sprite sheet, and distinguishing
between a sprite's source rectangle and its position in the game world.

The suggestions were not always immediately suitable. Some implementations
were longer than necessary, some needed gameplay tuning, and some
changes had to be corrected after compiling or testing them with the rest of the
project. The team compared suggestions with the assignment requirements,
simplified or adjusted them where necessary, tested the resulting behavior, and
made sure both members understood the final code.
