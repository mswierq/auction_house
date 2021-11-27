//
// Created by mswiercz on 21.11.2021.
//
#pragma once
#include "funds_type.h"
#include <list>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

namespace auction_house::engine {

struct UserAccount {
  FundsType funds;
  std::list<std::string> items;
};

class Accounts {
public:
  void deposit_item(const std::string &username, const std::string &item);
  bool deposit_funds(const std::string &username, const FundsType funds);
  bool withdraw_item(const std::string &username, const std::string &item);
  bool withdraw_funds(const std::string &username, const FundsType funds);
  FundsType get_funds(const std::string &username);
  std::string get_items(const std::string &username);

private:
  std::mutex _mutex;
  std::unordered_map<std::string, UserAccount> _accounts;
};
} // namespace auction_house::engine
