#ifndef STRINGCONCAT_H
#define STRINGCONCAT_H

#include <Arduino.h>
#include <array>
#include <utility>

template<typename... Args>
inline void concatString(String& destination, Args&&... args) {
  std::array<String, sizeof...(Args)> parts{String(std::forward<Args>(args))...};
  size_t totalLength = destination.length();
  for (const auto& part : parts) {
    totalLength += part.length();
  }
  destination.reserve(totalLength);
  for (const auto& part : parts) {
    destination += part;
  }
}

template<typename... Args>
inline String concatStrings(Args&&... args) {
  String result;
  concatString(result, std::forward<Args>(args)...);
  return result;
}

#endif // STRINGCONCAT_H
