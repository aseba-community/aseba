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

#include "imxcan.h"
#include "switch.h"
#include "../utils/utils.h"
#include <stdlib.h>
#include <poll.h>
#include <iostream>
#include <sys/ioctl.h>

namespace Aseba
{
	using namespace std;
	using namespace Streams;
	
	/** \addtogroup switch */
	/*@{*/
	/*
	// TODO: temporary disabled until new translator CAN interface
	// we should replace this by a translatorcan file
	
	imxCANFrameHeader::imxCANFrameHeader()
	{
		memset(this, 0, sizeof(*this));
	}
	
	void imxCANFrameHeader::dump(std::ostream &stream)
	{
		stream << "* Header: ";
		stream << "  id:  " << std::hex << id;
		stream << "  srs: " << std::hex << srs;
		stream << "  ide: " << std::hex << ide;
		stream << "  eid: " << std::hex << eid;
		stream << "  rtr: " << std::hex << rtr;
		stream << "  rb1: " << std::hex << rb1;
		stream << "  rb0: " << std::hex << rb0;
		stream << "  dlc: " << std::hex << dlc;
		stream << "\n";
	}
	
	imxCANFrame::imxCANFrame(const CanFrame &in)
	{
		header.id = ~(in.id);
		header.dlc = in.len;
		::memcpy(data, in.data, in.len);
	}
	
	imxCANFrame::operator CanFrame() const
	{
		CanFrame out;
		out.id = ~(header.id);
		out.len = header.dlc;
		::memcpy(out.data, data, header.dlc);
		out.used = 0;
		return out;
	}
	
	void imxCANFrame::dump(std::ostream &stream)
	{
		stream << "CAN Frame:\n";
		header.dump(stream);
		stream << "* Data: ";
		for (unsigned i = 0; i < header.dlc; i++)
			stream << std::hex << (unsigned)data[i] << " ";
		stream << "\n";
	}
	*/
	//! File descriptor corresponding to the physical CAN device
	Stream* canStream = 0;
	
	//! Send a CAN frame on the CAN file descriptor.
	void sendFrame(const CanFrame *frame)
	{
		/*imxCANFrame sendFrame = *frame;
		//sendFrame.dump(std::cout);
		Aseba::write(canfd, &sendFrame, sizeof(imxCANFrame));*/
		::AsebaCanFrameSent();
	}
	
	//! Return if there is free room for more frames in the physical CAN device.
	int isFrameRoom()
	{
		/*pollfd CANpoll = {canfd, POLLOUT, 0};
		
		int ret = poll(&CANpoll, 1, 100);
		if (ret <= 0)
			return 0;
		else
			return (CANpoll.revents & POLLOUT);*/
		return 1;
	}
	
	//! Called when received Packet got dropped.
	void receivedPacketDropped()
	{
		std::cerr << "Received Packet got dropped" << std::endl;
	}
	
	//! Called when sent Packet got dropped.
	void sentPacketDropped()
	{
		std::cerr << "Sent Packet got dropped" << std::endl;
	}
	
	/*@}*/
};
