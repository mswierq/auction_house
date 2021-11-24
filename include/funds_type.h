//
// Created by mswiercz on 22.11.2021.
//
#pragma once
#include <cstdint>
#include <string>

namespace auction_engine {
using FundsType = std::uint64_t;
#define parse_funds(funds) std::stoull(funds)
}
