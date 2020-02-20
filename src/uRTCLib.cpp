/**
 * \class uRTCLib
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
 * @file uRTCLib.cpp
 * @copyright Naguissa
 * @author Naguissa
 * @see <a href="https://github.com/Naguissa/uRTCLib">https://github.com/Naguissa/uRTCLib</a>
 * @see <a href="https://www.foroelectro.net/librerias-arduino-ide-f29/rtclib-arduino-libreria-simple-y-eficaz-para-rtc-y-t95.html">https://www.foroelectro.net/librerias-arduino-ide-f29/rtclib-arduino-libreria-simple-y-eficaz-para-rtc-y-t95.html</a>
 * @see <a href="mailto:naguissa@foroelectro.net">naguissa@foroelectro.net</a>
 * @see <a href="https://github.com/Naguissa/uEEPROMLib">See uEEPROMLib for EEPROM support.</a>
 * @version 6.2.4
 */

#include <Arduino.h>
#include <Wire.h>
#include "uRTCLib.h"

/**************************************************************************/
// utility code, some of this could be exposed in the DateTime API if needed
/**************************************************************************/

/**
  Number of days in each month, from January to November. December is not
  needed. Omitting it avoids an incompatibility with Paul Stoffregen's Time
  library. C.f. https://github.com/adafruit/RTClib/issues/114
*/
const uint8_t daysInMonth[] PROGMEM = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30};

/**************************************************************************/
/*!
    @brief  Given a date, return number of days since 2000/01/01, valid for 2001..2099
    @param y Year
    @param m Month
    @param d Day
    @return Number of days
*/
/**************************************************************************/
static uint16_t date2days(uint16_t y, uint8_t m, uint8_t d)
{
	if (y >= 2000)
		y -= 2000;
	uint16_t days = d;
	for (uint8_t i = 1; i < m; ++i)
		days += pgm_read_byte(daysInMonth + i - 1);
	if (m > 2 && y % 4 == 0)
		++days;
	return days + 365 * y + (y + 3) / 4 - 1;
}

/**************************************************************************/
/*!
    @brief  Given a number of days, hours, minutes, and seconds, return the total seconds
    @param days Days
    @param h Hours
    @param m Minutes
    @param s Seconds
    @return Number of seconds total
*/
/**************************************************************************/
static long time2long(uint16_t days, uint8_t h, uint8_t m, uint8_t s)
{
	return ((days * 24L + h) * 60 + m) * 60 + s;
}

/**************************************************************************/
/*!
    @brief  DateTime constructor from unixtime
    @param t Initial time in seconds since Jan 1, 1970 (Unix time)
*/
/**************************************************************************/
DateTime::DateTime(uint32_t t)
{
	t -= SECONDS_FROM_1970_TO_2000; // bring to 2000 timestamp from 1970

	ss = t % 60;
	t /= 60;
	mm = t % 60;
	t /= 60;
	hh = t % 24;
	uint16_t days = t / 24;
	uint8_t leap;
	for (yOff = 0;; ++yOff)
	{
		leap = yOff % 4 == 0;
		if (days < 365 + leap)
			break;
		days -= 365 + leap;
	}
	for (m = 1; m < 12; ++m)
	{
		uint8_t daysPerMonth = pgm_read_byte(daysInMonth + m - 1);
		if (leap && m == 2)
			++daysPerMonth;
		if (days < daysPerMonth)
			break;
		days -= daysPerMonth;
	}
	d = days + 1;
}

/**************************************************************************/
/*!
    @brief  DateTime constructor from Y-M-D H:M:S
    @param year Year, 2 or 4 digits (year 2000 or higher)
    @param month Month 1-12
    @param day Day 1-31
    @param hour 0-23
    @param min 0-59
    @param sec 0-59
*/
/**************************************************************************/
DateTime::DateTime(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t min, uint8_t sec)
{
	if (year >= 2000)
		year -= 2000;
	yOff = year;
	m = month;
	d = day;
	hh = hour;
	mm = min;
	ss = sec;
}

/**************************************************************************/
/*!
    @brief  DateTime copy constructor using a member initializer list
    @param copy DateTime object to copy
*/
/**************************************************************************/
DateTime::DateTime(const DateTime &copy) : yOff(copy.yOff),
																					 m(copy.m),
																					 d(copy.d),
																					 hh(copy.hh),
																					 mm(copy.mm),
																					 ss(copy.ss)
{
}

/**************************************************************************/
/*!
    @brief  Convert a string containing two digits to uint8_t, e.g. "09" returns 9
    @param p Pointer to a string containing two digits
*/
/**************************************************************************/
static uint8_t conv2d(const char *p)
{
	uint8_t v = 0;
	if ('0' <= *p && *p <= '9')
		v = *p - '0';
	return 10 * v + *++p - '0';
}

/**************************************************************************/
/*!
    @brief  A convenient constructor for using "the compiler's time":
            DateTime now (__DATE__, __TIME__);
            NOTE: using F() would further reduce the RAM footprint, see below.
    @param date Date string, e.g. "Dec 26 2009"
    @param time Time string, e.g. "12:34:56"
*/
/**************************************************************************/
DateTime::DateTime(const char *date, const char *time)
{
	// sample input: date = "Dec 26 2009", time = "12:34:56"
	yOff = conv2d(date + 9);
	// Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec
	switch (date[0])
	{
	case 'J':
		m = (date[1] == 'a') ? 1 : ((date[2] == 'n') ? 6 : 7);
		break;
	case 'F':
		m = 2;
		break;
	case 'A':
		m = date[2] == 'r' ? 4 : 8;
		break;
	case 'M':
		m = date[2] == 'r' ? 3 : 5;
		break;
	case 'S':
		m = 9;
		break;
	case 'O':
		m = 10;
		break;
	case 'N':
		m = 11;
		break;
	case 'D':
		m = 12;
		break;
	}
	d = conv2d(date + 4);
	hh = conv2d(time);
	mm = conv2d(time + 3);
	ss = conv2d(time + 6);
}

/**************************************************************************/
/*!
    @brief  A convenient constructor for using "the compiler's time":
            This version will save RAM by using PROGMEM to store it by using the F macro.
            DateTime now (F(__DATE__), F(__TIME__));
    @param date Date string, e.g. "Dec 26 2009"
    @param time Time string, e.g. "12:34:56"
*/
/**************************************************************************/
DateTime::DateTime(const __FlashStringHelper *date, const __FlashStringHelper *time)
{
	// sample input: date = "Dec 26 2009", time = "12:34:56"
	char buff[11];
	memcpy_P(buff, date, 11);
	yOff = conv2d(buff + 9);
	// Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec
	switch (buff[0])
	{
	case 'J':
		m = (buff[1] == 'a') ? 1 : ((buff[2] == 'n') ? 6 : 7);
		break;
	case 'F':
		m = 2;
		break;
	case 'A':
		m = buff[2] == 'r' ? 4 : 8;
		break;
	case 'M':
		m = buff[2] == 'r' ? 3 : 5;
		break;
	case 'S':
		m = 9;
		break;
	case 'O':
		m = 10;
		break;
	case 'N':
		m = 11;
		break;
	case 'D':
		m = 12;
		break;
	}
	d = conv2d(buff + 4);
	memcpy_P(buff, time, 8);
	hh = conv2d(buff);
	mm = conv2d(buff + 3);
	ss = conv2d(buff + 6);
}

/**************************************************************************/
/*!
    @brief  Return DateTime in based on user defined format.
    @param buffer: array of char for holding the format description and the formatted DateTime. 
                   Before calling this method, the buffer should be initialized by the user with 
                   a format string, e.g. "YYYY-MM-DD hh:mm:ss". The method will overwrite 
                   the buffer with the formatted date and/or time.
    @return a pointer to the provided buffer. This is returned for convenience, 
            in order to enable idioms such as Serial.println(now.toString(buffer));
*/
/**************************************************************************/

char *DateTime::toString(char *buffer)
{
	for (int i = 0; i < strlen(buffer) - 1; i++)
	{
		if (buffer[i] == 'h' && buffer[i + 1] == 'h')
		{
			buffer[i] = '0' + hh / 10;
			buffer[i + 1] = '0' + hh % 10;
		}
		if (buffer[i] == 'm' && buffer[i + 1] == 'm')
		{
			buffer[i] = '0' + mm / 10;
			buffer[i + 1] = '0' + mm % 10;
		}
		if (buffer[i] == 's' && buffer[i + 1] == 's')
		{
			buffer[i] = '0' + ss / 10;
			buffer[i + 1] = '0' + ss % 10;
		}
		if (buffer[i] == 'D' && buffer[i + 1] == 'D' && buffer[i + 2] == 'D')
		{
			static PROGMEM const char day_names[] = "SunMonTueWedThuFriSat";
			const char *p = &day_names[3 * dayOfTheWeek()];
			buffer[i] = pgm_read_byte(p);
			buffer[i + 1] = pgm_read_byte(p + 1);
			buffer[i + 2] = pgm_read_byte(p + 2);
		}
		else if (buffer[i] == 'D' && buffer[i + 1] == 'D')
		{
			buffer[i] = '0' + d / 10;
			buffer[i + 1] = '0' + d % 10;
		}
		if (buffer[i] == 'M' && buffer[i + 1] == 'M' && buffer[i + 2] == 'M')
		{
			static PROGMEM const char month_names[] = "JanFebMarAprMayJunJulAugSepOctNovDec";
			const char *p = &month_names[3 * (m - 1)];
			buffer[i] = pgm_read_byte(p);
			buffer[i + 1] = pgm_read_byte(p + 1);
			buffer[i + 2] = pgm_read_byte(p + 2);
		}
		else if (buffer[i] == 'M' && buffer[i + 1] == 'M')
		{
			buffer[i] = '0' + m / 10;
			buffer[i + 1] = '0' + m % 10;
		}
		if (buffer[i] == 'Y' && buffer[i + 1] == 'Y' && buffer[i + 2] == 'Y' && buffer[i + 3] == 'Y')
		{
			buffer[i] = '2';
			buffer[i + 1] = '0';
			buffer[i + 2] = '0' + (yOff / 10) % 10;
			buffer[i + 3] = '0' + yOff % 10;
		}
		else if (buffer[i] == 'Y' && buffer[i + 1] == 'Y')
		{
			buffer[i] = '0' + (yOff / 10) % 10;
			buffer[i + 1] = '0' + yOff % 10;
		}
	}
	return buffer;
}

/**************************************************************************/
/*!
    @brief  Return the day of the week for this object, from 0-6.
    @return Day of week 0-6 starting with Sunday, e.g. Sunday = 0, Saturday = 6
*/
/**************************************************************************/
uint8_t DateTime::dayOfTheWeek() const
{
	uint16_t day = date2days(yOff, m, d);
	return (day + 6) % 7; // Jan 1, 2000 is a Saturday, i.e. returns 6
}

/**************************************************************************/
/*!
    @brief  Return unix time, seconds since Jan 1, 1970.
    @return Number of seconds since Jan 1, 1970
*/
/**************************************************************************/
uint32_t DateTime::unixtime(void) const
{
	uint32_t t;
	uint16_t days = date2days(yOff, m, d);
	t = time2long(days, hh, mm, ss);
	t += SECONDS_FROM_1970_TO_2000; // seconds from 1970 to 2000

	return t;
}

/**************************************************************************/
/*!
    @brief  Convert the DateTime to seconds
    @return The object as seconds since 2000-01-01
*/
/**************************************************************************/
long DateTime::secondstime(void) const
{
	long t;
	uint16_t days = date2days(yOff, m, d);
	t = time2long(days, hh, mm, ss);
	return t;
}

/**************************************************************************/
/*!
    @brief  Add a TimeSpan to the DateTime object
    @param span TimeSpan object
    @return new DateTime object with span added to it
*/
/**************************************************************************/
DateTime DateTime::operator+(const TimeSpan &span)
{
	return DateTime(unixtime() + span.totalseconds());
}

/**************************************************************************/
/*!
    @brief  Subtract a TimeSpan from the DateTime object
    @param span TimeSpan object
    @return new DateTime object with span subtracted from it
*/
/**************************************************************************/
DateTime DateTime::operator-(const TimeSpan &span)
{
	return DateTime(unixtime() - span.totalseconds());
}

/**************************************************************************/
/*!
    @brief  Subtract one DateTime from another
    @param right The DateTime object to subtract from self (the left object)
    @return TimeSpan of the difference between DateTimes
*/
/**************************************************************************/
TimeSpan DateTime::operator-(const DateTime &right)
{
	return TimeSpan(unixtime() - right.unixtime());
}

/**************************************************************************/
/*!
    @brief  Is one DateTime object less than (older) than the other?
    @param right Comparison DateTime object
    @return True if the left object is older than the right object
*/
/**************************************************************************/
bool DateTime::operator<(const DateTime &right) const
{
	return unixtime() < right.unixtime();
}

/**************************************************************************/
/*!
    @brief  Is one DateTime object equal to the other?
    @param right Comparison DateTime object
    @return True if both DateTime objects are the same
*/
/**************************************************************************/
bool DateTime::operator==(const DateTime &right) const
{
	return unixtime() == right.unixtime();
}

/**************************************************************************/
/*!
    @brief  ISO 8601 Timestamp
    @param opt Format of the timestamp
    @return Timestamp string, e.g. "2000-01-01T12:34:56"
*/
/**************************************************************************/
String DateTime::timestamp(timestampOpt opt)
{
	char buffer[20];

	//Generate timestamp according to opt
	switch (opt)
	{
	case TIMESTAMP_TIME:
		//Only time
		sprintf(buffer, "%02d:%02d:%02d", hh, mm, ss);
		break;
	case TIMESTAMP_DATE:
		//Only date
		sprintf(buffer, "%d-%02d-%02d", 2000 + yOff, m, d);
		break;
	default:
		//Full
		sprintf(buffer, "%d-%02d-%02dT%02d:%02d:%02d", 2000 + yOff, m, d, hh, mm, ss);
	}
	return String(buffer);
}

/**************************************************************************/
/*!
    @brief  Create a new TimeSpan object in seconds
    @param seconds Number of seconds
*/
/**************************************************************************/
TimeSpan::TimeSpan(int32_t seconds) : _seconds(seconds)
{
}

/**************************************************************************/
/*!
    @brief  Create a new TimeSpan object using a number of days/hours/minutes/seconds
            e.g. Make a TimeSpan of 3 hours and 45 minutes: new TimeSpan(0, 3, 45, 0);
    @param days Number of days
    @param hours Number of hours
    @param minutes Number of minutes
    @param seconds Number of seconds
*/
/**************************************************************************/
TimeSpan::TimeSpan(int16_t days, int8_t hours, int8_t minutes, int8_t seconds) : _seconds((int32_t)days * 86400L + (int32_t)hours * 3600 + (int32_t)minutes * 60 + seconds)
{
}

/**************************************************************************/
/*!
    @brief  Copy constructor, make a new TimeSpan using an existing one
    @param copy The TimeSpan to copy
*/
/**************************************************************************/
TimeSpan::TimeSpan(const TimeSpan &copy) : _seconds(copy._seconds)
{
}

/**************************************************************************/
/*!
    @brief  Add two TimeSpans
    @param right TimeSpan to add
    @return New TimeSpan object, sum of left and right
*/
/**************************************************************************/
TimeSpan TimeSpan::operator+(const TimeSpan &right)
{
	return TimeSpan(_seconds + right._seconds);
}

/**************************************************************************/
/*!
    @brief  Subtract a TimeSpan
    @param right TimeSpan to subtract
    @return New TimeSpan object, right subtracted from left
*/
/**************************************************************************/
TimeSpan TimeSpan::operator-(const TimeSpan &right)
{
	return TimeSpan(_seconds - right._seconds);
}

/**
 * \brief Constructor
 */
uRTCLib::uRTCLib() {}

/**
 * \brief Constructor
 *
 * @param rtc_address I2C address of RTC
 */
uRTCLib::uRTCLib(const int rtc_address)
{
	_rtc_address = rtc_address;
}


	/**
	 * \brief Convert binary coded decimal to normal decimal numbers
	 */
static uint8_t bcd2bin (uint8_t val) { return val - 6 * (val >> 4); }

/**
	 * \brief Convert normal decimal numbers to binary coded decimal
	 */
static uint8_t bin2bcd (uint8_t val) { return val + 6 * (val / 10); }


/**
 * \brief Refresh data from HW RTC
 */
DateTime uRTCLib::now()
{

	Wire.beginTransmission(_rtc_address);
	Wire.write((byte)0);
	Wire.endTransmission();

	Wire.requestFrom(_rtc_address, 7);
	uint8_t ss = bcd2bin(Wire.read() & 0x7F);
	uint8_t mm = bcd2bin(Wire.read());
	uint8_t hh = bcd2bin(Wire.read());
	Wire.read();
	uint8_t d = bcd2bin(Wire.read());
	uint8_t m = bcd2bin(Wire.read());
	uint16_t y = bcd2bin(Wire.read()) + 2000;

	return DateTime(y, m, d, hh, mm, ss);
}

/**
 * \brief Returns lost power VBAT staus
 *
 * WARNING: DS1307 is known to not have it at a known address
 *
 * @return True if power was lost (both power sources, VCC and VBAT)
 */
bool uRTCLib::lostPower()
{
	Wire.beginTransmission(_rtc_address);
	Wire.write(0X0F);
	Wire.endTransmission();

	Wire.requestFrom(_rtc_address, 1);
	uint8_t status = Wire.read();

	return ((status & 0B10000000) == 0B10000000);
}

/**
 * \brief Clears lost power VBAT staus
 *
 * WARNING: DS1307 is known to not have it at a known address
 */
void uRTCLib::lostPowerClear()
{
	Wire.beginTransmission(_rtc_address);
	Wire.write(0X0F);
	Wire.endTransmission();

	Wire.requestFrom(_rtc_address, 1);
	uint8_t status = Wire.read();
	status &= 0b01111111;

	Wire.beginTransmission(_rtc_address);

	Wire.write(0x0F);

	Wire.write(status);

	Wire.endTransmission();
}

/**
 * \brief Returns actual temperature
 *
 * Temperature is returned as degress * 100; i.e.: 3050 is 30.50º
 *
 * WARNING: DS1307 has no temperature register, so it always returns #URTCLIB_TEMP_ERROR
 *
 * @return Current stored temperature
 */
int16_t uRTCLib::temp()
{
	return _temp;
}

/**
 * \brief Returns actual second
 *
 * @return Current stored second
 */
uint8_t uRTCLib::second()
{
	return _second;
}

/**
 * \brief Returns actual minute
 *
 * @return Current stored minute
 */
uint8_t uRTCLib::minute()
{
	return _minute;
}

/**
 * \brief Returns actual hour
 *
 * @return Current stored hour
 */
uint8_t uRTCLib::hour()
{
	return _hour;
}

/**
 * \brief Returns actual day
 *
 * @return Current stored day
 */
uint8_t uRTCLib::day()
{
	return _day;
}

/**
 * \brief Returns actual month
 *
 * @return Current stored month
 */
uint8_t uRTCLib::month()
{
	return _month;
}

/**
 * \brief Returns actual year
 *
 * @return Current stored year
 */
uint8_t uRTCLib::year()
{
	return _year;
}

/**
 * \brief Returns actual Day Of Week
 *
 * @return Current stored Day Of Week
 */
uint8_t uRTCLib::dayOfWeek()
{
	return _dayOfWeek;
}

/**
 * \brief Sets RTC i2 addres
 *
 * @param addr RTC i2C address
 */
void uRTCLib::set_rtc_address(const int addr)
{
	_rtc_address = addr;
}

/**
 * \brief Sets RTC datetime data
 *
 * @param second second to set to HW RTC
 * @param minute minute to set to HW RTC
 * @param hour hour to set to HW RTC
 * @param dayOfWeek day of week to set to HW RTC
 * @param dayOfMonth day of month to set to HW RTC
 * @param month month to set to HW RTC
 * @param year year to set to HW RTC
 */
void uRTCLib::adjust(const DateTime &dt)
{

	Wire.beginTransmission(_rtc_address);
	Wire.write(0);															 // set next input to start at the seconds register
	Wire.write(bin2bcd(dt.second));		 // set seconds
	Wire.write(bin2bcd(dt.minute));		 // set minutes
	Wire.write(bin2bcd(dt.hour));			 // set hours
	Wire.write(bin2bcd(0));						 // set day of week (1=Sunday, 7=Saturday)
	Wire.write(bin2bcd(dt.dayOfMonth)); // set date (1 to 31)
	Wire.write(bin2bcd(dt.month));			 // set month
	Wire.write(bin2bcd(dt.year));			 // set year (0 to 99)
	Wire.endTransmission();

	//
	Wire.beginTransmission(_rtc_address);
	Wire.write(0X0F);
	Wire.endTransmission();

	/* flip OSF bit --> Disabled, use lostPowerClear instead.
	Wire.requestFrom(_rtc_address, 1);
	uint8_t statreg = Wire.read();
	statreg &= ~0x80;
	
	Wire.beginTransmission(_rtc_address);
	Wire.write(0X0F);
	Wire.write((byte)statreg);
	Wire.endTransmission();
	*/
}

/*************  Alarms: ****************/

/**
 * \brief Sets any alarm
 *
 * This method can also be used to disable an alarm, but it's better to use alarmDisable(const uint8_t alarm) to do so.
 *
 * @param type Alarm type:
 *	 - #URTCLIB_ALARM_TYPE_1_NONE
 *	 - #URTCLIB_ALARM_TYPE_1_ALL_S
 *	 - #URTCLIB_ALARM_TYPE_1_FIXED_S
 *	 - #URTCLIB_ALARM_TYPE_1_FIXED_MS
 *	 - #URTCLIB_ALARM_TYPE_1_FIXED_HMS
 *	 - #URTCLIB_ALARM_TYPE_1_FIXED_DHMS
 *	 - #URTCLIB_ALARM_TYPE_1_FIXED_DOWHMS
 *	 - #URTCLIB_ALARM_TYPE_2_NONE
 *	 - #URTCLIB_ALARM_TYPE_2_ALL_M
 *	 - #URTCLIB_ALARM_TYPE_2_FIXED_M
 *	 - #URTCLIB_ALARM_TYPE_2_FIXED_HM
 *	 - #URTCLIB_ALARM_TYPE_2_FIXED_DHM
 *	 - #URTCLIB_ALARM_TYPE_2_FIXED_DOWHM
 * @param second second to set Alarm (ignored in Alarm 2)
 * @param minute minute to set Alarm
 * @param hour hour to set Alarm
 * @param day_dow Day of the month or DOW to set Alarm, depending on alarm type
 *
 * @return false in case of not supported (DS1307) or wrong parameters
 */
bool uRTCLib::alarmSet(const uint8_t type, const uint8_t second, const uint8_t minute, const uint8_t hour, const uint8_t day_dow)
{
	bool ret = false;
	uint8_t status;

	if (type == URTCLIB_ALARM_TYPE_1_NONE)
	{
		ret = true;

		// Disable Alarm:
		Wire.beginTransmission(_rtc_address);

		Wire.write(0x0E);

		Wire.endTransmission();

		Wire.requestFrom(_rtc_address, 1);
		status = Wire.read();
		status &= 0b11111110;
		Wire.beginTransmission(_rtc_address);

		Wire.write(0x0E);

		Wire.write(status);

		Wire.endTransmission();

		_a1_mode = type;
	}
	else if (type == URTCLIB_ALARM_TYPE_2_NONE)
	{
		ret = true;

		// Disable Alarm:
		Wire.beginTransmission(_rtc_address);

		Wire.write(0x0E);

		Wire.endTransmission();

		Wire.requestFrom(_rtc_address, 1);
		status = Wire.read();
		status &= 0b11111101;
		Wire.beginTransmission(_rtc_address);

		Wire.write(0x0E);

		Wire.write(status);

		Wire.endTransmission();

		_a2_mode = type;
	}
	else
	{
		switch (type & 0b10000000)
		{
		case 0b00000000: // Alarm 1
			ret = true;
			Wire.beginTransmission(_rtc_address);

			Wire.write(0x07); // set next input to start at the seconds register

			Wire.write((bin2bcd(second) & 0b01111111) | ((type & 0b00000001) << 7)); // set seconds & mode/bit1

			Wire.write((bin2bcd(minute) & 0b01111111) | ((type & 0b00000010) << 6)); // set minutes & mode/bit2

			Wire.write((bin2bcd(hour) & 0b00111111) | ((type & 0b00000100) << 5)); // set hours & mode/bit3

			Wire.write((bin2bcd(day_dow) & 0b00111111) | ((type & 0b00001000) << 4) | ((type & 0b00010000) << 2)); // set date / day of week (1=Sunday, 7=Saturday)  & mode/bit4 & mode/DY-DT

			Wire.endTransmission();

			// Enable Alarm:
			Wire.beginTransmission(_rtc_address);

			Wire.write(0x0E);

			Wire.endTransmission();

			Wire.requestFrom(_rtc_address, 1);

			status = Wire.read();
			status = status | 0b00000101; // INTCN and A1IE bits
			Wire.beginTransmission(_rtc_address);

			Wire.write(0x0E);

			Wire.write(status);

			Wire.endTransmission();

			_a1_mode = type;
			_a1_second = second;
			_a1_minute = minute;
			_a1_hour = hour;
			_a1_day_dow = day_dow;
			_sqwg_mode = URTCLIB_SQWG_OFF_1;

			break;

		case 0b10000000: // Alarm 2
			ret = true;
			Wire.beginTransmission(_rtc_address);

			Wire.write(0x0B); // set next input to start at the minutes register

			Wire.write((bin2bcd(minute) & 0b01111111) | ((type & 0b00000010) << 6)); // set minutes & mode/bit2

			Wire.write((bin2bcd(hour) & 0b00111111) | ((type & 0b00000100) << 5)); // set hours & mode/bit3

			Wire.write((bin2bcd(day_dow) & 0b00111111) | ((type & 0b00001000) << 4) | ((type & 0b00010000) << 2)); // set date / day of week (1=Sunday, 7=Saturday)  & mode/bit4 & mode/DY-DT (bit3)

			Wire.endTransmission();

			// Enable Alarm:
			Wire.beginTransmission(_rtc_address);

			Wire.write(0x0E);

			Wire.endTransmission();

			Wire.requestFrom(_rtc_address, 1);

			status = Wire.read();
			status = status | 0b00000110; // INTCN and A2IE bits
			Wire.beginTransmission(_rtc_address);

			Wire.write(0x0E);

			Wire.write(status);

			Wire.endTransmission();

			_a2_mode = type;
			_a2_minute = minute;
			_a2_hour = hour;
			_a2_day_dow = day_dow;
			_sqwg_mode = URTCLIB_SQWG_OFF_1;

			break;
		} // Alarm type switch

	} // if..else
	return ret;
}

/**
 * \brief Disables an alarm
 *
 * @param alarm Alarm number:
 *	 - #URTCLIB_ALARM_1
 *	 - #URTCLIB_ALARM_2
 *
 * @return false in case of not supported (DS1307) or wrong parameters
 */
bool uRTCLib::alarmDisable(const uint8_t alarm)
{
	uint8_t status, mask = 0;
	switch (alarm)
	{
	case URTCLIB_ALARM_1: // Alarm 1
		mask = 0b11111110;	// A1IE bit
		break;

	case URTCLIB_ALARM_2: // Alarm 2
		mask = 0b11111101;	// A2IE bit
		break;
	} // Alarm type switch
	if (mask)
	{
		// Disable Alarm:
		Wire.beginTransmission(_rtc_address);

		Wire.write(0x0E);

		Wire.endTransmission();

		Wire.requestFrom(_rtc_address, 1);
		status = Wire.read();
		status &= 0b11111110; // A1IE bit
		Wire.beginTransmission(_rtc_address);

		Wire.write(0x0E);

		Wire.write(status);

		Wire.endTransmission();

		_a1_mode = URTCLIB_ALARM_TYPE_1_NONE;
		return true;
	}
	return false;
}

/**
 * \brief Clears an alarm flag
 *
 * @param alarm Alarm number:
 *	 - #URTCLIB_ALARM_1
 *	 - #URTCLIB_ALARM_2
 *
 * @return false in case of not supported (DS1307) or wrong parameters
 */
bool uRTCLib::alarmClearFlag(const uint8_t alarm)
{

	uint8_t status, mask = 0;
	switch (alarm)
	{
	case URTCLIB_ALARM_1: // Alarm 1
		mask = 0b11111110;
		break;

	case URTCLIB_ALARM_2: // Alarm 2
		mask = 0b11111101;
		break;

	} // Alarm type switch
	if (mask)
	{
		// Clear Alarm Flag:
		Wire.beginTransmission(_rtc_address);

		Wire.write(0x0F);

		Wire.endTransmission();

		Wire.requestFrom(_rtc_address, 1);
		status = Wire.read();
		status &= mask; // A?F bit
		Wire.beginTransmission(_rtc_address);

		Wire.write(0x0F);

		Wire.write(status);

		Wire.endTransmission();

		return true;
	}
	return false;
}

/**
 * \brief Returns actual alarm mode.
 *
 * See URTCLIB_ALARM_TYPE_X_YYYYY defines to see modes
 *
 * @param alarm Alarm number:
 *	 - #URTCLIB_ALARM_1
 *	 - #URTCLIB_ALARM_2
 *
 * @return Current stored mode. 0b11111111 means error.
 *	 - #URTCLIB_ALARM_TYPE_1_NONE
 *	 - #URTCLIB_ALARM_TYPE_1_ALL_S
 *	 - #URTCLIB_ALARM_TYPE_1_FIXED_S
 *	 - #URTCLIB_ALARM_TYPE_1_FIXED_MS
 *	 - #URTCLIB_ALARM_TYPE_1_FIXED_HMS
 *	 - #URTCLIB_ALARM_TYPE_1_FIXED_DHMS
 *	 - #URTCLIB_ALARM_TYPE_1_FIXED_DOWHMS
 *	 -	...or...
 *	 - #URTCLIB_ALARM_TYPE_2_NONE
 *	 - #URTCLIB_ALARM_TYPE_2_ALL_M
 *	 - #URTCLIB_ALARM_TYPE_2_FIXED_M
 *	 - #URTCLIB_ALARM_TYPE_2_FIXED_HM
 *	 - #URTCLIB_ALARM_TYPE_2_FIXED_DHM
 *	 - #URTCLIB_ALARM_TYPE_2_FIXED_DOWHM
 */
uint8_t uRTCLib::alarmMode(const uint8_t alarm)
{

	switch (alarm)
	{
	case URTCLIB_ALARM_1: // Alarm 1
		return _a1_mode;
		break;

	case URTCLIB_ALARM_2: // Alarm 2
		return _a2_mode;
		break;
	} // Alarm type switch
	return 0b11111111;
}

/**
 * \brief Returns actual alarm second
 *
 * @param alarm Alarm number:
 *	 - #URTCLIB_ALARM_1
 *	 - #URTCLIB_ALARM_2
 *
 * @return Current stored second. 0b11111111 means error.
 */
uint8_t uRTCLib::alarmSecond(const uint8_t alarm)
{

	switch (alarm)
	{
	case URTCLIB_ALARM_1: // Alarm 1
		return _a1_second;
		break;

	case URTCLIB_ALARM_2: // Alarm 2
		return 0;
		break;
	} // Alarm type switch
	return 0b11111111;
}

/**
 * \brief Returns actual alarm minute
 *
 * @param alarm Alarm number:
 *	 - #URTCLIB_ALARM_1
 *	 - #URTCLIB_ALARM_2
 *
 * @return Current stored minute. 0b11111111 means error.
 */
uint8_t uRTCLib::alarmMinute(const uint8_t alarm)
{

	switch (alarm)
	{
	case URTCLIB_ALARM_1: // Alarm 1
		return _a1_minute;
		break;

	case URTCLIB_ALARM_2: // Alarm 2
		return _a2_minute;
		break;
	} // Alarm type switch
	return 0b11111111;
}

/**
 * \brief Returns actual alarm hour
 *
 * @param alarm Alarm number:
 *	 - #URTCLIB_ALARM_1
 *	 - #URTCLIB_ALARM_2
 *
 * @return Current stored hour. 0b11111111 means error.
 */
uint8_t uRTCLib::alarmHour(const uint8_t alarm)
{

	switch (alarm)
	{
	case URTCLIB_ALARM_1: // Alarm 1
		return _a1_hour;
		break;

	case URTCLIB_ALARM_2: // Alarm 2
		return _a2_hour;
		break;
	} // Alarm type switch
	return 0b11111111;
}

/**
 * \brief Returns actual alarm day or DOW
 *
 * @param alarm Alarm number:
 *	 - #URTCLIB_ALARM_1
 *	 - #URTCLIB_ALARM_2
 *
 * @return Current stored day or dow. 0b11111111 means error.
 */
uint8_t uRTCLib::alarmDayDow(const uint8_t alarm)
{

	switch (alarm)
	{
	case URTCLIB_ALARM_1: // Alarm 1
		return _a1_day_dow;
		break;

	case URTCLIB_ALARM_2: // Alarm 2
		return _a2_day_dow;
		break;
	} // Alarm type switch
	return 0b11111111;
}

/************** SQuare Wave Generator ****************/

/**
 * \brief Changes SQWG mode, including turning it off
 *
 * @param mode SQWG mode:
 *	 - #URTCLIB_SQWG_OFF_0
 *	 - #URTCLIB_SQWG_OFF_1
 *	 - #URTCLIB_SQWG_1H
 *	 - #URTCLIB_SQWG_1024H
 *	 - #URTCLIB_SQWG_4096H
 *	 - #URTCLIB_SQWG_8192H
 *	 - #URTCLIB_SQWG_32768H
 *
 * @return false in case of not supported (DS1307) or wrong parameters
 */
bool uRTCLib::sqwgSetMode(const uint8_t mode)
{
	uint8_t status, processAnd = 0b00000000, processOr = 0b00000000;

	switch (mode)
	{
	case URTCLIB_SQWG_OFF_1:
		processAnd = 0b11111111; //  nothing
		processOr = 0b00000100;	// INTCN
		break;

	case URTCLIB_SQWG_1H:
		processAnd = 0b11100011; //  RS1, RS0, INTCN
		processOr = 0b00000000;	// nothing
		break;

	case URTCLIB_SQWG_1024H:
		processAnd = 0b11101011; //  RS1, INTCN
		processOr = 0b00001000;	// RS0
		break;

	case URTCLIB_SQWG_4096H:
		processAnd = 0b11110011; //  RS0, INTCN
		processOr = 0b00010000;	// RS1
		break;

	case URTCLIB_SQWG_8192H:
		processAnd = 0b11111011; //  INTCN
		processOr = 0b00011000;	//  RS1, RS0
		break;

	} // mode switch

	if (processAnd || processOr)
	{ // Any bit change?
		Wire.beginTransmission(_rtc_address);

		Wire.write(0x0E);

		Wire.endTransmission();

		Wire.requestFrom(_rtc_address, 1);
		status = Wire.read();
		status = (status & processAnd) | processOr;
		Wire.beginTransmission(_rtc_address);

		Wire.write(0x0E);

		Wire.write(status);

		Wire.endTransmission();

		_sqwg_mode = mode;
		if (mode == URTCLIB_SQWG_OFF_1 || mode == URTCLIB_SQWG_OFF_0)
		{
			_a1_mode = URTCLIB_ALARM_TYPE_1_NONE;
			_a2_mode = URTCLIB_ALARM_TYPE_2_NONE;
		}
		return true;
	}

	return false;
}

/**
 * \brief Gets current SQWG mode
 *
 * @return SQWG mode:
 *	 - #URTCLIB_SQWG_OFF_0
 *	 - #URTCLIB_SQWG_OFF_1
 *	 - #URTCLIB_SQWG_1H
 *	 - #URTCLIB_SQWG_1024H
 *	 - #URTCLIB_SQWG_4096H
 *	 - #URTCLIB_SQWG_8192H
 *	 - #URTCLIB_SQWG_32768H
 */
uint8_t uRTCLib::sqwgMode()
{
	return _sqwg_mode;
}

/*** RAM functionality (Only DS1307. Addresses 08h to 3Fh so we offset 08h positions and limit to 38h as maximum address ***/

/**
 * \brief Reads a byte from RTC RAM
 *
 * @param address RAM Address
 *
 * @return content of that position. If any error it will return always 0xFF;
 */
byte uRTCLib::ramRead(const uint8_t address)
{
	uint8_t offset = 0xff;

	if (offset != 0xff)
	{
		Wire.beginTransmission(_rtc_address);

		Wire.write(address + offset);

		Wire.endTransmission();

		Wire.requestFrom(_rtc_address, 1);

		return Wire.read();
	}
	return 0xff;
}

/**
 * \brief Writes a byte to RTC RAM
 *
 * @param address RAM Address
 * @param data Content to write on that position
 *
 * @return true if correct
 */
bool uRTCLib::ramWrite(const uint8_t address, byte data)
{
	uint8_t offset = 0xff;

	if (offset != 0xff)
	{
		Wire.beginTransmission(_rtc_address);

		Wire.write(address + offset);

		Wire.write(data);

		Wire.endTransmission();

		return true;
	}
	return false;
}

/*** EEPROM functionality has been moved to separate library: https://github.com/Naguissa/uEEPROMLib ***/
