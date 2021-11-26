//
// Created by mswiercz on 19.11.2021.
//
#include "database.h"
#include "events.h"
#include "tasks.h"
#include "tasks_queue.h"
#include <spdlog/spdlog.h>
#include <thread>

int main() {
  spdlog::set_level(spdlog::level::info);

  auction_engine::Accounts accounts;
  auction_engine::AuctionList auctions;
  auction_engine::SessionManager sessions;
  auction_engine::Database database{accounts, auctions, sessions};
  auction_engine::TasksQueue queue;

  std::thread auctions_proc{[&database, &queue]() {
    for (;;) {
      database.auctions.wait_for_expired();
      auto expired_list = database.auctions.collect_expired();

      spdlog::debug("Collected {} expired auctions", expired_list.size());

      for (auto &auction : expired_list) {
        auto notify_seller =
            auction_engine::create_auction_task(std::move(auction), database);

        queue.enqueue(std::move(notify_seller));
      }
    }
  }};

  std::thread tasks_proc{[&queue]() {
    for (;;) {
      auto task = queue.pop();
      task.wait();

      try {
        auto event = task.get();
        if (event.session_id.has_value()) {
          spdlog::debug("Send to session {} data: {}", event.session_id.value(),
                        event.data);
        } else {
          spdlog::debug("Dropping event with data: {}", event.data);
        }
      } catch (const std::exception &e) {
        spdlog::error("Couldn't handle task: {}", e.what());
      }
    }
  }};

  sessions.start_session(1, 1);
  sessions.start_session(2, 2);
  queue.enqueue(auction_engine::create_command_task({{}, 1, "HELP"}, database));
  queue.enqueue(
      auction_engine::create_command_task({{}, 1, "LOGIN user"}, database));
  queue.enqueue(
      auction_engine::create_command_task({{}, 1, "SHOW FUNDS"}, database));
  queue.enqueue(
      auction_engine::create_command_task({"user", 1, "SHOW FUNDS"}, database));
  queue.enqueue(auction_engine::create_command_task(
      {"user", 1, "DEPOSIT FUNDS 100"}, database));
  queue.enqueue(
      auction_engine::create_command_task({"user", 1, "SHOW FUNDS"}, database));
  queue.enqueue(auction_engine::create_command_task(
      {"user", 1, "DEPOSIT ITEM item"}, database));
  queue.enqueue(
      auction_engine::create_command_task({"user", 1, "SHOW Items"}, database));
  queue.enqueue(
      auction_engine::create_command_task({"user", 1, "SHOW SALES"}, database));
  queue.enqueue(auction_engine::create_command_task(
      {"user", 1, "SELL item 100 2"}, database));
  queue.enqueue(
      auction_engine::create_command_task({"user", 1, "SHOW SALES"}, database));
  std::this_thread::sleep_for(std::chrono::seconds(3));
  queue.enqueue(
      auction_engine::create_command_task({"user", 1, "SHOW SALES"}, database));

  auctions_proc.join();
  tasks_proc.join();
}