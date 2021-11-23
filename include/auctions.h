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

namespace auction_engine {
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

struct OutbiddenBuyer {
  std::optional<std::string> buyer;
  BidResult result;
};

struct ExpiredAuctions {
  std::list<Auction> list;
  TimePoint nearest_expire;
};

class AuctionList {
public:
  // Adds a new auction, returns false if an error has occurred
  bool add_auction(Auction &&auction);

  // Returns an outbidden buyer, it can be a new buyer due to item that doesn't
  // exist, the new price has been too low or the owner tried to bid its own
  // item
  OutbiddenBuyer bid_item(AuctionId id, FundsType new_price,
                          const std::string &new_buyer);

  // Returns list of list auctions and time point when next checkup should
  // occur. Starts the search at the specified time point.
  ExpiredAuctions find_expired(const TimePoint &start_at);

  // Returns a vecotr of auctions as printable strings
  std::vector<std::string> get_printable_list();

private:
  std::unordered_map<AuctionId, Auction> _auctions;
  std::shared_mutex _mutex;
  std::condition_variable_any _cv;
  static AuctionId _next_id;
};
} // namespace auction_engine
