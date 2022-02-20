//
// Created by antonio on 17/09/20.
//

#ifndef OHBOI_LOGGER_H
#define OHBOI_LOGGER_H

#include <string>
#include <iostream>

namespace {
    bool mEnableInfo = false;
    bool mEnableWarning = false;
}
namespace Logger {

    static inline void info(const std::string& section, const std::string& message) { if ( mEnableInfo ) std::cout << "[+][" << section << "] - " << message << std::endl; }
    static inline void error(const std::string& section, const std::string& message) { if ( mEnableWarning ) std::cout << "[-][" << section << "] - " << message << std::endl; }
    static inline void warning(const std::string& section, const std::string& message) { std::cout << "[!][" << section << "] - " << message << std::endl; }

    static void toggle_info() {
        mEnableInfo = !mEnableInfo;
    }

    static void toggle_warning() {
        mEnableWarning = !mEnableWarning;
    }
}

#endif //OHBOI_LOGGER_H
