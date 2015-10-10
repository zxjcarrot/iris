#include <unistd.h>


#include <cstdio>
#include <chrono>
#include <limits>
#include <cmath>

#include <level_logger.h>
#include <file_writer.h>

#include <map>
#include <thread>
#include <vector>
/*
using log_t = reckless::severity_log<
    reckless::indent<4>,       // 4 spaces of indent
    ' '                       // Field separator
    >;
reckless::file_writer rwriter("log_reckless.txt");
log_t r_log(&rwriter, 102400, 65534, 102400);
*/

iris::file_writer writer("./log.txt");
iris::level_logger g_log(&writer, iris::INFO);
int freq_map[50000000];

long long g_max_lat;
long long g_min_lat;
#define ITERATIONS 1e6

void worker_thread() {
    std::hash<std::thread::id> hasher;
    long long max_lat = std::numeric_limits<long long>::lowest(), min_lat = std::numeric_limits<long long>::max(), avg_lat = 0, sum = 0;
    size_t tid = hasher(std::this_thread::get_id());
    g_log.set_thread_queue_size(65534);
    g_log.set_thread_ringbuf_size(655340);
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 1; i <= ITERATIONS; ++i) {
        auto start = std::chrono::high_resolution_clock::now();
        g_log.info("hello %s, world %d, there %d, here %d, idx: %d", "dsads", 231, 0, 0, i);
        auto finish = std::chrono::high_resolution_clock::now();
        long long lat = std::chrono::duration_cast<std::chrono::nanoseconds>(finish-start).count();
        //sum += lat;
        max_lat = std::max(max_lat, lat);
        min_lat = std::min(min_lat, lat);
        g_max_lat = std::max(max_lat, g_max_lat);
        g_min_lat = std::min(min_lat, g_min_lat);
        //avg_lat = sum / i;
        if (lat < 50000000)
            freq_map[lat]++;
        //if (i % 10000 == 0)
        //    usleep(1000);
    }
    auto finish = std::chrono::high_resolution_clock::now();
    long long lat = std::chrono::duration_cast<std::chrono::nanoseconds>(finish-start).count();
    printf("thread_id: %lu max_lat: %lldns, min_lat: %lldns, avg_lat: %lldns, latency sum %lldns\n", tid, 0ll, min_lat, (long long)lat / (long long)ITERATIONS, lat);
}
int main(int argc, char const *argv[]) {
    g_max_lat = 0;
    g_min_lat = std::numeric_limits<int>::max();
    int n = 1;
    if (argc > 1) {
        n = std::stoi(argv[1]);
    }
    std::vector<std::thread> workers;

    for (int i = 0; i < n; ++i) {
        workers.push_back(std::thread(worker_thread));
    }
    for (int i = 0; i < n; ++i) {
        workers[i].join();
    }
    printf("\nlatency,count\n");
    g_max_lat = std::min(50000000 - 1, (int)g_max_lat);
    for (int i = g_min_lat; i <= 400; ++i) {
        if (freq_map[i])
            printf("%d,%d\n", i, freq_map[i]);
    }
    //printf("\nno,latency\n");
    //for (size_t i = 0; i < lats.size(); ++i) {
    //    printf("%zu,%d\n", i + 1, lats[i]);
    //}
    //g_log.sync_and_close();
    return 0;
}
