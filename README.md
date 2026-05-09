# MBTA Green Line E — Real-Time Train Scheduler

A real-time C++ application that fetches live train predictions from the MBTA V3 API and helps users find upcoming departures between any two stations on the Green Line E — including to Medford/Tufts, home of Tufts University!!


## What It Does

- Fetches live train predictions from the MBTA V3 API on every run
- Lets users query departures between any two Green Line E stations
- Detects direction automatically (eastbound toward Medford/Tufts or westbound toward Heath Street)
- Returns the next 3 trains with departure time, arrival time, and travel duration
- Includes a full benchmark mode comparing Priority Queue, Sorted Vector, Hash Map, and Linear Search across latency, throughput, and memory usage


## Data Structures

### Priority Queue (Min-Heap)
Used to order all incoming train predictions by departure time. Supports O(log n) insertion per prediction, meaning new arrivals slot into the correct position without re-sorting the entire dataset. O(1) peek always returns the soonest departure.

For a real-time system where predictions update continuously, this is significantly more efficient than re-sorting a vector on every update cycle (O(n log n)).

### Hash Map — Station Name → Stop ID
Translates user-entered station names into internal MBTA stop IDs in O(1) time. Avoids O(n) linear search through all 25 stations on every query.

### Hash Map — Trip ID → Arrivals List
Groups all 300+ arrival predictions by trip ID. Used by the scheduling algorithm to find trains that serve both origin and destination stations, and to verify correct direction of travel.

---

## Benchmark

Type `benchmark` at the station prompt to run a full performance comparison. The benchmark runs 1000 iterations of each operation on a scaled dataset (100x the live arrivals) and reports:

- Latency — average time per operation in milliseconds
- Throughput — operations per second
- Estimated Memory — bytes used by each structure

Structures compared:
- Priority Queue vs Sorted Vector (insert + retrieve minimum)
- Hash Map vs Linear Vector Search (station name lookup)


## How to Compile and Run

### Requirements
- macOS with Xcode installed
- Internet connection (live API data fetched on every run)

### Commands
- Enter a station name to begin a route lookup
- Type `benchmark` to run the full performance benchmark
- Type `quit` to exit


## System Architecture

```
MBTA V3 API → libcurl → nlohmann/json → Priority Queue + Hash Maps → CLI
```


## API

Uses the [MBTA V3 API](https://api-v3.mbta.com) — free and public.
Endpoint: `/predictions?filter[route]=Green-E&include=stop`

---

## Author

Emma Bernstein — Advanced Data Structures, Durham Academy, 2026
Tufts University Class of 2030!!!!
