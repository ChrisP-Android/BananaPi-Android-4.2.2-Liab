/*
 * Copyright (C) 2007 The Android Open Source Project
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

package com.softwinner;

import java.io.IOException;
import android.os.ServiceManager;
import java.lang.Integer;
import java.lang.String;
import android.util.Log;

/**
 * Class that provides access to some of the gpio management functions.
 *
 * {@hide}
 */
public class Gpio
{
	public static final String TAG = "GPIO";

    // can't instantiate this class
    private Gpio()
    {
    }

	static {
		System.loadLibrary("gpio_jni");
		nativeInit();
	}

	private static native void nativeInit();
    private static native int nativeWriteGpio(String path, String value);
	private static native int nativeReadGpio(String path);

	private static final String  mPathstr      = "/sys/class/gpio_sw/P";
	private static final String  mDataName     = "/data";
	private static final String  mPullName     = "/pull";
	private static final String  mDrvLevelName = "/drv_level";
	private static final String  mMulSelName   = "/mul_sel";

	public static int writeGpio(char group, int num, int value){
	    String dataPath = composePinPath(group, num).concat(mDataName);

		return nativeWriteGpio(dataPath, Integer.toString(value));
	}

	public static int readGpio(char group, int num){
	    String dataPath = composePinPath(group, num).concat(mDataName);

		return nativeReadGpio(dataPath);
	}

	public static int setPull(char group, int num, int value){
	    String dataPath = composePinPath(group, num).concat(mPullName);

		return nativeWriteGpio(dataPath, Integer.toString(value));
	}

	public static int getPull(char group, int num){
	    String dataPath = composePinPath(group, num).concat(mPullName);

		return nativeReadGpio(dataPath);
	}

	public static int setDrvLevel(char group, int num, int value){
	    String dataPath = composePinPath(group, num).concat(mDrvLevelName);

		return nativeWriteGpio(dataPath, Integer.toString(value));
	}

	public static int getDrvLevel(char group, int num){
	    String dataPath = composePinPath(group, num).concat(mDrvLevelName);

		return nativeReadGpio(dataPath);
	}

	public static int setMulSel(char group, int num, int value){
	    String dataPath = composePinPath(group, num).concat(mMulSelName);

		return nativeWriteGpio(dataPath, Integer.toString(value));
	}

	public static int getMulSel(char group, int num){
	    String dataPath = composePinPath(group, num).concat(mMulSelName);

		return nativeReadGpio(dataPath);
	}

	private static String composePinPath(char group, int num){
		String  numstr;
		String  groupstr;

		groupstr = String.valueOf(group).toUpperCase();
		numstr = Integer.toString(num);
        return mPathstr.concat(groupstr).concat(numstr);
    }
}

