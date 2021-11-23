//
// Created by mswiercz on 22.11.2021.
//
#include "auctions.h"
#include <algorithm>

namespace auction_engine {

AuctionId AuctionList::_next_id = 0;

bool AuctionList::add_auction(Auction &&auction) {
  auto result = false;
  {
    std::unique_lock _l{_mutex};
    auto id = _next_id++;
    // In this simulation is highly unlikely that we outrun of ids, but just in
    // case...
    if (_auctions.find(id) == _auctions.end()) {
      _auctions[id] = std::move(auction);
      result = true;
    }
  }
  _cv.notify_one();
  return result;
}

OutbiddenBuyer AuctionList::bid_item(AuctionId id, FundsType new_price,
                                     const std::string &new_buyer) {
  std::unique_lock _l{_mutex};
  auto auction_it = _auctions.find(id);
  if (auction_it == _auctions.end()) {
    return {std::move(new_buyer), BidResult::DoesNotExist};
  }
  if (auction_it->second.owner == new_buyer) {
    return {std::move(new_buyer), BidResult::OwnerBid};
  }
  if (auction_it->second.price >= new_price) {
    return {std::move(new_buyer), BidResult::TooLowPrice};
  }
  OutbiddenBuyer outbidden{new_buyer, BidResult::Successful};
  outbidden.buyer.swap(auction_it->second.buyer);
  auction_it->second.price = new_price;
  return outbidden;
}

ExpiredAuctions AuctionList::find_expired(const TimePoint &start_at) {
  std::unique_lock lck{_mutex};
  // waits for the auction nearest to expire or for a new auction to be added
  _cv.wait_until(lck, start_at);
  // waits if there is no auctions at all
  _cv.wait(lck, [this]() { return !this->_auctions.empty(); });
  // collects list auctions
  ExpiredAuctions expired{{}, TimePoint::max()};
  for (auto it = _auctions.begin(); it != _auctions.end();) {
    auto &[_, auction] = *it;
    if (auction.expiration_time < Clock::now()) {
      expired.list.push_back(std::move(auction));
      it = _auctions.erase(it);
    } else {
      expired.nearest_expire =
          std::min(expired.nearest_expire, auction.expiration_time);
      ++it;
    }
  }
  return expired;
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