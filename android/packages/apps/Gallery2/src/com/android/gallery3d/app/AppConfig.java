package com.android.gallery3d.app;

import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;

import android.content.Context;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.preference.PreferenceManager;
import android.util.Log;


public class AppConfig {
	private static AppConfig mInstance;
	private final Context mContext;
	private final SharedPreferences mConfigs;
	private final Editor mEditor; 
	
	public static final String TAG = "AppConfig";
	
	public static final String LAST_UPDATE_TIME="last_update_time";
         public static final String OLD_UPDATE_VERSION = "old_update_version";
	
	//external config
	public static final String FIRMWARE_VERSION = "firmware_ver";
	
	public static int mChipVersion;
	//private static final String TAG = "AppConfig";
	
	public static AppConfig getInstance(Context context) {
		if(mInstance == null) {
			mInstance = new AppConfig(context);
		}
		return mInstance;
	}
	
	private AppConfig(Context context) {
		mContext = context.getApplicationContext();
		mConfigs = PreferenceManager.getDefaultSharedPreferences(mContext);
		mEditor = mConfigs.edit();
	}

	public boolean getBoolean(String key, boolean def) {
		return mConfigs.getBoolean(key, def);
	}
	
	public void setBoolean(String key, boolean value) {
		mEditor.putBoolean(key, value);
		mEditor.commit();
	}
	
	public int getInt(String key, int def) {
		return mConfigs.getInt(key, def);
	}
	
	public void setInt(String key, int value) {
		mEditor.putInt(key, value);
		mEditor.commit();
	}	
	
	public String getString(String key, String def) {
		return mConfigs.getString(key, def);
	}

	public void setString(String key, String value) {
		mEditor.putString(key, value);
		mEditor.commit();
	}

	public Float getFloat(String key, float def) {
		return mConfigs.getFloat(key, def);
	}

	public void setFloat(String key, float value) {
		mEditor.putFloat(key, value);
		mEditor.commit();
	}

	public long getLong(String key, Long def) {
		return mConfigs.getLong(key, def);
	}

	public void setLong(String key, long value) {
		mEditor.putLong(key, value);
		mEditor.commit();
	}
}
