# tessera
### High-Performance 2D Industrial Automation Simulation Engine

> A data-oriented simulation engine written in C++20 featuring a dense/sparse-set ECS,
> custom slab allocators, and a flat spatial hash grid — engineered for 100,000+ active
> entities at deterministic fixed-timestep tick rates.

---

## System Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                          App (Orchestrator)                      │
│   ┌───────────────┐   ┌──────────────────┐  ┌────────────────┐  │
│   │ TickScheduler │   │   InputHandler   │  │    Renderer    │  │
│   └───────┬───────┘   └────────┬─────────┘  └───────┬────────┘  │
│           │                   │                     │           │
│   ┌───────▼───────────────────▼─────────────────────▼────────┐  │
│   │                       Registry (ECS World)                │  │
│   │   ┌─────────────┐  ┌─────────────┐  ┌─────────────────┐  │  │
│   │   │SparseSet<T> │  │SparseSet<T> │  │  SparseSet<T>   │  │  │
│   │   │  Position   │  │  BeltLane   │  │    Resource     │  │  │
│   │   │[dense array]│  │[dense array]│  │  [dense array]  │  │  │
│   │   └─────────────┘  └─────────────┘  └─────────────────┘  │  │
│   └───────────────────────────────────────────────────────────┘  │
│                                                                   │
│   ┌────────────────────────┐   ┌─────────────────────────────┐   │
│   │     SpatialGrid        │   │      Memory Subsystem       │   │
│   │  (flat open-address    │   │  PoolAllocator (fixed slab) │   │
│   │   hash, O(1) query)    │   │  ArenaAllocator (bump ptr)  │   │
│   └────────────────────────┘   └─────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────┘

Simulation Systems (stateless, operate on dense arrays):
  BeltSystem → InserterSystem → ResourceSystem
```

---

## Technical Highlights

### Custom Memory Allocators (`src/memory/`)

The engine bypasses the standard heap in all hot simulation paths using two
purpose-built allocators.

**PoolAllocator** — fixed-size slab allocator with an intrusive free list.
- Block size is fixed at construction time (e.g., `sizeof(BeltLane)`).
- The free list is embedded directly within free blocks: the first `sizeof(void*)`
  bytes of each free slot store a pointer to the next free slot. Zero metadata overhead.
- `allocate()` is a single pointer pop. `deallocate()` is a single pointer push.
  Both are O(1) with no branching on the allocation path.
- The entire pool lives in a single contiguous heap region, acquired once at startup.

**ArenaAllocator** — bump-pointer scratch allocator for per-frame temporary memory.
- `allocate(bytes, alignment)` performs a pointer advance with alignment rounding.
  No bookkeeping, no metadata, no fragmentation.
- `deallocate()` is a deliberate no-op. `reset()` resets the bump pointer to base
  in O(1), bulk-freeing the entire arena.
- Used for per-tick `View` construction, query result buffers, and temporary sort keys.
- Alignment is computed with: `aligned = (ptr + align - 1) & ~(align - 1)`

---

### Data-Oriented ECS Layout (`src/ecs/`)

The ECS uses a dense/sparse-set architecture. No object-oriented entity hierarchies,
no vtables, no pointer chasing in the simulation hot path.

**SparseSet\<T\>** — the core structural invariant:

| Array     | Size              | Purpose                                 |
|-----------|-------------------|-----------------------------------------|
| `sparse_` | MAX_ENTITIES      | EntityID → dense index (O(1) lookup)    |
| `dense_`  | live entity count | Dense index → EntityID (reverse lookup) |
| `data_`   | live entity count | Dense index → component value T         |

- `data_` is always packed. No holes. Iteration is a linear scan over a contiguous
  array — maximum cache line utilization.
- Removal uses swap-and-pop: the last element replaces the removed one. The `sparse_`
  entry of the swapped entity is updated to reflect its new dense index.
- `View<A, B, C>` iterates the smallest participating pool's `data_` array, checking
  membership in the others via O(1) `sparse_` lookup. No temporary entity lists,
  no allocations.

---

### Spatial Hash Grid (`src/spatial/`)

A flat, open-addressing hash grid for O(1) average-case proximity queries.

- The world is divided into fixed-size cells. Each entity maps to exactly one cell
  based on its `Position`.
- Cell lookup key is a `u64` hash of `(cell_x, cell_y)` using a high-quality
  integer mix hash (e.g., Murmur finalizer).
- A reverse map (`EntityID → cell_key`) enables O(1) removal without re-querying
  the entity's position.
- `query_radius()` writes into a caller-supplied stack buffer (`GridQueryBuffer`)
  to avoid any heap allocation on the query path. Results are available as a raw
  pointer + count pair.

---

## Performance Metrics

> Fill these in as you run your benchmarks. See `benchmarks/` for the harness.

### Allocator Performance vs `new`/`delete`

| Operation              | `new`/`delete` (ns) | PoolAllocator (ns) | Speedup |
|------------------------|---------------------|--------------------|---------|
| Single alloc (64B)     |                     |                    |         |
| Single dealloc (64B)   |                     |                    |         |
| 10,000 allocs (64B)    |                     |                    |         |
| 10,000 deallocs (64B)  |                     |                    |         |
| Arena: 10k allocs      |                     |                    |         |
| Arena: reset()         |                     |                    |         |

### ECS Throughput

| Scenario                          | Entity Count | Frame Time (ms) | Ticks/sec |
|-----------------------------------|--------------|-----------------|-----------|
| BeltSystem update only            |              |                 |           |
| BeltSystem + InserterSystem       |              |                 |           |
| Full simulation tick              |              |                 |           |
| Full simulation tick (100k ents)  |              |                 |           |
| View<Position, BeltLane> iterate  |              |                 |           |

### Spatial Grid

| Query Type           | Entity Count | Cell Size | Avg Query Time (ns) | Max Query Time (ns) |
|----------------------|--------------|-----------|---------------------|---------------------|
| query_radius (r=1)   |              |           |                     |                     |
| query_radius (r=5)   |              |           |                     |                     |
| insert + remove cycle|              |           |                     |                     |

### Memory Footprint

| Subsystem              | Allocation Strategy | Peak Usage (MB) |
|------------------------|---------------------|-----------------|
| ECS Registry (100k)    | PoolAllocator       |                 |
| SpatialGrid            | std::vector         |                 |
| Per-frame Arena        | ArenaAllocator      |                 |
| Total working set      |                     |                 |

---

## Build Requirements

| Dependency  | Version  | Notes                            |
|-------------|----------|----------------------------------|
| C++ Standard| C++20    | Concepts, `std::span`, designated inits |
| CMake       | ≥ 3.21   |                                  |
| SDL2        | ≥ 2.0.18 | Rendering and input only         |
| Clang/GCC   | ≥ 14     | Full C++20 support required      |
| macOS SDK   | ≥ 12.0   | Primary development target       |

---

## Build Instructions

```bash
git clone https://github.com/yourname/tessera.git
cd tessera
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
./build/tessera
```

To run benchmarks:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DTESSERA_BUILD_BENCHMARKS=ON
cmake --build build --parallel
./build/benchmarks/bench_allocators
./build/benchmarks/bench_ecs
```

---

## Implementation Roadmap

### Week 1 — Memory Foundation & ECS Core
> Goal: A provably correct, benchmarked allocator layer and a working `SparseSet`.

| Day | Task                                                                |
|-----|---------------------------------------------------------------------|
| 1   | `core/types.hpp`, `core/assert.hpp`, `core/platform.hpp`           |
| 2   | `memory/memory_utils.hpp` — all pointer arithmetic helpers          |
| 3   | `memory/pool_allocator` — intrusive free list, full invariant tests |
| 4   | `memory/arena_allocator` — bump pointer, alignment math, tests      |
| 5   | `benchmarks/bench_allocators` — prove speedup vs `new`/`delete`    |
| 6   | `ecs/entity.hpp` — versioned EntityID, bitmask helpers              |
| 7   | `ecs/sparse_set.hpp` — dense/sparse arrays, swap-and-pop removal    |

### Week 2 — ECS Registry, Spatial Grid & Simulation Systems
> Goal: A working Registry, all simulation components, and a functioning spatial grid.

| Day | Task                                                                |
|-----|---------------------------------------------------------------------|
| 8   | `ecs/component_pool.hpp` — typed wrapper over `SparseSet`          |
| 9   | `ecs/registry.hpp/.cpp` — entity lifecycle, component management   |
| 10  | `ecs/view.hpp` — multi-component iteration, no allocation           |
| 11  | `simulation/components.hpp` — all POD structs, static_assert guards |
| 12  | `spatial/spatial_grid.hpp/.cpp` — hash grid, insert/remove/query   |
| 13  | `spatial/grid_query.hpp` — stack-local query buffer                 |
| 14  | `benchmarks/bench_ecs`, `benchmarks/bench_spatial` — measure both  |

### Week 3 — Simulation Systems, Render Pipeline & Integration
> Goal: A running simulation visible on screen with all three systems active.

| Day | Task                                                                |
|-----|---------------------------------------------------------------------|
| 15  | `simulation/belt_system` — dense array iteration, no `get()` in loop|
| 16  | `simulation/inserter_system` — spatial query integration            |
| 17  | `simulation/resource_system` — depletion, swap-pop-safe iteration  |
| 18  | `simulation/tick_scheduler.hpp` — fixed dt accumulator, spiral cap  |
| 19  | `render/camera.hpp`, `render/tile_atlas` — coordinate transform     |
| 20  | `render/renderer` — SDL2 pipeline, camera-space tile rendering      |
| 21  | `app/app`, `app/input_handler` — full integration, camera pan/zoom  |

---

## Architecture Invariants

These are non-negotiable constraints enforced throughout the codebase:

- **No `new`/`delete` in hot paths.** The simulation tick and render loop must not
  call the global allocator. All runtime memory is pre-allocated at startup.
- **No virtual dispatch in simulation systems.** Systems are stateless structs with
  concrete `update()` methods. The compiler sees the full call graph.
- **No `Registry::get<T>()` inside inner loops.** Systems must acquire raw array
  pointers from `view_data()` before the loop and index directly.
- **All component structs are trivially copyable.** Enforced via `static_assert`.
  No constructors with side effects, no destructors.
- **`SparseSet` dense arrays are never holey.** Swap-and-pop is the only removal
  strategy. Ordered removal is prohibited.
- **`query_radius()` never allocates.** Callers provide output buffers.
- **`ArenaAllocator::deallocate()` is always a no-op.** Per-frame scratch memory
  is bulk-freed via `reset()` only.

---

## Project Structure

```
tessera/
├── src/
│   ├── core/          # types, assert macros, platform detection
│   ├── memory/        # PoolAllocator, ArenaAllocator, IAllocator interface
│   ├── ecs/           # SparseSet, ComponentPool, Registry, View
│   ├── simulation/    # POD components, BeltSystem, InserterSystem, ResourceSystem
│   ├── spatial/       # SpatialGrid, GridQueryBuffer
│   ├── render/        # Renderer, TileAtlas, Camera
│   └── app/           # App orchestrator, InputHandler
├── benchmarks/        # Allocator, ECS, and spatial grid benchmarks
├── tests/             # Unit tests per subsystem
└── docs/              # Architecture notes, memory model diagram
```

---

## License

MIT — see `LICENSE`.