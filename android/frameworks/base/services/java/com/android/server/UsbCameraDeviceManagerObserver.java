/*
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.server;

import android.app.ActivityManagerNative;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Handler;
import android.os.Message;
import android.os.PowerManager;
import android.os.PowerManager.WakeLock;
import android.os.UEventObserver;
import android.util.Slog;
import android.hardware.usb.UsbCameraManager;

import android.util.Log;
import android.os.Bundle;
import android.os.UserHandle;

import java.lang.String;
import java.lang.Exception;


/**
 * UsbCameraDeviceManagerObserver monitors for UsbCamera devices on the main board.
 */
public class UsbCameraDeviceManagerObserver extends UEventObserver {
    private static final String TAG = UsbCameraDeviceManagerObserver.class.getSimpleName();

    private static final String uUsbCameraDevicesPath   = "/sys/class/video4linux";
    private static final String uEventSubsystem     = "SUBSYSTEM=video";
    private static final String uEventDevPath       = "DEVPATH";
    private static final String uEventDevName       = "DEVNAME";
    private static final String uEventAction        = "ACTION";
    private static final String uVideoDev           = "video";

    private int mUsbCameraTotalNumber = 0;

    private final Context mContext;
    private final WakeLock mWakeLock;  // held while there is a pending route change

    private UsbCameraDeviceManagerObserver(Context context) {
        mContext = context;
        PowerManager pm = (PowerManager)context.getSystemService(Context.POWER_SERVICE);
        mWakeLock = pm.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, "UsbCameraDeviceManagerObserver");
        mWakeLock.setReferenceCounted(false);

        Log.d(TAG, "UsbCameraDeviceManagerObserver construct");

        context.registerReceiver(new BootCompletedReceiver(),
            new IntentFilter(Intent.ACTION_BOOT_COMPLETED), null, null);
    }

    private static UsbCameraDeviceManagerObserver mUsbCameraObserver = null;

    public synchronized static UsbCameraDeviceManagerObserver getInstance(Context context){
        if(mUsbCameraObserver == null){
            mUsbCameraObserver = new UsbCameraDeviceManagerObserver(context);
        }
        return mUsbCameraObserver;
    }

    private final class BootCompletedReceiver extends BroadcastReceiver
    {
        @Override
        public void onReceive(Context context, Intent intent) {        
            //monitor usb camera
            startObserving(uEventSubsystem);
            mUsbCameraTotalNumber = android.hardware.Camera.getNumberOfCameras();
            Log.d(TAG, "UsbCameraDeviceManagerObserver startObserving mUsbCameraTotalNumber = " + mUsbCameraTotalNumber );
        }
    }

    @Override
    public void onUEvent(UEventObserver.UEvent event) {
        Log.d(TAG, "UsbCamera device change: " + event.toString());
        try{
            String devPath = event.get(uEventDevPath);
            String devName = event.get(uEventDevName);
            String action = event.get(uEventAction);

            if(action.equals("add")){
                mUsbCameraTotalNumber++;
                Log.i(TAG,"action.equals(add) mUsbCameraTotalNumber = " +mUsbCameraTotalNumber);
                updateState(devName,mUsbCameraTotalNumber,UsbCameraManager.PLUG_IN);
            }else if(action.equals("remove")){
                if(mUsbCameraTotalNumber>0){
                    mUsbCameraTotalNumber--;
                }
                Log.i(TAG,"action.equals(remove) mUsbCameraTotalNumber = " +mUsbCameraTotalNumber); 
                updateState(devName,mUsbCameraTotalNumber,UsbCameraManager.PLUG_OUT);
            }
        }catch(Exception e){
            Slog.e(TAG,"",e);
        }

    }

    public synchronized final void updateState(String name, int state ,int totalNum){
        updateState(name,totalNum,state,null);
    }

    public synchronized final void updateState(String name,int state,int totalNum, String extraMng) {
        Log.d(TAG, "name: " + name + " state: " + state + " totalNum: " + totalNum);
        Intent intent;

        intent = new Intent(UsbCameraManager.ACTION_USB_CAMERA_PLUG_IN_OUT);
        intent.addFlags(Intent.FLAG_RECEIVER_REGISTERED_ONLY);
        Bundle bundle = new Bundle();
        bundle.putInt(UsbCameraManager.USB_CAMERA_STATE,state);
        bundle.putString(UsbCameraManager.USB_CAMERA_NAME,name);
        bundle.putInt(UsbCameraManager.USB_CAMERA_TOTAL_NUMBER,totalNum);
        if(extraMng != null){
            bundle.putString(UsbCameraManager.EXTRA_MNG, extraMng);
        }
        intent.putExtras(bundle);
        ActivityManagerNative.broadcastStickyIntent(intent, null, UserHandle.USER_ALL);
    }

}
