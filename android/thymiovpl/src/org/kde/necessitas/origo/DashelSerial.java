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

package org.kde.necessitas.origo;

import android.util.Log;
import android.hardware.usb.UsbManager;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbDeviceConnection;
import android.hardware.usb.UsbInterface;
import android.content.Context;
import android.content.Intent;

import org.kde.necessitas.origo.QtActivity;

import java.util.HashMap;
import java.util.Iterator;

import java.util.concurrent.CountDownLatch;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.IntentFilter;
import android.app.PendingIntent;

public class DashelSerial {
    private UsbDevice device;
    private UsbManager manager;
    private UsbDeviceConnection connection;
    private static final String ACTION_USB_PERMISSION = "com.android.example.USB_PERMISSION";
    private CountDownLatch latch = new CountDownLatch(1);

    private final BroadcastReceiver mUsbReceiver = new BroadcastReceiver() {
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (ACTION_USB_PERMISSION.equals(action)) {
                // We don't need synchronization, as the other thread is waiting on us.
                device = (UsbDevice)intent.getParcelableExtra(UsbManager.EXTRA_DEVICE);
                latch.countDown();
            }
        }
    };

    void RequestPermission() throws InterruptedException  {
        PendingIntent mPermissionIntent = PendingIntent.getBroadcast(QtActivity.getQtActivityInstance(), 0, new Intent(ACTION_USB_PERMISSION), 0);
        IntentFilter filter = new IntentFilter(ACTION_USB_PERMISSION);
        QtActivity.getQtActivityInstance().registerReceiver(mUsbReceiver, filter);
        manager.requestPermission(device, mPermissionIntent);
        latch.await();
    }

    public DashelSerial() throws InterruptedException {
        manager = (UsbManager) QtActivity.getQtActivityInstance().getSystemService(Context.USB_SERVICE);
        device = (UsbDevice)  QtActivity.getQtActivityInstance().getIntent().getParcelableExtra(UsbManager.EXTRA_DEVICE);
        if(device == null) {
            HashMap<String, UsbDevice> deviceList = manager.getDeviceList();
            Iterator<UsbDevice> deviceIterator = deviceList.values().iterator();
            while(deviceIterator.hasNext()){
                device = deviceIterator.next();
                if(device.getVendorId() == 0x0617 || device.getProductId() == 0x000a) {
                    RequestPermission();
                    break;
                }
            }
            Log.i("DashelJ", "Unable to find usb device");
            return;
        }
    }

    int OpenDevice()
    {
        if (device == null)
            return -1;

        connection = manager.openDevice(device);

        if(connection == null) {
            Log.i("DashelJ", "Unable to open device");
            return -1;
        }

        int i;
        // TODO Claim only interface 0 & cdc-acm one.
        for(i = 0; i < device.getInterfaceCount(); i++) {
            UsbInterface intf = device.getInterface(i);
            connection.claimInterface(intf, true);
        }

        Log.i("DashelJ", "Device successfully opened");
        return connection.getFileDescriptor();
    }

    boolean CloseDevice()
    {
        connection.close(); // Linux take care of releasing everything else ...
        return true;
    }
}
