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

package com.android.settings;

import android.content.ContentResolver;
import android.content.Context;
import android.database.ContentObserver;
import android.os.Bundle;
import android.os.Handler;
import android.os.IPowerManager;
import android.os.Parcel;
import android.os.Parcelable;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.preference.SeekBarDialogPreference;
import android.provider.Settings;
import android.provider.Settings.SettingNotFoundException;
import android.util.AttributeSet;
import android.view.DisplayManagerAw;
import android.view.View;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.SeekBar;

public class TVBrightnessPreference extends SeekBarDialogPreference implements
        SeekBar.OnSeekBarChangeListener{

	/* SeekBar */
    private SeekBar mSeekBar;
 
    /* 用于保存原始亮度值 */
    private int mOldBrightness;

    private boolean mRestoredOldState;

    // Backlight range is from 0 - 255. Need to make sure that user
    // doesn't set the backlight to 0 and get stuck
    private int mScreenBrightnessDim =
	    getContext().getResources().getInteger(com.android.internal.R.integer.config_screenBrightnessDim);
    private static final int MAXIMUM_BACKLIGHT = 100;

    /* 用于监听当前亮度值的改变 */
    private ContentObserver mBrightnessObserver = new ContentObserver(new Handler()) {
        @Override
        public void onChange(boolean selfChange) {
            onBrightnessChanged();
        }
    };

    public TVBrightnessPreference(Context context, AttributeSet attrs) {
        super(context, attrs);

        //设置UI
        setDialogLayoutResource(R.layout.preference_dialog_tv_brightness);
        setDialogIcon(R.drawable.ic_settings_display);
    }

    @Override
    protected void showDialog(Bundle state) {
        super.showDialog(state);

        //注册内容监听器
        getContext().getContentResolver().registerContentObserver(
                Settings.System.getUriFor(Settings.System.COLOR_BRIGHTNESS), true,
                mBrightnessObserver);

        mRestoredOldState = false;
    }

    @Override
    protected void onBindDialogView(View view) {
        super.onBindDialogView(view);

        //设置seekbar的值
        mSeekBar = getSeekBar(view);
        mSeekBar.setMax(MAXIMUM_BACKLIGHT - mScreenBrightnessDim);
        mOldBrightness = getBrightness(0);
        mSeekBar.setProgress(mOldBrightness - mScreenBrightnessDim);
        
        mSeekBar.setOnSeekBarChangeListener(this);
    }

    @Override
    public void onProgressChanged(SeekBar seekBar, int progress,
            boolean fromTouch) {
        setBrightness(progress + mScreenBrightnessDim);
    }

    @Override
    public void onStartTrackingTouch(SeekBar seekBar) {
        // NA
    }

    @Override
    public void onStopTrackingTouch(SeekBar seekBar) {
        // NA
    }

    private int getBrightness(int defaultValue) {
        int brightness = defaultValue;
        try {
            brightness = Settings.System.getInt(getContext().getContentResolver(),
                    Settings.System.COLOR_BRIGHTNESS);
        } catch (SettingNotFoundException snfe) {
        }
        return brightness;
    }

    private void onBrightnessChanged() {
        int brightness = getBrightness(MAXIMUM_BACKLIGHT);
        mSeekBar.setProgress(brightness - mScreenBrightnessDim);
    }

    @Override
    protected void onDialogClosed(boolean positiveResult) {
        super.onDialogClosed(positiveResult);

        final ContentResolver resolver = getContext().getContentResolver();

        if (positiveResult) {
            Settings.System.putInt(resolver,
                    Settings.System.COLOR_BRIGHTNESS,
                    mSeekBar.getProgress() + mScreenBrightnessDim);
        } else {
            restoreOldState();
        }

        resolver.unregisterContentObserver(mBrightnessObserver);
    }

    private void restoreOldState() {
        if (mRestoredOldState) return;
        setBrightness(mOldBrightness);
        mRestoredOldState = true;
    }

    private void setBrightness(int brightness) {

        DisplayManagerAw dm = (DisplayManagerAw) getContext().getSystemService(Context.DISPLAY_SERVICE_AW);
        if (dm != null) {
            dm.setDisplayBright(0,brightness);
        }
    }

    @Override
    protected Parcelable onSaveInstanceState() {
        final Parcelable superState = super.onSaveInstanceState();
        if (getDialog() == null || !getDialog().isShowing()) return superState;

        // Save the dialog state
        final SavedState myState = new SavedState(superState);
        myState.progress = mSeekBar.getProgress();
        myState.oldProgress = mOldBrightness;

        // Restore the old state when the activity or dialog is being paused
        restoreOldState();
        return myState;
    }

    @Override
    protected void onRestoreInstanceState(Parcelable state) {
        if (state == null || !state.getClass().equals(SavedState.class)) {
            // Didn't save state for us in onSaveInstanceState
            super.onRestoreInstanceState(state);
            return;
        }

        SavedState myState = (SavedState) state;
        super.onRestoreInstanceState(myState.getSuperState());
        mOldBrightness = myState.oldProgress;
        setBrightness(myState.progress + mScreenBrightnessDim);
    }

    private static class SavedState extends BaseSavedState {

        int progress;
        int oldProgress;

        public SavedState(Parcel source) {
            super(source);
            progress = source.readInt();
            oldProgress = source.readInt();
        }

        @Override
        public void writeToParcel(Parcel dest, int flags) {
            super.writeToParcel(dest, flags);
            dest.writeInt(progress);
            dest.writeInt(oldProgress);
        }

        public SavedState(Parcelable superState) {
            super(superState);
        }

        public static final Parcelable.Creator<SavedState> CREATOR =
                new Parcelable.Creator<SavedState>() {

            public SavedState createFromParcel(Parcel in) {
                return new SavedState(in);
            }

            public SavedState[] newArray(int size) {
                return new SavedState[size];
            }
        };
    }
}

