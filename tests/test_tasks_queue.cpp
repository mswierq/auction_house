//
// Created by mswiercz on 22.11.2021.
//
#include "tasks_queue.h"
#include <catch2/catch.hpp>
#include <future>
#include <vector>

using namespace auction_engine;

auto compare_events = [](auto &a, auto &b) {
  return a.session_id == b.session_id && a.data == b.data;
};

TEST_CASE("Threads talks to each other over queue", "[TasksQueue]") {
  TasksQueue queue;
  std::vector<EgressEvent> results;

  SECTION("One writer and one reader") {
    std::thread reader{[&queue, &results]() {
      for (auto i = 0; i < 4; ++i) {
        auto f = queue.pop();
        f.wait();
        results.push_back(f.get());
      }
    }};

    std::thread writer{[&queue]() {
      queue.enqueue(std::async([]() { return EgressEvent{1, "data_0"}; }));
      queue.enqueue(std::async([]() { return EgressEvent{2, "data_1"}; }));
      queue.enqueue(std::async([]() { return EgressEvent{3, "data_2"}; }));
      queue.enqueue(std::async([]() { return EgressEvent{4, "data_3"}; }));
    }};

    reader.join();
    writer.join();

    std::vector expected{EgressEvent{1, "data_0"}, EgressEvent{2, "data_1"},
                         EgressEvent{3, "data_2"}, EgressEvent{4, "data_3"}};

    using Catch::Matchers::UnorderedEquals;
    REQUIRE(std::is_permutation(results.cbegin(), results.cend(),
                                expected.cbegin(), compare_events));
  }

  SECTION("Two writers and one reader") {
    std::thread reader{[&queue, &results]() {
      for (auto i = 0; i < 8; ++i) {
        auto f = queue.pop();
        f.wait();
        results.push_back(f.get());
      }
    }};

    std::thread writer_1{[&queue]() {
      queue.enqueue(std::async([]() { return EgressEvent{11, "data_0"}; }));
      queue.enqueue(std::async([]() { return EgressEvent{12, "data_1"}; }));
      queue.enqueue(std::async([]() { return EgressEvent{13, "data_2"}; }));
      queue.enqueue(std::async([]() { return EgressEvent{14, "data_3"}; }));
    }};

    std::thread writer_2{[&queue]() {
      queue.enqueue(std::async([]() { return EgressEvent{21, "data_0"}; }));
      queue.enqueue(std::async([]() { return EgressEvent{22, "data_1"}; }));
      queue.enqueue(std::async([]() { return EgressEvent{23, "data_2"}; }));
      queue.enqueue(std::async([]() { return EgressEvent{24, "data_3"}; }));
    }};

    reader.join();
    writer_1.join();
    writer_2.join();

    std::vector expected{EgressEvent{11, "data_0"}, EgressEvent{12, "data_1"},
                         EgressEvent{13, "data_2"}, EgressEvent{14, "data_3"},
                         EgressEvent{21, "data_0"}, EgressEvent{22, "data_1"},
                         EgressEvent{23, "data_2"}, EgressEvent{24, "data_3"}};

    REQUIRE(std::is_permutation(results.cbegin(), results.cend(),
                                expected.cbegin(), compare_events));
  }
}