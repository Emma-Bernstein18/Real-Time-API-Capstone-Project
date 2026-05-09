// Emma Bernstein
// Advanced Data Structures Capstone
// MBTA Green Line E - Real-Time Train Scheduler
// Fetches live train predictions from the MBTA V3 API and lets users
// find upcoming departures between any two stations on the Green Line E.
// Data structures used: priority queue (min-heap), two hash maps (unordered_map)

#include <iostream>
#include <queue>
#include <string>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <curl/curl.h>
#include "json.hpp"

using namespace std;
using json = nlohmann::json;
using namespace chrono;

// The Green Line E stations in order from west to east.
// Index position is used to determine travel direction —
// if destination index > origin index, train goes eastbound.
vector<string> lineOrder = {
    "place-hsmnl", "place-bckhl", "place-rvrwy", "place-mispk",
    "place-fenwd", "place-brmnl", "place-lngmd", "place-mfa",
    "place-nuniv", "place-symcl", "place-prmnl", "place-coecl",
    "place-armnl", "place-boyls", "place-pktrm", "place-gover",
    "place-haecl", "place-north", "place-spmnl", "place-lech",
    "place-esomr", "place-gilmn", "place-mgngl", "place-balsq",
    "place-mdftf"
};

// Hash map #1: internal MBTA stop ID -> human-readable station name
// Used to display readable names in output instead of raw API codes
unordered_map<string, string> stationNames = {
    {"place-hsmnl", "Heath Street"},
    {"place-bckhl", "Back of the Hill"},
    {"place-rvrwy", "Riverway"},
    {"place-mispk", "Mission Park"},
    {"place-fenwd", "Fenwood Road"},
    {"place-brmnl", "Brigham Circle"},
    {"place-lngmd", "Longwood Medical Area"},
    {"place-mfa",   "Museum of Fine Arts"},
    {"place-nuniv", "Northeastern University"},
    {"place-symcl", "Symphony"},
    {"place-prmnl", "Prudential"},
    {"place-coecl", "Copley"},
    {"place-armnl", "Arlington"},
    {"place-boyls", "Boylston"},
    {"place-pktrm", "Park Street"},
    {"place-gover", "Government Center"},
    {"place-haecl", "Haymarket"},
    {"place-north", "North Station"},
    {"place-spmnl", "Science Park/West End"},
    {"place-lech",  "Lechmere"},
    {"place-esomr", "East Somerville"},
    {"place-gilmn", "Gilman Square"},
    {"place-mgngl", "Magoun Square"},
    {"place-balsq", "Ball Square"},
    {"place-mdftf", "Medford/Tufts"}
};

// Hash map #2: station name -> internal MBTA stop ID
// Built at runtime by reversing stationNames.
// Allows O(1) lookup when user types a station name.
unordered_map<string, string> nameToId;

// Represents a single train arrival prediction from the MBTA API.
// arrivalTime is stored as seconds since midnight for easy comparison.
// The comparison operators allow TrainArrival to work inside a priority queue —
// operator> makes the priority queue behave as a min-heap (soonest time at top).
struct TrainArrival {
    int arrivalTime;       // seconds since midnight
    string tripId;         // unique ID for this train trip
    string parentStopId;   // parent station ID (e.g. "place-symcl")

    bool operator>(const TrainArrival& other) const {
        return arrivalTime > other.arrivalTime;
    }
    bool operator<(const TrainArrival& other) const {
        return arrivalTime < other.arrivalTime;
    }
};

// Required callback function for libcurl.
// curl doesn't return data directly — instead it calls this function
// repeatedly as data arrives, appending each chunk to our response string.
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, string* output) {
    output->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// Makes an HTTP GET request to the given URL using libcurl
// and returns the full response body as a string.
string fetchData(const string& url) {
    CURL* curl = curl_easy_init();
    string response;
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_perform(curl);
        curl_easy_cleanup(curl);
    }
    return response;
}

// Converts total seconds since midnight into a readable HH:MM:SS string.
// Example: 73800 -> "20:30:00"
string formatTime(int totalSeconds) {
    int h = totalSeconds / 3600;
    int m = (totalSeconds % 3600) / 60;
    int s = totalSeconds % 60;
    return to_string(h) + ":" + (m < 10 ? "0" : "") + to_string(m)
         + ":" + (s < 10 ? "0" : "") + to_string(s);
}

// Benchmarks the three main data structures against simpler alternatives.
// Runs 1000 iterations of each operation on a scaled dataset to get
// stable average results, then reports latency, throughput, and memory.
void runBenchmark(const vector<TrainArrival>& allArrivals) {

    const int ITERATIONS = 1000;

    // Scale up the dataset 100x to simulate a larger real-time workload.
    // This makes performance differences between structures more visible.
    vector<TrainArrival> benchmarkData;
    for (int i = 0; i < 100; i++) {
        benchmarkData.insert(benchmarkData.end(), allArrivals.begin(), allArrivals.end());
    }

    int n = benchmarkData.size();

    cout << "\n========== BENCHMARK RESULTS ==========\n";
    cout << "Dataset Size: " << n << " simulated arrivals\n\n";
    cout << left
         << setw(20) << "Operation"
         << setw(20) << "Structure"
         << setw(20) << "Latency"
         << setw(25) << "Throughput"
         << setw(20) << "Estimated Memory"
         << "\n";
    cout << string(105, '-') << "\n";

    // -------------------------------------------------------
    // PRIORITY QUEUE vs SORTED VECTOR
    // Both insert all arrivals and retrieve the minimum.
    // Priority queue uses O(log n) per insert.
    // Sorted vector uses O(n log n) to sort the whole thing.
    // -------------------------------------------------------

    // Priority Queue benchmark
    long long totalPQ = 0;
    for (int i = 0; i < ITERATIONS; i++) {
        auto start = high_resolution_clock::now();

        // Insert all arrivals into a min-heap priority queue.
        // The smallest arrivalTime automatically floats to the top.
        priority_queue<TrainArrival, vector<TrainArrival>, greater<TrainArrival>> pq;
        for (auto& a : benchmarkData) pq.push(a);
        volatile auto top = pq.top(); // peek at the soonest arrival

        auto end = high_resolution_clock::now();
        totalPQ += duration_cast<nanoseconds>(end - start).count();
    }

    double avgPQns = (double)totalPQ / ITERATIONS;
    double avgPQms = avgPQns / 1e6;
    double pqThroughput = n / (avgPQns / 1e9);
    size_t pqMemory = n * sizeof(TrainArrival);

    cout << left
         << setw(20) << "Insert + Peek"
         << setw(20) << "PriorityQueue"
         << setw(20) << (to_string(avgPQms).substr(0, 8) + "ms")
         << setw(25) << (to_string((long long)pqThroughput) + " ops/sec")
         << setw(20) << (to_string(pqMemory) + " bytes")
         << "\n";

    // Sorted Vector benchmark
    long long totalVec = 0;
    for (int i = 0; i < ITERATIONS; i++) {
        auto start = high_resolution_clock::now();

        // Copy the data and sort it using std::sort (introsort under the hood).
        // This is O(n log n) — fast for one-time sorting but expensive
        // if the dataset updates frequently.
        vector<TrainArrival> sortedVec = benchmarkData;
        sort(sortedVec.begin(), sortedVec.end());
        volatile auto first = sortedVec[0];

        auto end = high_resolution_clock::now();
        totalVec += duration_cast<nanoseconds>(end - start).count();
    }

    double avgVecns = (double)totalVec / ITERATIONS;
    double avgVecms = avgVecns / 1e6;
    double vecThroughput = n / (avgVecns / 1e9);
    size_t vecMemory = n * sizeof(TrainArrival);

    cout << left
         << setw(20) << "Full Sort"
         << setw(20) << "Sorted Vector"
         << setw(20) << (to_string(avgVecms).substr(0, 8) + "ms")
         << setw(25) << (to_string((long long)vecThroughput) + " ops/sec")
         << setw(20) << (to_string(vecMemory) + " bytes")
         << "\n";

    // -------------------------------------------------------
    // HASH MAP vs LINEAR SEARCH
    // Both look up station names. Hash map is O(1).
    // Linear search scans the whole list — O(n).
    // We run 100,000 lookups per iteration to get measurable times.
    // -------------------------------------------------------

    vector<string> stationKeys;
    for (auto& pair : nameToId) stationKeys.push_back(pair.first);
    const int HASH_WORKLOAD = 100000;

    // Hash Map benchmark
    long long totalHash = 0;
    for (int i = 0; i < ITERATIONS; i++) {
        auto start = high_resolution_clock::now();

        for (int j = 0; j < HASH_WORKLOAD; j++) {
            const string& key = stationKeys[j % stationKeys.size()];
            // O(1) average lookup — goes directly to the bucket
            volatile auto result = nameToId.find(key);
        }

        auto end = high_resolution_clock::now();
        totalHash += duration_cast<nanoseconds>(end - start).count();
    }

    double avgHashns = (double)totalHash / ITERATIONS;
    double avgHashms = avgHashns / 1e6;
    double hashThroughput = HASH_WORKLOAD / (avgHashns / 1e9);
    size_t hashMemory = nameToId.size() * (sizeof(string) * 2);

    cout << left
         << setw(20) << "Lookup"
         << setw(20) << "Hash Map"
         << setw(20) << (to_string(avgHashms).substr(0, 8) + "ms")
         << setw(25) << (to_string((long long)hashThroughput) + " ops/sec")
         << setw(20) << (to_string(hashMemory) + " bytes")
         << "\n";

    // Linear Search benchmark
    vector<pair<string,string>> nameVec(nameToId.begin(), nameToId.end());
    long long totalLinear = 0;
    for (int i = 0; i < ITERATIONS; i++) {
        auto start = high_resolution_clock::now();

        for (int j = 0; j < HASH_WORKLOAD; j++) {
            const string& target = stationKeys[j % stationKeys.size()];
            // O(n) linear scan — checks every element until it finds a match
            volatile auto it = find_if(nameVec.begin(), nameVec.end(),
                [&](const auto& q) { return q.first == target; });
        }

        auto end = high_resolution_clock::now();
        totalLinear += duration_cast<nanoseconds>(end - start).count();
    }

    double avgLinearns = (double)totalLinear / ITERATIONS;
    double avgLinearMs = avgLinearns / 1e6;
    double linearThroughput = HASH_WORKLOAD / (avgLinearns / 1e9);
    size_t linearMemory = nameVec.size() * (sizeof(string) * 2);

    cout << left
         << setw(20) << "Linear Search"
         << setw(20) << "Vector"
         << setw(20) << (to_string(avgLinearMs).substr(0, 8) + "ms")
         << setw(25) << (to_string((long long)linearThroughput) + " ops/sec")
         << setw(20) << (to_string(linearMemory) + " bytes")
         << "\n";

    cout << string(105, '=') << "\n\n";

    // Calculate and print the hash map speedup vs linear search
    double speedup = avgLinearns / avgHashns;

    cout << fixed << setprecision(2);
    cout << "Key Takeaways:\n\n";
    cout << "Priority Queue:\n";
    cout << "  - O(log n) insertion — new arrivals slot in without re-sorting\n";
    cout << "  - Always returns the soonest departure in O(1)\n";
    cout << "  - Better than sorted vector for continuously updating data\n\n";
    cout << "Sorted Vector:\n";
    cout << "  - O(n log n) sorting — fast for one-time static datasets\n";
    cout << "  - Would require full re-sort on every new prediction\n\n";
    cout << "Hash Map:\n";
    cout << "  - O(1) average lookup — goes directly to the answer\n";
    cout << "  - " << speedup << "x faster than scanning a vector for the same key\n\n";
    cout << "Scalability:\n";
    cout << "  - Optimized data structures show larger advantages as dataset grows\n\n";
}

int main() {
    string apiKey = "c2000c13affe4ac8855e0ebb50034080";

    // Build the reverse lookup map: station name -> stop ID
    // This lets the user type a name and get the MBTA internal ID in O(1)
    for (auto& pair : stationNames) nameToId[pair.second] = pair.first;

    // Build a position index: stop ID -> index in lineOrder
    // Used to determine which direction a trip is traveling
    unordered_map<string, int> stationIndex;
    for (int i = 0; i < (int)lineOrder.size(); i++) stationIndex[lineOrder[i]] = i;

    cout << "========================================\n";
    cout << "   MBTA Green Line E - Train Scheduler\n";
    cout << "   Powered by Real-Time Arrival Data\n";
    cout << "========================================\n\n";

    // Fetch live predictions from the MBTA API.
    // We use include=stop so the response includes parent station data,
    // which lets us map child platform IDs back to parent station IDs.
    string url = "https://api-v3.mbta.com/predictions?filter%5Broute%5D=Green-E&include=stop&api_key=" + apiKey;
    cout << "Fetching live train data...\n";
    string response = fetchData(url);
    json data = json::parse(response);

    // The MBTA API returns child stop IDs (specific platforms) in predictions,
    // but we need parent station IDs to match against our station list.
    // Build a child -> parent lookup map from the included stop data.
    unordered_map<string, string> childToParent;
    if (data.contains("included")) {
        for (auto& stop : data["included"]) {
            string id = stop["id"];
            if (!stop["relationships"]["parent_station"]["data"].is_null()) {
                childToParent[id] = stop["relationships"]["parent_station"]["data"]["id"];
            } else {
                childToParent[id] = id; // already a parent station
            }
        }
    }

    // Parse each prediction from the API response into a TrainArrival struct.
    // We prefer departure_time over arrival_time for consistent direction checking.
    vector<TrainArrival> allArrivals;
    for (auto& p : data["data"]) {
        auto& attrs = p["attributes"];
        string timeStr = "";
        if (!attrs["departure_time"].is_null()) timeStr = attrs["departure_time"];
        else if (!attrs["arrival_time"].is_null()) timeStr = attrs["arrival_time"];
        else continue; // skip predictions with no time data

        // Parse HH:MM:SS from the ISO timestamp and convert to seconds since midnight
        int h = stoi(timeStr.substr(11, 2));
        int m = stoi(timeStr.substr(14, 2));
        int s = stoi(timeStr.substr(17, 2));

        string childId = p["relationships"]["stop"]["data"]["id"];
        string parentId = childToParent.count(childId) ? childToParent[childId] : childId;

        allArrivals.push_back({h * 3600 + m * 60 + s,
                               p["relationships"]["trip"]["data"]["id"],
                               parentId});
    }

    cout << "Loaded " << allArrivals.size() << " live arrivals.\n\n";

    cout << "Available stations:\n";
    for (auto& id : lineOrder) cout << "  " << stationNames[id] << "\n";
    cout << "\nCommands: enter a station name to plan a route, 'benchmark' to run benchmark, 'quit' to exit.\n\n";

    // Main interaction loop
    while (true) {
        cout << "Enter your current station:\n> ";
        string origin;
        getline(cin, origin);

        if (origin == "quit") break;

        if (origin == "benchmark") {
            runBenchmark(allArrivals);
            continue;
        }

        cout << "Enter your destination station:\n> ";
        string destination;
        getline(cin, destination);

        // O(1) hash map lookup for both station names
        if (nameToId.count(origin) == 0 || nameToId.count(destination) == 0) {
            cout << "Station not found. Please check spelling.\n\n";
            continue;
        }

        string originId = nameToId[origin];
        string destId = nameToId[destination];

        // Compare positions in lineOrder to determine travel direction
        int originIdx = stationIndex[originId];
        int destIdx = stationIndex[destId];

        if (originIdx == destIdx) { cout << "You're already there!\n\n"; continue; }

        bool goingEast = destIdx > originIdx;
        cout << "\nDirection: "
             << (goingEast ? "Eastbound toward Medford/Tufts" : "Westbound toward Heath Street")
             << "\n";

        // Group all predictions by trip ID using a hash map.
        // This lets us check which trips serve both origin and destination.
        unordered_map<string, vector<TrainArrival>> byTrip;
        for (auto& a : allArrivals) byTrip[a.tripId].push_back(a);

        // Priority queue to rank qualifying trips by departure time at origin.
        // min-heap so the soonest departure always comes out first.
        priority_queue<pair<int,string>, vector<pair<int,string>>, greater<pair<int,string>>> pq;

        for (auto& trip : byTrip) {
            int originTime = -1, destTime = -1;

            // Check if this trip has predictions at both origin and destination
            for (auto& a : trip.second) {
                if (a.parentStopId == originId) originTime = a.arrivalTime;
                if (a.parentStopId == destId)   destTime   = a.arrivalTime;
            }

            // Only include trips going in the right direction.
            // Eastbound: origin time must be before destination time.
            // Westbound: origin time must be after destination time.
            if (originTime != -1 && destTime != -1) {
                bool correct = goingEast ? (originTime < destTime) : (originTime > destTime);
                if (correct) pq.push({originTime, trip.first});
            }
        }

        if (pq.empty()) {
            cout << "No trains currently running that route.\n\n";
            continue;
        }

        // Pop the top 3 soonest departures from the priority queue and display them
        cout << "\nNext trains from " << origin << " to " << destination << ":\n";
        int shown = 0;
        while (!pq.empty() && shown < 3) {
            auto [depTime, tripId] = pq.top(); pq.pop();

            // Look up the arrival time at the destination for this trip
            int arrTime = -1;
            for (auto& a : byTrip[tripId])
                if (a.parentStopId == destId) arrTime = a.arrivalTime;

            cout << "  Train " << (shown + 1) << ": Departs at " << formatTime(depTime);
            if (arrTime != -1) {
                int travelMins = abs(arrTime - depTime) / 60;
                cout << " | Arrives at " << formatTime(arrTime);
                if (travelMins < 1) cout << " (< 1 min)";
                else cout << " (" << travelMins << " min)";
            }
            cout << "\n";
            shown++;
        }
        cout << "\n";
    }

    cout << "Goodbye! Safe travels!\n";
    return 0;
}