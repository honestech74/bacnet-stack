/*####COPYRIGHTBEGIN####
 -------------------------------------------
 Copyright (C) 2007 Steve Karg <skarg@users.sourceforge.net>

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to:
 The Free Software Foundation, Inc.
 59 Temple Place - Suite 330
 Boston, MA  02111-1307, USA.

 As a special exception, if other files instantiate templates or
 use macros or inline functions from this file, or you compile
 this file and link it with other works to produce a work based
 on this file, this file does not by itself cause the resulting
 work to be covered by the GNU General Public License. However
 the source code for this file must still be made available in
 accordance with section (3) of the GNU General Public License.

 This exception does not invalidate any other reasons why a work
 based on this file might be covered by the GNU General Public
 License.
 -------------------------------------------
####COPYRIGHTEND####*/
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "datetime.h"

/* BACnet Date */
/* year = years since 1900 */
/* month 1=Jan */
/* day = day of month 1..31 */
/* wday 1=Monday...7=Sunday */

/* Wildcards:
  A value of X'FF' in any of the four octets 
  shall indicate that the value is unspecified.
  If all four octets = X'FF', the corresponding
  time or date may be interpreted as "any" or "don't care"
*/

static bool is_leap_year(uint16_t year)
{
    if ((year % 4) == 0 && ((year % 100) != 0 || (year % 400) == 0))
        return (true);
    else
        return (false);
}

static uint8_t month_days(uint16_t year, uint8_t month)
{
    /* note: start with a zero in the first element to save us from a
       month - 1 calculation in the lookup */
    int month_days[13] =
        { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

    /* February */
    if ((month == 2) && is_leap_year(year))
        return 29;
    else if (month >= 1 && month <= 12)
        return month_days[month];
    else
        return 0;
}

static uint32_t days_since_epoch(uint16_t year, uint8_t month, uint8_t day)
{
    uint32_t days = 0;          /* return value */
    uint8_t monthdays;          /* days in a month */
    uint16_t years = 0;         /* loop counter for years */
    uint8_t months = 0;         /* loop counter for months */

    monthdays = month_days(year, month);
    if ((year >= 1900) && (monthdays) && (day >= 1) && (day <= monthdays)) {
        for (years = 1900; years < year; years++) {
            days += 365;
            if (is_leap_year(years))
                days++;
        }
        for (months = 1; months < month; months++) {
            days += month_days(years, months);
        }
        days += (day - 1);
    }

    return (days);
}

static void days_since_epoch_into_ymd(uint32_t days,
    uint16_t * pYear, uint8_t * pMonth, uint8_t * pDay)
{
    uint16_t year = 1900;
    uint8_t month = 1;
    uint8_t day = 1;

    while (days >= 365) {
        if ((is_leap_year(year)) && (days == 365))
            break;
        days -= 365;
        if (is_leap_year(year))
            --days;
        year++;
    }

    while (days >= (uint32_t) month_days(year, month)) {
        days -= month_days(year, month);
        month++;
    }

    day += ((uint8_t) days);

    if (pYear)
        *pYear = year;
    if (pMonth)
        *pMonth = month;
    if (pDay)
        *pDay = day;

    return;
}


/* Jan 1, 1900 is a Monday */
/* wday 1=Monday...7=Sunday */
static uint8_t day_of_week(uint16_t year, uint8_t month, uint8_t day)
{
    return ((uint8_t) (days_since_epoch(year, month, day) % 7) + 1);
}

/* if the date1 is the same as date2, return is 0
   if date1 is after date2, returns positive
   if date1 is before date2, returns negative */
int datetime_compare_date(BACNET_DATE * date1, BACNET_DATE * date2)
{
    int diff = 0;

    if (date1 && date2) {
        diff = (int) date1->year - (int) date2->year;
        if (diff == 0) {
            diff = (int) date1->month - (int) date2->month;
            if (diff == 0) {
                diff = (int) date1->day - (int) date2->day;
            }
        }
    }

    return diff;
}

/* if the time1 is the same as time2, return is 0
   if time1 is after time2, returns positive
   if time1 is before time2, returns negative */
int datetime_compare_time(BACNET_TIME * time1, BACNET_TIME * time2)
{
    int diff = 0;

    if (time1 && time2) {
        diff = (int) time1->hour - (int) time2->hour;
        if (diff == 0) {
            diff = (int) time1->min - (int) time2->min;
            if (diff == 0) {
                diff = (int) time1->sec - (int) time2->sec;
                if (diff == 0) {
                    diff =
                        (int) time1->hundredths - (int) time2->hundredths;
                }
            }
        }
    }

    return diff;
}

/* if the datetime1 is the same as datetime2, return is 0
   if datetime1 is before datetime2, returns negative
   if datetime1 is after datetime2, returns positive */
int datetime_compare(BACNET_DATE_TIME * datetime1,
    BACNET_DATE_TIME * datetime2)
{
    int diff = 0;

    diff = datetime_compare_date(&datetime1->date, &datetime2->date);
    if (diff == 0) {
        diff = datetime_compare_time(&datetime1->time, &datetime2->time);
    }

    return diff;
}

void datetime_copy_date(BACNET_DATE * dest_date, BACNET_DATE * src_date)
{
    if (dest_date && src_date) {
        dest_date->year = src_date->year;
        dest_date->month = src_date->month;
        dest_date->day = src_date->day;
        dest_date->wday = src_date->wday;
    }
}

void datetime_copy_time(BACNET_TIME * dest_time, BACNET_TIME * src_time)
{
    if (dest_time && src_time) {
        dest_time->hour = src_time->hour;
        dest_time->min = src_time->min;
        dest_time->sec = src_time->sec;
        dest_time->hundredths = src_time->hundredths;
    }
}

void datetime_copy(BACNET_DATE_TIME * dest_datetime,
    BACNET_DATE_TIME * src_datetime)
{
    datetime_copy_time(&dest_datetime->time, &src_datetime->time);
    datetime_copy_date(&dest_datetime->date, &src_datetime->date);
}

void datetime_set_date(BACNET_DATE * bdate,
    uint16_t year, uint8_t month, uint8_t day)
{
    if (bdate) {
        bdate->year = year;
        bdate->month = month;
        bdate->day = day;
        bdate->wday = day_of_week(year, month, day);
    }
}

void datetime_set_time(BACNET_TIME * btime,
    uint8_t hour, uint8_t minute, uint8_t seconds, uint8_t hundredths)
{
    if (btime) {
        btime->hour = hour;
        btime->min = minute;
        btime->sec = seconds;
        btime->hundredths = hundredths;
    }
}

void datetime_set(BACNET_DATE_TIME * bdatetime,
    BACNET_DATE * bdate, BACNET_TIME * btime)
{
    if (bdate && btime && bdatetime) {
        bdatetime->time.hour = btime->hour;
        bdatetime->time.min = btime->min;
        bdatetime->time.sec = btime->sec;
        bdatetime->time.hundredths = btime->hundredths;
        bdatetime->date.year = bdate->year;
        bdatetime->date.month = bdate->month;
        bdatetime->date.day = bdate->day;
        bdatetime->date.wday = bdate->wday;
    }
}

void datetime_set_values(BACNET_DATE_TIME * bdatetime,
    uint16_t year, uint8_t month, uint8_t day,
    uint8_t hour, uint8_t minute, uint8_t seconds, uint8_t hundredths)
{
    if (bdatetime) {
        bdatetime->date.year = year;
        bdatetime->date.month = month;
        bdatetime->date.day = day;
        bdatetime->date.wday = day_of_week(year, month, day);
        bdatetime->time.hour = hour;
        bdatetime->time.min = minute;
        bdatetime->time.sec = seconds;
        bdatetime->time.hundredths = hundredths;
    }
}

static uint32_t seconds_since_midnight(uint8_t hours, uint8_t minutes,
    uint8_t seconds)
{
    return ((hours * 60 * 60) + (minutes * 60) + seconds);
}

static void seconds_since_midnight_into_hms(uint32_t seconds,
    uint8_t * pHours, uint8_t * pMinutes, uint8_t * pSeconds)
{
    uint8_t hour = 0;
    uint8_t minute = 0;

    hour = (uint8_t) (seconds / (60 * 60));
    seconds -= (hour * 60 * 60);
    minute = (uint8_t) (seconds / 60);
    seconds -= (minute * 60);

    if (pHours)
        *pHours = hour;
    if (pMinutes)
        *pMinutes = minute;
    if (pSeconds)
        *pSeconds = (uint8_t) seconds;
}

void datetime_add_minutes(BACNET_DATE_TIME * bdatetime, uint32_t minutes)
{
    uint32_t bdatetime_minutes = 0;
    uint32_t bdatetime_days = 0;
    uint32_t days = 0;

    /* convert bdatetime to seconds and days */
    bdatetime_minutes = seconds_since_midnight(bdatetime->time.hour,
        bdatetime->time.min, bdatetime->time.sec) / 60;
    bdatetime_days = days_since_epoch(bdatetime->date.year,
        bdatetime->date.month, bdatetime->date.day);

    /* add */
    days = minutes / (24 * 60);
    bdatetime_days += days;
    minutes -= (days * 24 * 60);
    bdatetime_minutes += minutes;
    days = bdatetime_minutes / (24 * 60);
    bdatetime_days += days;

    /* convert bdatetime from seconds and days */
    seconds_since_midnight_into_hms(bdatetime_minutes * 60,
        &bdatetime->time.hour, &bdatetime->time.min, &bdatetime->time.sec);
    days_since_epoch_into_ymd(bdatetime_days,
        &bdatetime->date.year,
        &bdatetime->date.month, &bdatetime->date.day);
    bdatetime->date.wday = day_of_week(bdatetime->date.year,
        bdatetime->date.month, bdatetime->date.day);
}

bool datetime_wildcard(BACNET_DATE_TIME * bdatetime)
{
    bool wildcard_present = false;

    if (bdatetime) {
        if ((bdatetime->date.year == (1900 + 0xFF)) &&
            (bdatetime->date.month == 0xFF) &&
            (bdatetime->date.day == 0xFF) &&
            (bdatetime->date.wday == 0xFF) &&
            (bdatetime->time.hour == 0xFF) &&
            (bdatetime->time.min == 0xFF) &&
            (bdatetime->time.sec == 0xFF) &&
            (bdatetime->time.hundredths == 0xFF)) {
            wildcard_present = true;
        }
    }

    return wildcard_present;
}

void datetime_wildcard_set(BACNET_DATE_TIME * bdatetime)
{
    if (bdatetime) {
        bdatetime->date.year = 1900 + 0xFF;
        bdatetime->date.month = 0xFF;
        bdatetime->date.day = 0xFF;
        bdatetime->date.wday = 0xFF;
        bdatetime->time.hour = 0xFF;
        bdatetime->time.min = 0xFF;
        bdatetime->time.sec = 0xFF;
        bdatetime->time.hundredths = 0xFF;
    }
}


#ifdef TEST
#include <assert.h>
#include <string.h>
#include "ctest.h"

void testBACnetDateTimeWildcard(Test * pTest)
{
    BACNET_DATE_TIME bdatetime;
    bool status = false;

    datetime_set_values(&bdatetime, 1900, 1, 1, 0, 0, 0, 0);
    status = datetime_wildcard(&bdatetime);
    ct_test(pTest, status == false);

    datetime_wildcard_set(&bdatetime);
    status = datetime_wildcard(&bdatetime);
    ct_test(pTest, status == true);
}

void testBACnetDateTimeAdd(Test * pTest)
{
    BACNET_DATE_TIME bdatetime, test_bdatetime;
    uint32_t minutes = 0;
    int diff = 0;

    datetime_set_values(&bdatetime, 1900, 1, 1, 0, 0, 0, 0);
    datetime_copy(&test_bdatetime, &bdatetime);
    datetime_add_minutes(&bdatetime, minutes);
    diff = datetime_compare(&test_bdatetime, &bdatetime);
    ct_test(pTest, diff == 0);

    datetime_set_values(&bdatetime, 1900, 1, 1, 0, 0, 0, 0);
    datetime_add_minutes(&bdatetime, 60);
    datetime_set_values(&test_bdatetime, 1900, 1, 1, 1, 0, 0, 0);
    diff = datetime_compare(&test_bdatetime, &bdatetime);
    ct_test(pTest, diff == 0);

    datetime_set_values(&bdatetime, 1900, 1, 1, 0, 0, 0, 0);
    datetime_add_minutes(&bdatetime, (24 * 60));
    datetime_set_values(&test_bdatetime, 1900, 1, 2, 0, 0, 0, 0);
    diff = datetime_compare(&test_bdatetime, &bdatetime);
    ct_test(pTest, diff == 0);

    datetime_set_values(&bdatetime, 1900, 1, 1, 0, 0, 0, 0);
    datetime_add_minutes(&bdatetime, (31 * 24 * 60));
    datetime_set_values(&test_bdatetime, 1900, 2, 1, 0, 0, 0, 0);
    diff = datetime_compare(&test_bdatetime, &bdatetime);
    ct_test(pTest, diff == 0);
}



void testBACnetDateTimeSeconds(Test * pTest)
{
    uint8_t hour = 0, minute = 0, second = 0;
    uint8_t test_hour = 0, test_minute = 0, test_second = 0;
    uint32_t seconds = 0, test_seconds;

    for (hour = 0; hour < 24; hour++) {
        for (minute = 0; minute < 60; minute += 3) {
            for (second = 0; second < 60; second += 17) {
                seconds = seconds_since_midnight(hour, minute, second);
                seconds_since_midnight_into_hms(seconds,
                    &test_hour, &test_minute, &test_second);
                test_seconds =
                    seconds_since_midnight(test_hour, test_minute,
                    test_second);
                ct_test(pTest, seconds == test_seconds);
            }
        }
    }
}

void testBACnetDate(Test * pTest)
{
    BACNET_DATE bdate1, bdate2;
    int diff = 0;

    datetime_set_date(&bdate1, 1900, 1, 1);
    datetime_copy_date(&bdate2, &bdate1);
    diff = datetime_compare_date(&bdate1, &bdate2);
    ct_test(pTest, diff == 0);
    datetime_set_date(&bdate2, 1900, 1, 2);
    diff = datetime_compare_date(&bdate1, &bdate2);
    ct_test(pTest, diff < 0);
    datetime_set_date(&bdate2, 1900, 2, 1);
    diff = datetime_compare_date(&bdate1, &bdate2);
    ct_test(pTest, diff < 0);
    datetime_set_date(&bdate2, 1901, 1, 1);
    diff = datetime_compare_date(&bdate1, &bdate2);
    ct_test(pTest, diff < 0);

    /* midpoint */
    datetime_set_date(&bdate1, 2007, 7, 15);
    datetime_copy_date(&bdate2, &bdate1);
    diff = datetime_compare_date(&bdate1, &bdate2);
    ct_test(pTest, diff == 0);
    datetime_set_date(&bdate2, 2007, 7, 14);
    diff = datetime_compare_date(&bdate1, &bdate2);
    ct_test(pTest, diff > 0);
    datetime_set_date(&bdate2, 2007, 7, 1);
    diff = datetime_compare_date(&bdate1, &bdate2);
    ct_test(pTest, diff > 0);
    datetime_set_date(&bdate2, 2007, 7, 31);
    diff = datetime_compare_date(&bdate1, &bdate2);
    ct_test(pTest, diff < 0);
    datetime_set_date(&bdate2, 2007, 8, 15);
    diff = datetime_compare_date(&bdate1, &bdate2);
    ct_test(pTest, diff < 0);
    datetime_set_date(&bdate2, 2007, 12, 15);
    diff = datetime_compare_date(&bdate1, &bdate2);
    ct_test(pTest, diff < 0);
    datetime_set_date(&bdate2, 2007, 6, 15);
    diff = datetime_compare_date(&bdate1, &bdate2);
    ct_test(pTest, diff > 0);
    datetime_set_date(&bdate2, 2007, 1, 15);
    diff = datetime_compare_date(&bdate1, &bdate2);
    ct_test(pTest, diff > 0);
    datetime_set_date(&bdate2, 2006, 7, 15);
    diff = datetime_compare_date(&bdate1, &bdate2);
    ct_test(pTest, diff > 0);
    datetime_set_date(&bdate2, 1900, 7, 15);
    diff = datetime_compare_date(&bdate1, &bdate2);
    ct_test(pTest, diff > 0);
    datetime_set_date(&bdate2, 2008, 7, 15);
    diff = datetime_compare_date(&bdate1, &bdate2);
    ct_test(pTest, diff < 0);
    datetime_set_date(&bdate2, 2154, 7, 15);
    diff = datetime_compare_date(&bdate1, &bdate2);
    ct_test(pTest, diff < 0);

    return;
}

void testBACnetTime(Test * pTest)
{
    BACNET_TIME btime1, btime2;
    int diff = 0;

    datetime_set_time(&btime1, 0, 0, 0, 0);
    datetime_copy_time(&btime2, &btime1);
    diff = datetime_compare_time(&btime1, &btime2);
    ct_test(pTest, diff == 0);

    datetime_set_time(&btime1, 23, 59, 59, 99);
    datetime_copy_time(&btime2, &btime1);
    diff = datetime_compare_time(&btime1, &btime2);
    ct_test(pTest, diff == 0);

    /* midpoint */
    datetime_set_time(&btime1, 12, 30, 30, 50);
    datetime_copy_time(&btime2, &btime1);
    diff = datetime_compare_time(&btime1, &btime2);
    ct_test(pTest, diff == 0);
    datetime_set_time(&btime2, 12, 30, 30, 51);
    diff = datetime_compare_time(&btime1, &btime2);
    ct_test(pTest, diff < 0);
    datetime_set_time(&btime2, 12, 30, 31, 50);
    diff = datetime_compare_time(&btime1, &btime2);
    ct_test(pTest, diff < 0);
    datetime_set_time(&btime2, 12, 31, 30, 50);
    diff = datetime_compare_time(&btime1, &btime2);
    ct_test(pTest, diff < 0);
    datetime_set_time(&btime2, 13, 30, 30, 50);
    diff = datetime_compare_time(&btime1, &btime2);
    ct_test(pTest, diff < 0);

    datetime_set_time(&btime2, 12, 30, 30, 49);
    diff = datetime_compare_time(&btime1, &btime2);
    ct_test(pTest, diff > 0);
    datetime_set_time(&btime2, 12, 30, 29, 50);
    diff = datetime_compare_time(&btime1, &btime2);
    ct_test(pTest, diff > 0);
    datetime_set_time(&btime2, 12, 29, 30, 50);
    diff = datetime_compare_time(&btime1, &btime2);
    ct_test(pTest, diff > 0);
    datetime_set_time(&btime2, 11, 30, 30, 50);
    diff = datetime_compare_time(&btime1, &btime2);
    ct_test(pTest, diff > 0);

    return;
}

void testBACnetDateTime(Test * pTest)
{
    BACNET_DATE_TIME bdatetime1, bdatetime2;
    BACNET_DATE bdate;
    BACNET_TIME btime;
    int diff = 0;

    datetime_set_values(&bdatetime1, 1900, 1, 1, 0, 0, 0, 0);
    datetime_copy(&bdatetime2, &bdatetime1);
    diff = datetime_compare(&bdatetime1, &bdatetime2);
    ct_test(pTest, diff == 0);
    datetime_set_time(&btime, 0, 0, 0, 0);
    datetime_set_date(&bdate, 1900, 1, 1);
    datetime_set(&bdatetime1, &bdate, &btime);
    diff = datetime_compare(&bdatetime1, &bdatetime2);
    ct_test(pTest, diff == 0);

    /* midpoint */
    /* if datetime1 is before datetime2, returns negative */
    datetime_set_values(&bdatetime1, 2000, 7, 15, 12, 30, 30, 50);
    datetime_set_values(&bdatetime2, 2000, 7, 15, 12, 30, 30, 51);
    diff = datetime_compare(&bdatetime1, &bdatetime2);
    ct_test(pTest, diff < 0);
    datetime_set_values(&bdatetime2, 2000, 7, 15, 12, 30, 31, 50);
    diff = datetime_compare(&bdatetime1, &bdatetime2);
    ct_test(pTest, diff < 0);
    datetime_set_values(&bdatetime2, 2000, 7, 15, 12, 31, 30, 50);
    diff = datetime_compare(&bdatetime1, &bdatetime2);
    ct_test(pTest, diff < 0);
    datetime_set_values(&bdatetime2, 2000, 7, 15, 13, 30, 30, 50);
    diff = datetime_compare(&bdatetime1, &bdatetime2);
    ct_test(pTest, diff < 0);
    datetime_set_values(&bdatetime2, 2000, 7, 16, 12, 30, 30, 50);
    diff = datetime_compare(&bdatetime1, &bdatetime2);
    ct_test(pTest, diff < 0);
    datetime_set_values(&bdatetime2, 2000, 8, 15, 12, 30, 30, 50);
    diff = datetime_compare(&bdatetime1, &bdatetime2);
    ct_test(pTest, diff < 0);
    datetime_set_values(&bdatetime2, 2001, 7, 15, 12, 30, 30, 50);
    diff = datetime_compare(&bdatetime1, &bdatetime2);
    ct_test(pTest, diff < 0);
    datetime_set_values(&bdatetime2, 2000, 7, 15, 12, 30, 30, 49);
    diff = datetime_compare(&bdatetime1, &bdatetime2);
    ct_test(pTest, diff > 0);
    datetime_set_values(&bdatetime2, 2000, 7, 15, 12, 30, 29, 50);
    diff = datetime_compare(&bdatetime1, &bdatetime2);
    ct_test(pTest, diff > 0);
    datetime_set_values(&bdatetime2, 2000, 7, 15, 12, 29, 30, 50);
    diff = datetime_compare(&bdatetime1, &bdatetime2);
    ct_test(pTest, diff > 0);
    datetime_set_values(&bdatetime2, 2000, 7, 15, 11, 30, 30, 50);
    diff = datetime_compare(&bdatetime1, &bdatetime2);
    ct_test(pTest, diff > 0);
    datetime_set_values(&bdatetime2, 2000, 7, 14, 12, 30, 30, 50);
    diff = datetime_compare(&bdatetime1, &bdatetime2);
    ct_test(pTest, diff > 0);
    datetime_set_values(&bdatetime2, 2000, 6, 15, 12, 30, 30, 50);
    diff = datetime_compare(&bdatetime1, &bdatetime2);
    ct_test(pTest, diff > 0);
    datetime_set_values(&bdatetime2, 1999, 7, 15, 12, 30, 30, 50);
    diff = datetime_compare(&bdatetime1, &bdatetime2);
    ct_test(pTest, diff > 0);


    return;
}

void testDateEpoch(Test * pTest)
{
    uint32_t days = 0;
    uint16_t year = 0, test_year = 0;
    uint8_t month = 0, test_month = 0;
    uint8_t day = 0, test_day = 0;

    days = days_since_epoch(1900, 1, 1);
    ct_test(pTest, days == 0);
    days_since_epoch_into_ymd(days, &year, &month, &day);
    ct_test(pTest, year == 1900);
    ct_test(pTest, month == 1);
    ct_test(pTest, day == 1);


    for (year = 1900; year <= 2154; year++) {
        for (month = 1; month <= 12; month++) {
            for (day = 1; day <= month_days(year, month); day++) {
                days = days_since_epoch(year, month, day);
                days_since_epoch_into_ymd(days,
                    &test_year, &test_month, &test_day);
                ct_test(pTest, year == test_year);
                ct_test(pTest, month == test_month);
                ct_test(pTest, day == test_day);
            }
        }
    }
}

void testBACnetDayOfWeek(Test * pTest)
{
    uint8_t dow = 0;

    /* 1/1/1900 is a Monday */
    dow = day_of_week(1900, 1, 1);
    ct_test(pTest, dow == 1);

    /* 1/1/2007 is a Monday */
    dow = day_of_week(2007, 1, 1);
    ct_test(pTest, dow == 1);
    dow = day_of_week(2007, 1, 2);
    ct_test(pTest, dow == 2);
    dow = day_of_week(2007, 1, 3);
    ct_test(pTest, dow == 3);
    dow = day_of_week(2007, 1, 4);
    ct_test(pTest, dow == 4);
    dow = day_of_week(2007, 1, 5);
    ct_test(pTest, dow == 5);
    dow = day_of_week(2007, 1, 6);
    ct_test(pTest, dow == 6);
    dow = day_of_week(2007, 1, 7);
    ct_test(pTest, dow == 7);

    dow = day_of_week(2007, 1, 31);
    ct_test(pTest, dow == 3);
}

#ifdef TEST_DATE_TIME
int main(void)
{
    Test *pTest;
    bool rc;

    pTest = ct_create("BACnet Date Time", NULL);
    /* individual tests */
    rc = ct_addTestFunction(pTest, testBACnetDate);
    assert(rc);
    rc = ct_addTestFunction(pTest, testBACnetTime);
    assert(rc);
    rc = ct_addTestFunction(pTest, testBACnetDateTime);
    assert(rc);
    rc = ct_addTestFunction(pTest, testBACnetDayOfWeek);
    assert(rc);
    rc = ct_addTestFunction(pTest, testDateEpoch);
    assert(rc);
    rc = ct_addTestFunction(pTest, testBACnetDateTimeSeconds);
    assert(rc);
    rc = ct_addTestFunction(pTest, testBACnetDateTimeAdd);
    assert(rc);
    rc = ct_addTestFunction(pTest, testBACnetDateTimeWildcard);
    assert(rc);

    ct_setStream(pTest, stdout);
    ct_run(pTest);
    (void) ct_report(pTest);
    ct_destroy(pTest);

    return 0;
}
#endif                          /* TEST_DATE_TIME */
#endif                          /* TEST */
