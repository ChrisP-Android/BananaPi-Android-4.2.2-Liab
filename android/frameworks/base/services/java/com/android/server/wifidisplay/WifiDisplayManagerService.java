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

import android.app.ActivityManagerNative;
import android.app.IActivityManager;
import android.content.Context;
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
import android.util.*;
import android.view.IWindowManager;
import java.io.BufferedOutputStream;
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
import android.view.MotionEvent;
import android.os.SystemProperties;
import android.wifidisplay.IWifiDisplayThread;
import android.wifidisplay.IWifiDisplayManager;
import com.android.server.power.PowerManagerService;


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
public class WifiDisplayManagerService extends IWifiDisplayManager.Stub 
{
    private static final String TAG = "WifiDisplayManagerService";

    private static final boolean LOCAL_LOGV = true;
    private static final boolean DEBUG_CLIENT = true;
	
    private final Context 		mContext;
	private final 				PowerManagerService mPM;
	private IWindowManager 		mWindowManager;
	final ArrayList<WifiDisplayClient> mClients = new ArrayList<WifiDisplayClient>();
	
    public WifiDisplayManagerService(Context context,PowerManagerService pm) 
    {
        mContext = context;
		mPM		 = pm;

		Log.d(TAG,"WifiDisplayManagerService Starting.......!");
		
		mWindowManager	= IWindowManager.Stub.asInterface(ServiceManager.getService(Context.WINDOW_SERVICE));
    }
    
    public void systemReady()
    {
    	
    }
    
    private final class ThreadDeathRecipient implements IBinder.DeathRecipient 
    {
        private  	IBinder  			mClientBinder;
        private  	IWifiDisplayThread  mClient;

        ThreadDeathRecipient(IWifiDisplayThread client) 
        {
            mClient 		= client;
            mClientBinder 	= client.asBinder();
        }

        public void binderDied() 
        {
            synchronized(WifiDisplayManagerService.this) 
            {
                WifiDisplayManagerService.this.removeWifiDisplayClient(mClient);
            }
        }
    }
    
    public class WifiDisplayClient
	{
		public static final int   WIFIDISPLAY_CLIENT_NONE   	= 0;
		public static final int   WIFIDISPLAY_CLIENT_SENDING  	= 1;
		public static final int   WIFIDISPLAY_CLIENT_RECEIVEING = 2;
		
        private  	IBinder  				mClientBinder;
        private  	IWifiDisplayThread  	mClient;
        private  	int           			mStatus; 
        private		ThreadDeathRecipient 	deathRecipient;
        
        public WifiDisplayClient(IWifiDisplayThread client)
        {
        	deathRecipient = new ThreadDeathRecipient(client);
        	
        	mClientBinder 		= client.asBinder();
        	mClient 			= client;
        	mStatus = WIFIDISPLAY_CLIENT_NONE;
        	try 
        	{
            	mClient.asBinder().linkToDeath(deathRecipient, 0);
        	} 
        	catch (RemoteException e) 
        	{

        	}
        }
        
        public void setWifiDisplayClientStatus(int status)
        {
        	mStatus = status;
        }
        
        public int getWifiDisplayClientStatus()
        {
        	return  mStatus;
        }
        
        public IWifiDisplayThread getWifiDisplayClient()
        {
        	return mClient;
        }
    }
    
    
    
    public int addWifiDisplayClient(IWifiDisplayThread client)
    {
    	int		 i;
    	boolean  find = false;
    	
    	for(i = 0;i < mClients.size();i++)
    	{
    		if(mClients.get(i).getWifiDisplayClient().asBinder().equals(client.asBinder()))
    		{
    			find = true;
    			
    			break;
    		}
    	}
    	
    	if (DEBUG_CLIENT) Slog.d(TAG, "addWifiDisplayClient find =  " + find + " IWifiDisplayThread = " + client);
    	
    	if(find == false)
    	{
	    	mClients.add(new WifiDisplayClient(client));
    	}

    	return 0;
    }
    
    public int removeWifiDisplayClient(IWifiDisplayThread client)
    {
    	int		 i;
    	boolean  find = false;
    	
    	for(i = 0;i < mClients.size();i++)
    	{
    		if(mClients.get(i).getWifiDisplayClient().asBinder().equals(client.asBinder()))
    		{
    			find = true;
    			
    			break;
    		}
    	}
    	
    	if (DEBUG_CLIENT) Slog.d(TAG, "removeWifiDisplayClient find =  " + find + " IWifiDisplayThread = " + client);
    	
    	if(find == true)
    	{
	    	mClients.remove(i);
	        	
	        return 0;
    	}
    	
    	return -1;
    }
    
    public int startWifiDisplaySend(IWifiDisplayThread client)
    {
    	int		 i;
    	boolean  find = false;
    	
    	for(i = 0;i < mClients.size();i++)
    	{
    		if(mClients.get(i).getWifiDisplayClient().asBinder().equals(client.asBinder()))
    		{
    			find = true;
    			
    			break;
    		}
    	}
    	
    	if (DEBUG_CLIENT) Slog.d(TAG, "startWifiDisplaySend find =  " + find + " IWifiDisplayThread = " + client);
    	
    	if(find == true)
    	{
	    	mClients.get(i).setWifiDisplayClientStatus(WifiDisplayClient.WIFIDISPLAY_CLIENT_SENDING);
	        	
	        return 0;
    	}
    	
    	return -1;
    }
    
	public int endWifiDisplaySend(IWifiDisplayThread client)
    {
    	int		 i;
    	boolean  find = false;
    	
    	for(i = 0;i < mClients.size();i++)
    	{
    		if(mClients.get(i).getWifiDisplayClient().asBinder().equals(client.asBinder()))
    		{
    			find = true;
    			
    			break;
    		}
    	}
    	
    	Slog.d(TAG,"endWifiDisplaySend mClients.size() = " + mClients.size());
    	
    	if (DEBUG_CLIENT) Slog.d(TAG, "endWifiDisplaySend find =  " + find + " IWifiDisplayThread = " + client);
    	
    	if(find == true)
    	{
	    	mClients.get(i).setWifiDisplayClientStatus(WifiDisplayClient.WIFIDISPLAY_CLIENT_NONE);
	        	
	        return 0;
    	}
    	
    	return -1;
    }
    
	public int startWifiDisplayReceive(IWifiDisplayThread client)
    {
    	int		 i;
    	boolean  find = false;
    	
    	Slog.d(TAG,"startWifiDisplayReceive mClients.size() = " + mClients.size());
    	for(i = 0;i < mClients.size();i++)
    	{
    		if(mClients.get(i).getWifiDisplayClient().asBinder().equals(client.asBinder()))
    		{
    			find = true;
    			
    			break;
    		}
    	}
    	
    	if (DEBUG_CLIENT) Slog.d(TAG, "startWifiDisplayReceive find =  " + find + " IWifiDisplayThread = " + client);
    	
    	if(find == true)
    	{
	    	mClients.get(i).setWifiDisplayClientStatus(WifiDisplayClient.WIFIDISPLAY_CLIENT_RECEIVEING);
	        	
	        return 0;
    	}
    	
    	return -1;
    }
    
	public int endWifiDisplayReceive(IWifiDisplayThread client)
    {
    	int		 i;
    	boolean  find = false;
    	
    	Slog.d(TAG,"endWifiDisplayReceive mClients.size() = " + mClients.size());
    	for(i = 0;i < mClients.size();i++)
    	{
    		if(mClients.get(i).getWifiDisplayClient().asBinder().equals(client.asBinder()))
    		{
    			find = true;
    			
    			break;
    		}
    	}
    	
    	if (DEBUG_CLIENT) Slog.d(TAG, "endWifiDisplayReceive find =  " + find + " IWifiDisplayThread = " + client);
    	
    	if(find == true)
    	{
	    	mClients.get(i).setWifiDisplayClientStatus(WifiDisplayClient.WIFIDISPLAY_CLIENT_NONE);
	        	
	        return 0;
    	}
    	
    	return -1;
    }
    
    public boolean exitWifiDisplayReceive()
    {
    	int		 				i;  
    	boolean					ret = false; 	
    	
    	Slog.d(TAG,"endWifiDisplayReceive mClients.size() = " + mClients.size());
    	for(i = 0;i < mClients.size();i++)
    	{
    		if(mClients.get(i).getWifiDisplayClientStatus() == WifiDisplayClient.WIFIDISPLAY_CLIENT_RECEIVEING)
    		{
    			IWifiDisplayThread wifidisplaythread = mClients.get(i).getWifiDisplayClient();
	
    			try 
		        {
		            wifidisplaythread.exitWifiDisplayReceive();
		            
		            ret = true;
		        } 
		        catch (RemoteException ex) 
		        {
		            break;
		        }
    		}
    	}
    	
    	return ret;
    }
    
	public boolean exitWifiDisplaySend()
	{
		int		 				i;   
		boolean					ret = false; 		
    	
    	for(i = 0;i < mClients.size();i++)
    	{
    		if(mClients.get(i).getWifiDisplayClientStatus() == WifiDisplayClient.WIFIDISPLAY_CLIENT_RECEIVEING)
    		{
    			IWifiDisplayThread wifidisplaythread = mClients.get(i).getWifiDisplayClient();
    			
    			try 
		        {
		            wifidisplaythread.exitWifiDisplaySend();
		            
		            ret = true;
		        } 
		        catch (RemoteException ex) 
		        {
		            break;
		        }
    		}
    	}
    	
    	return ret;
	}
    
    public int getWifiDisplayStatus()
	{
		int		 			i;   	
    	
    	Slog.d(TAG,"getWifiDisplayStatus mClients.size() = " + mClients.size());
    	
    	for(i = 0;i < mClients.size();i++)
    	{
    		if(mClients.get(i).getWifiDisplayClientStatus() != WifiDisplayClient.WIFIDISPLAY_CLIENT_NONE)
    		{
    			Slog.d(TAG,"getWifiDisplayStatus mClients.get(i).getWifiDisplayClientStatus() = " + mClients.get(i).getWifiDisplayClientStatus());
    			return mClients.get(i).getWifiDisplayClientStatus();
    		}
    	}
    	
    	return 0;
	}
	
	public void injectMotionEvent(MotionEvent motionevent)
	{
		int		 			i;   	
    	
    	Slog.d(TAG,"injectMotionEvent() = " + mClients.size());
    	
    	for(i = 0;i < mClients.size();i++)
    	{
    		if(mClients.get(i).getWifiDisplayClientStatus() == WifiDisplayClient.WIFIDISPLAY_CLIENT_RECEIVEING)
    		{
    			IWifiDisplayThread wifidisplaythread = mClients.get(i).getWifiDisplayClient();
    			
    			try 
		        {
		            wifidisplaythread.dispatchMotionEvent(motionevent);
		        } 
		        catch (RemoteException ex) 
		        {
		            break;
		        }
    		}
    	}
	}
}

