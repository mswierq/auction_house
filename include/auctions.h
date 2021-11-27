//
// Created by mswiercz on 22.11.2021.
//
#pragma once
#include "funds_type.h"
#include <chrono>
#include <condition_variable>
#include <list>
#include <optional>
#include <shared_mutex>
#include <unordered_map>
#include <vector>

namespace auction_house::engine {
using AuctionId = std::uint64_t;
using Clock = std::chrono::steady_clock;
using TimePoint = std::chrono::time_point<Clock>;

struct Auction {
  std::string owner;
  std::optional<std::string> buyer;
  FundsType price;
  std::string item;
  TimePoint expiration_time;
};

enum class BidResult { DoesNotExist, TooLowPrice, OwnerBid, Successful };

using ExpiredAuctions = std::list<Auction>;

class AuctionList {
public:
  // Adds a new auction, returns false if an error has occurred
  bool add_auction(Auction &&auction);

  // Bids an item by its id and returns the bid result, it can be:
  // - a success, bidder gave a better price,
  // - a fail due too low offer,
  // - a fail when item doesn't exit,
  // - a fail, because the owner tried to bid its own item
  BidResult bid_item(AuctionId id, FundsType new_price,
                     const std::string &new_buyer);

  // Returns list of list auctions that has expired
  ExpiredAuctions collect_expired();

  // Waits for at least one auction to expire, this also unblocks when a new
  // item is added to the map.
  void wait_for_expired();

  // Returns a vector of auctions as printable strings
  std::vector<std::string> get_printable_list();

private:
  std::unordered_map<AuctionId, Auction> _auctions;
  std::shared_mutex _mutex;
  std::condition_variable_any _cv_empty_list;
  std::condition_variable_any _cv_timer;
  AuctionId _next_id = 0;
  TimePoint _nearest_expire = TimePoint::max();
};
} // namespace auction_house::engine
