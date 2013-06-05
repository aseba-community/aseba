#ifndef DASHEL_PLUGIN_ANDROID_H
#define DASHEL_PLUGIN_ANDROID_H

#include <jni.h>
#include <dashel/dashel-posix.h>
#include <dashel/dashel-private.h>
#include <string>
#include <linux/usbdevice_fs.h>

class AndroidStream: public Dashel::SelectableStream
{
public:
    AndroidStream(const std::string &targetName);
    ~AndroidStream();
    virtual void write(const void *data, const size_t size);
    virtual void read(void *data, size_t size);
    virtual void flush();
    virtual bool receiveDataAndCheckDisconnection();
    virtual bool isDataInRecvBuffer() const;

private:
    void usb_control_msg(int type, int req, int value, int index, char * bytes, int size, int timeout);
    void schedule_read();
    void set_ctrl_line(unsigned int status);
    void lock_java();
    void unlock_java();

    jobject m_DashelSerialObject;
    JNIEnv * env;

    struct usbdevfs_urb rx_urb; // Used only for reception
    unsigned char rx_data[512];
    bool urb_is_in_flight;
    int epin;
    int epout;
    int disconnected;
};

#endif // DASHEL_PLUGIN_ANDROID_H
