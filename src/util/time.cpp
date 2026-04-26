#include "../include/util/time.h"
#include <iomanip>
#include <sstream>

namespace base {
namespace util {

int64_t Time::timestamp() {
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

int64_t Time::timestampMillis() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

std::string Time::format(const std::string& formatStr) {
    std::time_t t = std::time(nullptr);
    std::tm* tm_info = std::localtime(&t);
    std::stringstream ss;
    for (size_t i = 0; i < formatStr.size(); ++i) {
        char c = formatStr[i];
        if (c == '%' && i + 1 < formatStr.size()) {
            char next = formatStr[++i];
            switch (next) {
                case 'Y': ss << std::setfill('0') << std::setw(4) << tm_info->tm_year + 1900; break;
                case 'm': ss << std::setfill('0') << std::setw(2) << tm_info->tm_mon + 1; break;
                case 'd': ss << std::setfill('0') << std::setw(2) << tm_info->tm_mday; break;
                case 'H': ss << std::setfill('0') << std::setw(2) << tm_info->tm_hour; break;
                case 'M': ss << std::setfill('0') << std::setw(2) << tm_info->tm_min; break;
                case 'S': ss << std::setfill('0') << std::setw(2) << tm_info->tm_sec; break;
                default: ss << c << next;
            }
        } else {
            ss << c;
        }
    }
    return ss.str();
}

std::string Time::getCurrentDate() {
    return format("%Y-%m-%d");
}

std::string Time::getCurrentTime() {
    return format("%H:%M:%S");
}

Time::Timer::Timer() : m_start(std::chrono::steady_clock::now()) {}

void Time::Timer::reset() {
    m_start = std::chrono::steady_clock::now();
}

double Time::Timer::elapsedSeconds() const {
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration<double>(now - m_start).count();
}

int64_t Time::Timer::elapsedMilliseconds() const {
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now - m_start).count();
}

} // namespace util
} // namespace base