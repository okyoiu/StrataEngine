# tessera
### High-Performance 2D Industrial Automation Simulation Engine

A simple mock-up game that resembles Factorio in how its game mechanics work — extract resources, carry them on conveyor belts, and sell them for profit in an automation loop that never really ends.

_Note_: This should be noted that this build was made with the assistance of Artificial Intelligence during this project.

## Demo
[tessera-use-demo.mov](../../../Downloads/tessera-use-demo.mov)

## What this project demonstrates
- A custom data-oriented ECS (sparse sets, dense arrays, cache-friendly iteration)
- Hand-written memory allocators (pool + arena) instead of relying on `new`/`delete`
- A flat open-addressing spatial hash grid for O(1) tile lookups
- A real-time game loop with a fixed-timestep simulation and SDL2 rendering
- Game design on top of the engine: an economy, a building system, and an objective system that scales forever

## Tech stack
C++20 · CMake · SDL2 / SDL2_ttf

## Run it
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
./build/tessera_engine
```

## Controls
`1`-`4` select building · `R` rotate · Left click place · Right click bulldoze · `WASD`/arrows pan · Scroll wheel zoom · `Esc` quit
