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

  //Get operation code from string "code||...."
  int getOPCode(std::string info) {
    // Find the position of the first "||" in the input string
    std::size_t pos = info.find("||");
    if (pos != std::string::npos) {
      // Extract the substring from the beginning of the input string until the first "||"
      std::string numberStr = info.substr(0, pos);

      // Convert the extracted substring to an integer
      int number;
      try {
          number = std::stoi(numberStr);
      } catch (const std::exception& ex) {
          log("Error converting the number " + numberStr + ": " + std::string(ex.what()));
          return -1;
      }

      // Use the extracted integer value (number) as needed
      log("Extracted number: " + std::to_string(number));
      return number;
    } else {
      int number;
      try {
          number = std::stoi(info);
          return number;
      } catch (const std::exception& ex) {
          log("Error converting the number: " + std::string(ex.what()));
          return -1;
      }
      log("Invalid input format. Could not find '||' or number in the input string.");
      return -1;
    }
  }

 private:
  Helper() {}
};

#endif  // HELPER_H
