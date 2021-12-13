#include "cmos.hpp"
#include "ports/Ports.hpp"
#include <string.h>

CMOS cmos;
uint64_t CMOS::TimeAtBoot = 0;

void CMOS::Initialize()
{
    TimeAtBoot = CurrentTime();
}

uint64_t CMOS::CurrentTime()
{
    auto rtc = ReadRTC();

    return SecsOfYears(rtc.year - 1) + SecsOfMonth(rtc.month - 1, rtc.year) +
        (rtc.day - 1) * 86400 + rtc.hours * 3600 + rtc.minutes * 60 + rtc.seconds;
}

CMOSRTC CMOS::ReadRTC()
{
    CMOSRTC rtc;
    CMOSRTC last;

    while(Updating());

    rtc.seconds = ReadRegister(CMOS_REG_SECONDS);
    rtc.minutes = ReadRegister(CMOS_REG_MINUTES);
    rtc.hours = ReadRegister(CMOS_REG_HOURS);
    rtc.weekdays = ReadRegister(CMOS_REG_WEEKDAYS);
    rtc.day = ReadRegister(CMOS_REG_DAY);
    rtc.month = ReadRegister(CMOS_REG_MONTH);
    rtc.year = ReadRegister(CMOS_REG_YEAR);
    rtc.century = ReadRegister(CMOS_REG_CENTURY);

    do
    {
        memcpy(&last, &rtc, sizeof(CMOSRTC));

        while(Updating());

        rtc.seconds = ReadRegister(CMOS_REG_SECONDS);
        rtc.minutes = ReadRegister(CMOS_REG_MINUTES);
        rtc.hours = ReadRegister(CMOS_REG_HOURS);
        rtc.weekdays = ReadRegister(CMOS_REG_WEEKDAYS);
        rtc.day = ReadRegister(CMOS_REG_DAY);
        rtc.month = ReadRegister(CMOS_REG_MONTH);
        rtc.year = ReadRegister(CMOS_REG_YEAR);
        rtc.century = ReadRegister(CMOS_REG_CENTURY);
    } while(!(rtc == last));

    uint8_t b = ReadRegister(CMOS_REG_STATUS_B);

    //Convert BCD back into valid value
    if(!(b & 0x04))
    {
        rtc.seconds = (rtc.seconds & 0x0F) + ((rtc.seconds / 16) * 10);
        rtc.minutes = (rtc.minutes & 0x0F) + ((rtc.minutes / 16) * 10);
        rtc.hours = ((rtc.hours & 0x0F) + (((rtc.hours & 0x70) / 16) * 10)) |
                    (rtc.hours & 0x80);
        rtc.weekdays = (rtc.weekdays & 0x0F) + ((rtc.weekdays / 16) * 10);
        rtc.day = (rtc.day & 0x0F) + ((rtc.day / 16) * 10);
        rtc.month = (rtc.month & 0x0F) + ((rtc.month / 16) * 10);
        rtc.year = (rtc.year & 0x0F) + ((rtc.year / 16) * 10);
        rtc.century = (rtc.century & 0x0F) + ((rtc.century / 16) * 10);
    }

    if(!(b & 0x02) && (rtc.hours & 0x80))
    {
        // Convert 12 hour clock to 24 hour clock if necessary
        rtc.hours = ((rtc.hours & 0x7F) + 12) % 24;
    }

    rtc.year += rtc.century * 100;

    return rtc;
}

bool CMOS::Updating()
{
    outport8(CMOS_COMMAND_PORT, CMOS_REG_STATUS_A);

    return inport8(CMOS_DATA_PORT) & 0x80;
}

uint8_t CMOS::ReadRegister(uint8_t reg)
{
    outport8(CMOS_COMMAND_PORT, (1 << 7) | reg);

    return inport8(CMOS_DATA_PORT);
}

uint64_t CMOS::SecsOfYears(uint64_t years)
{
    uint64_t days = 0;

    while(years > 1969)
    {
        days += 365;

        if(years % 4 == 0)
        {
            if(years % 100 == 0)
            {
                if(years % 400 == 0)
                {
                    days++;
                }
            }
            else
            {
                days++;
            }
        }

        years--;
    }

    return days * 86400;
}

uint64_t CMOS::SecsOfMonth(uint64_t months, uint64_t year)
{
    uint64_t days = 0;

    switch(months)
    {
        case 11:
          days += 30;

        case 10:
            days += 31;

        case 9:
            days += 30;

        case 8:
            days += 31;

        case 7:
            days += 31;

        case 6:
            days += 30;

        case 5:
            days += 31;

        case 4:
            days += 30;

        case 3:
            days += 31;

        case 2:
            days += 28;

            if ((year % 4 == 0) && ((year % 100 != 0) || (year % 400 == 0))) {
                days++;
            }

        case 1:
            days += 31;

        default:
            break;
    }

    return days * 86400;
}
