package com.android.server;

import java.io.BufferedReader;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.util.List;
import java.util.Iterator;

import android.R.bool;
import android.app.ActivityManager;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageManager;
import android.hardware.usb.UsbManager;
import android.os.BatteryManager;
import android.os.Handler;
import android.os.SystemClock;
import android.provider.Settings;
import android.database.ContentObserver;
import android.content.ContentResolver;
import android.net.Uri;
import android.util.Log;

public class DynamicPowerPolicyChangeThread extends Thread {

    // Broadcast
    private final String ACTION_WINDOW_ROTATION = "android.window.action.ROTATION";

    // app config file name
    private final String APP_DETECT_CONFIG_FILE_NAME = "app_list.conf";

    private final String CMD_SET_CPU_TO_EXTREMITY_POLICY = "echo extremity > /sys/class/sw_powernow/mode";

    private final String CMD_SET_CPU_TO_USER_EVENT_NOTIFY = "echo userevent > /sys/class/sw_powernow/mode";

    private final String CMD_SET_CPU_TO_PERFORMANCE_POLICY = "echo performance > /sys/class/sw_powernow/mode";

    private final String CMD_SET_CPU_TO_FANTASYS_POLICY = "echo fantasys > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor";

    private final String CMD_SET_CPU_TO_MAX_POWER_POLICY = "echo maxpower > /sys/class/sw_powernow/mode";

    private Context mContext;
    private ActivityManager mActivityManager;

    private boolean mIsNeedToSetWorkMode = false;

	public static boolean mGlobleLauncherRotateState = false;

    public DynamicPowerPolicyChangeThread(Context context) {
        mContext = context;
        mActivityManager = (ActivityManager) mContext.getSystemService(Context.ACTIVITY_SERVICE);

        // Anyway set fantasys policy
        setCPUWorkMode(CMD_SET_CPU_TO_FANTASYS_POLICY);

        setCPUWorkMode(CMD_SET_CPU_TO_PERFORMANCE_POLICY);

        IntentFilter intentFilter = new IntentFilter();
        intentFilter.addAction(ACTION_WINDOW_ROTATION);
        mContext.registerReceiver(mWindowRotationReceiver, intentFilter);
    }

    @Override
    protected void finalize() throws Throwable {
        mContext.unregisterReceiver(mWindowRotationReceiver);
        super.finalize();
    }

    private BroadcastReceiver mWindowRotationReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            setCPUWorkMode(CMD_SET_CPU_TO_MAX_POWER_POLICY);

            if (isLauncherApk()) {
                List l = mActivityManager.getRunningAppProcesses();
                Iterator i = l.iterator();
                PackageManager pm = mContext.getPackageManager();
                String processName = null;
                mGlobleLauncherRotateState = true;

                try {
                    processName = pm.getApplicationInfo("com.android.launcher", 0).processName;
                } catch (Exception e) {
                    e.printStackTrace();
                }

                while (i.hasNext()) {
                    ActivityManager.RunningAppProcessInfo info = (ActivityManager.RunningAppProcessInfo)(i.next());
                    try {
                        if (info.processName.equals(processName)) {
                            setCPUWorkMode("echo " + info.pid + " > /dev/cpuctl/tasks");
                            Log.d("mWindowRotationReceiver", "set pid ok!");
                            break;
                        }
                    } catch (Exception e) {
                        e.printStackTrace();
                    }
                }
            }
        }
    };

    @Override
    public void run() {
        super.run();

        while (true) {
            boolean isInBenchmarkMode =  isNeedToSetBenchmarkMode();
            if (isInBenchmarkMode && !mIsNeedToSetWorkMode) {
                setCPUWorkMode(CMD_SET_CPU_TO_EXTREMITY_POLICY);

                ComponentName componentName = mActivityManager.getRunningTasks(1).get(0).topActivity;
                String packageName = componentName.getPackageName();

                List l = mActivityManager.getRunningAppProcesses();
                Iterator i = l.iterator();
                PackageManager pm = mContext.getPackageManager();
                String processName = null;

                try {
                    processName = pm.getApplicationInfo(packageName, 0).processName;
                } catch (Exception e) {
                    e.printStackTrace();
                }

                while (i.hasNext()) {
                    ActivityManager.RunningAppProcessInfo info = (ActivityManager.RunningAppProcessInfo)(i.next());
                    try {
                        if (info.processName.equals(processName)) {
                            setCPUWorkMode("echo " + info.pid + " > /dev/cpuctl/tasks");
                            mIsNeedToSetWorkMode = true;
                            Log.d("task", "set pid ok!");
                            break;
                        }
                    } catch (Exception e) {
                        e.printStackTrace();
                    }
                }
            } else if (!isInBenchmarkMode && mIsNeedToSetWorkMode) {
                mIsNeedToSetWorkMode = false;
                setCPUWorkMode(CMD_SET_CPU_TO_PERFORMANCE_POLICY);
            }

            try {
                Thread.sleep(1500);
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        }
    }

    public boolean  isNeedToSetBenchmarkMode() {
        ComponentName componentName = mActivityManager.getRunningTasks(1).get(0).topActivity;
        String packageName = componentName.getPackageName();
        try {
            BufferedReader br = new BufferedReader(new InputStreamReader(mContext.getAssets().open(APP_DETECT_CONFIG_FILE_NAME)));
            String tmpStr = null;

            while ((tmpStr = br.readLine()) != null) {
                if (tmpStr.compareTo(packageName) == 0) {
                    return true;
                }
            }
        } catch (IOException e) {
            e.printStackTrace();
            return false;
        }
        return false;
    }

    public boolean  isLauncherApk() {
        ComponentName componentName = mActivityManager.getRunningTasks(1).get(0).topActivity;
        String packageName = componentName.getPackageName();
        String tmpStr = "com.android.launcher";

        if (tmpStr.compareTo(packageName) == 0) {
            return true;
        }

        return false;
    }

    public void setCPUWorkMode(String cmd) {
        executeShellCmd(cmd);
    }

    public void executeRootCmd(String cmd) {
        DataOutputStream dos = null;
        DataInputStream dis = null;
        try {
            Process p = Runtime.getRuntime().exec("su");
            dos = new DataOutputStream(p.getOutputStream());
            cmd += "\n";
            dos.writeBytes(cmd);
            dos.flush();
            dos.writeBytes("exit\n");
            dos.flush();
            p.waitFor();
        } catch (IOException e) {
            e.printStackTrace();
        } catch (InterruptedException e) {
            e.printStackTrace();
        } finally {
            try {
                if (dos != null)
                    dos.close();
                if (dis != null)
                    dis.close();
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
    }

    public String executeShellCmd(String cmd) {
        Process process = null;
        BufferedReader br = null;
        String s = null;
        String resultStr = "";
        try {
            process = Runtime.getRuntime().exec(
                          new String[] { "/system/bin/sh", "-c", cmd });
            br = new BufferedReader(new InputStreamReader(
                                        process.getInputStream()));
            while ((s = br.readLine()) != null) {
                if (s != null) {
                    resultStr += s;
                }
            }
        } catch (IOException e) {
            e.printStackTrace();
        } finally {
            if (process != null) {
                process.destroy();
            }
            process = null;
            if (br != null) {
                try {
                    br.close();
                } catch (Exception e) {
                }
                br = null;
            }
        }
        return resultStr;
    }
}
