/*
	Aseba - an event-based framework for distributed robot control
	Copyright (C) 2007--2016:
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

#ifndef _HARDWARE_H_
#define _HARDWARE_H_

#include <gpio/gpio.h>

#define U_UART UART_1
#define CAN_SELECT_MODE GPIO_MAKE_ID(GPIO_PORTD,2)
#define UART_CTS GPIO_MAKE_ID(GPIO_PORTD,9)
#define UART_RTS GPIO_MAKE_ID(GPIO_PORTF,6)
#define LED0 GPIO_MAKE_ID(GPIO_PORTD, 0)
#define LED1 GPIO_MAKE_ID(GPIO_PORTD, 1)

#endif

