#pragma once

#include <chrono>
#include <iostream>

#define PROFILE_CONCAT_INTERNAL(X,Y) X ## Y
#define PROFILE_CONCAT(X,Y) PROFILE_CONCAT_INTERNAL(X,Y)
#define UNIQ_VAR_NAME_PROFILE PROFILE_CONCAT(profileGuard, __LINE__)
//#define LOG_DURATION(x) LogDuration UNIQ_VAR_NAME_PROFILE(x)
#define LOG_DURATION_STREAM(x) LogDuration UNIQ_VAR_NAME_PROFILE(x)

class LogDuration {
public:
    // заменим имя типа std::chrono::steady_clock
    // с помощью using для удобства
    using Clock = std::chrono::steady_clock;

    LogDuration(std::string s, std::ostream& out = std::cerr) : msg_(s), out_(out) {
        out_ << msg_ << std::endl;
    }

    ~LogDuration() {
        using namespace std::chrono;
        using namespace std::literals;

        const auto end_time = Clock::now();
        const auto dur = end_time - start_time_;
        out_ << "Operation time: "s << duration_cast<milliseconds>(dur).count() << " ms"s << std::endl;
    }

private:
    const Clock::time_point start_time_ = Clock::now();
    std::string msg_;
    std::ostream& out_;
};