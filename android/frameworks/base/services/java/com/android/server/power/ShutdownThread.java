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

 
package com.android.server.power;

import android.app.ActivityManagerNative;
import android.app.Activity;
import android.app.ActivityManager;
import android.app.Application;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.IActivityManager;
import android.app.ProgressDialog;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.IBluetoothManager;
import android.nfc.NfcAdapter;
import android.nfc.INfcAdapter;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Handler;
import android.os.UserHandle;
import android.os.Message;
import android.os.PowerManager;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.os.SystemClock;
import android.os.SystemProperties;
import android.os.UserHandle;
import android.os.Vibrator;
import android.os.SystemVibrator;
import android.os.storage.IMountService;
import android.os.storage.IMountShutdownObserver;

import com.android.internal.telephony.ITelephony;
import android.provider.Settings;

import android.util.Log;
import android.view.WindowManager;
import android.view.Surface;

import android.media.AmrInputStream;
import android.media.MediaPlayer;
import android.view.IWindowManager;
import android.view.WindowManagerPolicy;
import java.util.ArrayList;
import java.util.List;
import java.io.IOException;
import java.lang.InterruptedException;
import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.io.InputStream;
import java.lang.StringBuffer;
import dalvik.system.Zygote;

public final class ShutdownThread extends Thread {
    // constants
    private static final String TAG = "ShutdownThread";
    private static final int PHONE_STATE_POLL_SLEEP_MSEC = 500;
    // maximum time we wait for the shutdown broadcast before going on.
    private static final int MAX_BROADCAST_TIME = 10*1000;
    private static final int MAX_SHUTDOWN_WAIT_TIME = 20*1000;
    private static final int MAX_RADIO_WAIT_TIME = 12*1000;

    // length of vibration before shutting down
    private static final int SHUTDOWN_VIBRATE_MS = 500;
    
    private static final int CLOSE_PROCESS_DIALOG = 2;
    
    private static final int MAX_SERVICES = 100;
    
    private static final int MAX_ACTIVITYS = 100;
    
    private static final int BOOTFAST_WAIT_TIME = 2000;
    // state tracking
    private static Object sIsStartedGuard = new Object();
    private static boolean sIsStarted = false;
    
    private static boolean mReboot;
    private static boolean mRebootSafeMode;
	private static WindowManagerPolicy mPolicy;
    private static String mRebootReason;

    // Provides shutdown assurance in case the system_server is killed
    public static final String SHUTDOWN_ACTION_PROPERTY = "sys.shutdown.requested";

    // Indicates whether we are rebooting into safe mode
    public static final String REBOOT_SAFEMODE_PROPERTY = "persist.sys.safemode";

    // static instance of this thread
    private static ShutdownThread sInstance;
    
    private final Object mActionDoneSync = new Object();
    private boolean mActionDone;
    private Context mContext;
    private PowerManager mPowerManager;
    private PowerManager.WakeLock mCpuWakeLock;
    private PowerManager.WakeLock mScreenWakeLock;
    private Handler mHandler;

    private static boolean mBootFastEnable = false;

    private static AlertDialog sConfirmDialog;
    
    private ShutdownThread() {
    }
 
    /**
     * Request a clean shutdown, waiting for subsystems to clean up their
     * state etc.  Must be called from a Looper thread in which its UI
     * is shown.
     *
     * @param context Context used to display the shutdown progress dialog.
     * @param confirm true if user confirmation is needed before shutting down.
     */
    public static void shutdown(final Context context, boolean confirm) {
        mReboot = false;
        mRebootSafeMode = false;
        shutdownInner(context, confirm);
    }

	public static void shutdown(final Context context, boolean confirm,WindowManagerPolicy policy){
		mReboot = false;
        mRebootSafeMode = false;
		mPolicy = policy;
        shutdownInner(context, confirm);
    }

    static void shutdownInner(final Context context, boolean confirm) {
        // ensure that only one thread is trying to power down.
        // any additional calls are just returned
        synchronized (sIsStartedGuard) {
            if (sIsStarted) {
                Log.d(TAG, "Request to shutdown already running, returning.");
                return;
            }
        }

        final int longPressBehavior = context.getResources().getInteger(
                        com.android.internal.R.integer.config_longPressOnPowerBehavior);
        final int resourceId = mRebootSafeMode
                ? com.android.internal.R.string.reboot_safemode_confirm
                : (longPressBehavior == 2
                        ? com.android.internal.R.string.shutdown_confirm_question
                        : com.android.internal.R.string.shutdown_confirm);

        Log.d(TAG, "Notifying thread to start shutdown longPressBehavior=" + longPressBehavior);

        if (confirm) {
            final CloseDialogReceiver closer = new CloseDialogReceiver(context);
            if (sConfirmDialog != null) {
                sConfirmDialog.dismiss();
            }
			if(mRebootSafeMode == true){
            	sConfirmDialog = new AlertDialog.Builder(context)
                    .setTitle(mRebootSafeMode
                            ? com.android.internal.R.string.reboot_safemode_title
                            : com.android.internal.R.string.power_off)
                    .setMessage(resourceId)
                    .setPositiveButton(com.android.internal.R.string.yes, new DialogInterface.OnClickListener() {
                        public void onClick(DialogInterface dialog, int which) {
                            beginShutdownSequence(context);
                        }
                    })
                    .setNegativeButton(com.android.internal.R.string.no, null)
                    .create();
			}else{
				if(Settings.Global.getInt(context.getContentResolver(), Settings.Global.DEVICE_PROVISIONED, 1)==1 &&
					SystemProperties.getBoolean("ro.sys.bootfast", false)){
					boolean [] enableBootFast = {false};
					enableBootFast[0] = Settings.System.getIntForUser(context.getContentResolver(), Settings.System.BOOT_FAST_ENABLE, 0,UserHandle.USER_CURRENT)==0?false:true;
					sConfirmDialog = new AlertDialog.Builder(context)
                    	.setTitle(mRebootSafeMode
                            ? com.android.internal.R.string.reboot_safemode_title
                            : com.android.internal.R.string.power_off)
                    	.setMultiChoiceItems(com.android.internal.R.array.quick_boot_mode,enableBootFast,new DialogInterface.OnMultiChoiceClickListener(){
                    	public void onClick(DialogInterface dialog, int which, boolean isChecked) {
							Log.d(TAG,"which = "+ which + "isChecked = " + isChecked);
							if(which==0){
								if(isChecked){
									Settings.System.putIntForUser(context.getContentResolver(), Settings.System.BOOT_FAST_ENABLE, 1,UserHandle.USER_CURRENT);
								}else{
									Settings.System.putIntForUser(context.getContentResolver(), Settings.System.BOOT_FAST_ENABLE, 0,UserHandle.USER_CURRENT);
								}
							}
                    	}
                    	})
                    	.setPositiveButton(com.android.internal.R.string.yes, new DialogInterface.OnClickListener() {
                        public void onClick(DialogInterface dialog, int which) {
							if(mPolicy!=null)
								mPolicy.acquireBAView();
                            beginShutdownSequence(context);
                        }
                    	})
                    	.setNegativeButton(com.android.internal.R.string.no, null)
                    	.create();
				} else{
					sConfirmDialog = new AlertDialog.Builder(context)
                    .setTitle(mRebootSafeMode
                            ? com.android.internal.R.string.reboot_safemode_title
                            : com.android.internal.R.string.power_off)
                    .setMessage(resourceId)
                    .setPositiveButton(com.android.internal.R.string.yes, new DialogInterface.OnClickListener() {
                        public void onClick(DialogInterface dialog, int which) {
                            beginShutdownSequence(context);
                        }
                    })
                    .setNegativeButton(com.android.internal.R.string.no, null)
                    .create();
				}
			}
            closer.dialog = sConfirmDialog;
            sConfirmDialog.setOnDismissListener(closer);
            sConfirmDialog.getWindow().setType(WindowManager.LayoutParams.TYPE_KEYGUARD_DIALOG);
            sConfirmDialog.show();
        } else {
            beginShutdownSequence(context);
        }
    }

    private static class CloseDialogReceiver extends BroadcastReceiver
            implements DialogInterface.OnDismissListener {
        private Context mContext;
        public Dialog dialog;

        CloseDialogReceiver(Context context) {
            mContext = context;
            IntentFilter filter = new IntentFilter(Intent.ACTION_CLOSE_SYSTEM_DIALOGS);
            context.registerReceiver(this, filter);
        }

        @Override
        public void onReceive(Context context, Intent intent) {
            dialog.cancel();
        }

        public void onDismiss(DialogInterface unused) {
            mContext.unregisterReceiver(this);
        }
    }

    /**
     * Request a clean shutdown, waiting for subsystems to clean up their
     * state etc.  Must be called from a Looper thread in which its UI
     * is shown.
     *
     * @param context Context used to display the shutdown progress dialog.
     * @param reason code to pass to the kernel (e.g. "recovery"), or null.
     * @param confirm true if user confirmation is needed before shutting down.
     */
    public static void reboot(final Context context, String reason, boolean confirm) {
        mReboot = true;
        mRebootSafeMode = false;
        mRebootReason = reason;
		if(reason!=null){
			Log.d(TAG,"reboot reason is " + mRebootReason);
		}
        shutdownInner(context, confirm);
    }

    /**
     * Request a reboot into safe mode.  Must be called from a Looper thread in which its UI
     * is shown.
     *
     * @param context Context used to display the shutdown progress dialog.
     * @param confirm true if user confirmation is needed before shutting down.
     */
    public static void rebootSafeMode(final Context context, boolean confirm) {
        mReboot = true;
        mRebootSafeMode = true;
        mRebootReason = null;
        shutdownInner(context, confirm);
    }

		 private void saveAirplaneModeState(boolean enabled) {
		 	boolean lastState = Settings.Global.getInt(mContext.getContentResolver(), Settings.Global.AIRPLANE_MODE_ON_FAST_BOOT,0) == 1 ? true : false;
			if (lastState == enabled) {
				return;
			}
			Settings.Global.putInt(mContext.getContentResolver(), Settings.Global.AIRPLANE_MODE_ON_FAST_BOOT,
                                enabled ? 1 : 0);
		 }
		 
		 private void enableAirplaneModeState() {
		 	Log.d(TAG,"enableAirplaneModeState");
			boolean enable = Settings.Global.getInt(mContext.getContentResolver(), Settings.Global.AIRPLANE_MODE_ON,0) == 1 ? true : false;
			saveAirplaneModeState(enable);
			if(enable) {
				Log.d(TAG,"already enable airplane mode");
				return;
			}
			Settings.Global.putInt(mContext.getContentResolver(), Settings.Global.AIRPLANE_MODE_ON, 1);
			 // Post the intent
			 Intent intent = new Intent(Intent.ACTION_AIRPLANE_MODE_CHANGED);
			 intent.putExtra("state", enable);
			 mContext.sendBroadcast(intent);
		 }
		 private void setAirplaneModeState(boolean enabled) {
			 // TODO: Sets the view to be "awaiting" if not already awaiting
	
			Log.d(TAG,"setAirplaneModeState = " + enabled);
			if(Settings.Global.getInt(mContext.getContentResolver(), Settings.Global.AIRPLANE_MODE_ON,0)==1){
				Log.d(TAG,"already in Airplane mode return");
				return;
			}
			Settings.Global.putInt(mContext.getContentResolver(), Settings.Global.AIRPLANE_MODE_ON,
                                enabled ? 1 : 0);
			 // Post the intent
			 Intent intent = new Intent(Intent.ACTION_AIRPLANE_MODE_CHANGED);
			 intent.putExtra("state", enabled);
			 mContext.sendBroadcast(intent);
		 }

    private static void beginShutdownSequence(Context context) {
        synchronized (sIsStartedGuard) {
            if (sIsStarted) {
                Log.d(TAG, "Shutdown sequence already running, returning.");
                return;
            }
            sIsStarted = true;
        }
        
        MediaPlayer mediaplayer = new MediaPlayer();
        try{
                       
           mediaplayer.setDataSource("/system/media/shutdown.mp3");
           mediaplayer.prepare();
           mediaplayer.setLooping(true);
           mediaplayer.start();
        }catch (IOException e){
        	
        } 
		SystemProperties.set("sys.start_shutdown", "1");
		
		if(SystemProperties.getBoolean("ro.sys.bootfast", false)&&(1==Settings.System.getIntForUser(context.getContentResolver(), Settings.System.BOOT_FAST_ENABLE, 0,UserHandle.USER_CURRENT))){
            mBootFastEnable = true;
        }else{
            mBootFastEnable = false;
        }
		if(mReboot){
			mBootFastEnable = false;
			Log.d(TAG,"reboot!");
		}
		if(mRebootSafeMode){
			mBootFastEnable = false;
			Log.d(TAG,"Go Into Safe Mode real reboot");
		}
		if(Zygote.systemInSafeMode){
			mBootFastEnable = false;
			Log.d(TAG,"In Safe Mode real reboot");
		}
		if(mRebootReason!=null){
			mBootFastEnable = false;
			Log.d(TAG,"have reason " + mRebootReason + "real reboot");
		}
		if(SystemProperties.getInt("sys.battery_zero",0)==1){
			mBootFastEnable = false;
			Log.d(TAG,"Battery to low we really shutdown!");
		}
		if(SystemProperties.getInt("sys.temperature_high",0)==1){
			mBootFastEnable = false;
			Log.d(TAG,"temperature high we relly shutdown!");
		}

        // throw up an indeterminate system dialog to indicate radio is
        // shutting down.
        final ProgressDialog pd = new ProgressDialog(context);
        pd.setTitle(context.getText(com.android.internal.R.string.power_off));
        pd.setMessage(context.getText(com.android.internal.R.string.shutdown_progress));
        pd.setIndeterminate(true);
        pd.setCancelable(false);
        pd.getWindow().setType(WindowManager.LayoutParams.TYPE_KEYGUARD_DIALOG);

        pd.show();

		sInstance = new ShutdownThread();
        sInstance.mContext = context;
        sInstance.mPowerManager = (PowerManager)context.getSystemService(Context.POWER_SERVICE);

        // make sure we never fall asleep again
        sInstance.mCpuWakeLock = null;
        try {
            sInstance.mCpuWakeLock = sInstance.mPowerManager.newWakeLock(
                    PowerManager.PARTIAL_WAKE_LOCK, TAG + "-cpu");
            sInstance.mCpuWakeLock.setReferenceCounted(false);
            sInstance.mCpuWakeLock.acquire();
        } catch (SecurityException e) {
            Log.w(TAG, "No permission to acquire wake lock", e);
            sInstance.mCpuWakeLock = null;
        }

        // also make sure the screen stays on for better user experience
        sInstance.mScreenWakeLock = null;
        if (sInstance.mPowerManager.isScreenOn()) {
            try {
                sInstance.mScreenWakeLock = sInstance.mPowerManager.newWakeLock(
                        PowerManager.FULL_WAKE_LOCK, TAG + "-screen");
                sInstance.mScreenWakeLock.setReferenceCounted(false);
                sInstance.mScreenWakeLock.acquire();
            } catch (SecurityException e) {
                Log.w(TAG, "No permission to acquire wake lock", e);
                sInstance.mScreenWakeLock = null;
            }
        }

        // start the thread that initiates shutdown
        sInstance.mHandler = new Handler() {
        	@Override
        	public void handleMessage(Message msg){
        		switch(msg.what) {
        			case  CLOSE_PROCESS_DIALOG:
        					Log.v(TAG,"close process dialog now");
        					pd.dismiss();
        					break;
        		}
        	}
        };
        sInstance.start();
    }

    void actionDone() {
        synchronized (mActionDoneSync) {
            mActionDone = true;
            mActionDoneSync.notifyAll();
        }
    }

    /**
     * Makes sure we handle the shutdown gracefully.
     * Shuts off power regardless of radio and bluetooth state if the alloted time has passed.
     */
    public void run() {
        BroadcastReceiver br = new BroadcastReceiver() {
            @Override public void onReceive(Context context, Intent intent) {
                // We don't allow apps to cancel this, so ignore the result.
                actionDone();
            }
        };
        if(!mBootFastEnable){
        /*
         * Write a system property in case the system_server reboots before we
         * get to the actual hardware restart. If that happens, we'll retry at
         * the beginning of the SystemServer startup.
         */
        {
            String reason = (mReboot ? "1" : "0") + (mRebootReason != null ? mRebootReason : "");
            SystemProperties.set(SHUTDOWN_ACTION_PROPERTY, reason);
        }

        /*
         * If we are rebooting into safe mode, write a system property
         * indicating so.
         */
        if (mRebootSafeMode) {
            SystemProperties.set(REBOOT_SAFEMODE_PROPERTY, "1");
        }

		killRemoveActivity(mContext);
    	killRemoveService(mContext);
        Log.i(TAG, "Sending shutdown broadcast...");
        
        // First send the high-level shut down broadcast.
        mActionDone = false;
        mContext.sendOrderedBroadcastAsUser(new Intent(Intent.ACTION_SHUTDOWN),
                UserHandle.ALL, null, br, mHandler, 0, null, null);
        
        final long endTime = SystemClock.elapsedRealtime() + MAX_BROADCAST_TIME;
        synchronized (mActionDoneSync) {
            while (!mActionDone) {
                long delay = endTime - SystemClock.elapsedRealtime();
                if (delay <= 0) {
                    Log.w(TAG, "Shutdown broadcast timed out");
                    break;
                }
                try {
                    mActionDoneSync.wait(delay);
                } catch (InterruptedException e) {
                }
            }
        }
        
        Log.i(TAG, "Shutting down activity manager...");
        
        final IActivityManager am =
            ActivityManagerNative.asInterface(ServiceManager.checkService("activity"));
        if (am != null) {
            try {
                am.shutdown(MAX_BROADCAST_TIME);
            } catch (RemoteException e) {
            }
        }

        // Shutdown radios.
        //shutdownRadios(MAX_RADIO_WAIT_TIME);
        if(SystemProperties.get("ro.sw.embeded.telephony").equals("true")) {
        	shutdownRadios(MAX_RADIO_WAIT_TIME);
        }

        // Shutdown MountService to ensure media is in a safe state
        IMountShutdownObserver observer = new IMountShutdownObserver.Stub() {
            public void onShutDownComplete(int statusCode) throws RemoteException {
                Log.w(TAG, "Result code " + statusCode + " from MountService.shutdown");
                actionDone();
            }
        };

        Log.i(TAG, "Shutting down MountService");

        // Set initial variables and time out time.
        mActionDone = false;
        final long endShutTime = SystemClock.elapsedRealtime() + MAX_SHUTDOWN_WAIT_TIME;
        synchronized (mActionDoneSync) {
            try {
                final IMountService mount = IMountService.Stub.asInterface(
                        ServiceManager.checkService("mount"));
                if (mount != null) {
                    mount.shutdown(observer);
                } else {
                    Log.w(TAG, "MountService unavailable for shutdown");
                }
            } catch (Exception e) {
                Log.e(TAG, "Exception during MountService shutdown", e);
            }
            while (!mActionDone) {
                long delay = endShutTime - SystemClock.elapsedRealtime();
                if (delay <= 0) {
                    Log.w(TAG, "Shutdown wait timed out");
                    break;
                }
                try {
                    mActionDoneSync.wait(delay);
                } catch (InterruptedException e) {
                }
            }
        }

        rebootOrShutdown(mReboot, mRebootReason);
        }
		if (SHUTDOWN_VIBRATE_MS > 0) {
            // vibrate before shutting down
            Vibrator vibrator = new SystemVibrator();
            try {
                vibrator.vibrate(SHUTDOWN_VIBRATE_MS);
            } catch (Exception e) {
                // Failure to vibrate shouldn't interrupt shutdown.  Just log it.
                Log.w(TAG, "Failed to vibrate during shutdown.", e);
            }
		}
		IWindowManager 	mWindowManager;
		mWindowManager = IWindowManager.Stub.asInterface(ServiceManager.getService("window"));
		if(mWindowManager != null){
			try{
                mWindowManager.freezeRotation(Surface.ROTATION_0);
                mWindowManager.updateRotation(true, true);
                mWindowManager.setEventDispatching(false);
            }catch (RemoteException e) {
            }
		}
		//shutdownRadios(MAX_RADIO_WAIT_TIME);
		//setAirplaneModeState(true); //set airplane mode
		enableAirplaneModeState();
    	killRemoveActivity(mContext);
    	killRemoveService(mContext);
		if (MobileDirectController.getInstance().isMobileModeAvailable()) {
			MobileDirectController.getInstance().setNetworkEnable(false);
		}

		SystemClock.sleep(BOOTFAST_WAIT_TIME);
		
		sIsStarted = false;
		try{
			sInstance.mCpuWakeLock.release();
			sInstance.mScreenWakeLock.release();
		}catch(SecurityException e){
			SystemProperties.set("sys.start_shutdown", "0");
		}
		sInstance.mHandler.sendEmptyMessage(CLOSE_PROCESS_DIALOG);
		Log.v(TAG,"CLOSE_PROCESS_DIALOG");
		SystemProperties.set("sys.start_shutdown", "0");
		sInstance.mPowerManager.goToBootFastSleep(SystemClock.uptimeMillis());
    }
	private  void killRemoveActivity(Context context)
    {
    	ActivityManager am = (ActivityManager)context.getSystemService(Context.ACTIVITY_SERVICE);
    	List<ActivityManager.RecentTaskInfo> recents = am.getRecentTasks(MAX_ACTIVITYS,ActivityManager.RECENT_WITH_EXCLUDED);
       if(recents != null)
         {
         		Log.v(TAG,"Task Size is " + recents.size());
         		for(int i=0;i<recents.size();i++)
         		{
         				ActivityManager.RecentTaskInfo task = recents.get(i);
                		ActivityManager.TaskThumbnails thumbs = am.getTaskThumbnails(task.persistentId);
                		if (task != null) 
                		{
                			if(task.persistentId > 0)
                			{
                				if(!task. baseIntent.getComponent().getPackageName().equals("com.android.launcher"))
                				{
                    					am.removeTask(task.persistentId, ActivityManager.REMOVE_TASK_KILL_PROCESS);
                     				Log.v(TAG,"remove a task " + task.baseIntent.getComponent().getPackageName());
                     				if(thumbs.numSubThumbbails > 0)
                     					{
                     					for(int j=0;j<thumbs.numSubThumbbails;j++)
                     						{
                    		 						am.removeSubTask(task.persistentId, j);
                    		 						Log.v(TAG,"remove a sub task ");
                     						}
                    					}
                				}
                			}
                		}
             	}
         }  
    }

	private void killRemoveService(Context context){
		 ActivityManager am = (ActivityManager)context.getSystemService(Context.ACTIVITY_SERVICE);
        List<ActivityManager.RunningServiceInfo> services = am.getRunningServices(MAX_SERVICES);
        int NS = services != null ? services.size() : 0;
        for (int i=0; i<NS; i++) {
            ActivityManager.RunningServiceInfo si = services.get(i);
            // We are not interested in services that have not been started
            // and don't have a known client, because
            // there is nothing the user can do about them.
            if (!si.started && si.clientLabel == 0) {
                services.remove(i);
                i--;
                NS--;
                continue;
            }
            // We likewise don't care about services running in a
            // persistent process like the system or phone.
            if ((si.flags&ActivityManager.RunningServiceInfo.FLAG_PERSISTENT_PROCESS)
                    != 0) {
                services.remove(i);
                i--;
                NS--;
                continue;
            }
            Log.v(TAG,"service = " + services.get(i).process);
            context.stopService(new Intent().setComponent(services.get(i).service));
        }
    }

    private void shutdownRadios(int timeout) {
        // If a radio is wedged, disabling it may hang so we do this work in another thread,
        // just in case.
        final long endTime = SystemClock.elapsedRealtime() + timeout;
        final boolean[] done = new boolean[1];
        Thread t = new Thread() {
            public void run() {
                boolean nfcOff;
                boolean bluetoothOff;
                boolean radioOff;

                final INfcAdapter nfc =
                        INfcAdapter.Stub.asInterface(ServiceManager.checkService("nfc"));
                final ITelephony phone =
                        ITelephony.Stub.asInterface(ServiceManager.checkService("phone"));
                final IBluetoothManager bluetooth =
                        IBluetoothManager.Stub.asInterface(ServiceManager.checkService(
                                BluetoothAdapter.BLUETOOTH_MANAGER_SERVICE));

                try {
                    nfcOff = nfc == null ||
                             nfc.getState() == NfcAdapter.STATE_OFF;
                    if (!nfcOff) {
                        Log.w(TAG, "Turning off NFC...");
                        nfc.disable(false); // Don't persist new state
                    }
                } catch (RemoteException ex) {
                Log.e(TAG, "RemoteException during NFC shutdown", ex);
                    nfcOff = true;
                }

                try {
                    bluetoothOff = bluetooth == null || !bluetooth.isEnabled();
                    if (!bluetoothOff) {
                        Log.w(TAG, "Disabling Bluetooth...");
                        bluetooth.disable(false);  // disable but don't persist new state
                    }
                } catch (RemoteException ex) {
                    Log.e(TAG, "RemoteException during bluetooth shutdown", ex);
                    bluetoothOff = true;
                }

                try {
                    radioOff = phone == null || !phone.isRadioOn();
                    if (!radioOff) {
                        Log.w(TAG, "Turning off radio...");
                        phone.setRadio(false);
                    }
                } catch (RemoteException ex) {
                    Log.e(TAG, "RemoteException during radio shutdown", ex);
                    radioOff = true;
                }

                Log.i(TAG, "Waiting for NFC, Bluetooth and Radio...");

                while (SystemClock.elapsedRealtime() < endTime) {
                    if (!bluetoothOff) {
                        try {
                            bluetoothOff = !bluetooth.isEnabled();
                        } catch (RemoteException ex) {
                            Log.e(TAG, "RemoteException during bluetooth shutdown", ex);
                            bluetoothOff = true;
                        }
                        if (bluetoothOff) {
                            Log.i(TAG, "Bluetooth turned off.");
                        }
                    }
                    if (!radioOff) {
                        try {
                            radioOff = !phone.isRadioOn();
                        } catch (RemoteException ex) {
                            Log.e(TAG, "RemoteException during radio shutdown", ex);
                            radioOff = true;
                        }
                        if (radioOff) {
                            Log.i(TAG, "Radio turned off.");
                        }
                    }
                    if (!nfcOff) {
                        try {
                            nfcOff = nfc.getState() == NfcAdapter.STATE_OFF;
                        } catch (RemoteException ex) {
                            Log.e(TAG, "RemoteException during NFC shutdown", ex);
                            nfcOff = true;
                        }
                        if (radioOff) {
                            Log.i(TAG, "NFC turned off.");
                        }
                    }

                    if (radioOff && bluetoothOff && nfcOff) {
                        Log.i(TAG, "NFC, Radio and Bluetooth shutdown complete.");
                        done[0] = true;
                        break;
                    }
                    SystemClock.sleep(PHONE_STATE_POLL_SLEEP_MSEC);
                }
            }
        };

        t.start();
        try {
            t.join(timeout);
        } catch (InterruptedException ex) {
        }
        if (!done[0]) {
            Log.w(TAG, "Timed out waiting for NFC, Radio and Bluetooth shutdown.");
        }
    }

    /**
     * Do not call this directly. Use {@link #reboot(Context, String, boolean)}
     * or {@link #shutdown(Context, boolean)} instead.
     *
     * @param reboot true to reboot or false to shutdown
     * @param reason reason for reboot
     */
    public static void rebootOrShutdown(boolean reboot, String reason) {
        if (reboot) {
            Log.i(TAG, "Rebooting, reason: " + reason);
            try {
                PowerManagerService.lowLevelReboot(reason);
            } catch (Exception e) {
                Log.e(TAG, "Reboot failed, will attempt shutdown instead", e);
            }
        } else if (SHUTDOWN_VIBRATE_MS > 0) {
            // vibrate before shutting down
            Vibrator vibrator = new SystemVibrator();
            try {
                vibrator.vibrate(SHUTDOWN_VIBRATE_MS);
            } catch (Exception e) {
                // Failure to vibrate shouldn't interrupt shutdown.  Just log it.
                Log.w(TAG, "Failed to vibrate during shutdown.", e);
            }

            // vibrator is asynchronous so we need to wait to avoid shutting down too soon.
            try {
                Thread.sleep(SHUTDOWN_VIBRATE_MS);
            } catch (InterruptedException unused) {
            }
        }

        // Shutdown power
        Log.i(TAG, "Performing low-level shutdown...");
        PowerManagerService.lowLevelShutdown();
    }
}
