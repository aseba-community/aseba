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

#include <gpio/gpio.h>
#include <error/error.h>
#include <types/types.h>
#include <clock/clock.h>

// One dot is 0.5 sec
// One dash is 3 dots
// a space between two letter is three dots
// a space between two words is seven dots

static void wait_dot(void) {
	int i;
	for(i = 0; i < 50; i++)
		clock_delay_us(10000);
}	
	
	
static void wait_dash(void) {
	int i;
	for(i = 0; i < 3; i++)
		wait_dot();
}

static void wait_morse_letter(void) {
	int i;
	for(i = 0; i < 3; i++)
		wait_dot();
}

static void wait_morse_word(void) {
	int i;
	for(i = 0; i < 7; i++)
		wait_dot();
}

static void morse_dash(gpio led, int on, int off) {
	gpio_write(led, on);
	wait_dash();
	gpio_write(led, off);
	wait_dot();
}

static void morse_dot(gpio led, int on, int off) {
	gpio_write(led, on);
	wait_dot();
	gpio_write(led, off);
	wait_dot();
}

static void morse_number(unsigned int disp, gpio led, int on, int off) {
	switch(disp) {
		case 0:
			morse_dash(led, on, off);
			morse_dash(led, on, off);
			morse_dash(led, on, off);
			morse_dash(led, on, off);
			morse_dash(led, on, off);
			break;
		case 1:
			morse_dot(led, on, off);
			morse_dash(led, on, off);
			morse_dash(led, on, off);
			morse_dash(led, on, off);
			morse_dash(led, on, off);
			break;
		case 2:
			morse_dot(led, on, off);
			morse_dot(led, on, off);
			morse_dash(led, on, off);
			morse_dash(led, on, off);
			morse_dash(led, on, off);
			break;
		case 3:
			morse_dot(led, on, off);
			morse_dot(led, on, off);
			morse_dot(led, on, off);
			morse_dash(led, on, off);
			morse_dash(led, on, off);
			break;
		case 4:
			morse_dot(led, on, off);
			morse_dot(led, on, off);
			morse_dot(led, on, off);
			morse_dot(led, on, off);
			morse_dash(led, on, off);
			break;
		case 5:
			morse_dot(led, on, off);
			morse_dot(led, on, off);
			morse_dot(led, on, off);
			morse_dot(led, on, off);
			morse_dot(led, on, off);
			break;
		case 6:
			morse_dash(led, on, off);
			morse_dot(led, on, off);
			morse_dot(led, on, off);
			morse_dot(led, on, off);
			morse_dot(led, on, off);
			break;
		case 7:
			morse_dash(led, on, off);
			morse_dash(led, on, off);
			morse_dot(led, on, off);
			morse_dot(led, on, off);
			morse_dot(led, on, off);
			break;
		case 8:
			morse_dash(led, on, off);
			morse_dash(led, on, off);
			morse_dash(led, on, off);
			morse_dot(led, on, off);
			morse_dot(led, on, off);
			break;
		case 9:
			morse_dash(led, on, off);
			morse_dash(led, on, off);
			morse_dash(led, on, off);
			morse_dash(led, on, off);
			morse_dot(led, on, off);
			break;
		case 0xa:
			morse_dot(led, on, off);
			morse_dash(led, on, off);
			break;
		case 0xb:
			morse_dash(led, on, off);
			morse_dot(led, on, off);
			morse_dot(led, on, off);
			morse_dot(led, on, off);
			break;
		case 0xc:
			morse_dash(led, on, off);
			morse_dot(led, on, off);
			morse_dash(led, on, off);
			morse_dot(led, on, off);
			break;
		case 0xd:
			morse_dash(led, on, off);
			morse_dot(led, on, off);
			morse_dot(led, on, off);
			break;
		case 0xe:
			morse_dot(led, on, off);
			break;
		case 0xf:
			morse_dot(led, on, off);
			morse_dot(led, on, off);
			morse_dash(led, on, off);
			morse_dot(led, on, off);
			break;
	}
}

void output_error(unsigned int value, gpio led, int on, int off) {
	int shift;
	bool hasSent = false;
	for (shift = 12; shift >= 0; shift -= 4)
	{
		unsigned disp = ((value & (0xF << shift)) >> shift);

		if ((disp != 0) || (hasSent) || (shift == 0))
		{
			hasSent = true;
			morse_number(disp, led, on, off);
			wait_morse_letter();
		}
	}
	wait_morse_word();
}

