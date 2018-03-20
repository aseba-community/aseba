/*
	Aseba - an event-based framework for distributed robot control
	Created by Stéphane Magnenat <stephane at magnenat dot net> (http://stephane.magnenat.net)
	with contributions from the community.
	Copyright (C) 2007--2018 the authors, see authors.txt for details.

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU Lesser General Public License as published
	by the Free Software Foundation, version 3 of the License.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public License
	along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include <cstdlib>
#include <cstring>
#ifndef WIN32
	#include <sys/time.h>
#else // WIN32
	#include <sys/types.h>
	#include <windows.h>
	#define atoll _atoi64
#endif // WIN32
#include <ctime>
#include <cerrno>
#include <iomanip>
#include <ostream>
#include <sstream>
#include <cassert>
#include <limits>
#include <vector>
#include <stdexcept>
#include <cstdint>
#include <locale>
#include <codecvt>
#include "utils.h"


// workaround for broken libstdc++ on Android
// see https://github.com/android-ndk/ndk/issues/82
#ifdef __ANDROID__
namespace std
{
    size_t log2(size_t v) { return log2(v); }
}
#endif // __ANDROID__

namespace Aseba
{
	/** \addtogroup utils */
	/*@{*/

	UnifiedTime::UnifiedTime()
	{
		#ifndef WIN32
		struct timeval tv;
		gettimeofday(&tv, nullptr);
		value = (Value(tv.tv_sec) * 1000) + Value(tv.tv_usec) / 1000;
		#else // WIN32
		FILETIME        ft;
		LARGE_INTEGER   li;
		__int64         t;
		GetSystemTimeAsFileTime(&ft);
		li.LowPart = ft.dwLowDateTime;
		li.HighPart = ft.dwHighDateTime;
		t = li.QuadPart;                        // In 100-nanosecond intervals
		value = t / 10000l;                      // In milliseconds
		#endif // WIN32
	}

	UnifiedTime::UnifiedTime(Value ms) :
		value(ms)
	{
	}

	UnifiedTime::UnifiedTime(Value seconds, Value milliseconds) :
		value(seconds * 1000 + milliseconds)
	{
	}

	void UnifiedTime::sleep() const
	{
		#ifndef WIN32
		struct timespec ts;
		ts.tv_sec = (value / 1000);
		ts.tv_nsec = ((value % 1000) * 1000000);
		nanosleep(&ts, nullptr);
		#else // WIN32
		assert(value <= 4294967295);
		Sleep((DWORD)value);
		#endif // WIN32
	}

	std::string UnifiedTime::toHumanReadableStringFromEpoch() const
	{
		std::ostringstream oss;
		Value seconds(value / 1000);
		Value milliseconds(value % 1000);
		time_t t(seconds);
		char *timeString = ctime(&t);
		timeString[strlen(timeString) - 1] = 0;
		oss << "[";
		oss << timeString << " ";
		oss << std::setfill('0') << std::setw(3) << milliseconds;
		oss << "]";
		return oss.str();
	}

	std::string UnifiedTime::toRawTimeString() const
	{
		std::ostringstream oss;
		Value seconds(value / 1000);
		Value milliseconds(value % 1000);
		oss << std::dec << seconds << "." << std::setfill('0') << std::setw(3) << milliseconds;
		return oss.str();
	}

	UnifiedTime UnifiedTime::fromRawTimeString(const std::string& rawTimeString)
	{
		size_t dotPos(rawTimeString.find('.'));
		assert(dotPos != std::string::npos);
		return UnifiedTime(atoll(rawTimeString.substr(0, dotPos).c_str()), atoll(rawTimeString.substr(dotPos + 1, std::string::npos).c_str()));
	}


	void dumpTime(std::ostream &stream, bool raw)
	{
		if (raw)
			stream << UnifiedTime().toRawTimeString();
		else
			stream << UnifiedTime().toHumanReadableStringFromEpoch();
		stream << " ";
	}

	SoftTimer::SoftTimer(Callback callback, double period):
		callback(callback),
		period(period),
		left(period)
	{
		if (period < 0)
			throw std::domain_error("Period must be greater or equal to zero");
	}

	void SoftTimer::step(double dt)
	{
		if (period == 0)
			return;

		assert(dt >= 0);
		left -= dt;
		while (left < 0)
		{
			callback();
			left += period;
		}
	}

	void SoftTimer::setPeriod(double period)
	{
		if (period < 0)
			throw std::domain_error("Period must be greater or equal to zero");

		this->period = period;
		left = period;
	}

	std::string WStringToUTF8(const std::wstring& s)
	{
		std::string os;
		for (wchar_t c : s)
		{
			if (c < 0x80)
			{
				os += static_cast<uint8_t>(c);
			}
			else if (c < 0x800)
			{
				os += static_cast<uint8_t>(((c>>6)&0x1F)|0xC0);
				os += static_cast<uint8_t>((c&0x3F)|0x80);
			}
			else if (c < 0xd800)
			{
				os += static_cast<uint8_t>(((c>>12)&0x0F)|0xE0);
				os += static_cast<uint8_t>(((c>>6)&0x3F)|0x80);
				os += static_cast<uint8_t>((c&0x3F)|0x80);
			}
			else
			{
				os += '?';
			}
		}
		// TODO: add >UTF16 support
		return os;
	}

    /* //FIXME:
	* Lacking proper unicode facilities,
	* we rely on the locale to do the caracter categorization for us.
	* But because we want the categorization to obey the unicode specification while the
	* current locale may not be based on unicode, we need to force a locale.
	*
	* Note that this approach does not work reliably
	*  - There is no reason for us to use wchat_t outside of win32 api calls boundaries
	*  - A wchar_t may not encode a unicode character at all
	*  - Even if it does, we may be dealing with a surrogate pair which would be encoded as 2 wchar_t
	*  - Or a multiple-codepoint grapheme
	*/
    bool is_utf8_alpha_num(wchar_t c) {
    #ifdef _WIN32
        return IsCharAlphaNumericW(c);
    #else
        static std::locale utf8Locale("en_US.UTF-8");
        return std::isalnum(c, utf8Locale);
    #endif
    }

	/*

	This code is heavily inspired by http://www.cplusplus.com/forum/general/7142/
	released under the following license:

	* Copyright (c) 2009, Helios (helios.vmg@gmail.com)
	* All rights reserved.
	*
	* Redistribution and use in source and binary forms, with or without
	* modification, are permitted provided that the following conditions are met:
	*     * Redistributions of source code must retain the above copyright notice,
	*       this list of conditions and the following disclaimer.
	*     * Redistributions in binary form must reproduce the above copyright
	*       notice, this list of conditions and the following disclaimer in the
	*       documentation and/or other materials provided with the distribution.
	*
	* THIS SOFTWARE IS PROVIDED BY HELIOS "AS IS" AND ANY EXPRESS OR IMPLIED
	* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
	* MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
	* EVENT SHALL HELIOS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
	* EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
	* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
	* OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
	* WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
	* OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
	* OF THE POSSIBILITY OF SUCH DAMAGE.
	*/

	std::wstring UTF8ToWString(const std::string& s)
	{
		std::wstring res;
		size_t left(s.size());
		for (const char & c : s)
		{
			const char *a = &c;
			if (!(*a&128))
			{
				//Byte represents an ASCII character. Direct copy will do.
				res += *a;
			}
			else if ((*a&192)==128)
			{
				//Byte is the middle of an encoded character. Ignore.
				continue;
			}
			else if ((*a&224)==192)
			{
				//Byte represents the start of an encoded character in the range
				//U+0080 to U+07FF
				if (left < 2)
					throw std::runtime_error("Invalid UTF8 string");
				res += ((*a&31)<<6)|(a[1]&63);
			}
			else if ((*a&240)==224)
			{
				//Byte represents the start of an encoded character in the range
				//U+07FF to U+FFFF
				if (left < 3)
					throw std::runtime_error("Invalid UTF8 string");
				res += ((*a&15)<<12)|((a[1]&63)<<6)|(a[2]&63);
			}
			else if ((*a&248)==240)
			{
				//Byte represents the start of an encoded character beyond the
				//U+FFFF limit of 16-bit integers
				res += '?';
			}
			--left;
		}
		// TODO: add >UTF16 support
		return res;
	}

	/*
	 * Code from http://www.nongnu.org/avr-libc/
	 * Released under http://www.nongnu.org/avr-libc/LICENSE.txt
	 * which is GPL- and DFSG- compatible
	 */
	static uint16_t crc_xmodem_update (uint16_t crc, uint8_t data)
	{
		int i;

		crc = crc ^ ((uint16_t)data << 8);
		for (i=0; i<8; i++)
		{
			if (crc & 0x8000)
				crc = (crc << 1) ^ 0x1021;
			else
				crc <<= 1;
		}

		return crc;
	}

	static uint16_t crc_xmodem_update (uint16_t crc, const uint8_t* data, size_t len)
	{
		for (size_t i = 0; i < len; ++i)
			crc = crc_xmodem_update(crc, data[i]);
		return crc;
	}

	uint16_t crcXModem(const uint16_t oldCrc, const std::wstring& s)
	{
		std::string utf8s(WStringToUTF8(s));
		size_t l = utf8s.size();
		if (l & 0x1)
			++l;
		return crc_xmodem_update(oldCrc, reinterpret_cast<const uint8_t*>(utf8s.c_str()), l);
	}

	uint16_t crcXModem(const uint16_t oldCrc, const uint16_t v)
	{
		return crc_xmodem_update(oldCrc, reinterpret_cast<const uint8_t*>(&v), 2);
	}

	template<typename T>
	std::vector<T> split(const T& s, const T& delim)
	{
		std::vector<T> result;
		size_t delimPos(0);
		size_t nextPos(0);
		while ((nextPos=s.find_first_of(delim, delimPos)) != T::npos)
		{
			result.push_back(s.substr(delimPos, nextPos-delimPos));
			delimPos = s.find_first_not_of(delim, nextPos);
		}
		if (delimPos != T::npos && delimPos < s.size())
			result.push_back(s.substr(delimPos));
		return result;
	}

	template<>
	std::vector<std::string> split(const std::string& s)
	{
		return split<std::string>(s, "\t ");
	}

	template<>
	std::vector<std::wstring> split(const std::wstring& s)
	{
		return split<std::wstring>(s, L"\t ");
	}

	template<typename T>
	T join(typename std::vector<T>::const_iterator first, typename std::vector<T>::const_iterator last, const T& delim)
	{
		T result;
		while (first != last)
		{
			result += *first;
			if (first + 1 != last)
				result += delim;
			++first;
		}
		return result;
	}

	template<typename C>
	typename C::value_type join(const C& values, const typename C::value_type& delim)
	{
		return join<typename C::value_type>(values.begin(), values.end(), delim);
	}

	template std::string join(const std::vector<std::string>&, const std::string& delim);
	template std::wstring join(const std::vector<std::wstring>&, const std::wstring& delim);

	/*@}*/
}
