# Real-Time-API-Capstone-Project


# MBTA Green Line E - Real-Time Route Finder

A real-time C++ application that fetches live train arrival data from the MBTA API and helps users find the fastest route between any two stations on the Green Line E — including to Medford/Tufts, home of Tufts University.

## Data Structures Used

- **Priority Queue (Min-Heap)**: Orders incoming train arrivals by departure time in O(log n) per insertion, enabling efficient real-time scheduling without re-sorting the entire dataset on each update.
- **Hash Map (unordered_map)**: Provides O(1) station name lookup, allowing instant conversion between station names and internal MBTA stop IDs.

## How to Run

### Requirements
- macOS with Xcode installed
- Internet connection (live API data)

### Compile
```bash
/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/clang++ -std=c++17 -isysroot /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk -o mbta main.cpp -lcurl -I. 
```

### Run
```bash
./mbta
```

## Features
- Live train predictions from the MBTA V3 API
- Route finder between any two Green Line E stations
- Eastbound and westbound direction detection
- Shows next 3 trains with departure and arrival times
- Built-in benchmark mode comparing Priority Queue vs Sorted Vector

## Benchmark
Type `benchmark` at the station prompt to compare Priority Queue vs Sorted Vector performance on the live dataset. The Priority Queue offers O(log n) insertion vs O(n log n) for re-sorting a vector — critical for real-time streaming systems.

## API
Uses the [MBTA V3 API](https://api-v3.mbta.com) — free and public.
