#include "clock.h"

Clock::Clock(uint8_t h, uint8_t m, uint8_t s) {
    hours_ = h;
    minutes_ = m;
    seconds_ = s;
    milliseconds_ = 0;
    time_ = milliseconds_;
    time_ += (uint32_t)seconds_ * 1000;
    time_ += (uint32_t)minutes_ * 1000 * 60;
    time_ += (uint32_t)hours_ * 1000 * 60 * 60;
}

void Clock::tick(uint32_t msdiff) {
    time_ += msdiff;
    while (time_ >= 24ul*60*60*1000) time_ -= 24ul*60*60*1000;
    uint32_t t = time_;
    milliseconds_ = t % 1000;
    t /= 1000;
    seconds_ = t % 60;
    t /= 60;
    minutes_ = t % 60;
    t /= 60;
    hours_ = t % 24;
}

uint8_t Clock::getHours() {
    return hours_;
}

uint8_t Clock::getMinutes() {
    return minutes_;
}

uint8_t Clock::getSeconds() {
    return seconds_;
}

uint16_t Clock::getMilliseconds() {
    return milliseconds_;
}