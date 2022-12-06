#include <atomic>
#include <boost/lockfree/spsc_queue.hpp>
#include <chrono>
#include <fstream>
#include <iostream>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

constexpr int    kQueueSize = 2000;
constexpr int    kIterations = 200 * 1200;  // 20 minute test
std::atomic_bool done{false};
std::ofstream    data_file;

struct Data {
  int64_t last_send_time;

  // Fill this with some data so it's not a very small struct.
  int64_t v1 = 1'000'000;
  int64_t v2 = 4'242'424;
  int64_t v3 = 8'858'858;
  int64_t v4 = 100'000'000;
  int64_t v5 = 102'231'413;
};

using BoostSpscQueue = boost::lockfree::spsc_queue<Data, boost::lockfree::capacity<kQueueSize>>;

BoostSpscQueue queue;

void Publisher() {
  // The first iteration's last_send_time will always be wrong, and we're OK with that
  int64_t last_send_time = -1;
  for (auto i = 0; i < kIterations; ++i) {
    Data data{last_send_time};

    auto start = std::chrono::high_resolution_clock::now();
    queue.push(data);
    auto end = std::chrono::high_resolution_clock::now();

    last_send_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();

    // 200 Hz
    std::this_thread::sleep_for(5ms);
  }

  done.store(true);
}

void WriteDataAndClearBuf(std::vector<Data>& buf) {
  std::cerr << "writing " << buf.size() << " data points to file\n";
  for (const auto& data : buf) {
    data_file << data.last_send_time << "," << data.v1 << "," << data.v2 << "," << data.v3 << "," << data.v4 << "\n";
  }

  buf.clear();
}

void Subscriber() {
  Data              data;
  std::vector<Data> buf;
  buf.reserve(kQueueSize * 2);

  auto last_write_time = std::chrono::high_resolution_clock::now();

  while (true) {
    auto now = std::chrono::high_resolution_clock::now();
    if (now - last_write_time > 1s) {
      WriteDataAndClearBuf(buf);
      last_write_time = now;
    }

    if (queue.pop(data)) {
      buf.push_back(data);
      continue;
    }

    if (done.load(std::memory_order_relaxed)) {
      break;
    }

    std::this_thread::sleep_for(10ms);
  }

  WriteDataAndClearBuf(buf);
}

int main() {
  data_file.open("data/boost_spsc.csv");

  std::thread sub(Subscriber);
  std::thread pub(Publisher);

  pub.join();
  sub.join();

  data_file.close();
  return 0;
}
