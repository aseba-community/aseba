/*
	Aseba - an event-based middleware for distributed robot control
	Copyright (C) 2006 - 2007:
		Stephane Magnenat <stephane at magnenat dot net>
		(http://stephane.magnenat.net)
		Valentin Longchamp <valentin dot longchamp at epfl dot ch>
		Laboratory of Robotics Systems, EPFL, Lausanne
	
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

#ifndef ASEBA_IMXCAN
#define ASEBA_IMXCAN

#include "../can/can.h"
#include <iostream>

namespace Aseba
{
	/** \addtogroup switch */
	/*@{*/
	
	//! The header of a CAN frame as seen by the iMX CAN Linux driver
	struct imxCANFrameHeader
	{
		unsigned id:11;
		unsigned srs:1;
		unsigned ide:1;
		unsigned eid:18;
		unsigned rtr:1;
		unsigned rb1:1;
		unsigned rb0:1;
		unsigned dlc:4;
		
		//! Default constructor, fill the header with zeros
		imxCANFrameHeader();
		//! Dump the content of this CAN header to a stream
		void dump(std::ostream &stream);
	};
	
	//! The CAN frame as seen by the iMX CAN Linux driver
	struct imxCANFrame
	{
		imxCANFrameHeader header;
		uint8 data[8];
		
		//! Default constructor, do nothing (the header constructor is called)
		imxCANFrame() { }
		//! Copy constructor from an aseba net CAN frame
		imxCANFrame(const CanFrame &in);
		//! Conversion operator to an aseba net CAN frame
		operator CanFrame() const;
		//! Dump the content of this CAN frame to a stream
		void dump(std::ostream &stream);
	};
	
	void sendFrame (const CanFrame *frame);
	
	int isFrameRoom();
	
	void receivedPacketDropped();
	
	void sentPacketDropped();

	/*@}*/
};

#endif //IMXCAN

