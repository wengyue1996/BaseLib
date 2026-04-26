#include <string>
#include <chrono>
#include <ctime>
#include <cstdint>

namespace base {
namespace util {

class Time {
public:
    static int64_t timestamp();
    static int64_t timestampMillis();
    static std::string format(const std::string& formatStr);
    static std::string getCurrentDate();
    static std::string getCurrentTime();

    class Timer {
    public:
        Timer();
        void reset();
        double elapsedSeconds() const;
        int64_t elapsedMilliseconds() const;
    private:
        std::chrono::steady_clock::time_point m_start;
    };
};

} // namespace util
} // namespace base