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

#include <p30f6014a.h>

#include <e_epuck_ports.h>
#include <e_uart_char.h>

int main()
{
	e_init_port();
	e_init_uart1();
	
	int i;
	for (i = 0; i < 14; i++)
	{
		char c;
		while (!e_ischar_uart1());
		e_getchar_uart1(&c);
	}
	
	while (1)
	{
		char c;
		while (!e_ischar_uart1());
		e_getchar_uart1(&c);
		e_send_uart1_char(&c, 1);
		while (e_uart1_sending());
	}
	return 0;
}