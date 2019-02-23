/*
 Clock.h - Base class for timming
 Copyright (c) 2016 Oscar Blanco.  All right reserved.
 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.
 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.
 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <Arduino.h>

const unsigned long NTP_TIMEOUT = 3000; // 3s

class Clock
{
public:
  static unsigned long getUnixTime();
  static unsigned long getDayInSeconds();
  static String getHumanTime(unsigned long unixTime);
  static String getHumanDate(unsigned long unixTime);
  static String getHumanDateTime(unsigned long unixTime);
  static bool updateTime();

protected:
  static unsigned long _unixTime;
  static unsigned long _lastUpdate;
};