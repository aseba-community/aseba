/*
    Aseba - an event-based framework for distributed robot control
    Copyright (C) 2007--2013:
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

/*
    This file was originally created by Philippe Rétornaz
*/

#include "common/consts.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <termios.h>
#include <unistd.h>

#include <string.h>

#include <stdio.h>
#include <stdlib.h>

static struct termios oldtio;

int open_port(char* file) {
    struct termios newtio;
    int fd;

    fd = open(file, O_RDWR);
    if(fd < 0)
        return fd;

    tcgetattr(fd, &oldtio);

    memset(&newtio, 0, sizeof(newtio));

    newtio.c_cflag |= CLOCAL;
    newtio.c_cflag |= CREAD;
    newtio.c_cflag |= CS8;
    newtio.c_cflag |= CRTSCTS;
    newtio.c_cflag |= B0;  // So DTR is not set
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;
    newtio.c_lflag = 0;
    newtio.c_cc[VTIME] = 0;
    newtio.c_cc[VMIN] = 1;

    tcflush(fd, TCIOFLUSH);
    tcsetattr(fd, TCSANOW, &newtio);

    return fd;
}

void close_port(int fd) {
    if(fd < 0)
        return;

    tcsetattr(fd, TCSANOW, &oldtio);

    close(fd);
}

#define CHANNEL(x) (15 + 5 * x)

struct __attribute((packed)) {
    unsigned short nodeid;
    unsigned short panid;
    unsigned char channel;
    unsigned char txpower;
    unsigned char version;
    unsigned char c;
} settings = {0xffff, 0xffff, 0, 0, 0};

static void usage(char* s) {
    printf("Thymio Wireless Network Configurator CLI %s\n", ASEBA_VERSION);
    printf("Aseba protocol %d\n", ASEBA_PROTOCOL_VERSION);
    printf("Licence LGPLv3: GNU LGPL version 3 <http://www.gnu.org/licenses/lgpl.html>\n\n");
    printf("Usage:\n");
    printf("\t%s [-w] [-i] [-n NodeID] [-p PanID] [-c 0-2] serialPort\n\n", s);
    printf("Mandatory arguements:\n");
    printf("\t serialPort : 	The serial port file\n");
    printf("\nOptional arguments:\n");
    printf("\t -w :           Write into flash\n");
    printf("\t -i :           Enter/exit paring mode\n");  // ADD MIC launch paring
    printf("\t -n NodeID :    Set the node ID (do not change if not mandatory)\n");
    printf("\t -p PanID :     Set the Pan ID\n");
    printf("\t -c 0-2 :       Set the channel number\n");
    // printf("\t -t power :     Set the TX Power (unused)\n");

    exit(1);
}

static void option_error(char* s, const char* e) {
    fprintf(stderr, "Error in command line option: %s\n\n", e);
    usage(s);
}

int main(int argc, char** argv) {
    int i;
    int fd;
    int count;
    settings.c = 0;
    if(argc < 2)
        option_error(argv[0], "missing serial port file");

    for(i = 1; i < argc; i++) {
        if(!strcmp(argv[i], "-w")) {
            settings.c += 0x1;
            continue;
        }
        if(!strcmp(argv[i], "-i"))  // ADD MIC launch paring
        {
            settings.c += 0x4;
            continue;
        }
        if(!strcmp(argv[i], "-n")) {
            i++;
            if(argc == i)
                option_error(argv[0], "missing NodeID");
            settings.nodeid = strtol(argv[i], NULL, 0);
            if(settings.nodeid == 0 || settings.nodeid == 0xFFFF)
                option_error(argv[i], "invalid NodeID");
            continue;
        }
        if(!strcmp(argv[i], "-p")) {
            i++;
            if(argc == i)
                option_error(argv[0], "missing PanID");
            settings.panid = strtol(argv[i], NULL, 0);
            if(settings.panid == 0 || settings.panid == 0xFFFF)
                option_error(argv[0], "invalid PanID");
            continue;
        }
        if(!strcmp(argv[i], "-c")) {
            int c;
            i++;
            if(argc == i)
                option_error(argv[0], "missing channel");
            c = atoi(argv[i]);
            if(c < 0 || c > 2)
                option_error(argv[0], "invalid channel");
            settings.channel = CHANNEL(c);
            continue;
        }
        if(!strcmp(argv[i], "-t")) {
            i++;
            if(argc == i)
                option_error(argv[0], "missing power");
            // Unused for now...
            continue;
        }
        if(i + 1 != argc) {
            // Not an option but still some option .. error
            option_error(argv[0], "unknown option");
        }
    }

    fd = open_port(argv[argc - 1]);

    if(fd < 0) {
        fprintf(stderr, "Error opening serial port file %s\n", argv[argc - 1]);
        usage(argv[0]);
    }

    count = write(fd, &settings, sizeof(settings));
    if(count != sizeof(settings)) {
        fprintf(stderr, "Error writing settings, written %d instead of %lu bytes\n", count, sizeof(settings));
        exit(2);
    }
    count = read(fd, &settings, sizeof(settings) - 1);
    if(count != sizeof(settings) - 1) {
        fprintf(stderr, "Error reading settings, read %d instead of %lu bytes\n", count, sizeof(settings) - 1);
        exit(3);
    }

    printf("Got back:\n");
    printf("\tNodeID: 0x%04x\n", settings.nodeid);
    printf("\tPanID: 0x%04x\n", settings.panid);
    printf("\tchannel: %d\n", (settings.channel - 15) / 5);
    printf("\ttxpower: %d\n", settings.txpower);
    printf("\tVersion: %d\n", settings.version);
    close_port(fd);

    return 0;
}
