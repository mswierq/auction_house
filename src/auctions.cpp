//
// Created by mswiercz on 22.11.2021.
//
#include "auctions.h"
#include <algorithm>

namespace auction_engine {

bool AuctionList::add_auction(Auction &&auction) {
  auto result = false;
  auto change_timeout = false;
  {
    std::unique_lock _l{_mutex};
    auto id = _next_id++;
    // In this simulation is highly unlikely that we outrun of ids, but just in
    // case...
    if (_auctions.find(id) == _auctions.end()) {
      _auctions[id] = std::move(auction);
      result = true;
      if (auction.expiration_time < _nearest_expire) {
        _nearest_expire = auction.expiration_time;
        change_timeout = true;
      }
    }
  }
  if(result) {
    if(change_timeout) {
      _cv_timer.notify_one();
    }
    _cv_empty_list.notify_one();
  }
  return result;
}

BidResult AuctionList::bid_item(AuctionId id, FundsType new_price,
                                const std::string &new_buyer) {
  std::unique_lock _l{_mutex};
  auto auction_it = _auctions.find(id);
  if (auction_it == _auctions.end()) {
    return BidResult::DoesNotExist;
  }
  if (auction_it->second.owner == new_buyer) {
    return BidResult::OwnerBid;
  }
  if (auction_it->second.price >= new_price) {
    return BidResult::TooLowPrice;
  }
  auction_it->second.buyer = new_buyer;
  auction_it->second.price = new_price;
  return BidResult::Successful;
}

ExpiredAuctions AuctionList::collect_expired() {
  std::unique_lock lck{_mutex};
  // collects list auctions
  ExpiredAuctions expired;
  for (auto it = _auctions.begin(); it != _auctions.end();) {
    auto &[_, auction] = *it;
    if (auction.expiration_time < Clock::now()) {
      expired.push_back(std::move(auction));
      it = _auctions.erase(it);
    } else {
      _nearest_expire = std::min(_nearest_expire, auction.expiration_time);
      ++it;
    }
  }
  if(_auctions.empty()) {
    _nearest_expire = TimePoint::max();
  }
  return expired;
}

void AuctionList::wait_for_expired() {
  std::shared_lock lck{_mutex};
  // waits for at least one expired auction
  _cv_timer.wait_until(lck, _nearest_expire, [this]() {
    return this->_nearest_expire <= Clock::now();
  });
  // waits if there is no auctions at all
  _cv_empty_list.wait(lck, [this]() { return !this->_auctions.empty(); });
}

std::vector<std::string> AuctionList::get_printable_list() {
  std::shared_lock _l{_mutex};
  std::vector<std::string> auctions_vec{};
  auctions_vec.reserve(_auctions.size());
  for (auto &[key, auction] : _auctions) {
    auctions_vec.emplace_back("ID: " + std::to_string(key) + "; ITEM: " +
                              auction.item + "; OWNER: " + auction.owner +
                              "; PRICE: " + std::to_string(auction.price) +
                              "; BUYER: " + auction.buyer.value_or(""));
  }
  return auctions_vec;
}
} // namespace auction_engine