#include <atomic>
#include <chrono>
#include <fstream>
#include <iceoryx_posh/internal/roudi/roudi.hpp>
#include <iceoryx_posh/popo/publisher.hpp>
#include <iceoryx_posh/popo/subscriber.hpp>
#include <iceoryx_posh/roudi/iceoryx_roudi_components.hpp>
#include <iceoryx_posh/runtime/posh_runtime_single_process.hpp>
#include <iostream>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

constexpr int    kQueueSize = 256;
constexpr int    kIterations = 200 * 1200;
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

void Publisher() {
  iox::popo::PublisherOptions publisherOptions;
  publisherOptions.historyCapacity = 0U;
  iox::popo::Publisher<Data> publisher(iox::capro::ServiceDescription("single", "process", "demo"), publisherOptions);

  // The first iteration's last_send_time will always be wrong, and we're OK with that
  int64_t last_send_time = -1;
  for (auto i = 0; i < kIterations; ++i) {
    auto start = std::chrono::high_resolution_clock::now();
    publisher.loan().and_then([&last_send_time](auto& sample) {
      sample->last_send_time = last_send_time;
      sample.publish();
    });
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
  iox::popo::SubscriberOptions options;
  options.queueCapacity = kQueueSize;
  options.historyRequest = 0U;
  iox::popo::Subscriber<Data> subscriber(iox::capro::ServiceDescription("single", "process", "demo"), options);

  Data              data;
  std::vector<Data> buf;
  buf.reserve(kQueueSize * 2);

  while (subscriber.getSubscriptionState() != iox::SubscribeState::SUBSCRIBED) {
    continue;
  }

  auto last_write_time = std::chrono::high_resolution_clock::now();

  while (true) {
    auto now = std::chrono::high_resolution_clock::now();
    if (now - last_write_time > 1s) {
      WriteDataAndClearBuf(buf);
      last_write_time = now;
    }

    bool has_more_samples = true;
    subscriber.take()
      .and_then([&](auto& sample) {
        buf.push_back(*sample);
      })
      .or_else([&](auto& result) {
        has_more_samples = false;
        if (result != iox::popo::ChunkReceiveResult::NO_CHUNK_AVAILABLE) {
          // TODO...
        }
      });

    if (has_more_samples) {
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
  iox::RouDiConfig_t                 default_config = iox::RouDiConfig_t().setDefaults();
  iox::roudi::IceOryxRouDiComponents iceoryx_roudi_components(default_config);
  iox::roudi::RouDi                  iceoryx_roudi(
                     iceoryx_roudi_components.rouDiMemoryManager,
                     iceoryx_roudi_components.portManager,
                     iox::roudi::RouDi::RoudiStartupParameters(iox::roudi::MonitoringMode::OFF, false));
  iox::runtime::PoshRuntimeSingleProcess iceoryx_runtime("iceoryx_mpnc");

  data_file.open("data/iceoryx_mpnc.csv");
  std::thread sub(Subscriber);
  std::thread pub(Publisher);

  pub.join();
  sub.join();

  data_file.close();
  return 0;
}
