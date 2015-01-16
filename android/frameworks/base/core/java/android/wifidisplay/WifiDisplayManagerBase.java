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

package android.wifidisplay;

import android.os.AsyncTask;
import android.os.Bundle;
import android.os.Debug;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.os.Message;
import android.os.MessageQueue;
import android.os.ParcelFileDescriptor;
import android.os.Process;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.os.StrictMode;
import android.os.SystemClock;
import android.util.EventLog;
import android.util.Log;
import android.util.LogPrinter;
import android.util.Slog;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.TimeZone;
import java.util.regex.Pattern;
import android.content.Context;
import android.view.MotionEvent;
import android.view.IWindowManager;
import android.wifidisplay.IWifiDisplayManager;
import android.wifidisplay.IWifiDisplayThread;


/**
 * This manages the execution of the main thread in an
 * application process, scheduling and executing activities,
 * broadcasts, and other operations on it as the activity
 * manager requests.
 *
 * {@hide}
 */
public abstract class WifiDisplayManagerBase 
{
    /** @hide */
    public static final String TAG = "WifiDisplayManagerBase";
    static final boolean DEBUG_MESSAGES = false;
    static final boolean DEBUG_REMOTE = true;
    private IWindowManager 	mWindowManager;
    private IWifiDisplayManager mWifiDisplayManager;
    private IWifiDisplayThread.Stub mWifiDisplayThread = null;
    final H mH = new H();
    
    protected abstract void onExitWifiDisplayReceive();
    protected abstract void onExitWifiDisplaySend();
    protected abstract void onStartWifiDisplaySend();
    protected abstract void onDispatchMotionEvent(MotionEvent motionevent);
    
    public WifiDisplayManagerBase()
    {
    	mWindowManager	= IWindowManager.Stub.asInterface(ServiceManager.getService(Context.WINDOW_SERVICE));
    	mWifiDisplayManager	= IWifiDisplayManager.Stub.asInterface(ServiceManager.getService(Context.WIFIDISPLAY_SERVICE));	
    }
 
 	// if the thread hasn't started yet, we don't have the handler, so just
    // save the messages until we're ready.
    private void queueOrSendMessage(int what, Object obj) 
    {
        queueOrSendMessage(what, obj, 0, 0);
    }

    private void queueOrSendMessage(int what, Object obj, int arg1) 
    {
        queueOrSendMessage(what, obj, arg1, 0);
    }

    private void queueOrSendMessage(int what, Object obj, int arg1, int arg2) 
    {
        synchronized (this) 
        {
            if (DEBUG_MESSAGES) Slog.v(
                TAG, "SCHEDULE " + what + " " + mH.codeToString(what)
                + ": " + arg1 + " / " + obj);
            Message msg = Message.obtain();
            msg.what = what;
            msg.obj = obj;
            msg.arg1 = arg1;
            msg.arg2 = arg2;
            mH.sendMessage(msg);
        }
    }
    
    private class WifiDisplayThread extends IWifiDisplayThread.Stub
    {
        public final void exitWifiDisplayReceive()
        {
            queueOrSendMessage(H.EXIT_WIFIDISPLAY_RECEIVE,null);
        }
        
        public final void exitWifiDisplaySend()
        {
            queueOrSendMessage(H.EXIT_WIFIDISPLAY_SEND,null);
        }
        
        public final void startWifiDisplaySend()
        {
            queueOrSendMessage(H.START_WIFIDISPLAY_SEND,null);
        }
        
        public final void dispatchMotionEvent(MotionEvent motionevent)
        {
            queueOrSendMessage(H.DISPATCH_WIFIDISPLAY_EVEVT,motionevent);
        }
    }

    private class H extends Handler 
    {
        public static final int EXIT_WIFIDISPLAY_RECEIVE         = 100;
        public static final int EXIT_WIFIDISPLAY_SEND         	 = 101;
        public static final int START_WIFIDISPLAY_SEND         	 = 102;
        public static final int	DISPATCH_WIFIDISPLAY_EVEVT		 = 103;
        String codeToString(int code) 
        {
            if (DEBUG_MESSAGES) 
            {
                switch (code) 
                {
                    case EXIT_WIFIDISPLAY_RECEIVE: return "EXIT_WIFIDISPLAY_RECEIVE";
                    case EXIT_WIFIDISPLAY_SEND: return "EXIT_WIFIDISPLAY_SEND";
                    case START_WIFIDISPLAY_SEND: return "START_WIFIDISPLAY_SEND";
                    case DISPATCH_WIFIDISPLAY_EVEVT: return "DISPATCH_WIFIDISPLAY_EVEVT";
                }
            }
            return "(unknown)";
        }
        
        public void handleMessage(Message msg) 
        {
            if (DEBUG_MESSAGES) Slog.v(TAG, ">>> handling: " + msg.what);
            switch (msg.what) 
            {
                case EXIT_WIFIDISPLAY_RECEIVE: 
                {
                    onExitWifiDisplayReceive();
                } 
                break;
                
                case EXIT_WIFIDISPLAY_SEND: 
                {
                    onExitWifiDisplaySend();
                } 
                break;
                
                case START_WIFIDISPLAY_SEND: 
                {
                    onStartWifiDisplaySend();
                } 
                
                case DISPATCH_WIFIDISPLAY_EVEVT:
                {
                	onDispatchMotionEvent((MotionEvent)msg.obj);
                }
                break;
            }
            
            if (DEBUG_MESSAGES) Slog.v(TAG, "<<< done: " + msg.what);
        }
	}  
	
	public int startWifiDisplaySend()
	{
		try 
        {
            if(mWifiDisplayThread == null)
            {
            	mWifiDisplayThread	= new WifiDisplayThread();
            	
            	mWifiDisplayManager.addWifiDisplayClient(mWifiDisplayThread);
        	}
        	
        	if (DEBUG_REMOTE) Slog.v(
                TAG, "startWifiDisplaySend  mWifiDisplayThread = " + mWifiDisplayThread);
            mWifiDisplayManager.startWifiDisplaySend(mWifiDisplayThread);
		
			return 0;
        } 
        catch (RemoteException ex) 
        {
            return -1;
        }
	}  
	
	public int startWifiDisplayReceive()
	{
		try 
        {
            if(mWifiDisplayThread == null)
            {
            	mWifiDisplayThread	= new WifiDisplayThread();
            	
            	mWifiDisplayManager.addWifiDisplayClient(mWifiDisplayThread);
        	}
        	
        	if (DEBUG_REMOTE) Slog.v(
                TAG, "startWifiDisplayReceive  mWifiDisplayThread = " + mWifiDisplayThread);
        	
            mWifiDisplayManager.startWifiDisplayReceive(mWifiDisplayThread);
		
			return 0;
        } 
        catch (RemoteException ex) 
        {
            return -1;
        }
	}
	
	public int endWifiDisplaySend()
	{
		try 
        {
            if(mWifiDisplayThread == null)
            {
            	mWifiDisplayThread	= new WifiDisplayThread();
            	
            	mWifiDisplayManager.addWifiDisplayClient(mWifiDisplayThread);
        	}
        	
        	if (DEBUG_REMOTE) Slog.v(
                TAG, "endWifiDisplaySend  mWifiDisplayThread = " + mWifiDisplayThread);
                
            mWifiDisplayManager.endWifiDisplaySend(mWifiDisplayThread);
		
			return 0;
        } 
        catch (RemoteException ex) 
        {
            return -1;
        }
	}
	
	public int endWifiDisplayReceive()
	{
		try 
        {
        	if (DEBUG_REMOTE) Slog.v(
                TAG, "endWifiDisplayReceive ");
            if(mWifiDisplayThread == null)
            {
            	mWifiDisplayThread	= new WifiDisplayThread();
            	
            	mWifiDisplayManager.addWifiDisplayClient(mWifiDisplayThread);
        	}
        	
        	if (DEBUG_REMOTE) Slog.v(
                TAG, "endWifiDisplayReceive  mWifiDisplayThread = " + mWifiDisplayThread);
                
            mWifiDisplayManager.endWifiDisplayReceive(mWifiDisplayThread);
		
			return 0;
        } 
        catch (RemoteException ex) 
        {
            return -1;
        }
	}
}
