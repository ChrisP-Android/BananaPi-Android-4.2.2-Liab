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

package com.android.gallery3d.app; 

import com.android.gallery3d.R;

import android.app.Activity;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.media.AudioManager;
import android.os.Bundle;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.provider.MediaStore;
import android.view.IWindowManager;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.widget.ListView;

/**
 * This activity plays a video from a specified URI.
 */
public class MovieActivity extends Activity {
    @SuppressWarnings("unused")
    private static final String TAG = "MovieActivity";
    
    public static final String KEY_LOGO_BITMAP = "logo-bitmap";
    public static final String KEY_TREAT_UP_AS_BACK = "treat-up-as-back";
    
    private MovieViewControl mControl;
    private boolean mFinishOnCompletion;

	private IWindowManager mWindowManager;
	private float mAnimationScale;

    @Override
    public void onCreate(Bundle icicle) {
        super.onCreate(icicle);
		//setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
        
        //Bevis...add to fix hdmi output color key bug
        mWindowManager = IWindowManager.Stub.asInterface(ServiceManager.getService("window"));
        try {
	        mAnimationScale = mWindowManager.getAnimationScale(1);
	        mWindowManager.setAnimationScale(1, 0);
        }catch (RemoteException e) {
        }
        //Bevis...
        
        getActionBar().hide();
        
        setContentView(R.layout.movie_view);
        View rootView = findViewById(R.id.root);
        Intent intent = getIntent();
    	mFinishOnCompletion = intent.getBooleanExtra(MediaStore.EXTRA_FINISH_ON_COMPLETION, true);
        mControl = new MovieViewControl(rootView, this, intent) {
            @Override
            public void onCompletion() {
                if (mFinishOnCompletion) {
                    finish();
                } else {
                    super.onCompletion();
                    if( super.toQuit() ){
                        finish();
                    }
                }
            }
        };
        if (intent.hasExtra(MediaStore.EXTRA_SCREEN_ORIENTATION)) {
            int orientation = intent.getIntExtra(
                    MediaStore.EXTRA_SCREEN_ORIENTATION,
                    ActivityInfo.SCREEN_ORIENTATION_UNSPECIFIED);
            if (orientation != getRequestedOrientation()) {
                setRequestedOrientation(orientation);
            }
        }
        Window win = getWindow();
        WindowManager.LayoutParams winParams = win.getAttributes();
        winParams.buttonBrightness = WindowManager.LayoutParams.BRIGHTNESS_OVERRIDE_OFF;
        win.setAttributes(winParams);
    }

    @Override
    public void onStart() {
        ((AudioManager) getSystemService(AUDIO_SERVICE))
                .requestAudioFocus(null, AudioManager.STREAM_MUSIC,
                AudioManager.AUDIOFOCUS_GAIN_TRANSIENT);
        super.onStart();
    }

    @Override
    protected void onStop() {
        ((AudioManager) getSystemService(AUDIO_SERVICE))
                .abandonAudioFocus(null);
        super.onStop();
    }
    public void onPause() {
        mControl.onPause();
        super.onPause();
    }

    @Override
    public void onResume() {
        mControl.onResume();
        super.onResume();
    }
    
    @Override
    public void onDestroy() {
        mControl.onDestroy();
    	super.onDestroy();
    	
    	//Bevis...add to fix hdmi output color key bug
        try {
	    	mWindowManager.setAnimationScale(1, mAnimationScale);
        }catch (RemoteException e) {
        }
        //Bevis...
    }
}
