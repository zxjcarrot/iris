#include <unistd.h>


#include <cstdio>
#include <chrono>
#include <limits>
#include <cmath>
#include <log.h>
#include <file_writer.h>
#include <map>
iris::file_writer writer("./log.txt");
iris::logger g_log(iris::INFO, &writer);

std::map<int, int> freq_map;
//std::vector<int>   lats;
#define ITERATIONS 1e4
int main(int argc, char const *argv[]) {
    long long max_lat = std::numeric_limits<long long>::lowest(), min_lat = std::numeric_limits<long long>::max(), avg_lat = 0, sum = 0;
    for (int i = 1; i <= ITERATIONS; ++i) {
        auto start = std::chrono::high_resolution_clock::now();
        g_log.info("hello %s, world %d, there %lf, here %u", "dsads", 231, 0.11, 0);
        auto finish = std::chrono::high_resolution_clock::now();
        long long lat = std::chrono::duration_cast<std::chrono::nanoseconds>(finish-start).count();
        sum += lat;
        max_lat = std::max(max_lat, lat);
        min_lat = std::min(min_lat, lat);
        avg_lat = sum / i;
        freq_map[lat]++;
        //lats.push_back(lat);
    }
    printf("max_lat: %lldns, min_lat: %lldns, avg_lat: %lldns, latency sum %lldns\n", max_lat, min_lat, avg_lat, sum);
    printf("\nlatency,count\n");
    for (auto kv : freq_map) {
        printf("%d,%d\n", kv.first, kv.second);
    }
    //printf("\nno,latency\n");
    //for (size_t i = 0; i < lats.size(); ++i) {
    //    printf("%zu,%d\n", i + 1, lats[i]);
    //}
    return 0;
}