/**
 * \mainpage
 * \brief DS1307, DS3231 and DS3232 RTCs basic library
 *
 * Really tiny library to basic RTC functionality on Arduino.
 *
 * Supported features:
 *     * SQuare Wave Generator
 *     * Fixed output pin for DS1307
 *     * RAM for DS1307 and DS3232
 *     * temperature sensor for DS3231 and DS3232
 *     * Alarms (1 and 2) for DS3231 and DS3232
 *     * Power failure check for DS3231 and DS3232
 *
 * See uEEPROMLib for EEPROM support, https://github.com/Naguissa/uEEPROMLib
 *
 * @see <a href="https://github.com/Naguissa/uRTCLib">https://github.com/Naguissa/uRTCLib</a>
 * @see <a href="https://www.foroelectro.net/librerias-arduino-ide-f29/rtclib-arduino-libreria-simple-y-eficaz-para-rtc-y-t95.html">https://www.foroelectro.net/librerias-arduino-ide-f29/rtclib-arduino-libreria-simple-y-eficaz-para-rtc-y-t95.html</a>
 * @see <a href="mailto:naguissa@foroelectro.net">naguissa@foroelectro.net</a>
 * @see <a href="https://github.com/Naguissa/uEEPROMLib">See uEEPROMLib for EEPROM support.</a>
 * @version 6.2.4
 */
/** \file uRTCLib.h
 *   \brief uRTCLib header file
 */
#ifndef URTCLIB
/**
	 * \brief Prevent multiple inclussion
	 */
#define URTCLIB
#include "Arduino.h"
#include "Wire.h"

/**
	 * \brief Default RTC I2C address
	 *
	 * Usual address is 0x68
	 */
#define URTCLIB_ADDRESS 0x68

/************	ALARM SELECTION: ***********/
//Note: Not valid for DS1307!

/**
	 * \brief Alarm 1 - Disabled
	 * 
	 * Alarm1 structure:
	 * 
	 * bit 0 - A1M1
	 * bit 1 - A1M2
	 * bit 2 - A1M3
	 * bit 3 - A1M4
	 * bit 4 - A1 DT/DY
	 * bit 5 - A1 Enabled
	 * bit 6 - Unused, always 0
	 * bit 7 - Always 0
	 */
#define URTCLIB_ALARM_TYPE_1_NONE 0b00000000

/**
	 * \brief Alarm 1 - Trigger every second
	 */
#define URTCLIB_ALARM_TYPE_1_ALL_S 0b00101111

/**
	 * \brief Alarm 1 - Trigger every minute at a fixed second
	 */
#define URTCLIB_ALARM_TYPE_1_FIXED_S 0b00101110

/**
	 * \brief Alarm 1 - Trigger every hour at a fixed minute and second
	 */
#define URTCLIB_ALARM_TYPE_1_FIXED_MS 0b00101100

/**
	 * \brief Alarm 1 - Trigger every day at a fixed hour, minute and second
	 */
#define URTCLIB_ALARM_TYPE_1_FIXED_HMS 0b00101000

/**
	 * \brief Alarm 1 - Trigger every month at a fixed day, hour, minute and second
	 */
#define URTCLIB_ALARM_TYPE_1_FIXED_DHMS 0b00100000

/**
	 * \brief Alarm 1 - Trigger every week at a fixed day-of-week, hour, minute and second
	 */
#define URTCLIB_ALARM_TYPE_1_FIXED_DOWHMS 0b00110000

/**
	 * \brief Alarm 2 - Disabled
	 * 
	 * Alarm1 structure:
	 * 
	 * bit 0 - A2M1 - Unused, always 0
	 * bit 1 - A2M2
	 * bit 2 - A2M3
	 * bit 3 - A2M4
	 * bit 4 - A2 DT/DY
	 * bit 5 - A2 Enabled
	 * bit 6 - Unused, always 0
	 * bit 7 - Always 1
	 */
#define URTCLIB_ALARM_TYPE_2_NONE 0b10000000

/**
	 * \brief Alarm 2 - Trigger every minute at 00 seconds
	 */
#define URTCLIB_ALARM_TYPE_2_ALL_M 0b10101110

/**
	 * \brief Alarm 2 - Trigger every hour at minute and 00 seconds
	 */
#define URTCLIB_ALARM_TYPE_2_FIXED_M 0b10101100

/**
	 * \brief Alarm 2 - Trigger every day at hour, minute and 00 seconds
	 */
#define URTCLIB_ALARM_TYPE_2_FIXED_HM 0b10101000

/**
	 * \brief Alarm 2 - Trigger every month at day, hour, minute and 00 seconds
	 */
#define URTCLIB_ALARM_TYPE_2_FIXED_DHM 0b10100000

/**
	 * \brief Alarm 2 - Trigger every week at day-of-week, hour, minute and 00 seconds
	 */
#define URTCLIB_ALARM_TYPE_2_FIXED_DOWHM 0b10110000

/**
	 * \brief When requesting for Alarm 1
	 */
#define URTCLIB_ALARM_1 URTCLIB_ALARM_TYPE_1_NONE

/**
	 * \brief When requesting for Alarm 2
	 */
#define URTCLIB_ALARM_2 URTCLIB_ALARM_TYPE_2_NONE

/************	SQWG SELECTION: ***********/

/**
	 * \brief SQWG OFF, keeps output low
	 *
	 * Only valid for DS1307
	 */
#define URTCLIB_SQWG_OFF_0 0b11111111

/**
	 * \brief SQWG OFF, keeps output hight
	 */
#define URTCLIB_SQWG_OFF_1 0b11111110

/**
	 * \brief SQWG running at 1Hz
	 */
#define URTCLIB_SQWG_1H 0b00000000

/**
	 * \brief SQWG running at 1024Hz
	 *
	 * Not  valid for DS1307
	 */
#define URTCLIB_SQWG_1024H 0b00001000

/**
	 * \brief SQWG running at 4096Hz
	 */
#define URTCLIB_SQWG_4096H 0b00010000

/**
	 * \brief SQWG running at 8192Hz
	 */
#define URTCLIB_SQWG_8192H 0b00011000

/**
	 * \brief SQWG running at 32768Hz
	 *
	 * Only valid for DS1307
	 */
#define URTCLIB_SQWG_32768H 0b00000011

/************	TEMPERATURE ***********/
/**
	 * \brief Temperarure read error indicator return value
	 *
	 * 327.67º, obviously erroneous
	 */
#define URTCLIB_TEMP_ERROR 32767

/**************************************************************************/
/*!
    @brief  Simple general-purpose date/time class (no TZ / DST / leap second handling!).
            See http://en.wikipedia.org/wiki/Leap_second
*/
/**************************************************************************/
/** Constants */
class TimeSpan;
#define SECONDS_PER_DAY       86400L  ///< 60 * 60 * 24
#define SECONDS_FROM_1970_TO_2000 946684800  ///< Unixtime for 2000-01-01 00:00:00, useful for initialization

class DateTime
{
public:
	DateTime(uint32_t t = SECONDS_FROM_1970_TO_2000);
	DateTime(uint16_t year, uint8_t month, uint8_t day,
					 uint8_t hour = 0, uint8_t min = 0, uint8_t sec = 0);
	DateTime(const DateTime &copy);
	DateTime(const char *date, const char *time);
	DateTime(const __FlashStringHelper *date, const __FlashStringHelper *time);
	char *toString(char *buffer);

	/*!
      @brief  Return the year, stored as an offset from 2000
      @return uint16_t year
  */
	uint16_t year() const { return 2000 + yOff; }
	/*!
      @brief  Return month
      @return uint8_t month
  */
	uint8_t month() const { return m; }
	/*!
      @brief  Return day
      @return uint8_t day
  */
	uint8_t day() const { return d; }
	/*!
      @brief  Return hours
      @return uint8_t hours
  */
	uint8_t hour() const { return hh; }
	/*!
      @brief  Return minutes
      @return uint8_t minutes
  */
	uint8_t minute() const { return mm; }
	/*!
      @brief  Return seconds
      @return uint8_t seconds
  */
	uint8_t second() const { return ss; }

	uint8_t dayOfTheWeek() const;

	/** 32-bit times as seconds since 1/1/2000 */
	long secondstime() const;

	/** 32-bit times as seconds since 1/1/1970 */
	uint32_t unixtime(void) const;

	/** ISO 8601 Timestamp function */
	enum timestampOpt
	{
		TIMESTAMP_FULL, // YYYY-MM-DDTHH:MM:SS
		TIMESTAMP_TIME, // HH:MM:SS
		TIMESTAMP_DATE	// YYYY-MM-DD
	};
	String timestamp(timestampOpt opt = TIMESTAMP_FULL);

	DateTime operator+(const TimeSpan &span);
	DateTime operator-(const TimeSpan &span);
	TimeSpan operator-(const DateTime &right);
	bool operator<(const DateTime &right) const;
	/*!
      @brief  Test if one DateTime is greater (later) than another
      @param right DateTime object to compare
      @return True if the left object is greater than the right object, false otherwise
  */
	bool operator>(const DateTime &right) const { return right < *this; }
	/*!
      @brief  Test if one DateTime is less (earlier) than or equal to another
      @param right DateTime object to compare
      @return True if the left object is less than or equal to the right object, false otherwise
  */
	bool operator<=(const DateTime &right) const { return !(*this > right); }
	/*!
      @brief  Test if one DateTime is greater (later) than or equal to another
      @param right DateTime object to compare
      @return True if the left object is greater than or equal to the right object, false otherwise
  */
	bool operator>=(const DateTime &right) const { return !(*this < right); }
	bool operator==(const DateTime &right) const;
	/*!
      @brief  Test if two DateTime objects not equal
      @param right DateTime object to compare
      @return True if the two objects are not equal, false if they are
  */
	bool operator!=(const DateTime &right) const { return !(*this == right); }

protected:
	uint8_t yOff; ///< Year offset from 2000
	uint8_t m;		///< Month 1-12
	uint8_t d;		///< Day 1-31
	uint8_t hh;		///< Hours 0-23
	uint8_t mm;		///< Minutes 0-59
	uint8_t ss;		///< Seconds 0-59
};

/**************************************************************************/
/*!
    @brief  Timespan which can represent changes in time with seconds accuracy.
*/
/**************************************************************************/
class TimeSpan
{
public:
	TimeSpan(int32_t seconds = 0);
	TimeSpan(int16_t days, int8_t hours, int8_t minutes, int8_t seconds);
	TimeSpan(const TimeSpan &copy);

	/*!
      @brief  Number of days in the TimeSpan
              e.g. 4
      @return int16_t days
  */
	int16_t days() const { return _seconds / 86400L; }
	/*!
      @brief  Number of hours in the TimeSpan
              This is not the total hours, it includes the days
              e.g. 4 days, 3 hours - NOT 99 hours
      @return int8_t hours
  */
	int8_t hours() const { return _seconds / 3600 % 24; }
	/*!
      @brief  Number of minutes in the TimeSpan
              This is not the total minutes, it includes days/hours
              e.g. 4 days, 3 hours, 27 minutes
      @return int8_t minutes
  */
	int8_t minutes() const { return _seconds / 60 % 60; }
	/*!
      @brief  Number of seconds in the TimeSpan
              This is not the total seconds, it includes the days/hours/minutes
              e.g. 4 days, 3 hours, 27 minutes, 7 seconds
      @return int8_t seconds
  */
	int8_t seconds() const { return _seconds % 60; }
	/*!
      @brief  Total number of seconds in the TimeSpan, e.g. 358027
      @return int32_t seconds
  */
	int32_t totalseconds() const { return _seconds; }

	TimeSpan operator+(const TimeSpan &right);
	TimeSpan operator-(const TimeSpan &right);

protected:
	int32_t _seconds; ///< Actual TimeSpan value is stored as seconds
};

/************	MISC  ***********/

class uRTCLib
{
public:
	/******* Constructors *******/
	uRTCLib();
	uRTCLib(const int);
	uRTCLib(const int, const uint8_t);

	/******* RTC functions ********/
	DateTime now();
	uint8_t second();
	uint8_t minute();
	uint8_t hour();
	uint8_t day();
	uint8_t month();
	uint8_t year();
	uint8_t dayOfWeek();
	int16_t temp();
	void adjust(const DateTime &dt);
	void set_rtc_address(const int);

	/******* Lost power ********/
	bool lostPower();
	void lostPowerClear();

	/******** Alarms ************/
	bool alarmSet(const uint8_t, const uint8_t, const uint8_t, const uint8_t, const uint8_t); // Seconds will be ignored on Alarm 2
	bool alarmDisable(const uint8_t);
	bool alarmClearFlag(const uint8_t);
	uint8_t alarmMode(const uint8_t);
	uint8_t alarmSecond(const uint8_t);
	uint8_t alarmMinute(const uint8_t);
	uint8_t alarmHour(const uint8_t);
	uint8_t alarmDayDow(const uint8_t);

	/*********** SQWG ************/
	uint8_t sqwgMode();
	bool sqwgSetMode(const uint8_t);

	/************ RAM *************/
	// Only DS1307 and DS3232.
	// DS1307: Addresses 08h to 3Fh so we offset 08h positions and limit to 38h as maximum address
	// DS3232: Addresses 14h to FFh so we offset 14h positions and limit to EBh as maximum address
	byte ramRead(const uint8_t);
	bool ramWrite(const uint8_t, byte);

private:
	// Address
	int _rtc_address = URTCLIB_ADDRESS;
	// RTC rad data
	uint8_t _second = 0;
	uint8_t _minute = 0;
	uint8_t _hour = 0;
	uint8_t _day = 0;
	uint8_t _month = 0;
	uint8_t _year = 0;
	uint8_t _dayOfWeek = 0;
	int16_t _temp = 9999;

	// Alarms:
	uint8_t _a1_mode = URTCLIB_ALARM_TYPE_1_NONE;
	uint8_t _a1_second = 0;
	uint8_t _a1_minute = 0;
	uint8_t _a1_hour = 0;
	uint8_t _a1_day_dow = 0;

	uint8_t _a2_mode = URTCLIB_ALARM_TYPE_2_NONE;
	uint8_t _a2_minute = 0;
	uint8_t _a2_hour = 0;
	uint8_t _a2_day_dow = 0;

	// SQWG
	uint8_t _sqwg_mode = URTCLIB_SQWG_OFF_1;
};

#endif
