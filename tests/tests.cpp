//
// Created by mswiercz on 21.11.2021.
//
#define CATCH_CONFIG_RUNNER
#include <catch2/catch.hpp>
#include <spdlog/spdlog.h>

int main(int argc, char *argv[]) {
  spdlog::set_level(spdlog::level::off);

  // global setup...
  int result = Catch::Session().run(argc, argv);

  // global clean-up...
  return result;
}