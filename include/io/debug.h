// include/io/debug.h
#pragma once
#include <format>
#include <iostream>
#include <string>
#include <utility> 
 
namespace Debug {

// --- Log Functions using std::format ---

template<typename... Args>
inline void Log(std::format_string<Args...> fmt_str, Args&&... args) {
    std::cout << std::format(fmt_str, std::forward<Args>(args)...)
                << std::endl;
}

template<typename... Args>
inline void LogError(std::format_string<Args...> fmt_str, Args&&... args) {
    std::cout << "\033[31m" // Red
                << std::format(fmt_str, std::forward<Args>(args)...)
                << "\033[0m"  // Reset
                << std::endl;
}

template<typename... Args>
inline void LogOK(std::format_string<Args...> fmt_str, Args&&... args) {
    std::cout << "\033[32m" // Green
                << std::format(fmt_str, std::forward<Args>(args)...)
                << "\033[0m"  // Reset
                << std::endl;
}

template<typename... Args>
inline void LogWarning(std::format_string<Args...> fmt_str, Args&&... args) {
    std::cout << "\033[33m" // Yellow
                << std::format(fmt_str, std::forward<Args>(args)...)
                << "\033[0m"  // Reset
                << std::endl;
}

} // namespace Debug
