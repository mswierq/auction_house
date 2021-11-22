//
// Created by mswiercz on 22.11.2021.
//
#include "tasks_queue.h"
#include <catch2/catch.hpp>
#include <future>
#include <vector>

using namespace auction_engine;

auto compare_events = [](auto &a, auto &b) {
  return a.username == b.username && a.session_id == b.session_id &&
         a.data == b.data;
};

TEST_CASE("Threads talks to each other over queue", "[TasksQueue]") {
  TasksQueue queue;
  std::vector<Event> results;

  SECTION("One writer and one reader") {
    std::thread reader{[&queue, &results]() {
      for(auto i = 0; i < 4; ++i) {
        auto f = queue.pop();
        f.wait();
        results.push_back(f.get());
      }
    }};

    std::thread writer{[&queue]() {
      queue.enqueue(std::async([]() { return Event{"user_0", 1, "data_0"}; }));
      queue.enqueue(std::async([]() { return Event{"user_1", 2, "data_1"}; }));
      queue.enqueue(std::async([]() { return Event{"user_2", 3, "data_2"}; }));
      queue.enqueue(std::async([]() { return Event{"user_3", 4, "data_3"}; }));
    }};

    reader.join();
    writer.join();

    std::vector expected{
        Event{"user_0", 1, "data_0"}, Event{"user_1", 2, "data_1"},
        Event{"user_2", 3, "data_2"}, Event{"user_3", 4, "data_3"}};

    using Catch::Matchers::UnorderedEquals;
    REQUIRE(std::is_permutation(results.cbegin(), results.cend(),
                                expected.cbegin(), compare_events));
  }

  SECTION("Two writers and one reader") {
    std::thread reader{[&queue, &results]() {
      for(auto i = 0; i < 8; ++i) {
        auto f = queue.pop();
        f.wait();
        results.push_back(f.get());
      }
    }};

    std::thread writer_1{[&queue]() {
      queue.enqueue(std::async([]() {
        return Event{"user_10", 11, "data_0"};
      }));
      queue.enqueue(std::async([]() {
        return Event{"user_11", 12, "data_1"};
      }));
      queue.enqueue(std::async([]() {
        return Event{"user_12", 13, "data_2"};
      }));
      queue.enqueue(std::async([]() {
        return Event{"user_13", 14, "data_3"};
      }));
    }};

    std::thread writer_2{[&queue]() {
      queue.enqueue(std::async([]() {
        return Event{"user_20", 21, "data_0"};
      }));
      queue.enqueue(std::async([]() {
        return Event{"user_21", 22, "data_1"};
      }));
      queue.enqueue(std::async([]() {
        return Event{"user_22", 23, "data_2"};
      }));
      queue.enqueue(std::async([]() {
        return Event{"user_23", 24, "data_3"};
      }));
    }};

    reader.join();
    writer_1.join();
    writer_2.join();

    std::vector expected{
        Event{"user_10", 11, "data_0"}, Event{"user_11", 12, "data_1"},
        Event{"user_12", 13, "data_2"}, Event{"user_13", 14, "data_3"},
        Event{"user_20", 21, "data_0"}, Event{"user_21", 22, "data_1"},
        Event{"user_22", 23, "data_2"}, Event{"user_23", 24, "data_3"}};

    REQUIRE(std::is_permutation(results.cbegin(), results.cend(),
                                expected.cbegin(), compare_events));
  }
}