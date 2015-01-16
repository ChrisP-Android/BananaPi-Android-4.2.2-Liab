/*
 * Copyright (C) 2006 The Android Open Source Project
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

import com.android.internal.app.IMediaContainerService;
import com.android.internal.app.ResolverActivity;
import com.android.internal.content.NativeLibraryHelper;
import com.android.internal.content.PackageHelper;
import com.android.internal.util.FastXmlSerializer;
import com.android.internal.util.JournaledFile;
import com.android.internal.util.XmlUtils;

import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlPullParserException;
import org.xmlpull.v1.XmlSerializer;

import android.app.ActivityManagerNative;
import android.app.IActivityManager;
import android.app.admin.IDevicePolicyManager;
import android.app.backup.IBackupManager;
import android.content.Context;
import android.content.ComponentName;
import android.content.IIntentReceiver;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.IntentSender;
import android.content.ServiceConnection;
import android.content.IntentSender.SendIntentException;
import android.content.pm.ActivityInfo;
import android.content.pm.ApplicationInfo;
import android.content.pm.ComponentInfo;
import android.content.pm.FeatureInfo;
import android.content.pm.IPackageDataObserver;
import android.content.pm.IPackageDeleteObserver;
import android.content.pm.IPackageInstallObserver;
import android.content.pm.IPackageManager;
import android.content.pm.IPackageMoveObserver;
import android.content.pm.IPackageStatsObserver;
import android.content.pm.InstrumentationInfo;
import android.content.pm.PackageInfo;
import android.content.pm.PackageInfoLite;
import android.content.pm.PackageManager;
import android.content.pm.PackageStats;
import static android.content.pm.PackageManager.COMPONENT_ENABLED_STATE_DEFAULT;
import static android.content.pm.PackageManager.COMPONENT_ENABLED_STATE_DISABLED;
import static android.content.pm.PackageManager.COMPONENT_ENABLED_STATE_ENABLED;
import android.content.pm.PackageParser;
import android.content.pm.PermissionInfo;
import android.content.pm.PermissionGroupInfo;
import android.content.pm.ProviderInfo;
import android.content.pm.ResolveInfo;
import android.content.pm.ServiceInfo;
import android.content.pm.Signature;
import android.net.Uri;
import android.os.Binder;
import android.os.Build;
import android.os.Bundle;
import android.os.Debug;
import android.os.HandlerThread;
import android.os.IBinder;
import android.os.Looper;
import android.os.Message;
import android.os.Parcel;
import android.os.RemoteException;
import android.os.Environment;
import android.os.FileObserver;
import android.os.FileUtils;
import android.os.Handler;
import android.os.ParcelFileDescriptor;
import android.os.Process;
import android.os.ServiceManager;
import android.os.SystemClock;
import android.os.SystemProperties;
import android.provider.Settings;
import android.security.SystemKeyStore;
import android.util.*;
import android.view.Display;
import android.view.WindowManager;
import android.view.IWindowManager;
import android.view.IDisplayManagerAw;
import android.view.DisplayManagerAw;
import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileDescriptor;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.FilenameFilter;
import java.io.IOException;
import java.io.InputStream;
import java.io.PrintWriter;
import java.security.NoSuchAlgorithmException;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.Collections;
import java.util.Comparator;
import java.util.Date;
import java.util.Enumeration;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.zip.ZipEntry;
import java.util.zip.ZipException;
import java.util.zip.ZipFile;
import java.util.zip.ZipOutputStream;
import android.provider.Settings;
import android.os.SystemProperties;
import com.android.server.power.PowerManagerService;
import android.os.UserHandle;

/**
 * Keep track of all those .apks everywhere.
 * 
 * This is very central to the platform's security; please run the unit
 * tests whenever making modifications here:
 * 
mmm frameworks/base/tests/AndroidTests
adb install -r -f out/target/product/passion/data/app/AndroidTests.apk
adb shell am instrument -w -e class com.android.unit_tests.PackageManagerTests com.android.unit_tests/android.test.InstrumentationTestRunner
 *
 */
public class DisplayManagerServiceAw extends IDisplayManagerAw.Stub 
{
    private static final String TAG = "DisplayManagerServiceAw";

    private static final boolean LOCAL_LOGV = true;
    
    private final Context   mContext;
    private final           PowerManagerService mPM;
    private IWindowManager  mWindowManager;
    private int             mHdmiPlugin;
    private int             mTvDacPlugin;
    private static          DisplayThread sThread;
    private static          boolean sThreadStarted = false;
    private int             mBacklightMode;
    private int             mCurrentType = -1;
    private int             mCurrentMode = -1;

    private native void nativeInit();
    private native int  nativeGetDisplayCount();
    private native int  nativeGetDisplayOutputType(int mDisplay);
    private native int  nativeGetDisplayOutputFormat(int mDisplay);
    private native int  nativeGetDisplayWidth(int mDisplay);
    private native int  nativeGetDisplayHeight(int mDisplay);
    private native int  nativeGetDisplayPixelFormat(int mDisplay);
    private native int  nativeSetDisplayMode(int mode);
    private native int  nativeSetDisplayOutputType(int mDisplay,int type,int format);
    private native int  nativeOpenDisplay(int mDisplay);
    private native int  nativeCloseDisplay(int mDisplay);
    private native int  nativeSetDisplayMaster(int mDisplay);
    private native int  nativeGetDisplayOpen(int mDisplay);
    private native int  nativeGetDisplayHotPlug(int mDisplay);
    private native int  nativeGetHdmiHotPlug();
    private native int  nativeGetTvDacHotPlug();
    private native int  nativeGetDisplayMode();
    private native int  nativeGetDisplayMaster();
    private native int  nativeGetMaxWidthDisplay();
    private native int  nativeGetMaxHdmiMode();
    private native int  nativeSetDisplayParameter(int mDisplay,int para0,int para1);
    private native int nativeSetDisplayBacklihgtMode(int mode);
    private native int  nativeSetDisplayAreaPercent(int mDisplay,int percent);
    private native int  nativeGetDisplayAreaPercent(int mDisplay);
    private native int  nativeSetDisplayBright(int mDisplay,int bright);
    private native int  nativeGetDisplayBright(int mDisplay);
    private native int  nativeSetDisplayContrast(int mDisplay,int contrast);
    private native int  nativeGetDisplayContrast(int mDisplay);
    private native int  nativeSetDisplaySaturation(int mDisplay,int saturation);
    private native int  nativeGetDisplaySaturation(int mDisplay);
    private native int  nativeSetDisplayHue(int mDisplay,int saturation);
    private native int  nativeGetDisplayHue(int mDisplay);
    private native int  nativeIsSupportHdmiMode(int mode);
    private native int  nativeIsSupport3DMode();
    private native int  nativeSet3DLayerOffset(int displayno, int left, int right);
    private native int  nativeStartWifiDisplaySend(int mDisplay,int mode);
    private native int  nativeEndWifiDisplaySend(int mDisplay);
    private native int  nativeStartWifiDisplayReceive(int mDisplay,int mode);
    private native int  nativeEndWifiDisplayReceive(int mDisplay);
    private native int  nativeUpdateSendClient(int mode);

    
    private final void sendHdmiIntent() 
    {
        //  Pack up the values and broadcast them to everyone
        Intent intent = new Intent(Intent.ACTION_HDMISTATUS_CHANGED);
        intent.addFlags(Intent.FLAG_RECEIVER_REGISTERED_ONLY
                | Intent.FLAG_RECEIVER_REPLACE_PENDING);
        
        intent.putExtra(DisplayManagerAw.EXTRA_HDMISTATUS, mHdmiPlugin);

        ActivityManagerNative.broadcastStickyIntent(intent, null, UserHandle.USER_ALL);
    }

    private final void sendTvDacIntent() 
    {
        //  Pack up the values and broadcast them to everyone
        Intent intent = new Intent(Intent.ACTION_TVDACSTATUS_CHANGED);
        intent.addFlags(Intent.FLAG_RECEIVER_REGISTERED_ONLY
                | Intent.FLAG_RECEIVER_REPLACE_PENDING);
        
        intent.putExtra(DisplayManagerAw.EXTRA_TVSTATUS, mTvDacPlugin);

        ActivityManagerNative.broadcastStickyIntent(intent, null, UserHandle.USER_ALL);
    }

    private final void sendOutputChangedIntent() {
        Intent intent = new Intent(Intent.ACTION_DISPLAY_OUTPUT_CHANGED);
        intent.addFlags(Intent.FLAG_RECEIVER_REGISTERED_ONLY
                | Intent.FLAG_RECEIVER_REPLACE_PENDING);

        intent.putExtra(DisplayManagerAw.EXTRA_DISPLAY_TYPE, mCurrentType);
        intent.putExtra(DisplayManagerAw.EXTRA_DISPLAY_MODE, mCurrentMode);

        ActivityManagerNative.broadcastStickyIntent(intent, null, UserHandle.USER_ALL);

    }
    
    public class DisplayThread extends Thread 
    {
        private final DisplayManagerServiceAw mService;
        private final Context mContext;
        private final PowerManagerService mPM;
        private int hdmihotplug = 0;
        private int tvdachotplug = 0;

        public DisplayThread(Context context,DisplayManagerServiceAw service,
                PowerManagerService pm) 
        {
            super("DisplayManagerPolicy");
            mService = service;
            mContext = context;
            mPM = pm;
        }

        public void run() 
        {
            //Looper.prepare();
            
            android.os.Process.setThreadPriority(
                    android.os.Process.THREAD_PRIORITY_FOREGROUND);
            android.os.Process.setCanSelfBackground(false);

            synchronized (this) 
            {
                notifyAll();
            }

            while (true) 
            {
                int  hotplug;
                
                hotplug = nativeGetHdmiHotPlug();
                if(hotplug != hdmihotplug)
                {
                    hdmihotplug = hotplug;
                    mService.setHdmiHotplugStatus(hdmihotplug);

                    sendHdmiIntent();
                }

                hotplug = nativeGetTvDacHotPlug();
                if(hotplug != tvdachotplug)
                {
                    tvdachotplug = hotplug;
                    mService.settvHotplugStatus(tvdachotplug);
                    
                    sendTvDacIntent();
                }

                int type = nativeGetDisplayOutputType(0);
                int mode = nativeGetDisplayOutputFormat(0);
                if (type != mCurrentType || mode != mCurrentMode) {
                    mCurrentType = type;
                    mCurrentMode = mode;
                    sendOutputChangedIntent();
                }

                try 
                {
                    Thread.sleep(500);
                } 
                catch (Exception e) 
                {
                    Log.d(TAG,"thread sleep failed!");
                } 
                
            }
        }
    }

    
    
    public DisplayManagerServiceAw(Context context,PowerManagerService pm) 
    {
        mContext = context;
        mPM      = pm;

        Log.d(TAG,"DisplayManagerServiceAw Starting.......!");
        
        nativeInit();
        
        // set initial hotplug status
        if(SystemProperties.get("ro.display.switch").equals("1"))
        {
            if (sThreadStarted == false) 
            {
                sThread = new DisplayThread(mContext,this,mPM);
                sThread.start();
                sThreadStarted = true;
            }
        }
        mWindowManager  = IWindowManager.Stub.asInterface(ServiceManager.getService(Context.WINDOW_SERVICE));
        //boolean enable = Settings.System.getInt(mContext.getContentResolver(),Settings.System.SMART_BRIGHTNESS_ENABLE,0) != 0 ? true : false;
        boolean enable = false;
        setDisplayBacklightMode(enable?1:0);
    }
    
    public void systemReady()
    {
        // init bright
        int brightness = Settings.System.getInt(mContext.getContentResolver(),
                Settings.System.COLOR_BRIGHTNESS, DisplayManagerAw.DEF_BRIGHTNESS);
        setDisplayBright(0, brightness);
        // init saturation
        int saturation = Settings.System.getInt(mContext.getContentResolver(),
                Settings.System.COLOR_SATURATION, DisplayManagerAw.DEF_SATURATION);
        setDisplaySaturation(0, saturation);
        // init contrast
        int contrast = Settings.System.getInt(mContext.getContentResolver(),
                Settings.System.COLOR_CONTRAST, DisplayManagerAw.DEF_CONTRAST);
        setDisplayContrast(0, contrast);
    }
    
    public int getDisplayCount()
    {
        return nativeGetDisplayCount();
    }
    
    public int getDisplayOutputType(int mDisplay)
    {
        return nativeGetDisplayOutputType(mDisplay);
    }
    
    public int getDisplayOutputFormat(int mDisplay)
    {
        return nativeGetDisplayOutputFormat(mDisplay);
    }
    
    public int getDisplayWidth(int mDisplay)
    {
        return nativeGetDisplayWidth(mDisplay);
    }
    
    public int getDisplayHeight(int mDisplay)
    {
        return nativeGetDisplayHeight(mDisplay);
    }
    
    public int getDisplayPixelFormat(int mDisplay)
    {
        return nativeGetDisplayPixelFormat(mDisplay);
    }
    
    public int setDisplayParameter(int mDisplay,int param0,int param1)
    {
        return nativeSetDisplayParameter(mDisplay,param0,param1);
    }
    
    public int setDisplayMode(int mode)
    {
        try
        {
            if(mode == 3 || mode == 4)
            {
                int hwrotation = 0;

                hwrotation = SystemProperties.getInt("ro.sf.hwrotation",0);
                if((hwrotation==90) || (hwrotation==270)) 
                {
                    mWindowManager.freezeRotation(1);
                }
                else
                {
                    if( getDisplayWidth(0) < getDisplayHeight(0) )
                    {
                        mWindowManager.freezeRotation(1);
                    }
                    else
                    {
                        mWindowManager.freezeRotation(0);
                    }
                    //mWindowManager.freezeRotation(1);
                }
            }
            else
            {
                mWindowManager.thawRotation();
            }
        }             
        catch (Exception e)
        {
            Log.d(TAG,"freezeRotation or thawRotation failed!");
            return -1;
        } 
        
        return nativeSetDisplayMode(mode);
    }
    
    public int setDisplayOutputType(int mDisplay,int type,int format)
    {
        return nativeSetDisplayOutputType(mDisplay,type,format);
    }
    
    public int getDisplayHotPlugStatus(int mDisplay)
    {
        return nativeGetDisplayHotPlug(mDisplay);
    }

    public int setDisplayBacklightMode(int mode)
    {
        mBacklightMode = mode;
        return nativeSetDisplayBacklihgtMode(mode);
    }

    public int getDisplayBacklightMode()
    {
        return mBacklightMode;
    }
    
    public int openDisplay(int mDisplay)
    {
        return nativeOpenDisplay(mDisplay);
    }
    
    public int closeDisplay(int mDisplay)
    {
        return nativeCloseDisplay(mDisplay);
    }
    
    public boolean getDisplayOpenStatus(int mDisplay)
    {
        int  ret;
        
        ret = nativeGetDisplayOpen(mDisplay);
        if(ret == 0)
        {
            return false;
        }
        
        return true;
    }

    public int getDisplayMode()
    {
        return nativeGetDisplayMode();
    }
    
    public int setDisplayMaster(int mDisplay)
    {
        return setDisplayMaster(mDisplay);
    }

    public int getDisplayMaster()
    {
        return nativeGetDisplayMaster();
    }
    
    public int getMaxWidthDisplay()
    {
        return nativeGetMaxWidthDisplay();
    }
    
    public int getMaxHdmiMode()
    {
        return nativeGetMaxHdmiMode();
    }

    public int isSupportHdmiMode(int mode)
    {
        return nativeIsSupportHdmiMode(mode);
    }
    
    public int isSupport3DMode()
    {
        return nativeIsSupport3DMode();
    }
    
    public int getHdmiHotPlugStatus()
    {
        return nativeGetHdmiHotPlug();
    }
    
    public int getTvHotPlugStatus()
    {
        return nativeGetTvDacHotPlug();
    }
    
    public int setDisplayAreaPercent(int displayno,int percent)
    {
        return nativeSetDisplayAreaPercent(displayno,percent);
    }
    
    public int getDisplayAreaPercent(int displayno)
    {
        return nativeGetDisplayAreaPercent(displayno);
    }
    
    public int setDisplayBright(int displayno,int bright)
    {
        return nativeSetDisplayBright(displayno,bright);
    }
    
    public int getDisplayBright(int displayno)
    {
        return nativeGetDisplayBright(displayno);
    }
    
    public int setDisplayContrast(int displayno,int contrast)
    {
        return nativeSetDisplayContrast(displayno,contrast);
    }
    
    public int getDisplayContrast(int displayno)
    {
        return nativeGetDisplayContrast(displayno);
    }
    
    public int setDisplaySaturation(int displayno,int saturation)
    {
        return nativeSetDisplaySaturation(displayno,saturation);
    }
    
    public int getDisplaySaturation(int displayno)
    {
        return nativeGetDisplaySaturation(displayno);
    }

    public int setDisplayHue(int displayno,int hue)
    {
        return nativeSetDisplayHue(displayno,hue);
    }
    
    public int getDisplayHue(int displayno)
    {
        return nativeGetDisplayHue(displayno);
    }

    public int setHdmiHotplugStatus(int status)
    {
        mHdmiPlugin = status;
        return 0;
    }

    public int settvHotplugStatus(int status)
    {
        mTvDacPlugin = status;
        return 0;
    }
    public int startWifiDisplaySend(int mDisplay,int mode)
    {
        return nativeStartWifiDisplaySend(mDisplay,mode);
    }
    
    public int endWifiDisplaySend(int mDisplay)
    {
        return nativeEndWifiDisplaySend(mDisplay);
    }
    
    public int startWifiDisplayReceive(int mDisplay,int mode)
    {
        return nativeStartWifiDisplayReceive(mDisplay,mode);
    }
    
    public int endWifiDisplayReceive(int mDisplay)
    {
        return nativeEndWifiDisplayReceive(mDisplay);
    }

    public int updateSendClient(int mode)
    {
        return nativeUpdateSendClient(mode);
    }

    public int set3DLayerOffset(int displayno, int left, int right){
        return nativeSet3DLayerOffset(displayno, left, right);
    }
}

