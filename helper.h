#ifndef HELPER_H
#define HELPER_H
#include <iomanip>
#include <sstream>
#include <string>

class Helper {
public:
  static Helper &get_instance() {
    static Helper instance;
    return instance;
  }

  /**
   * Formats time in human-readable format
   *
   * @param seconds Time in seconds which needs to formatting (type: int64_t)
   * @return Formatted time (type: std::string)
   */
  std::string format_time(int64_t seconds) {
    std::stringstream time_ss;
    int hours = seconds / 3600;
    int minutes = (seconds % 3600) / 60;
    int remaining_seconds = seconds % 60;
    if (hours > 0) {
      time_ss << std::setw(2) << std::setfill('0') << hours << ":";
    }
    time_ss << std::setw(2) << std::setfill('0') << minutes << ":"
            << std::setw(2) << std::setfill('0') << remaining_seconds;
    return time_ss.str();
  }

private:
  Helper() {}
};

#endif // HELPER_H
