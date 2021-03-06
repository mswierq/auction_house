//
// Created by mswiercz on 21.11.2021.
//
#include "user_account.h"
#include <algorithm>
#include <numeric>

namespace auction_house::engine {
void Accounts::deposit_item(const std::string &username,
                            const std::string &item) {
  std::lock_guard _l(_mutex);
  _accounts[username].items.push_back(item);
}

bool Accounts::deposit_funds(const std::string &username,
                             const FundsType funds) {
  std::lock_guard _l(_mutex);
  auto &account = _accounts[username];
  if ((std::numeric_limits<FundsType>::max() - account.funds) >= funds) {
    account.funds += funds;
    return true;
  }
  return false;
}

bool Accounts::withdraw_item(const std::string &username,
                             const std::string &item) {
  std::lock_guard _l(_mutex);
  auto &user_items = _accounts[username].items;
  auto item_to_erase = std::find(user_items.begin(), user_items.end(), item);
  if (item_to_erase != user_items.end()) {
    user_items.erase(item_to_erase);
    return true;
  }
  return false;
}

bool Accounts::withdraw_funds(const std::string &username,
                              const FundsType funds) {
  std::lock_guard _l(_mutex);
  if (_accounts[username].funds >= funds) {
    _accounts[username].funds -= funds;
    return true;
  }
  return false;
}

FundsType Accounts::get_funds(const std::string &username) {
  std::lock_guard _l(_mutex);
  return _accounts[username].funds;
}

std::string Accounts::get_items(const std::string &username) {
  std::lock_guard _l(_mutex);
  auto &user_items = _accounts[username].items;
  return std::accumulate(user_items.cbegin(), user_items.cend(), std::string{},
                         [](std::string &a, const std::string &b) {
                           if (!a.empty()) {
                             a.append("\n");
                           }
                           return a.append(b);
                         });
}
} // namespace auction_house::engine