//
// Created by mswiercz on 24.11.2021.
//
#include "tasks.h"
#include "auction_processor.h"
#include "command.h"
#include "database.h"

namespace auction_house::engine {
Task create_command_task(IngressEvent &&event, Database &database) {
  return std::async(
      std::launch::deferred,
      [&database](IngressEvent event) {
        return Command::parse(std::move(event))->execute(database);
      },
      std::move(event));
}

Task create_auction_task(Auction &&auction, Database &database) {
  return std::async(
      std::launch::deferred,
      [&database](Auction auction) {
        return process_auction(database, std::move(auction));
      },
      std::move(auction));
}
} // namespace auction_house::engine