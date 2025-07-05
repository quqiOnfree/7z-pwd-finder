#include <print>

#include "CMultiVolumeInStream.hpp"
#include "PasswordGenerator.hpp"

int main() {
  // CMultiVolumeInStream s({L""});
  PasswordGenerator pg(10, 20,
                       PasswordGenerator::Number | PasswordGenerator::Letter |
                           PasswordGenerator::SpecialCharacters);
  while (!pg.isEnd()) {
    auto current = pg.getNextPassword(10000);
    PasswordGenerator loc_pg(10,
                             PasswordGenerator::Number |
                                 PasswordGenerator::Letter |
                                 PasswordGenerator::SpecialCharacters,
                             current, 10000);
    while (!loc_pg.isEnd()) {
      std::print("{}\n", loc_pg.getNextPassword());
    }
    break;
  }
  return 0;
}
