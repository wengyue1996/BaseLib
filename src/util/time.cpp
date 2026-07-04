#include "../include/util/time.h"
#include <iomanip>
#include <sstream>
#include <ctime>

namespace base {
namespace util {

DateTime::DateTime() : year(0), month(0), day(0), hour(0), minute(0), second(0), millisecond(0) {}

DateTime DateTime::now() {
    DateTime dt;
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;

    std::tm tm_buf = {};
#ifdef _WIN32
    localtime_s(&tm_buf, &time);
#else
    localtime_r(&time, &tm_buf);
#endif
    dt.year = tm_buf.tm_year + 1900;
    dt.month = tm_buf.tm_mon + 1;
    dt.day = tm_buf.tm_mday;
    dt.hour = tm_buf.tm_hour;
    dt.minute = tm_buf.tm_min;
    dt.second = tm_buf.tm_sec;
    dt.millisecond = static_cast<int>(ms.count());
    return dt;
}

std::string DateTime::format(const std::string& formatStr) const {
    std::stringstream ss;
    for (size_t i = 0; i < formatStr.size(); ++i) {
        char c = formatStr[i];
        if (c == '%' && i + 1 < formatStr.size()) {
            char next = formatStr[++i];
            switch (next) {
                case 'Y': ss << std::setfill('0') << std::setw(4) << year; break;
                case 'm': ss << std::setfill('0') << std::setw(2) << month; break;
                case 'd': ss << std::setfill('0') << std::setw(2) << day; break;
                case 'H': ss << std::setfill('0') << std::setw(2) << hour; break;
                case 'I': {
                    int h12 = hour % 12;
                    if (h12 == 0) h12 = 12;
                    ss << std::setfill('0') << std::setw(2) << h12;
                    break;
                }
                case 'M': ss << std::setfill('0') << std::setw(2) << minute; break;
                case 'S': ss << std::setfill('0') << std::setw(2) << second; break;
                case 'f': ss << std::setfill('0') << std::setw(3) << millisecond; break;
                case 'p': ss << (hour < 12 ? "AM" : "PM"); break;
                case 'A': {
                    static const char* days[] = {"Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday"};
                    int wday = (day + 2*month + 3*(month+1)/5 + year + year/4 - year/100 + year/400 + 1) % 7;
                    ss << days[wday];
                    break;
                }
                case 'a': {
                    static const char* days[] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
                    int wday = (day + 2*month + 3*(month+1)/5 + year + year/4 - year/100 + year/400 + 1) % 7;
                    ss << days[wday];
                    break;
                }
                case 'B': {
                    static const char* months[] = {"January","February","March","April","May","June","July","August","September","October","November","December"};
                    ss << months[month - 1];
                    break;
                }
                case 'b': {
                    static const char* months[] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
                    ss << months[month - 1];
                    break;
                }
                default: ss << c << next;
            }
        } else {
            ss << c;
        }
    }
    return ss.str();
}

int64_t Time::timestamp() {
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

int64_t Time::timestampMillis() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

std::string Time::format(const std::string& formatStr) {
    return DateTime::now().format(formatStr);
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