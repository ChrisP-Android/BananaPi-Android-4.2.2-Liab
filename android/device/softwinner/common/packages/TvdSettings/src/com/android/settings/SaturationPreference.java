package com.android.settings;
/*
 ************************************************************************************
 *                                    Android Settings

 *                       (c) Copyright 2006-2010, huanglong Allwinner 
 *                                 All Rights Reserved
 *
 * File       : SaturationPreference.java
 * By         : huanglong
 * Version    : v1.0
 * Date       : 2011-9-5 16:20:00
 * Description: Add the Saturation settings to Display.
 * Update     : date                author      version     notes
 *           
 ************************************************************************************
 */

import android.content.Context;
import android.preference.SeekBarDialogPreference;
import android.preference.SeekBarPreference;
import android.provider.Settings;
import android.provider.Settings.SettingNotFoundException;
import android.util.AttributeSet;
import android.util.Log;
import android.view.DisplayManagerAw;
import android.view.View;
import android.widget.SeekBar;

public class SaturationPreference extends SeekBarDialogPreference implements 
        SeekBar.OnSeekBarChangeListener{

    private SeekBar mSeekBar;
    
    private int OldSaturationValue;

    private int MAXIMUM_VALUE = 80;
    private int MINIMUM_VALUE = 20;
    private DisplayManagerAw mDisplayManagerAw;
    
    public SaturationPreference(Context context, AttributeSet attrs) {
        super(context, attrs);
        mDisplayManagerAw = (DisplayManagerAw) context.getSystemService(Context.DISPLAY_SERVICE_AW);
        setDialogLayoutResource(R.layout.preference_dialog_saturation);
        setDialogIcon(R.drawable.ic_settings_saturation);        
    }
    
    protected void onBindDialogView(View view){
        super.onBindDialogView(view);        
        
        mSeekBar = getSeekBar(view);
        mSeekBar.setMax(MAXIMUM_VALUE-MINIMUM_VALUE);
        try{
            OldSaturationValue = getSysInt();
        }catch(SettingNotFoundException snfe){
            OldSaturationValue = MAXIMUM_VALUE;
        }
        Log.d("staturation","" + OldSaturationValue);
        mSeekBar.setProgress(OldSaturationValue - MINIMUM_VALUE);
        mSeekBar.setOnSeekBarChangeListener(this);
    }
    
    public void onProgressChanged(SeekBar seekBar, int progress,
            boolean fromTouch){
        setSaturation(progress + MINIMUM_VALUE);
    }
    @Override
    protected void onDialogClosed(boolean positiveResult){
        super.onDialogClosed(positiveResult);
        if(positiveResult){
            putSysInt(mSeekBar.getProgress() + MINIMUM_VALUE);            
        }else{
            setSaturation(OldSaturationValue);
        }
    }
    
    private int getSysInt() throws SettingNotFoundException
    {
        return Settings.System.getInt(getContext().getContentResolver(), 
                Settings.System.COLOR_SATURATION,MINIMUM_VALUE);
    }
    private boolean putSysInt(int value)
    {
        return Settings.System.putInt(getContext().getContentResolver(), 
                Settings.System.COLOR_SATURATION, value);
    }
    private void setSaturation(int saturation) {
        mDisplayManagerAw.setDisplaySaturation(0,saturation);
    }
    
    /*implements method in SeekBar.OnSeekBarChangeListener*/
    @Override
    public void onStartTrackingTouch(SeekBar arg0) {
        // NA
        
    }
    /*implements method in SeekBar.OnSeekBarChangeListener*/
    @Override
    public void onStopTrackingTouch(SeekBar arg0) {
        // NA
        
    }

}
