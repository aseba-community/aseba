/*
        Aseba - an event-based framework for distributed robot control
        Copyright (C) 2013:
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
#include <string>
#include <iostream>

#include <cstring>
#include <poll.h>

#include <dashel/dashel.h>
#include <dashel/dashel-posix.h>

#include "android.h"

#include <sys/ioctl.h>
#include <linux/usb_ch9.h>
#include <linux/usbdevice_fs.h>


static JavaVM* s_javaVM = 0;
static jclass s_DashelSerialClassID;
static jmethodID s_DashelSerialConstructorMethodID;
static jmethodID s_DashelSerialOpenDeviceMethodID;
static jmethodID s_DashelSerialCloseDeviceMethodID;
static jmethodID s_DashelSerialGetEpInMethodID;
static jmethodID s_DashelSerialGetEpOutMethodID;

using namespace Dashel;
using namespace std;

void AndroidStream::usb_control_msg(int type, int req, int value, int index, char * bytes, int size, int timeout) {
    struct usbdevfs_ctrltransfer ctrl;
    ctrl.bRequestType = type;
    ctrl.bRequest = req;
    ctrl.wValue = value;
    ctrl.wIndex = index;
    ctrl.wLength = size;
    ctrl.data = bytes;
    ctrl.timeout = timeout;

    ioctl(fd, USBDEVFS_CONTROL, &ctrl);
}

void AndroidStream::schedule_read() {
    urb_is_in_flight = 1;
    memset(&rx_urb, 0, sizeof(rx_urb));
    rx_urb.type = USBDEVFS_URB_TYPE_BULK;
    rx_urb.endpoint = epin;
    rx_urb.buffer = rx_data;
    rx_urb.buffer_length = sizeof(rx_data);
    if(ioctl(fd, USBDEVFS_SUBMITURB, &rx_urb) < 0) {
        disconnected = 1;
        throw DashelException(DashelException::IOError, 0, "Unable to read", this);
    }
}

void AndroidStream::set_ctrl_line(unsigned int status) {
    usb_control_msg(0x21, 0x22, status, 0, NULL, 0, 1000);
}

AndroidStream::AndroidStream(const string &targetName) :
            Stream("android"),
            SelectableStream("android")
{
   pollEvent = POLLOUT;
   lock_java();

    // Create an instance of DashelSerial in the java world
    m_DashelSerialObject = env->NewGlobalRef(env->NewObject(s_DashelSerialClassID,s_DashelSerialConstructorMethodID));
    if(!m_DashelSerialObject) {
        unlock_java();
        throw DashelException(DashelException::ConnectionFailed, 0, "Unable to create java object");
    }

    // Now, open the device and claim interface (in the Java code).
    fd = env->CallIntMethod(m_DashelSerialObject, s_DashelSerialOpenDeviceMethodID);

    if(fd < 0) {
        env->DeleteGlobalRef(m_DashelSerialObject);
        unlock_java();
        throw  DashelException(DashelException::ConnectionFailed, 0, "Unable to grab fd");
    }

    epin = env->CallIntMethod(m_DashelSerialObject, s_DashelSerialGetEpInMethodID);
    epout = env->CallIntMethod(m_DashelSerialObject, s_DashelSerialGetEpOutMethodID);

    unlock_java();

    disconnected = 0;

    // software-open the port
    set_ctrl_line(1);

    schedule_read();
}

AndroidStream::~AndroidStream()
{
    set_ctrl_line(0); // software-close the port

    lock_java();
    env->CallIntMethod(m_DashelSerialObject, s_DashelSerialCloseDeviceMethodID);
    env->DeleteGlobalRef(m_DashelSerialObject);
    unlock_java();
}

void AndroidStream::write(const void *data, const size_t size)
{
    struct usbdevfs_bulktransfer bulk;
    bulk.ep = epout;
    bulk.len = size;
    bulk.timeout = 1000;
    bulk.data = (void *) data;
    if(ioctl(fd, USBDEVFS_BULK, &bulk) < 0) {
	disconnected = 1;
        throw DashelException(DashelException::IOError, 0, "Unable to write", this);
    }
}

void AndroidStream::flush()
{
    // Nothing to do, everything is written immediately.
}

void AndroidStream::read(void *data, size_t size)
{
    struct usbdevfs_urb * urb;
    while(size) {
        if(urb_is_in_flight) {
            if(ioctl(fd, USBDEVFS_REAPURB, &urb) < 0) {
		disconnected = 1;
                throw DashelException(DashelException::IOError, 0, "Unable to read", this);
            }
            urb_is_in_flight = 0;
        }
        size_t cpy = size > (size_t) rx_urb.actual_length ? (size_t) rx_urb.actual_length : size;

        memcpy(data,rx_urb.buffer,cpy);

        rx_urb.buffer = (void *) (((size_t) rx_urb.buffer) + cpy);
        data = (void *) (((size_t) data) + cpy);
        size -= cpy;
        rx_urb.actual_length -= cpy;

        if (!rx_urb.actual_length)
            schedule_read();
    }
}

bool AndroidStream::receiveDataAndCheckDisconnection()
{
    struct usbdevfs_urb * urb;

    // Reap everthing
    if(ioctl(fd, USBDEVFS_REAPURBNDELAY, &urb) >= 0)
        urb_is_in_flight = 0;

    if(disconnected == 1)
	return true;
    return false;
}

bool AndroidStream::isDataInRecvBuffer() const
{
    if(urb_is_in_flight)
        return false;
    return true;
}

void AndroidStream::lock_java()
{
    // Kind of mutex_lock();	    
    if(s_javaVM->AttachCurrentThread(&env, NULL)<0)
        throw DashelException(DashelException::ConnectionFailed, 0, "Java lock failed", this);
}

void AndroidStream::unlock_java()
{
    s_javaVM->DetachCurrentThread();
}


namespace Dashel
{
	void initPlugins()
	{
        Dashel::streamTypeRegistry.reg("android", &createInstance<AndroidStream>);
	}
}


JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void * /*reserved*/)
{
    JNIEnv *env;
    if(vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK) {
        return -1;
    }

    s_javaVM = vm;

    // Do a kind of "linking".
    jclass clazz = env->FindClass("org/mobsya/thymiovpl/DashelSerial");
    if(!clazz)
    {
        return -1;
    }

    s_DashelSerialClassID = (jclass) env->NewGlobalRef(clazz);

    s_DashelSerialConstructorMethodID = env->GetMethodID(s_DashelSerialClassID, "<init>", "()V");
    if(!s_DashelSerialConstructorMethodID) {
        return -1;
    }

    s_DashelSerialOpenDeviceMethodID = env->GetMethodID(s_DashelSerialClassID, "OpenDevice", "()I");
    if(! s_DashelSerialOpenDeviceMethodID) {
        return -1;
    }

    s_DashelSerialCloseDeviceMethodID = env->GetMethodID(s_DashelSerialClassID, "CloseDevice", "()Z");
    if(! s_DashelSerialCloseDeviceMethodID) {
        return -1;
    }

    s_DashelSerialGetEpInMethodID = env->GetMethodID(s_DashelSerialClassID, "GetEpIn", "()I");
    if(! s_DashelSerialGetEpInMethodID) {
        return -1;
    }

    s_DashelSerialGetEpOutMethodID = env->GetMethodID(s_DashelSerialClassID, "GetEpOut", "()I");
    if(! s_DashelSerialGetEpOutMethodID) {
        return -1;
    }

    return JNI_VERSION_1_6;
}

