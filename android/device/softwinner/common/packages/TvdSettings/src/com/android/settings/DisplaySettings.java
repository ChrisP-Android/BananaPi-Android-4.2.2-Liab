/*
 * Copyright (C) 2010 The Android Open Source Project
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

package com.android.settings;

import static android.provider.Settings.System.SCREEN_OFF_TIMEOUT;

import android.app.ActivityManagerNative;
import android.app.Dialog;
import android.app.AlertDialog;
import android.app.admin.DevicePolicyManager;
import android.content.BroadcastReceiver;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.DialogInterface;
import android.content.DialogInterface.OnClickListener;
import android.content.res.Configuration;
import android.content.res.Resources;
import android.media.AudioManager;
import android.hardware.display.DisplayManager;
import android.hardware.display.WifiDisplay;
import android.hardware.display.WifiDisplayStatus;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.RemoteException;
import android.preference.CheckBoxPreference;
import android.preference.ListPreference;
import android.preference.Preference;
import android.preference.Preference.OnPreferenceClickListener;
import android.preference.PreferenceScreen;
import android.provider.Settings;
import android.provider.Settings.SettingNotFoundException;
import android.util.AttributeSet;
import android.util.Log;

import com.android.internal.view.RotationPolicy;
import com.android.settings.DreamSettings;
import android.os.PowerManager;
import android.view.DisplayManagerAw;
import android.view.DispList;

import java.util.ArrayList;
import java.util.HashMap;

public class DisplaySettings extends SettingsPreferenceFragment implements
        Preference.OnPreferenceChangeListener, OnPreferenceClickListener {
    private static final String TAG = "DisplaySettings";

    /** If there is no setting in the provider, use this. */
    private static final int FALLBACK_SCREEN_TIMEOUT_VALUE = 30000;
    private static final int FALLBACK_DISPLAY_MODE_TIMEOUT = 10;
    private static final String DISPLAY_MODE_AUTO_KEY = "display_mode_auto";

    private static final String KEY_SCREEN_TIMEOUT = "screen_timeout";
    private static final String KEY_ACCELEROMETER = "accelerometer";
    private static final String KEY_FONT_SIZE = "font_size";
    private static final String KEY_NOTIFICATION_PULSE = "notification_pulse";
    private static final String KEY_SCREEN_SAVER = "screensaver";
	private static final String KEY_ACCELEROMETER_COORDINATE = "accelerometer_coornadite";
	private static final String KEY_SMART_BRIGHTNESS = "smart_brightness";
	private static final String KEY_SMART_BRIGHTNESS_PREVIEW = "key_smart_brightness_preview";
    private static final String KEY_WIFI_DISPLAY = "wifi_display";
    private static final String KEY_TV_OUTPUT_MODE = "display_output_mode";
    private static final int DLG_GLOBAL_CHANGE_WARNING = 1;

    private DisplayManager mDisplayManager;

    private CheckBoxPreference mAccelerometer;
    private WarnedListPreference mFontSizePref;
    private CheckBoxPreference mNotificationPulse;
	/*显示输出模式*/
	private ListPreference mOutputMode;
	private ArrayList<DispList.DispFormat> mOutputModeItems;

    private final Configuration mCurConfig = new Configuration();
    
    private ListPreference mScreenTimeoutPreference;
    private Preference mScreenSaverPreference;

	private ListPreference mAccelerometerCoordinate;

	private CheckBoxPreference mSmartBrightness;
	private CheckBoxPreference mSmartBrightnessPreview;
	
    private WifiDisplayStatus mWifiDisplayStatus;
    private Preference mWifiDisplayPreference;

    private final RotationPolicy.RotationPolicyListener mRotationPolicyListener =
            new RotationPolicy.RotationPolicyListener() {
        @Override
        public void onChange() {
            updateAccelerometerRotationCheckbox();
        }
    };

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        ContentResolver resolver = getActivity().getContentResolver();

        addPreferencesFromResource(R.xml.display_settings);

        mAccelerometer = (CheckBoxPreference) findPreference(KEY_ACCELEROMETER);
        mAccelerometer.setPersistent(false);
        if (RotationPolicy.isRotationLockToggleSupported(getActivity())) {
            // If rotation lock is supported, then we do not provide this option in
            // Display settings.  However, is still available in Accessibility settings.
            getPreferenceScreen().removePreference(mAccelerometer);
        }

        mScreenSaverPreference = findPreference(KEY_SCREEN_SAVER);
        if (mScreenSaverPreference != null
                && getResources().getBoolean(
                        com.android.internal.R.bool.config_dreamsSupported) == false) {
            getPreferenceScreen().removePreference(mScreenSaverPreference);
        }
        
        mScreenTimeoutPreference = (ListPreference) findPreference(KEY_SCREEN_TIMEOUT);
        final long currentTimeout = Settings.System.getLong(resolver, SCREEN_OFF_TIMEOUT,
                FALLBACK_SCREEN_TIMEOUT_VALUE);
        mScreenTimeoutPreference.setValue(String.valueOf(currentTimeout));
        mScreenTimeoutPreference.setOnPreferenceChangeListener(this);
        disableUnusableTimeouts(mScreenTimeoutPreference);
        updateTimeoutPreferenceDescription(currentTimeout);

        mFontSizePref = (WarnedListPreference) findPreference(KEY_FONT_SIZE);
        mFontSizePref.setOnPreferenceChangeListener(this);
        mFontSizePref.setOnPreferenceClickListener(this);
        mNotificationPulse = (CheckBoxPreference) findPreference(KEY_NOTIFICATION_PULSE);
        if (mNotificationPulse != null
                && getResources().getBoolean(
                        com.android.internal.R.bool.config_intrusiveNotificationLed) == false) {
            getPreferenceScreen().removePreference(mNotificationPulse);
        } else {
            try {
                mNotificationPulse.setChecked(Settings.System.getInt(resolver,
                        Settings.System.NOTIFICATION_LIGHT_PULSE) == 1);
                mNotificationPulse.setOnPreferenceChangeListener(this);
            } catch (SettingNotFoundException snfe) {
                Log.e(TAG, Settings.System.NOTIFICATION_LIGHT_PULSE + " not found");
            }
        }

		mAccelerometerCoordinate = (ListPreference) findPreference(KEY_ACCELEROMETER_COORDINATE);
		if(mAccelerometerCoordinate != null)
		{            
			mAccelerometerCoordinate.setOnPreferenceChangeListener(this);
			String value = Settings.System.getString(getContentResolver(),
				Settings.System.ACCELEROMETER_COORDINATE);
			if ( value == null )
			{
				value = "default";
			}
			
			mAccelerometerCoordinate.setValue(value);
			updateAccelerometerCoordinateSummary(value);
		}


		mSmartBrightnessPreview = new CheckBoxPreference(this.getActivity());
		mSmartBrightnessPreview.setTitle(R.string.smart_brightness_preview);
		mSmartBrightnessPreview.setKey(KEY_SMART_BRIGHTNESS_PREVIEW);
		mSmartBrightness = (CheckBoxPreference)findPreference(KEY_SMART_BRIGHTNESS);
		mSmartBrightness.setOnPreferenceChangeListener(this);
		if(!getResources().getBoolean(R.bool.has_smart_brightness))
		{            
			getPreferenceScreen().removePreference(mSmartBrightness);
		}
		else
		{            
			boolean enable = Settings.System.getInt(getContentResolver(),
				Settings.System.SMART_BRIGHTNESS_ENABLE,0) != 0 ? true : false;
			mSmartBrightness.setChecked(enable);
			mSmartBrightnessPreview.setOnPreferenceChangeListener(this);
			if(enable)
			{                
				getPreferenceScreen().addPreference(mSmartBrightnessPreview);
			}       
		}
		

        mDisplayManager = (DisplayManager)getActivity().getSystemService(
                Context.DISPLAY_SERVICE);
        mWifiDisplayStatus = mDisplayManager.getWifiDisplayStatus();
        mWifiDisplayPreference = (Preference)findPreference(KEY_WIFI_DISPLAY);
        if (mWifiDisplayStatus.getFeatureState()
                == WifiDisplayStatus.FEATURE_STATE_UNAVAILABLE) {
            getPreferenceScreen().removePreference(mWifiDisplayPreference);
            mWifiDisplayPreference = null;
        }
		/* homlet 和 dongle上无休眠 */
		getPreferenceScreen().removePreference(mScreenTimeoutPreference);
		mOutputMode = (ListPreference) findPreference(KEY_TV_OUTPUT_MODE);
		if (mOutputMode != null) {
			mOutputMode.setOnPreferenceChangeListener(this);
			setOutputMode(mOutputMode);
		}
    }

    private void updateTimeoutPreferenceDescription(long currentTimeout) {
        ListPreference preference = mScreenTimeoutPreference;
        String summary;
        if (currentTimeout < 0) {
            // Unsupported value
            summary = "";
        } else {
            final CharSequence[] entries = preference.getEntries();
            final CharSequence[] values = preference.getEntryValues();
            if (entries == null || entries.length == 0) {
                summary = "";
            } else {
                int best = 0;
                for (int i = 0; i < values.length; i++) {
                    long timeout = Long.parseLong(values[i].toString());
                    if (currentTimeout >= timeout) {
                        best = i;
                    }
                }
                summary = preference.getContext().getString(R.string.screen_timeout_summary,
                        entries[best]);
            }
        }
        preference.setSummary(summary);
    }

    private void disableUnusableTimeouts(ListPreference screenTimeoutPreference) {
        final DevicePolicyManager dpm =
                (DevicePolicyManager) getActivity().getSystemService(
                Context.DEVICE_POLICY_SERVICE);
        final long maxTimeout = dpm != null ? dpm.getMaximumTimeToLock(null) : 0;
        if (maxTimeout == 0) {
            return; // policy not enforced
        }
        final CharSequence[] entries = screenTimeoutPreference.getEntries();
        final CharSequence[] values = screenTimeoutPreference.getEntryValues();
        ArrayList<CharSequence> revisedEntries = new ArrayList<CharSequence>();
        ArrayList<CharSequence> revisedValues = new ArrayList<CharSequence>();
        for (int i = 0; i < values.length; i++) {
            long timeout = Long.parseLong(values[i].toString());
            if (timeout <= maxTimeout) {
                revisedEntries.add(entries[i]);
                revisedValues.add(values[i]);
            }
        }
        if (revisedEntries.size() != entries.length || revisedValues.size() != values.length) {
            screenTimeoutPreference.setEntries(
                    revisedEntries.toArray(new CharSequence[revisedEntries.size()]));
            screenTimeoutPreference.setEntryValues(
                    revisedValues.toArray(new CharSequence[revisedValues.size()]));
            final int userPreference = Integer.parseInt(screenTimeoutPreference.getValue());
            if (userPreference <= maxTimeout) {
                screenTimeoutPreference.setValue(String.valueOf(userPreference));
            } else {
                // There will be no highlighted selection since nothing in the list matches
                // maxTimeout. The user can still select anything less than maxTimeout.
                // TODO: maybe append maxTimeout to the list and mark selected.
            }
        }
        screenTimeoutPreference.setEnabled(revisedEntries.size() > 0);
    }

    int floatToIndex(float val) {
        String[] indices = getResources().getStringArray(R.array.entryvalues_font_size);
        float lastVal = Float.parseFloat(indices[0]);
        for (int i=1; i<indices.length; i++) {
            float thisVal = Float.parseFloat(indices[i]);
            if (val < (lastVal + (thisVal-lastVal)*.5f)) {
                return i-1;
            }
            lastVal = thisVal;
        }
        return indices.length-1;
    }
    
    public void readFontSizePreference(ListPreference pref) {
        try {
            mCurConfig.updateFrom(ActivityManagerNative.getDefault().getConfiguration());
            //FIXME
            //Description: Don't write the locale, the data would droped if the power down.
            //Author: huanglong
            mCurConfig.locale = null;
        } catch (RemoteException e) {
            Log.w(TAG, "Unable to retrieve font size");
        }

        // mark the appropriate item in the preferences list
        int index = floatToIndex(mCurConfig.fontScale);
        pref.setValueIndex(index);

        // report the current size in the summary text
        final Resources res = getResources();
        String[] fontSizeNames = res.getStringArray(R.array.entries_font_size);
        pref.setSummary(String.format(res.getString(R.string.summary_font_size),
                fontSizeNames[index]));
    }
    
    @Override
    public void onResume() {
        super.onResume();

        RotationPolicy.registerRotationPolicyListener(getActivity(),
                mRotationPolicyListener);

        if (mWifiDisplayPreference != null) {
            getActivity().registerReceiver(mReceiver, new IntentFilter(
                    DisplayManager.ACTION_WIFI_DISPLAY_STATUS_CHANGED));
            mWifiDisplayStatus = mDisplayManager.getWifiDisplayStatus();
        }

        updateState();
    }

    @Override
    public void onPause() {
        super.onPause();

        RotationPolicy.unregisterRotationPolicyListener(getActivity(),
                mRotationPolicyListener);

        if (mWifiDisplayPreference != null) {
            getActivity().unregisterReceiver(mReceiver);
        }
    }

    @Override
    public Dialog onCreateDialog(int dialogId) {
        if (dialogId == DLG_GLOBAL_CHANGE_WARNING) {
            return Utils.buildGlobalChangeWarningDialog(getActivity(),
                    R.string.global_font_change_title,
                    new Runnable() {
                        public void run() {
                            mFontSizePref.click();
                        }
                    });
        }
        return null;
    }

    private void updateState() {
        updateAccelerometerRotationCheckbox();
        readFontSizePreference(mFontSizePref);
        updateScreenSaverSummary();
        updateWifiDisplaySummary();
		if(mAccelerometerCoordinate != null)
		{            
			updateAccelerometerCoordinateSummary(mAccelerometerCoordinate.getValue());
		}
    }

    private void updateScreenSaverSummary() {
        if (mScreenSaverPreference != null) {
            mScreenSaverPreference.setSummary(
                    DreamSettings.getSummaryTextWithDreamName(getActivity()));
        }
    }

    private void updateWifiDisplaySummary() {
        if (mWifiDisplayPreference != null) {
            switch (mWifiDisplayStatus.getFeatureState()) {
                case WifiDisplayStatus.FEATURE_STATE_OFF:
                    mWifiDisplayPreference.setSummary(R.string.wifi_display_summary_off);
                    break;
                case WifiDisplayStatus.FEATURE_STATE_ON:
                    mWifiDisplayPreference.setSummary(R.string.wifi_display_summary_on);
                    break;
                case WifiDisplayStatus.FEATURE_STATE_DISABLED:
                default:
                    mWifiDisplayPreference.setSummary(R.string.wifi_display_summary_disabled);
                    break;
            }
        }
    }

    private void updateAccelerometerRotationCheckbox() {
        if (getActivity() == null) return;

        mAccelerometer.setChecked(!RotationPolicy.isRotationLocked(getActivity()));
    }


	private void updateAccelerometerCoordinateSummary(Object value)
	{               
		CharSequence[] summaries = getResources().getTextArray(R.array.accelerometer_summaries);
		CharSequence[] values = mAccelerometerCoordinate.getEntryValues();
		for (int i=0; i<values.length; i++) 
		{            
			if (values[i].equals(value)) 
			{                
				mAccelerometerCoordinate.setSummary(summaries[i]);                
				break;            
			}        
		}    
	}

    public void writeFontSizePreference(Object objValue) {
        try {
            mCurConfig.fontScale = Float.parseFloat(objValue.toString());
            ActivityManagerNative.getDefault().updatePersistentConfiguration(mCurConfig);
        } catch (RemoteException e) {
            Log.w(TAG, "Unable to save font size");
        }
    }

    @Override
    public boolean onPreferenceTreeClick(PreferenceScreen preferenceScreen, Preference preference) {
        if (preference == mAccelerometer) {
            RotationPolicy.setRotationLockForAccessibility(
                    getActivity(), !mAccelerometer.isChecked());
        } else if (preference == mNotificationPulse) {
            boolean value = mNotificationPulse.isChecked();
            Settings.System.putInt(getContentResolver(), Settings.System.NOTIFICATION_LIGHT_PULSE,
                    value ? 1 : 0);
            return true;
        }
        return super.onPreferenceTreeClick(preferenceScreen, preference);
    }

    public boolean onPreferenceChange(Preference preference, Object objValue) {
        final String key = preference.getKey();
        if (KEY_SCREEN_TIMEOUT.equals(key)) {
            int value = Integer.parseInt((String) objValue);
            try {
                Settings.System.putInt(getContentResolver(), SCREEN_OFF_TIMEOUT, value);
                updateTimeoutPreferenceDescription(value);
				
            } catch (NumberFormatException e) {
                Log.e(TAG, "could not persist screen timeout setting", e);
            }
        }
        if (KEY_FONT_SIZE.equals(key)) {
            writeFontSizePreference(objValue);
        }

		if (KEY_ACCELEROMETER_COORDINATE.equals(key))
		{            
			String value = String.valueOf(objValue);
			try 
			{ 
				Settings.System.putString(getContentResolver(),
					Settings.System.ACCELEROMETER_COORDINATE, value);
				
				updateAccelerometerCoordinateSummary(objValue);
			}
			catch (NumberFormatException e) 
			{                
				Log.e(TAG, "could not persist key accelerometer coordinate setting", e); 
			}        
		}

		
		if (KEY_SMART_BRIGHTNESS.equals(key))
		{            
			int value = (Boolean)objValue == true ? 1 : 0;
			Settings.System.putInt(getContentResolver(),
				Settings.System.SMART_BRIGHTNESS_ENABLE, value);
			PowerManager pm = (PowerManager)getSystemService(Context.POWER_SERVICE);
//			pm.setWiseBacklightMode(value);
			if((Boolean)objValue)
			{                
				getPreferenceScreen().addPreference(mSmartBrightnessPreview);
			}
			else
			{            	
				getPreferenceScreen().removePreference(mSmartBrightnessPreview);
				mSmartBrightnessPreview.setChecked(false);            
			}        
		}

        if(KEY_TV_OUTPUT_MODE.equals(key)){
            int value = Integer.parseInt((String) objValue);
			if (value > mOutputModeItems.size()) {
				return false;
			} else if (value == -1) {
				DisplayManagerAw displayManager = (DisplayManagerAw) getSystemService(Context.DISPLAY_SERVICE_AW);
				ArrayList<DispList.DispFormat> items = DispList.getDispList();
				for (int i = 0; i < items.size(); i++) {
					DispList.DispFormat finalFormat = items.get(i);
					boolean isSupport = (displayManager
							.isSupportHdmiMode(finalFormat.mFormat) != 0);
					if (isSupport) {
						switchDispMode(finalFormat);
					}
				}
				Settings.System.putInt(getContentResolver(), DISPLAY_MODE_AUTO_KEY, 1);
				mOutputMode.setValue(String.valueOf(-1));
				return false;
			} else {
				Settings.System.putInt(getContentResolver(), DISPLAY_MODE_AUTO_KEY, 0);
			}
            
            //save the old value
            final int oldValue = Integer.parseInt(mOutputMode.getValue());
            
            DispList.DispFormat item = null;
            item = mOutputModeItems.get(value);
            switchDispMode(item);
            String databaseValue = DispList.ItemCode2Name(item);
            Log.d(TAG,"out put mode save value = " + databaseValue);
            Settings.System.putString(getContentResolver(), 
                    Settings.System.DISPLY_OUTPUT_FORMAT, databaseValue);
            
			
			OnClickListener listener = new OnClickListener() {

				@Override
				public void onClick(DialogInterface dialog, int btn) {
					if (btn == AlertDialog.BUTTON_POSITIVE) {
						
					} else if (btn == AlertDialog.BUTTON_NEGATIVE) {
						DispList.DispFormat item = null;
						if (oldValue == -1) {
							DisplayManagerAw displayManager = (DisplayManagerAw) getSystemService(Context.DISPLAY_SERVICE_AW);
							ArrayList<DispList.DispFormat> items = DispList
									.getDispList();
							for (int i = 0; i < items.size(); i++) {
								DispList.DispFormat finalFormat = items.get(i);
								boolean isSupport = (displayManager
										.isSupportHdmiMode(finalFormat.mFormat) != 0);
								if (isSupport) {
									switchDispMode(finalFormat);
								}
							}
							Settings.System.putInt(getContentResolver(),
									DISPLAY_MODE_AUTO_KEY, 1);
							mOutputMode.setValue(String.valueOf(-1));
						} else {
							item = mOutputModeItems.get(oldValue);
							switchDispMode(item);
							String databaseValue = DispList.ItemCode2Name(item);
							Log.d(TAG, "out put mode save value = "
									+ databaseValue);
							Settings.System.putString(getContentResolver(),
									Settings.System.DISPLY_OUTPUT_FORMAT,
									databaseValue);
							mOutputMode.setValue(Integer.toString(oldValue));
						}
					}
				}
			};
			
			String str = getString(com.android.settings.R.string.display_mode_time_out_desc);
            final AlertDialog dialog = new AlertDialog.Builder(this.getActivity())
                    .setTitle(com.android.settings.R.string.display_mode_time_out_title)
                    .setMessage(String.format(str, Integer.toString(FALLBACK_DISPLAY_MODE_TIMEOUT)))
                    .setPositiveButton(com.android.internal.R.string.ok, listener)
                    .setNegativeButton(com.android.internal.R.string.cancel, listener)
                    .create();
            dialog.show();
            new AsyncTask(){
				@Override
				protected Object doInBackground(Object... arg0) {
					int time = FALLBACK_DISPLAY_MODE_TIMEOUT;
					while(time >= 0 && dialog.isShowing()){
						publishProgress(time);
						try{
							Thread.sleep(1000);
						}catch(Exception e){}
						time--;
					}
					return null;
				}
				@Override
				protected void onPostExecute(Object result) {
					super.onPostExecute(result);
					if (dialog.isShowing()) {
						DispList.DispFormat item = null;
						if (oldValue == -1) {
							DisplayManagerAw displayManager = (DisplayManagerAw) getSystemService(Context.DISPLAY_SERVICE_AW);
							ArrayList<DispList.DispFormat> items = DispList
									.getDispList();
							for (int i = 0; i < items.size(); i++) {
								DispList.DispFormat finalFormat = items.get(i);
								boolean isSupport = (displayManager
										.isSupportHdmiMode(finalFormat.mFormat) != 0);
								if (isSupport) {
									switchDispMode(finalFormat);
								}
							}
							Settings.System.putInt(getContentResolver(),
									DISPLAY_MODE_AUTO_KEY, 1);
							mOutputMode.setValue(String.valueOf(-1));
						} else {
							item = mOutputModeItems.get(oldValue);
							switchDispMode(item);
							String databaseValue = DispList.ItemCode2Name(item);
							Log.d(TAG, "out put mode save value = "
									+ databaseValue);
							Settings.System.putString(getContentResolver(),
									Settings.System.DISPLY_OUTPUT_FORMAT,
									databaseValue);
							mOutputMode.setValue(Integer.toString(oldValue));
						}
						dialog.dismiss();
					}
				}
				@Override
				protected void onProgressUpdate(Object... values) {
					super.onProgressUpdate(values);
					int time = (Integer)values[0];
					String str = getString(com.android.settings.R.string.display_mode_time_out_desc);
					dialog.setMessage(String.format(str, Integer.toString(time)));
				}
				
            }.execute();
                    
        }
        return true;
    }

    private final BroadcastReceiver mReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            if (intent.getAction().equals(DisplayManager.ACTION_WIFI_DISPLAY_STATUS_CHANGED)) {
                mWifiDisplayStatus = (WifiDisplayStatus)intent.getParcelableExtra(
                        DisplayManager.EXTRA_WIFI_DISPLAY_STATUS);
                updateWifiDisplaySummary();
            }
        }
    };

    @Override
    public boolean onPreferenceClick(Preference preference) {
        if (preference == mFontSizePref) {
            if (Utils.hasMultipleUsers(getActivity())) {
                showDialog(DLG_GLOBAL_CHANGE_WARNING);
                return true;
            } else {
                mFontSizePref.click();
            }
        }
        return false;
    }
    private void setOutputMode(ListPreference preference){       
        ArrayList<DispList.DispFormat> items = DispList.getDispList();
        mOutputModeItems = items;
        HashMap<DispList.DispFormat,Integer> strMap = DispList.getItemStringIdList();
        String databaseValue = Settings.System.getString(getContentResolver(), 
                Settings.System.DISPLY_OUTPUT_FORMAT);
        int autotag = 0;
	int size = items.size() + autotag;
        CharSequence[]  entries = new CharSequence[size];
        CharSequence[]  entryValues = new CharSequence[size];
		for (int i = 0; i < size; i++) {
			if (i == 0 && autotag == 1) {
				entries[i] = getResources().getText(
						R.string.display_mode_auto_detect);
				entryValues[i] = String.valueOf(-1);
				continue;
			}
			entries[i] = getResources().getString(strMap.get(items.get(i - autotag)));
			entryValues[i] = String.valueOf(i - autotag);
			if (DispList.ItemName2Code(databaseValue).equals(items.get(i - autotag))) {
				preference.setValue(String.valueOf(i - autotag));
			}
		}
        
        preference.setEntries(entries);
        preference.setEntryValues(entryValues);
        int autoDetect = Settings.System.getInt(getContentResolver(), DISPLAY_MODE_AUTO_KEY,0);
        if(autoDetect == 1){
        	preference.setValue(String.valueOf(-1));
        }
    }
    
	/**
	 * 根据指定输出模式，设置显示输出模式
	 * 
	 * @param item
	 *            指定的显示输出模式对象
	 */
	private void switchDispMode(DispList.DispFormat item) {
		DisplayManagerAw displayManager = (DisplayManagerAw) getSystemService(Context.DISPLAY_SERVICE_AW);
		if (item == null) {
			return;
		}
		if (displayManager.getDisplayOutputType(0) != item.mOutputType
				|| displayManager.getDisplayOutputFormat(0) != item.mFormat) {
			// displayManager.setDisplayOutputType(0, item.mOutputType,
			// item.mFormat);
			displayManager.setDisplayParameter(0, item.mOutputType,
					item.mFormat);
			displayManager
					.setDisplayMode(DisplayManagerAw.DISPLAY_MODE_SINGLE_FB_GPU);
			// 设置音频输出模式 
			AudioManager audioManager = (AudioManager) getSystemService(Context.AUDIO_SERVICE);
			if (audioManager == null) {
				Log.w(TAG, "audioManager is null");
				return;
			}
			ArrayList<String> audioOutputChannels = audioManager
					.getActiveAudioDevices(AudioManager.AUDIO_OUTPUT_ACTIVE);
			if (item.mOutputType == DisplayManagerAw.DISPLAY_OUTPUT_TYPE_HDMI
					&& !audioOutputChannels.contains(AudioManager.AUDIO_NAME_HDMI)) {
				audioOutputChannels.clear();
				audioOutputChannels.add(AudioManager.AUDIO_NAME_HDMI);
				audioManager.setAudioDeviceActive(audioOutputChannels,AudioManager.AUDIO_OUTPUT_ACTIVE);
			} else if ((item.mOutputType == DisplayManagerAw.DISPLAY_OUTPUT_TYPE_TV || item.mOutputType == DisplayManagerAw.DISPLAY_OUTPUT_TYPE_VGA)
					&& !audioOutputChannels.contains(AudioManager.AUDIO_NAME_CODEC)) {
				audioOutputChannels.clear();
				audioOutputChannels.add(AudioManager.AUDIO_NAME_CODEC);
				audioManager.setAudioDeviceActive(audioOutputChannels,AudioManager.AUDIO_OUTPUT_ACTIVE);
			}
		}
	}
}
