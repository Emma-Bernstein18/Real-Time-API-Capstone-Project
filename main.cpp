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

vector<string> lineOrder = {
    "place-hsmnl", "place-bckhl", "place-rvrwy", "place-mispk",
    "place-fenwd", "place-brmnl", "place-lngmd", "place-mfa",
    "place-nuniv", "place-symcl", "place-prmnl", "place-coecl",
    "place-armnl", "place-boyls", "place-pktrm", "place-gover",
    "place-haecl", "place-north", "place-spmnl", "place-lech",
    "place-esomr", "place-gilmn", "place-mgngl", "place-balsq",
    "place-mdftf"
};

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

unordered_map<string, string> nameToId;

struct TrainArrival {
    int arrivalTime;
    string tripId;
    string parentStopId;
    bool operator>(const TrainArrival& other) const { return arrivalTime > other.arrivalTime; }
    bool operator<(const TrainArrival& other) const { return arrivalTime < other.arrivalTime; }
};

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, string* output) {
    output->append((char*)contents, size * nmemb);
    return size * nmemb;
}

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

string formatTime(int totalSeconds) {
    int h = totalSeconds / 3600;
    int m = (totalSeconds % 3600) / 60;
    int s = totalSeconds % 60;
    return to_string(h) + ":" + (m < 10 ? "0" : "") + to_string(m) + ":" + (s < 10 ? "0" : "") + to_string(s);
}

void runBenchmark(const vector<TrainArrival>& allArrivals) {

    const int ITERATIONS = 1000;

    // Create larger simulated dataset for scalability testing
    vector<TrainArrival> benchmarkData;

    for (int i = 0; i < 100; i++) {
        benchmarkData.insert(
            benchmarkData.end(),
            allArrivals.begin(),
            allArrivals.end()
        );
    }

    int n = benchmarkData.size();

    cout << "\n========== BENCHMARK RESULTS ==========\n";

    cout << "Dataset Size: "
         << n
         << " simulated arrivals\n\n";

    cout << left
         << setw(20) << "Operation"
         << setw(20) << "Structure"
         << setw(20) << "Latency"
         << setw(25) << "Throughput"
         << setw(20) << "Estimated Memory"
         << "\n";

    cout << string(105, '-') << "\n";

    // =====================================================
    // PRIORITY QUEUE BENCHMARK
    // =====================================================

    long long totalPQ = 0;

    for (int i = 0; i < ITERATIONS; i++) {

        auto start = high_resolution_clock::now();

        priority_queue<
            TrainArrival,
            vector<TrainArrival>,
            greater<TrainArrival>
        > pq;

        for (auto& a : benchmarkData) {
            pq.push(a);
        }

        volatile auto top = pq.top();

        auto end = high_resolution_clock::now();

        totalPQ += duration_cast<nanoseconds>(
            end - start
        ).count();
    }

    double avgPQns =
        (double)totalPQ / ITERATIONS;

    double avgPQms =
        avgPQns / 1e6;

    double pqThroughput =
        n / (avgPQns / 1e9);

    size_t pqMemory =
        n * sizeof(TrainArrival);

    cout << left
         << setw(20) << "Insert + Peek"
         << setw(20) << "PriorityQueue"
         << setw(20)
         << (to_string(avgPQms).substr(0, 8) + "ms")
         << setw(25)
         << (to_string((long long)pqThroughput) + " ops/sec")
         << setw(20)
         << (to_string(pqMemory) + " bytes")
         << "\n";

    // =====================================================
    // SORTED VECTOR BENCHMARK
    // =====================================================

    long long totalVec = 0;

    for (int i = 0; i < ITERATIONS; i++) {

        auto start = high_resolution_clock::now();

        vector<TrainArrival> sortedVec =
            benchmarkData;

        sort(
            sortedVec.begin(),
            sortedVec.end()
        );

        volatile auto first = sortedVec[0];

        auto end = high_resolution_clock::now();

        totalVec += duration_cast<nanoseconds>(
            end - start
        ).count();
    }

    double avgVecns =
        (double)totalVec / ITERATIONS;

    double avgVecms =
        avgVecns / 1e6;

    double vecThroughput =
        n / (avgVecns / 1e9);

    size_t vecMemory =
        n * sizeof(TrainArrival);

    cout << left
         << setw(20) << "Full Sort"
         << setw(20) << "Sorted Vector"
         << setw(20)
         << (to_string(avgVecms).substr(0, 8) + "ms")
         << setw(25)
         << (to_string((long long)vecThroughput) + " ops/sec")
         << setw(20)
         << (to_string(vecMemory) + " bytes")
         << "\n";

    // =====================================================
    // HASH MAP LOOKUP BENCHMARK
    // =====================================================

    vector<string> stationKeys;

    for (auto& pair : nameToId) {
        stationKeys.push_back(pair.first);
    }

    const int HASH_WORKLOAD = 100000;

    long long totalHash = 0;

    for (int i = 0; i < ITERATIONS; i++) {

        auto start = high_resolution_clock::now();

        for (int j = 0; j < HASH_WORKLOAD; j++) {

            const string& key =
                stationKeys[
                    j % stationKeys.size()
                ];

            volatile auto result =
                nameToId.find(key);
        }

        auto end = high_resolution_clock::now();

        totalHash += duration_cast<nanoseconds>(
            end - start
        ).count();
    }

    double avgHashns =
        (double)totalHash / ITERATIONS;

    double avgHashms =
        avgHashns / 1e6;

    double hashThroughput =
        HASH_WORKLOAD /
        (avgHashns / 1e9);

    size_t hashMemory =
        nameToId.size() *
        (sizeof(string) * 2);

    cout << left
         << setw(20) << "Lookup"
         << setw(20) << "Hash Map"
         << setw(20)
         << (to_string(avgHashms).substr(0, 8) + "ms")
         << setw(25)
         << (to_string((long long)hashThroughput) + " ops/sec")
         << setw(20)
         << (to_string(hashMemory) + " bytes")
         << "\n";

    // =====================================================
    // VECTOR LINEAR SEARCH BENCHMARK
    // =====================================================

    vector<pair<string,string>> nameVec(
        nameToId.begin(),
        nameToId.end()
    );

    long long totalLinear = 0;

    for (int i = 0; i < ITERATIONS; i++) {

        auto start = high_resolution_clock::now();

        for (int j = 0; j < HASH_WORKLOAD; j++) {

            const string& target =
                stationKeys[
                    j % stationKeys.size()
                ];

            volatile auto it =
                find_if(
                    nameVec.begin(),
                    nameVec.end(),

                    [&](const auto& q) {
                        return q.first == target;
                    }
                );
        }

        auto end = high_resolution_clock::now();

        totalLinear += duration_cast<nanoseconds>(
            end - start
        ).count();
    }

    double avgLinearns =
        (double)totalLinear / ITERATIONS;

    double avgLinearMs =
        avgLinearns / 1e6;

    double linearThroughput =
        HASH_WORKLOAD /
        (avgLinearns / 1e9);

    size_t linearMemory =
        nameVec.size() *
        (sizeof(string) * 2);

    cout << left
         << setw(20) << "Linear Search"
         << setw(20) << "Vector"
         << setw(20)
         << (to_string(avgLinearMs).substr(0, 8) + "ms")
         << setw(25)
         << (to_string((long long)linearThroughput) + " ops/sec")
         << setw(20)
         << (to_string(linearMemory) + " bytes")
         << "\n";

    cout << string(105, '=') << "\n\n";

    // =====================================================
    // ANALYSIS + TAKEAWAYS
    // =====================================================

    double speedup =
        avgLinearns / avgHashns;

    cout << fixed << setprecision(2);

    cout << "Key Takeaways:\n\n";

    cout << "Priority Queue:\n";
    cout << "  - O(log n) insertion\n";
    cout << "  - Avoids repeatedly sorting the entire dataset\n";
    cout << "  - Efficiently retrieves the next arriving train\n\n";

    cout << "Sorted Vector:\n";
    cout << "  - O(n log n) sorting\n";
    cout << "  - Fast for one-time sorting operations\n";
    cout << "  - STL sort is highly optimized for smaller datasets\n\n";

    cout << "Hash Map:\n";
    cout << "  - Average O(1) lookup time\n";
    cout << "  - "
         << speedup
         << "x faster than linear vector search\n";
    cout << "  - Ideal for fast station name lookups\n\n";

    cout << "Scalability Observation:\n";
    cout << "  - As dataset size increases, optimized data structures\n";
    cout << "    provide larger performance advantages.\n\n";
}

int main() {
    string apiKey = "c2000c13affe4ac8855e0ebb50034080";

    for (auto& pair : stationNames) nameToId[pair.second] = pair.first;

    unordered_map<string, int> stationIndex;
    for (int i = 0; i < (int)lineOrder.size(); i++) stationIndex[lineOrder[i]] = i;

    cout << "========================================\n";
    cout << "   MBTA Green Line E - Train Scheduler\n";
    cout << "   Powered by Real-Time Arrival Data\n";
    cout << "========================================\n\n";

    string url = "https://api-v3.mbta.com/predictions?filter%5Broute%5D=Green-E&include=stop&api_key=" + apiKey;
    cout << "Fetching live train data...\n";
    string response = fetchData(url);
    json data = json::parse(response);

    unordered_map<string, string> childToParent;
    if (data.contains("included")) {
        for (auto& stop : data["included"]) {
            string id = stop["id"];
            if (!stop["relationships"]["parent_station"]["data"].is_null()) {
                childToParent[id] = stop["relationships"]["parent_station"]["data"]["id"];
            } else {
                childToParent[id] = id;
            }
        }
    }

    vector<TrainArrival> allArrivals;
    for (auto& p : data["data"]) {
        auto& attrs = p["attributes"];
        string timeStr = "";
        if (!attrs["departure_time"].is_null()) timeStr = attrs["departure_time"];
        else if (!attrs["arrival_time"].is_null()) timeStr = attrs["arrival_time"];
        else continue;

        int h = stoi(timeStr.substr(11, 2));
        int m = stoi(timeStr.substr(14, 2));
        int s = stoi(timeStr.substr(17, 2));
        string childId = p["relationships"]["stop"]["data"]["id"];
        string parentId = childToParent.count(childId) ? childToParent[childId] : childId;
        allArrivals.push_back({h * 3600 + m * 60 + s, p["relationships"]["trip"]["data"]["id"], parentId});
    }

    cout << "Loaded " << allArrivals.size() << " live arrivals.\n\n";

    cout << "Available stations:\n";
    for (auto& id : lineOrder) cout << "  " << stationNames[id] << "\n";
    cout << "\nCommands: enter a station name to plan a route, 'benchmark' to run benchmark, 'quit' to exit.\n\n";

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

        if (nameToId.count(origin) == 0 || nameToId.count(destination) == 0) {
            cout << "Station not found. Please check spelling.\n\n";
            continue;
        }

        string originId = nameToId[origin];
        string destId = nameToId[destination];
        int originIdx = stationIndex[originId];
        int destIdx = stationIndex[destId];

        if (originIdx == destIdx) { cout << "You're already there!\n\n"; continue; }

        bool goingEast = destIdx > originIdx;
        cout << "\nDirection: " << (goingEast ? "Eastbound toward Medford/Tufts" : "Westbound toward Heath Street") << "\n";

        unordered_map<string, vector<TrainArrival>> byTrip;
        for (auto& a : allArrivals) byTrip[a.tripId].push_back(a);

        priority_queue<pair<int,string>, vector<pair<int,string>>, greater<pair<int,string>>> pq;

        for (auto& trip : byTrip) {
            int originTime = -1, destTime = -1;
            for (auto& a : trip.second) {
                if (a.parentStopId == originId) originTime = a.arrivalTime;
                if (a.parentStopId == destId)   destTime   = a.arrivalTime;
            }
            if (originTime != -1 && destTime != -1) {
                bool correct = goingEast ? (originTime < destTime) : (originTime > destTime);
                if (correct) pq.push({originTime, trip.first});
            }
        }

        if (pq.empty()) {
            cout << "No trains currently running that route.\n\n";
            continue;
        }

        cout << "\nNext trains from " << origin << " to " << destination << ":\n";
        int shown = 0;
        while (!pq.empty() && shown < 3) {
            auto [depTime, tripId] = pq.top(); pq.pop();
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