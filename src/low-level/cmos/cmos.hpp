#pragma once
#include <stdint.h>

#define CMOS_COMMAND_PORT 0x70
#define CMOS_DATA_PORT    0x71

// cf. http://wiki.osdev.org/CMOS
#define CMOS_REG_SECONDS  0x00
#define CMOS_REG_MINUTES  0x02
#define CMOS_REG_HOURS    0x04
#define CMOS_REG_WEEKDAYS 0x06
#define CMOS_REG_DAY      0x07
#define CMOS_REG_MONTH    0x08
#define CMOS_REG_YEAR     0x09
#define CMOS_REG_CENTURY  0x32
#define CMOS_REG_STATUS_A 0x0A
#define CMOS_REG_STATUS_B 0x0B

struct CMOSRTC
{
    uint8_t seconds;
    uint8_t minutes;
    uint8_t hours;
    uint8_t weekdays;
    uint8_t day;
    uint8_t month;
    uint16_t year;
    uint8_t century;

    bool operator==(const CMOSRTC &other)
    {
        return this->seconds == other.seconds &&
            this->minutes == other.minutes &&
            this->hours == other.hours &&
            this->weekdays == other.weekdays &&
            this->day == other.day &&
            this->month == other.month &&
            this->year == other.year &&
            this->century == other.century;
    }
};

class CMOS
{
private:
    static uint64_t TimeAtBoot;

    uint8_t ReadRegister(uint8_t reg);
    bool Updating();
    uint64_t SecsOfMonth(uint64_t months, uint64_t year);
    uint64_t SecsOfYears(uint64_t years);
public:
    void Initialize();

    uint64_t BootTime() const
    {
        return TimeAtBoot;
    }

    uint64_t CurrentTime();

    CMOSRTC ReadRTC();
};

extern CMOS cmos;