package com.android.settings;

import android.os.Bundle;
import android.preference.CheckBoxPreference;
import android.preference.ListPreference;
import android.preference.Preference;
import android.preference.PreferenceActivity;
import android.preference.PreferenceScreen;
import android.util.Log;
import android.opengl.GLSurfaceView;
import android.provider.Settings;
import android.view.WindowManager;
import android.view.Display;

public class ScreenAdaption extends PreferenceActivity 
             implements Preference.OnPreferenceChangeListener{
    
    private static final String TAG = "Settings.Display";
    private static final Boolean LOGD = false;
    
    private static final String KEY_TOGGLE_SA_TOGGLE = "toggle_screen_adapt";
    private static final String KEY_SA_MODE_SELECTOR = "display_mode_selector";
    
    private static final String VALUES_FULL_SCREEN = "full";
    private static final String VALUES_UPON_SCREEN = "upon";
    private static final String VALUES_BELOW_SCREEN = "below";
    private static final String VALUES_CENTER_SCREEN = "center";
    
    private CheckBoxPreference mSAToggle;
    private ListPreference mSAModeSelector;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        addPreferencesFromResource(R.xml.screen_adaption);
        String   screenAdaptionMode;
        
        mSAToggle = (CheckBoxPreference)findPreference(KEY_TOGGLE_SA_TOGGLE);
        mSAModeSelector = (ListPreference)findPreference(KEY_SA_MODE_SELECTOR);
        
        mSAModeSelector.setOnPreferenceChangeListener(this);
        
        int  PosMode;
	    screenAdaptionMode = Settings.System.getString(getContentResolver(),Settings.System.DISPLAY_ADAPTION_MODE);
	        //set current values
	    if(screenAdaptionMode == null){
	        screenAdaptionMode = new String(VALUES_CENTER_SCREEN);
	    }
	    mSAModeSelector.setValue(screenAdaptionMode);
	    
	    boolean isAdaptionEnable = Settings.System.getInt(getContentResolver(), 
	            Settings.System.DISPLAY_ADAPTION_ENABLE, 1) == 1;
	    
	    mSAToggle.setChecked(isAdaptionEnable);
	        
	    WindowManager wm = (WindowManager)getSystemService(WINDOW_SERVICE);
	    android.view.Display display = wm.getDefaultDisplay();
	    int width     = display.getRawWidth();
	    int height    = display.getRawHeight();
	        
	    String summary = getResources().getString(R.string.screen_adaption_summary_off);
	    mSAToggle.setSummaryOff(summary + " " + width + " * " + height);
	    mSAToggle.setSummaryOn(summary + " " + width + " * " + height);
    }
    
    public boolean onPreferenceTreeClick(PreferenceScreen preferenceScreen,
            Preference preference) 
    {
    	if(preference == mSAToggle) {
        	if(mSAToggle.isChecked() == false){
                Settings.System.putInt(getContentResolver(), Settings.System.DISPLAY_ADAPTION_ENABLE, 0);
            }else{
                Settings.System.putInt(getContentResolver(), Settings.System.DISPLAY_ADAPTION_ENABLE, 1);
            }

            WindowManager wm = (WindowManager)getSystemService(WINDOW_SERVICE);
            Display display = wm.getDefaultDisplay();
            int width     = display.getRawWidth();
            int height    = display.getRawHeight();
            
            String summary = getResources().getString(R.string.screen_adaption_summary_off);
            mSAToggle.setSummaryOff(summary + " " + width + " * " + height);
            mSAToggle.setSummaryOn(summary + " " + width + " * " + height);
        }
        return false;
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        if(preference == mSAModeSelector) {
            if(LOGD){
                Log.d(TAG,"change the screen mode to " + newValue);
            }
            if(newValue.equals(VALUES_FULL_SCREEN)) {
                Settings.System.putString(getContentResolver(),Settings.System.DISPLAY_ADAPTION_MODE,VALUES_FULL_SCREEN);
                mSAModeSelector.setValue(VALUES_FULL_SCREEN);
            }
            else if(newValue.equals(VALUES_UPON_SCREEN)) {
                Settings.System.putString(getContentResolver(),Settings.System.DISPLAY_ADAPTION_MODE,VALUES_UPON_SCREEN);
                mSAModeSelector.setValue(VALUES_UPON_SCREEN);
            }
            else if(newValue.equals(VALUES_BELOW_SCREEN)) {
                Settings.System.putString(getContentResolver(),Settings.System.DISPLAY_ADAPTION_MODE,VALUES_BELOW_SCREEN);
                mSAModeSelector.setValue(VALUES_BELOW_SCREEN);
            }
            else if(newValue.equals(VALUES_CENTER_SCREEN)) {
                Settings.System.putString(getContentResolver(),Settings.System.DISPLAY_ADAPTION_MODE,VALUES_CENTER_SCREEN);
                mSAModeSelector.setValue(VALUES_CENTER_SCREEN);
            }
        }
        
        return false;
    }
}
