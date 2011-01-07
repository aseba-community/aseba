/*
	Small tool to change the event filter of the e-puck's bluetooth module.
	Copyright (C) 2008 -- 2009:
		Michael Bonani <michael dot bonani at epfl dot ch>
		Stephane Magnenat <stephane at magnenat dot net>
		Mobots group, Laboratory of Robotics Systems, EPFL, Lausanne
	
	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU Lesser General Public License as published
	by the Free Software Foundation, version 3 of the License.
	
	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Lesser General Public License for more details.
	
	You should have received a copy of the GNU Lesser General Public License
	along with this program. If not, see <http://www.gnu.org/licenses/>.
	
	Usage: on e-puck reset:
		- if selector == 0, then fully transparent mode (no event), all leds on
		- if selector != 0, then normal mode (connection and disconnection events), led 1 on
	The bluetooth module retains this settings across power off/on.
*/


#include <motor_led/e_epuck_ports.h>
#include <stdio.h>
#include <uart/e_uart_char.h>
#include <bluetooth/e_bluetooth.h>
#include <motor_led/e_init_port.h>
#include <motor_led/e_led.h>
#include <string.h>

int getselector()
{
	return SELECTOR0 + 2*SELECTOR1 + 4*SELECTOR2 + 8*SELECTOR3;
}

void delay()
{
	int wait;
	for (wait=0;wait<30000;wait++);
}

int main(void)
{
	int i;
	char event;
	
	e_init_port();   //Configure port pins
	e_init_uart1();   //Initialize UART to 115200Kbaud
	
	if(RCONbits.POR)	//Reset if Power on (some problem for few robots)
	{
		RCONbits.POR=0;
		__asm__ volatile ("reset");
	}
	
	if(getselector()==0)
	{
		e_bt_set_event_filter(3);
		for (i=0;i<8;i++)
		{
			e_set_led(i,1);
			delay();
			e_set_led(i-1,0);
			delay();
		}
		e_set_led(7,0);
		delay();
		e_set_led(8,1);
	}
	else
	{
		e_bt_set_event_filter(1);
		for (i=0;i<8;i++)
		{
			e_set_led(i,1);
			delay();
			e_set_led(i-1,0);
			delay();
		}
		e_set_led(7,0);
		delay();
		event = e_bt_get_event_filter();
		e_set_led(event,1);
	}
	
	while(1)
		;
}
