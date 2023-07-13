#ifndef HELPER_H
#define HELPER_H
#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
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

  /**
   * Prints given string into stdout
   *
   * @param msg Message to print (type: std::string)
   */
  void log(std::string msg, bool new_string = true, bool date = true) {
    auto now = std::chrono::system_clock::now();
    std::time_t time = std::chrono::system_clock::to_time_t(now);
    std::string datetime = std::ctime(&time);
    datetime = datetime.substr(0, datetime.length() - 1);

    if (date) std::cout << "[" << datetime << "] ";
    std::cout << msg;
    if (new_string) std::cout << std::endl;
  }

  // Find the first digit
  int firstDigit(int n) {
    // Remove last digit from number
    // till only one digit is left
    while (n >= 10) n /= 10;

    // return the first digit
    return n;
  }

 private:
  Helper() {}
};

#endif  // HELPER_H
