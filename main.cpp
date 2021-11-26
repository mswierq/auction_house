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
  auction_engine::Network network{database, queue};

  // Auctions processor
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

  // Tasks processor
  std::thread tasks_proc{[&database, &queue, &network]() {
    for (;;) {
      auto task = queue.pop();
      task.wait();

      try {
        auto event = task.get();
        if (event.session_id.has_value()) {
          auto session_id = event.session_id.value();
          auto connection =
              database.sessions.get_connection_id(event.session_id.value());
          if (connection.has_value()) {
            auto connection_id = connection.value();
            spdlog::debug("Sending reply to session {}, connection {}, data {}", session_id, connection_id, event.data);
            network.send_data(connection_id, event.data);
          } else {
            spdlog::debug("Dropping event, lack of connection "
                          "for session {}, data: {}",
                          event.session_id.value(), event.data);
          }
        } else {
          spdlog::debug("Dropping event with data: {}", event.data);
        }
      } catch (const std::exception &e) {
        spdlog::error("Couldn't handle task: {}", e.what());
      }
    }
  }};

  // Sessions processor
  network.receive_data();

  auctions_proc.join();
  tasks_proc.join();
}