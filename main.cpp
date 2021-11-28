//
// Created by mswiercz on 19.11.2021.
//
#include "database.h"
#include "network.h"
#include "session_processor.h"
#include "tasks.h"
#include "tasks_queue.h"
#include <iostream>
#include <spdlog/spdlog.h>
#include <thread>

std::uint16_t parse_arguments(int argc, char *argv[]) {
  spdlog::set_level(spdlog::level::info);
  auto port = 10000; // default
  try {
    if (argc < 5) {
      auto read_port = false;
      for (auto i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--debug") == 0 && read_port == false) {
          spdlog::set_level(spdlog::level::debug);
        } else if (std::strcmp(argv[i], "--port") == 0 && read_port == false) {
          read_port = true;
        } else if (read_port) {
          auto parsed_port =
              std::stoul(std::string{argv[i], std::strlen(argv[i])});
          if (parsed_port > 65535) {
            throw std::out_of_range{""};
          }
          port = static_cast<std::uint16_t>(parsed_port);
          read_port = false;
        } else {
          throw std::invalid_argument{""};
        }
      }
      if (read_port) {
        throw std::invalid_argument{""};
      }
    }
  } catch (std::invalid_argument &) {
    std::cerr << "Wrong arguments! Allowed: [--port <port>] [--debug]"
              << std::endl;
    std::exit(1);
  } catch (std::out_of_range &) {
    std::cerr << "Incorrect port number!" << std::endl;
    std::exit(1);
  }
  return port;
}

int main(int argc, char *argv[]) {
  auto port = parse_arguments(argc, argv);

  auction_house::engine::Accounts accounts;
  auction_house::engine::AuctionList auctions;
  auction_house::engine::SessionManager sessions;
  auction_house::engine::Database database{accounts, auctions, sessions};
  auction_house::engine::TasksQueue queue;
  auction_house::engine::SessionProcessor session_proc{database, queue};

  // Auctions processor
  std::thread auctions_proc{[&database, &queue]() {
    for (;;) {
      database.auctions.wait_for_expired();
      auto expired_list = database.auctions.collect_expired();

      spdlog::debug("Collected {} expired auctions", expired_list.size());

      for (auto &auction : expired_list) {
        auto notify_seller = auction_house::engine::create_auction_task(
            std::move(auction), database);

        queue.enqueue(std::move(notify_seller));
      }
    }
  }};

  // Tasks processor
  std::thread tasks_proc{[&database, &queue]() {
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
            spdlog::debug("Sending reply to session {}, connection {}, data {}",
                          session_id, connection_id, event.data);
            auction_house::network::send_data(connection_id,
                                              std::move(event.data));
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
  session_proc.serve_ingress(port);

  auctions_proc.join();
  tasks_proc.join();
}