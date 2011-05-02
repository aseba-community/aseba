/*
	Aseba - an event-based framework for distributed robot control
	Copyright (C) 2007--2010:
		Stephane Magnenat <stephane at magnenat dot net>
		(http://stephane.magnenat.net)
		and other contributors, see authors.txt for details
	
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
#include <time.h>
#include <errno.h>
#include <iomanip>
#include <ostream>
#include <sstream>
#include <cassert>
#include <limits>
#include <stdint.h>
#include "utils.h"

namespace Aseba
{
	/** \addtogroup utils */
	/*@{*/
	
	UnifiedTime::UnifiedTime()
	{
		#ifndef WIN32
		struct timeval tv;
		gettimeofday(&tv, NULL);
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
		nanosleep(&ts, 0);
		#else // WIN32
		assert(value < (1 << 32));
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
	
	std::string WStringToUTF8(const std::wstring& s)
	{
		//std::wcerr << "converting to utf " << s << std::endl;
		mbstate_t state;
		memset (&state, '\0', sizeof (state));
		const wchar_t* src = s.c_str();
		const size_t count = wcsrtombs(0, &src, 0, &state);
		//if (count == std::numeric_limits<size_t>::max())
		//	return "Invalid input string";
		std::string os;
		os.resize(count);
		memset (&state, '\0', sizeof (state));
		src = s.c_str();
		wcsrtombs(&os[0], &src, count, &state);
		//std::cerr << "result " << os << std::endl;
		return os;
	}
	
	#define BOM8A 0xEF
	#define BOM8B 0xBB
	#define BOM8C 0xBF
	
	// code from http://www.cplusplus.com/forum/general/7142/
	// TODO: if it works, keep it and add (c)
	
	std::wstring UTF8ToWString(const std::string& s)
	{
		std::wstring res;
		const char * string(s.c_str());
		
		long b=0, c=0;
		if ((uint8_t)string[0]==BOM8A && (uint8_t)string[1]==BOM8B && (uint8_t)string[2]==BOM8C)
			string+=3;
		for (const char *a=string;*a;a++)
			if (((uint8_t)*a)<128 || (*a&192)==192)
				c++;
		res.resize(c);
		for (uint8_t *a=(uint8_t*)string;*a;a++)
		{
			if (!(*a&128))
				//Byte represents an ASCII character. Direct copy will do.
				res[b]=*a;
			else if ((*a&192)==128)
				//Byte is the middle of an encoded character. Ignore.
				continue;
			else if ((*a&224)==192)
				//Byte represents the start of an encoded character in the range
				//U+0080 to U+07FF
				res[b]=((*a&31)<<6)|a[1]&63;
			else if ((*a&240)==224)
				//Byte represents the start of an encoded character in the range
				//U+07FF to U+FFFF
				res[b]=((*a&15)<<12)|((a[1]&63)<<6)|a[2]&63;
			else if ((*a&248)==240){
				//Byte represents the start of an encoded character beyond the
				//U+FFFF limit of 16-bit integers
				res[b]='?';
			}
			b++;
		}
		return res;
	}
	
	/*@}*/
}
