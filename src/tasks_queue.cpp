//
// Created by mswiercz on 22.11.2021.
//
#include "tasks_queue.h"

namespace auction_engine {
void TasksQueue::enqueue(Task&& task) {
  {
    std::lock_guard _l {_mutex};
    _queue.push_back(std::move(task));
  }
  _cv.notify_one();
}

Task TasksQueue::pop() {
  std::unique_lock l{_mutex};
  _cv.wait(l, [this]{return !this->_queue.empty();});
  auto task = std::move(_queue.front());
  _queue.pop_front();
  return task;
}
}