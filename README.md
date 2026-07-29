# Emergency Resource Allocator

A C++ data-structures project for hospital or disaster-response triage. The system prioritizes emergencies with a max-heap, tracks resources with a hash table, keeps overflow cases in a queue, and records dispatch history.

## Why this project works well for a lab final

- Uses core syllabus topics: heaps, priority queues, hashing, queues, and complexity analysis.
- Has an intuitive real-world story: the most critical patient or rescue case is handled first.
- Supports a clear viva/demo flow: add requests, serve the top priority case, update severity, release resources, and inspect the state.

## Tech Stack

- Core allocator: C++17
- Build system: CMake
- CLI demo: console menu in `src/main.cpp`
- Web demo: static HTML, CSS, and vanilla JavaScript in `ui/`, served by the C++ backend

## Project Structure

- `include/` contains the header-only data structures and service layer.
- `src/main.cpp` provides the interactive CLI.
- `ui/` contains a lightweight browser demo that mirrors the triage operations.
- `build/` is the CMake output directory.

## Features

- Add emergency requests with severity and required resource type.
- Insert and prioritize requests using a max-heap.
- Update a request’s severity and re-order the heap.
- Add, look up, assign, and release resources using a hash table.
- Keep overflow cases in a FIFO waiting queue.
- Track allocation history and print system status.
- Run a built-in demo scenario for presentation.

## Time Complexity

- Insert emergency: `O(log n)`
- Serve next emergency: `O(log n)`
- Update severity: `O(log n)` with indexed heap access
- Resource lookup: average `O(1)`
- Assign or release resource: average `O(1)`
- Waiting queue push/pop: `O(1)`
- Print current status: `O(n)` across the visible collections

## How to Build and Run the C++ App

### Configure and build

```bash
cmake -S . -B build
cmake --build build
```

### Run the CLI

```bash
./build/emergency_allocator
```

On Windows PowerShell, use:

```powershell
.\build\emergency_allocator.exe
```

### Run the web backend

```powershell
.\build\emergency_allocator_web.exe
```

Then open `http://localhost:8080/` in a browser.

### Demo flow

Use menu option `7` to run the scripted scenario. It initializes resources, loads emergencies, serves the highest-priority cases, updates a case severity, releases a resource, and prints the resulting status.

## How to Use the Web Demo

Run `emergency_allocator_web` first, then open `http://localhost:8080/` in a browser.

The web demo is now backed by the C++ server, so the browser is only a presentation layer and the allocator state lives in the backend.

## Suggested Viva Talking Points

- Why a heap is better than a sorted array or FIFO queue for triage.
- How the severity and arrival time tie-breaker keeps the ordering fair.
- Why hashing helps resource lookup and status updates.
- How the queue preserves overflow cases when resources are exhausted.
- Where the `O(log n)` and `O(1)` operations appear in the design.

## Limitations and Future Work

- The current project is a simplified simulator, not a production hospital system.
- Persistence is not included yet.
- Graph-based ambulance routing can be added later with BFS or Dijkstra.
- Multi-hospital coordination can be added as a future extension.

## Recommended Demo Script

1. Add 5 to 10 resources.
2. Add 8 to 12 emergencies with mixed severities.
3. Show the top priorities.
4. Serve the highest-priority case several times.
5. Increase the severity of a waiting case and show it move up.
6. Release a resource and show the next case being handled.
7. End with a complexity summary and status view.
