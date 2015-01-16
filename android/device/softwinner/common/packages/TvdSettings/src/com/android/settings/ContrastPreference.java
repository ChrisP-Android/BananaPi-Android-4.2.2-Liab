package com.android.settings;
/*
 ************************************************************************************
 *                                    Android Settings

 *                       (c) Copyright 2006-2010, huanglong Allwinner 
 *                                 All Rights Reserved
 *
 * File       : ContrastPreference.java
 * By         : huanglong
 * Version    : v1.0
 * Date       : 2011-9-5 14:27:00
 * Description: Add the Contrast settings to Display.
 * Update     : date                author      version     notes
 *           
 ************************************************************************************
 */


import android.content.Context;
import android.os.IPowerManager;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.preference.SeekBarDialogPreference;
import android.preference.SeekBarPreference;
import android.provider.Settings;
import android.provider.Settings.SettingNotFoundException;
import android.util.AttributeSet;
import android.util.Log;
import android.view.DisplayManagerAw;
import android.view.View;
import android.widget.SeekBar;

public class ContrastPreference extends SeekBarDialogPreference implements
        SeekBar.OnSeekBarChangeListener{
    
    private SeekBar mSeekBar;
    
    private int     mOldContrastValue;
    private DisplayManagerAw mDisplayManagerAw;
    
    private int MAXIMUM_VALUE = 80;
    private int MINIMUM_VALUE = 20;

    public ContrastPreference(Context context, AttributeSet attrs) {
        super(context, attrs);
        mDisplayManagerAw = (DisplayManagerAw) context.getSystemService(Context.DISPLAY_SERVICE_AW);
        setDialogLayoutResource(R.layout.preference_dialog_contrast);
        setDialogIcon(R.drawable.ic_settings_contrast);   
    }
    
    @Override
    protected void onBindDialogView(View view){
        super.onBindDialogView(view);
        
        mSeekBar = getSeekBar(view);
        mSeekBar.setMax(MAXIMUM_VALUE-MINIMUM_VALUE);
        try{
            mOldContrastValue = getSysInt();
        }catch(SettingNotFoundException snfe){
            mOldContrastValue = MAXIMUM_VALUE;
        }
        Log.d("staturation","" + mOldContrastValue);
        mSeekBar.setProgress(mOldContrastValue - MINIMUM_VALUE);
        mSeekBar.setOnSeekBarChangeListener(this);
    }
    
    public void onProgressChanged(SeekBar seekBar, int progress,
            boolean fromTouch){
        setContrast(progress + MINIMUM_VALUE);
    }
    @Override
    protected void onDialogClosed(boolean positiveResult){
        super.onDialogClosed(positiveResult);
        if(positiveResult){
            putSysInt(mSeekBar.getProgress() + MINIMUM_VALUE);            
        }else{
            setContrast(mOldContrastValue);
        }
    }

    private int getSysInt() throws SettingNotFoundException
    {
        return Settings.System.getInt(getContext().getContentResolver(), 
                Settings.System.COLOR_CONTRAST,MINIMUM_VALUE);
    }
    private boolean putSysInt(int value)
    {
        return Settings.System.putInt(getContext().getContentResolver(), 
                Settings.System.COLOR_CONTRAST,value);
    }
    private void setContrast(int Contrasrt) {
        mDisplayManagerAw.setDisplayContrast(0,Contrasrt);
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
