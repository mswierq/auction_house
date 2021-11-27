//
// Created by mswiercz on 22.11.2021.
//
#pragma once
#include "events.h"
#include "tasks.h"
#include <deque>
#include <future>
#include <mutex>

namespace auction_house::engine {
class TasksQueue {
public:
  void enqueue(Task &&task);
  Task pop();

private:
  std::deque<Task> _queue;
  std::mutex _mutex;
  std::condition_variable _cv;
};
} // namespace auction_house::engine