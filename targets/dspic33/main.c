/*
        Aseba - an event-based framework for distributed robot control
        Copyright (C) 2006 - 2009:
                Philippe Rétornaz <philippe.retornaz at epfl dot ch>
                and other contributors, see authors.txt for details
                Mobots group, Laboratory of Robotics Systems, EPFL, Lausanne

        This program is free software: you can redistribute it and/or modify
        it under the terms of the GNU General Public License as published by
        the Free Software Foundation, either version 3 of the License, or
        any other version as decided by the two original authors
        Stephane Magnenat and Valentin Longchamp.

        This program is distributed in the hope that it will be useful,
        but WITHOUT ANY WARRANTY; without even the implied warranty of
        MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
        GNU General Public License for more details.

        You should have received a copy of the GNU General Public License
        along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/




#include <p33fxxxx.h>

_FWDT(FWDTEN_OFF);              // Watchdog Timer disabled
_FOSCSEL(FNOSC_FRCPLL); // override oscillator configuration bits
_FOSC(POSCMD_NONE & OSCIOFNC_ON);
// ICD communicates on PGC1/EMUC1 and disable JTAG
_FICD(ICS_PGD1 & JTAGEN_OFF);

#include <gpio/gpio.h>
#include <clock/clock.h>
#include <error/error.h>
#include <types/types.h>
#include <vm/natives.h>
#include <vm/vm.h>
#include <common/consts.h>

#include <skel.h>



void update_aseba_variables_read(void) {

}


void update_aseba_variables_write(void) {
	
}


	

int main(void) {
	clock_init_internal_rc_40();
	
	init_aseba_and_can();
	if( ! load_settings_from_flash()) {
                // Todo
    }
	
	run_aseba_main_loop();
}

