#ifndef _CLOCK_H_
#define _CLOCK_H_
#include <stdint.h>

class Clock {
public:
    Clock(uint8_t h = 0, uint8_t m = 0, uint8_t s = 0);
    void tick(uint32_t msdiff);
    uint8_t getHours();
    uint8_t getMinutes();
    uint8_t getSeconds();
    uint16_t getMilliseconds();
private:
    uint32_t time_;  // time since 0:00:00.000 in ms
    uint8_t hours_;
    uint8_t minutes_;
    uint8_t seconds_;
    uint16_t milliseconds_;
};

#endif