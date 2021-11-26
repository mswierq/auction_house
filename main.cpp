//
// Created by mswiercz on 19.11.2021.
//
#include "database.h"
#include "tasks.h"
#include "tasks_queue.h"
#include <thread>

int main() {
  auction_engine::Accounts accounts;
  auction_engine::AuctionList auctions;
  auction_engine::SessionManager sessions;
  auction_engine::Database database{accounts, auctions, sessions};
  auction_engine::TasksQueue queue;

  auto auctions_proc{[&database, &queue]() {
    for (;;) {
      database.auctions.wait_for_expired();
      auto expired_list = database.auctions.collect_expired();

      for (auto &auction : expired_list) {
        auto notify_seller =
            auction_engine::create_auction_task(std::move(auction), database);

        queue.enqueue(std::move(notify_seller));
      }
    }
  }};
}