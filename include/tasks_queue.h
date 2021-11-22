//
// Created by mswiercz on 22.11.2021.
//
#pragma once
#include <deque>
#include <mutex>
#include <future>
#include "events.h"

namespace auction_engine {
using Task = std::future<Event>;

class TasksQueue {
public:
  void enqueue(Task&& task);
  Task pop();

private:
  std::deque<Task> _queue;
  std::mutex _mutex;
  std::condition_variable _cv;
};
}