package com.android.settings;

import java.io.BufferedOutputStream;
import java.io.BufferedReader;
import java.io.File;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.IOException;
import java.util.List;
import java.util.regex.Pattern;

import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.net.ethernet.EthernetDevInfo;
import android.net.ethernet.EthernetManager;
import android.os.Bundle;
import android.preference.CheckBoxPreference;
import android.preference.EditTextPreference;
import android.preference.ListPreference;
import android.preference.Preference;
import android.provider.Settings;
import android.util.Log;

public class PPPoESettings extends SettingsPreferenceFragment implements
		Preference.OnPreferenceChangeListener {

	static private final String TAG = "PPPOE_SETTING";
	static private final boolean DEBUG = false;
	static private final String PPPOE_INFO_SAVE_FILE = "/data/system/pap-secrets";
	static private final int MAX_INFO_LENGTH = 128;
	static private final String KEY_DEVICES = "devices";
	static private final String KEY_USERNAME = "username";
	static private final String KEY_PASSWORD = "password";
	static private final String KEY_AUTO_CONNECTING = "auto_connecting";
	static private final String KEY_TOGGLE = "toggle";

	private ListPreference mDevices;
	private EditTextPreference mUser;
	private EditTextPreference mPassword;
	private CheckBoxPreference mAutoConecting;
	private CheckBoxPreference mToggle;

	private String mLoginFormat = "\"%s\" * \"%s\"";
	private String mLoginUsername;
	private String mLoginPassword;

	private EthernetManager mEthernetManager;

	private boolean mDateChanged = false;

	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		addPreferencesFromResource(R.xml.pppoe_settings);
		mDevices = (ListPreference) findPreference(KEY_DEVICES);

		// add ethernet interface
		mEthernetManager = EthernetManager.getInstance();
		List<EthernetDevInfo> devices = mEthernetManager.getDeviceNameList();
		String entries[] = new String[0];
		String entriesValues[] = new String[0];
		;
		if (devices != null) {
			entries = new String[devices.size()];
			entriesValues = new String[devices.size()];
			for (int i = 0; i < devices.size(); i++) {
				entries[i] = devices.get(i).getIfName();
				entriesValues[i] = devices.get(i).getIfName();
			}
		}
		// add wifi interface
		entries = addStringArray(entries, getWifiInterface());
		entriesValues = addStringArray(entriesValues, getWifiInterface());
		mDevices.setEntries(entries);
		mDevices.setEntryValues(entriesValues);
		mDevices.setOnPreferenceChangeListener(this);

		// set devices list value
		String iface = Settings.Secure.getString(getContentResolver(),
				Settings.Secure.PPPOE_INTERFACE);
		for (int i = 0; i < entriesValues.length; i++) {
			if (iface != null && iface.equals(entriesValues[i])) {
				mDevices.setValue(iface);
				break;
			}
		}

		// set username value
		String loginInfo = readLoginInfo();
		String temp[] = new String[0];
		if (loginInfo != null) {
			temp = loginInfo.split(Pattern.quote("*"));
		}
		if (temp.length == 2) {
			temp[0] = temp[0].replace('\"', ' ');
			temp[1] = temp[1].replace('\"', ' ');
			mLoginUsername = temp[0].trim();
			mLoginPassword = temp[1].trim();
			if (DEBUG) {
				Log.d(TAG, "username = " + mLoginUsername + " password = "
						+ mLoginPassword);
			}
		}
		mUser = (EditTextPreference) findPreference(KEY_USERNAME);
		mUser.setOnPreferenceChangeListener(this);
		mUser.setText(mLoginUsername);
		mPassword = (EditTextPreference) findPreference(KEY_PASSWORD);
		mPassword.setOnPreferenceChangeListener(this);
		mAutoConecting = (CheckBoxPreference) findPreference(KEY_AUTO_CONNECTING);
		mAutoConecting.setOnPreferenceChangeListener(this);
		mToggle = (CheckBoxPreference) findPreference(KEY_TOGGLE);
		mToggle.setOnPreferenceChangeListener(this);
		try {
			boolean toggle = Settings.Secure.getInt(getContentResolver(),
					Settings.Secure.PPPOE_ENABLE) != 0 ? true : false;
			boolean autoConn = Settings.Secure.getInt(getContentResolver(),
					Settings.Secure.PPPOE_AUTO_CONN) != 0 ? true : false;
			mAutoConecting.setChecked(autoConn);
			mToggle.setChecked(toggle);
		} catch (Exception e) {
			e.printStackTrace();
		}
	}

	private String[] addStringArray(String[] str1, String[] str2) {
		String[] result = new String[str1.length + str2.length];
		for (int i = 0; i < str1.length; i++) {
			result[i] = str1[i];
		}
		for (int i = str1.length; i < str1.length + str2.length; i++) {
			result[i] = str2[i - str1.length];
		}
		return result;
	}

	private String[] getWifiInterface() {
		String[] wifi = { "wlan0" };
		return wifi;
	}

	private String readLoginInfo() {
		File file = new File(PPPOE_INFO_SAVE_FILE);
		char[] buf = new char[MAX_INFO_LENGTH];
		String loginInfo = new String();
		FileReader in;
		try {
			in = new FileReader(file);
			BufferedReader bufferedreader = new BufferedReader(in);
			loginInfo = bufferedreader.readLine();
			if (DEBUG) {
				Log.d(TAG, "read form " + PPPOE_INFO_SAVE_FILE
						+ " login info = " + loginInfo);
			}
			bufferedreader.close();
			in.close();
		} catch (IOException e) {
			Log.w(TAG, "Read " + PPPOE_INFO_SAVE_FILE + " failed! " + e);
		}
		return loginInfo;
	}

	private boolean writeLoginInfo(String username, String password) {
		File file = new File(PPPOE_INFO_SAVE_FILE);
		String loginInfo = String.format(mLoginFormat, username, password);
		try {
			BufferedOutputStream out = new BufferedOutputStream(
					new FileOutputStream(file));
			out.write(loginInfo.getBytes(), 0, loginInfo.length());
			if (DEBUG) {
				Log.d(TAG, "write to " + PPPOE_INFO_SAVE_FILE
						+ " login info = " + loginInfo);
			}
			out.flush();
			out.close();
		} catch (IOException e) {
			Log.w(TAG, "Write " + PPPOE_INFO_SAVE_FILE + " failed! " + e);
			return false;
		}
		return true;
	}

	@Override
	public boolean onPreferenceChange(Preference preference, Object value) {
		mDateChanged = true;
		ContentResolver cr = this.getContentResolver();
		if (preference.equals(mDevices)) {
			Settings.Secure.putString(cr, Settings.Secure.PPPOE_INTERFACE,
					(String) value);
		} else if (preference.equals(mUser)) {
			mLoginUsername = (String) value;
			writeLoginInfo(mLoginUsername, mLoginPassword);
		} else if (preference.equals(mPassword)) {
			mLoginPassword = (String) value;
			writeLoginInfo(mLoginUsername, mLoginPassword);
		} else if (preference.equals(mAutoConecting)) {
			mAutoConecting.setChecked((Boolean) value);
			Settings.Secure.putInt(cr, Settings.Secure.PPPOE_AUTO_CONN,
					(Boolean) value ? 1 : 0);
		} else if (preference.equals(mToggle)) {
			mToggle.setChecked((Boolean) value);
			Settings.Secure.putInt(cr, Settings.Secure.PPPOE_ENABLE,
					(Boolean) value ? 1 : 0);
			getActivity().sendBroadcast(
					new Intent("com.softwinner.pppoe.ACTION_STATE_CHANGE"));
		}
		return true;
	}

}
