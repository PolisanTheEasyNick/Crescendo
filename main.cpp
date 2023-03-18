#include "helper.h"
#include "player.h"
#include <iostream>

int main() {
  Player player;
  std::cout << "Printing players" << std::endl;
  player.print_players();
  std::cout << "Printing names" << std::endl;
  player.print_players_names();
  player.select_player(0);
  int input = -1;
  std::cout << "1. Play" << std::endl;
  std::cout << "2. Pause" << std::endl;
  std::cout << "3. PlayPause" << std::endl;
  std::cout << "4. Next" << std::endl;
  std::cout << "5. Previous" << std::endl;
  std::cout << "6. Get metadata" << std::endl;
  std::cout << "7. Get isShuffle" << std::endl;
  std::cout << "8. Toggle shuffle" << std::endl;
  std::cout << "9. Get seek" << std::endl;
  std::cout << "10. Set position to 0:30" << std::endl;
  std::cout << "11. Get volume" << std::endl;
  std::cout << "12. Set volume to 0.25" << std::endl;
  while (input != 0) {
    std::cin >> input;
    switch (input) {
    case 1:
      player.send_play();
      break;
    case 2:
      player.send_pause();
      break;
    case 3:
      player.send_play_pause();
      break;
    case 4:
      player.send_next();
      break;
    case 5:
      player.send_previous();
      break;
    case 6: {
      auto meta = player.get_metadata();
      std::cout << "Received: " << meta.size() << " elements." << std::endl;
      for (auto &data : meta) {
        std::cout << data.first << ": " << data.second << std::endl;
      }
      break;
    }
    case 7: {
      bool isShuffle = player.get_shuffle();
      std::cout << "Is shuffle: " << isShuffle << std::endl;
      break;
    }
    case 8: {
      bool isShuffle = player.get_shuffle();
      player.set_shuffle(!isShuffle);
      break;
    }
    case 9: {
      int64_t seek = player.get_position();
      std::cout << "Pos: " << seek << std::endl;
      std::cout << "Time: " << Helper::getInstance().format_time(seek)
                << std::endl;
      break;
    }
    case 10: {
      player.set_position(30 * 1000000);
      break;
    }
    case 11: {
      double volume = player.get_volume();
      std::cout << "Volume: " << volume << std::endl;
      break;
    }
    case 12: {
      player.set_volume(0.25);
      break;
    }
    }
  }
  return 0;
}
