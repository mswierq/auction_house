//
// Created by mswiercz on 27.11.2021.
//
#pragma once
#include <cstdint>

namespace auction_house {
#ifndef WIN32
using ConnectionId = int; // linux
#else
using ConnectionId = int64_t; // windows
#endif
}
