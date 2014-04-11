/*
        Aseba - an event-based framework for distributed robot control
        Copyright (C) 2012:
                Philippe Retornaz <philippe.retornaz@epfl.ch>
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

#include <errno.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <net/if.h>
#include <poll.h>

#include <sys/socket.h>
#include <linux/can.h>
#include <linux/can/raw.h>

#include <unistd.h>

#include <string>
#include <iostream>

#include <cstring>
#include <cstdio>

#include <dashel/dashel.h>
#include <dashel/dashel-posix.h>

#include "../../common/consts.h"

#ifndef AF_CAN
#define AF_CAN		29
#endif
#ifndef PF_CAN
#define PF_CAN 		AF_CAN
#endif

#ifndef SO_RXQ_OVFL
#define SO_RXQ_OVFL	40
#endif

using namespace Dashel;
using namespace std;


/* CanStream implementation
 * This is the protocol used in the MarxBot and Handbot robots.
 * It needs a SocketCAN interface (Linux native CAN interfaces).
 *
 * This stream (named "can") accept only one parameter:
 * 	if : The CAN interface to use
 *
 * The interface must already be configured & upped by your distribution script.
 *
 * Usage example:
 * 	"can:if=can0"
 *
 */

class CanStream: public SelectableStream
{
#define TYPE_SMALL_PACKET 0x3
#define TYPE_PACKET_NORMAL 0x0
#define TYPE_PACKET_START 0x1
#define TYPE_PACKET_STOP 0x2

#define CANID_TO_TYPE(canid) ((canid) >> 8)
#define CANID_TO_ID(canid) ((int) ((canid) & 0xFF))
#define TO_CANID(type,id) (((type) << 8) | (id))

	protected:
		unsigned char tx_buffer[ASEBA_MAX_OUTER_PACKET_SIZE];
		int tx_len;
		unsigned char rx_buffer[ASEBA_MAX_OUTER_PACKET_SIZE];
		unsigned int rx_len;
		unsigned int rx_p;

		struct iovec iov;
		struct msghdr msg;
		struct sockaddr_can addr;
		char ctrlmsg[CMSG_SPACE(sizeof(struct timeval)) + CMSG_SPACE(sizeof(__u32))];
		struct can_frame rframe;

#define RX_CAN_SIZE 1000
		struct {
			struct can_frame f;
			int used;
		} rx_fifo[RX_CAN_SIZE];
		int rx_insert;
		int rx_consume;
	public:
		CanStream(const string &targetName) :
			Stream("can"),
			SelectableStream("can")
		{
			struct ifreq ifr;

			target.add("can:if=can0");
			target.add(targetName.c_str());
			string ifName;
			if(target.isSet("if"))
			{
				target.addParam("if", NULL, true);
				ifName = target.get("if");
			}
			fd = socket(PF_CAN, SOCK_RAW, CAN_RAW);
			if(fd < 0)
				throw DashelException(DashelException::ConnectionFailed, 0, "Socket creation failed", this);
			
			addr.can_family = AF_CAN;
			if(strlen(ifName.c_str()) >= IFNAMSIZ)
				throw DashelException(DashelException::ConnectionFailed, 0, "Interface name too long", this);
			
			strcpy(ifr.ifr_name, ifName.c_str());
			if(ioctl(fd, SIOCGIFINDEX, &ifr) < 0)
				throw DashelException(DashelException::ConnectionFailed, 0, "Unable to get interface", this);
			
                        // Try to have 20Mb RX buffer
			int options = 20*1024*1024;
			if (setsockopt(fd, SOL_SOCKET, SO_RCVBUFFORCE, &options, sizeof(options)))
				perror("socketcan: cannot set Rx buffer");

                        // Enable monitoring of dropped packets
			options = 1;
			if (setsockopt(fd, SOL_SOCKET, SO_RXQ_OVFL, &options, sizeof(options)))
				perror("socketcan: cannot set monitoring for dropped packets");

			addr.can_ifindex = ifr.ifr_ifindex;
			if(bind(fd, (struct sockaddr *) &addr, sizeof(addr)) < 0)
				throw DashelException(DashelException::ConnectionFailed, 0, "Unable to bind", this);

			rx_insert = rx_consume = 0;
			tx_len = 0;
			rx_len = 0;
			rx_p = 0;
			memset(rx_fifo, 0, sizeof(rx_fifo));
			iov.iov_base = &rframe;
			msg.msg_iov = &iov;
			msg.msg_name = &addr;
			msg.msg_namelen = sizeof(addr);
			msg.msg_iovlen = 1;
			msg.msg_control = ctrlmsg;
			msg.msg_controllen = sizeof(ctrlmsg);
			msg.msg_flags = 0;
		}
	private:
		int is_packet_tx(void)
		{
			int packet_len;
			if(tx_len < 6)
				return 0;
			packet_len = tx_buffer[0] | (tx_buffer[1] << 8); // Little endian
			if(tx_len >= packet_len + 6)
				return 1;
			return 0;
		}

		void can_write_frame(struct can_frame * f)
		{
			ssize_t ret;

			do {
				ret = ::write(fd, f, sizeof(*f));

				if(ret == -1 && errno == ENOBUFS)
				{
					struct pollfd pf;
					pf.fd = fd;
					pf.events = POLLOUT | POLLHUP;
					pf.revents = 0;
					if (poll(&pf, 1, 0) == -1)
						perror("socketcan: can_write_frame: error while polling the socket");
					continue;
				}

				if(ret != sizeof(*f))
					throw DashelException(DashelException::IOError, 0, "Write error", this);
			} while(0);
		}

		void send_aseba_packet()
		{
			struct can_frame frame;
			unsigned int packet_len = tx_buffer[0] | (tx_buffer[1] << 8); // Little endian;
			unsigned int nodeId = tx_buffer[2] | (tx_buffer[3] << 8);
			unsigned int msgId = tx_buffer[4] | (tx_buffer[5] << 8);
			
			if(packet_len  <= 6) 
			{
				// Small packet
				frame.can_id = TO_CANID(TYPE_SMALL_PACKET, nodeId);
				frame.can_dlc = packet_len + 2;
				frame.data[0] = msgId;
				frame.data[1] = msgId >> 8;
				memcpy(&frame.data[2], &tx_buffer[6], packet_len);
				can_write_frame(&frame);
			} 
			else 
			{
				unsigned char * p = &tx_buffer[6];
				frame.can_id = TO_CANID(TYPE_PACKET_START, nodeId);
				frame.can_dlc = 8;
				frame.data[0] = msgId;
				frame.data[1] = msgId >> 8;
				memcpy(&frame.data[2], p, 6);
				p += 6;
				packet_len -= 6;
				can_write_frame(&frame);

				while(packet_len > 8)
				{
					frame.can_id = TO_CANID(TYPE_PACKET_NORMAL, nodeId);
					frame.can_dlc = 8;
					memcpy(frame.data, p, 8);
					p+=8;
					packet_len -= 8;
					can_write_frame(&frame);
				}
				frame.can_id = TO_CANID(TYPE_PACKET_STOP, nodeId);
				frame.can_dlc = packet_len;
				memcpy(frame.data, p, packet_len);
				can_write_frame(&frame);
			}
			tx_len = 0;
		}
	public:
		virtual void write(const void *data, const size_t size)
		{
			size_t s = size;
			const unsigned char * d = (const unsigned char * ) data;
			while(s--)
			{
				tx_buffer[tx_len++] = *d++;
				if(is_packet_tx())
					send_aseba_packet();
			}
		}

		virtual void flush() 
		{
		}
	private:
		void pack_fifo()
		{
			while(!rx_fifo[rx_consume].used && rx_consume != rx_insert)
			{
				if(++rx_consume == RX_CAN_SIZE)
					rx_consume = 0;
			}
		}

		// 1 if defragment was sucessfull.
		// 0 if not,
		// -1 if needs to recall (one packet was dropped because not full)
		int defragment(void)
		{
			int i;
			int stopId;
			int stopPos = -1;
			int ignore = 0;
			for(i = rx_consume; i != rx_insert; ) 
			{
				if(rx_fifo[i].used)
				{
					if(CANID_TO_TYPE(rx_fifo[i].f.can_id) == TYPE_SMALL_PACKET)
					{
						if(rx_fifo[i].f.can_dlc < 2)
							throw DashelException(DashelException::IOError, 0, "Packet too short", this);

						rx_buffer[0] = rx_fifo[i].f.can_dlc - 2;
						rx_buffer[1] = 0;
						rx_buffer[2] = CANID_TO_ID(rx_fifo[i].f.can_id);
						rx_buffer[3] = 0;
						memcpy(&rx_buffer[4], rx_fifo[i].f.data, rx_fifo[i].f.can_dlc);
						rx_p = 0;
						rx_len = rx_fifo[i].f.can_dlc + 4;
						 
						rx_fifo[i].used = 0; // Free the frame (pack_fifo())
						return 1;
					}
					if(CANID_TO_TYPE(rx_fifo[i].f.can_id) == TYPE_PACKET_STOP)
					{
						stopPos = i;
						stopId = CANID_TO_ID(rx_fifo[i].f.can_id);
						break;
					}
				}
				i++;
				if(i == RX_CAN_SIZE)
					i = 0;
			}
			if(stopPos < 0)
				return 0;
				
			i = rx_consume;
			// Len will be filled lated
			rx_buffer[2] = stopId;
			rx_buffer[3] = 0;
			rx_len = 4;
			while(1) 
			{
				if(rx_fifo[i].used && CANID_TO_ID(rx_fifo[i].f.can_id) == stopId)
				{
					if(rx_len == 4 && CANID_TO_TYPE(rx_fifo[i].f.can_id) != TYPE_PACKET_START)
						// We got a stop, but not a start, let's ignore this packet
						ignore = 1;

					if(rx_len + rx_fifo[i].f.can_dlc > sizeof(rx_buffer))
						throw DashelException(DashelException::IOError, 0, "Packet too large!", this);

					memcpy(&rx_buffer[rx_len], rx_fifo[i].f.data, rx_fifo[i].f.can_dlc);
					rx_len += rx_fifo[i].f.can_dlc;
					rx_fifo[i].used = 0;	

					if(i == stopPos)
						break;
				}
				if(++i == RX_CAN_SIZE)
					i = 0;
			}
			if(ignore)
			{
				rx_len = 0;
				rx_p = 0;
				return -1;
			} 
			else
			{
				rx_buffer[0] = rx_len - 6;
				rx_buffer[1] = (rx_len - 6) >> 8;
				rx_p = 0;
				return 1;
			}
		}
		int fifo_full() 
		{
			int i = rx_insert + 1;
			if(i == RX_CAN_SIZE)
				i = 0;
			return i == rx_consume;
		}

		void read_iface(void)
		{
			int def;
			while(1)
			{
				while((def = defragment()) == -1);
				if(def == 1)
					break;

				struct cmsghdr *cmsg;
				iov.iov_len = sizeof(rframe);
				msg.msg_flags = 0;
				if(recvmsg(fd, &msg, 0) < (int) sizeof(rframe))
					throw DashelException(DashelException::IOError, 0, "Read error", this);

				for(cmsg = CMSG_FIRSTHDR(&msg); 
					cmsg && (cmsg->cmsg_level == SOL_SOCKET);
					cmsg = CMSG_NXTHDR(&msg,cmsg))
				{
					if(cmsg->cmsg_type == SO_RXQ_OVFL)
					{
						__u32 * dropcnt = (__u32 *) CMSG_DATA(cmsg);
						if(*dropcnt)
							throw DashelException(DashelException::IOError, 0, "Packet dropped", this);
					}
				}
			
				if(fifo_full())
					throw DashelException(DashelException::IOError, 0, "Fifo full", this);

				// push to fifo ...
				memcpy(&rx_fifo[rx_insert].f,&rframe,sizeof(rframe));
				rx_fifo[rx_insert++].used = 1;
				if(rx_insert == RX_CAN_SIZE)
					rx_insert = 0;
			}
			pack_fifo();
		}
	public:
		virtual void read(void *data, size_t size) 
		{
			unsigned char * d = (unsigned char *) data;
			while(size)
			{
				if(rx_len)
				{
					*d++ = rx_buffer[rx_p++];
					rx_len--;
					size--;
				} 
				else 
				{
					read_iface();
				}
			}
		}

		virtual bool receiveDataAndCheckDisconnection()
		{
			return false;
		}

		virtual bool isDataInRecvBuffer() const 
		{
			struct can_frame f;
			if(rx_len || rx_insert != rx_consume)
				return true;

			if(recv(fd, &f, sizeof(f), MSG_DONTWAIT | MSG_PEEK) == sizeof(f))
				return true;

			return false;
		}


};

namespace Dashel
{
	void initPlugins()
	{
		Dashel::streamTypeRegistry.reg("can", &createInstance<CanStream>);
	}
}
