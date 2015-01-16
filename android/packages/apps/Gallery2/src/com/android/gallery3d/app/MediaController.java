/*
 * Copyright (C) 2006 The Android Open Source Project
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

import android.content.Context;
import android.graphics.PixelFormat;
import android.graphics.Point;
import android.graphics.Rect;
import android.media.AudioManager;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.view.WindowManager;
import android.widget.FrameLayout;
import android.widget.ProgressBar;
import android.widget.SeekBar;
import android.widget.SeekBar.OnSeekBarChangeListener;
import android.widget.TextView;

import com.android.gallery3d.R;
import com.android.internal.policy.PolicyManager;

import java.text.DateFormat;
import java.util.Date;
import java.util.Formatter;
import java.util.Locale;

/**
 * A view containing controls for a MediaPlayer. Typically contains the
 * buttons like "Play/Pause", "Rewind", "Fast Forward" and a progress
 * slider. It takes care of synchronizing the controls with the state
 * of the MediaPlayer.
 * <p>
 * The way to use this class is to instantiate it programatically.
 * The MediaController will create a default set of controls
 * and put them in a window floating above your application. Specifically,
 * the controls will float above the view specified with setAnchorView().
 * The window will disappear if left idle for three seconds and reappear
 * when the user touches the anchor view.
 * <p>
 * Functions like show() and hide() have no effect when MediaController
 * is created in an xml layout.
 * 
 * MediaController will hide and
 * show the buttons according to these rules:
 * <ul>
 * <li> The "previous" and "next" buttons are hidden until setPrevNextListeners()
 *   has been called
 * <li> The "previous" and "next" buttons are visible but disabled if
 *   setPrevNextListeners() was called with null listeners
 * <li> The "rewind" and "fastforward" buttons are shown unless requested
 *   otherwise by using the MediaController(Context, boolean) constructor
 *   with the boolean set to false
 * </ul>
 */
public class MediaController extends FrameLayout {
    private static final String TAG = "MediaController";

    private MediaPlayerControl  mPlayer;
    private Context             mContext;
    private View                mAnchor;
    private View                mRoot, mStatus, mMediaControl, mSettings, mRootView;
    private WindowManager       mWindowManager;
    private Window              mWindow;
    private View                mDecor;
    private ProgressBar         mProgress;
    private TextView            mEndTime, mCurrentTime, mSytemTime, mFileName, mBatteryRemain;
    private String				mFileNameText, mBatteryText;
    private DateFormat 			mDateFormat;
    private boolean             mShowing, mHolding;
    private boolean             mDragging;
    private static final int    sDefaultTimeout = 5000;
    private static final int    FADE_OUT = 1;
    private static final int    SHOW_PROGRESS = 2;
    private static final int    HIDE_SYSTEM_BAR = 3;
    private View.OnClickListener mNextListener, mPrevListener;
    private int mInitSubPos, mUpSubPos;
    StringBuilder                mFormatBuilder;
    Formatter                    mFormatter;
    private boolean 			prevNextVisible = false;
    
    private View.OnClickListener mSetListener;
    private View.OnClickListener mBackListener;
    private View.OnClickListener mVolumeIncListener;
    private View.OnClickListener mVolumeDecListener;
    private View.OnClickListener mSubGateListener;
    private View.OnClickListener mSubSelectListener;
    private View.OnClickListener mSubCharSetListener;
    private View.OnClickListener mSubColorListener;
    private View.OnClickListener mSubCharSizeListener;
    private View.OnClickListener mSubOffSetListener;
    private View.OnClickListener mSubDelayListener;
    private View.OnClickListener mZoomListener;
    private View.OnClickListener m3DListener;
    private View.OnClickListener mTrackListener;
    private View.OnClickListener mRepeatListener;

    private android.widget.ImageButton         mPauseButton;
    private android.widget.ImageButton         mFfwdButton;
    private android.widget.ImageButton         mRewButton;
    private android.widget.ImageButton         mNextButton;
    private android.widget.ImageButton         mPrevButton;
    private android.widget.ImageButton         mVolumeIncButton;
    private android.widget.ImageButton         mVolumeDecButton;
    private android.widget.ImageButton         mSetButton;
    private android.widget.ImageButton         mBackButton;
    private android.widget.ImageButton         mSubGateButton;
    private android.widget.ImageButton         mSubSelectButton;
    private android.widget.ImageButton         mSubCharSetButton;
    private android.widget.ImageButton         mSubColorButton;
    private android.widget.ImageButton         mSubCharSizeButton;
    private android.widget.ImageButton         mSubOffSetButton;
    private android.widget.ImageButton         mSubDelayButton;
    private android.widget.ImageButton         m3DButton;
    private android.widget.ImageButton         mZoomButton;
    private android.widget.ImageButton         mTrackButton;
    private android.widget.ImageButton		   mRepeatButton;
    private android.widget.ImageButton         mUpButton;


    @Override
    public void onFinishInflate() {
        if (mRoot != null)
            initControllerView(mRoot);
    }

    public MediaController(Context context, View view) {
        super(context);
        mContext = context;
        mRootView = view;
        initFloatingWindow();
           
    }

    private void initFloatingWindow() {
        mWindowManager = (WindowManager)mContext.getSystemService(Context.WINDOW_SERVICE);
        mWindow = PolicyManager.makeNewWindow(mContext);
        mWindow.setWindowManager(mWindowManager, null, null);
        mWindow.requestFeature(Window.FEATURE_NO_TITLE);
        mDecor = mWindow.getDecorView();
        mDecor.setOnTouchListener(mTouchListener);
        mWindow.setContentView(this);
        mWindow.setBackgroundDrawableResource(android.R.color.transparent);
        
        // While the media controller is up, the volume control keys should
        // affect the media stream type
        mWindow.setVolumeControlStream(AudioManager.STREAM_MUSIC);

        setFocusable(true);
        setFocusableInTouchMode(true);
        setDescendantFocusability(ViewGroup.FOCUS_AFTER_DESCENDANTS);
        requestFocus();
    }

    private OnTouchListener mTouchListener = new OnTouchListener() {
        public boolean onTouch(View v, MotionEvent event) {
            if (event.getAction() == MotionEvent.ACTION_DOWN) {
                if (mShowing) {
                    hide();
                }
            }
            return false;
        }
    };
    
    public void setMediaPlayer(MediaPlayerControl player) {
        mPlayer = player;
        updatePausePlay();
    }

    /**
     * Set the view that acts as the anchor for the control view.
     * This can for example be a VideoView, or your Activity's main view.
     * @param view The view to which to anchor the controller when it is visible.
     */
    public void setAnchorView(View view) {
        mAnchor = view;

        FrameLayout.LayoutParams frameParams = new FrameLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.MATCH_PARENT
        );

        removeAllViews();
        View v = makeControllerView();
        View statusView = makeStatusView();
        addView(v, frameParams);
        addView(statusView, frameParams);
    }
    
    /**
     * Create the view that holds the widgets that control playback.
     * Derived classes can override this to create their own.
     * @return The controller view.
     * @hide This doesn't work as advertised
     */
    protected View makeStatusView() {
        LayoutInflater inflate = (LayoutInflater) mContext.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        mStatus = inflate.inflate(R.layout.media_status, null);
  	
        initStatusView(mStatus);

        return mStatus;
    }
    private void initStatusView(View v) {
    	mDateFormat = DateFormat.getInstance();
        mSytemTime = (TextView) v.findViewById(R.id.system_time);
        mFileName = (TextView) v.findViewById(R.id.file_path);
        mBatteryRemain = (TextView) v.findViewById(R.id.current_battery);
        mFileName.setText(mFileNameText);
        mBatteryRemain.setText(mBatteryText);
        
        mProgress = (ProgressBar) v.findViewById(R.id.mediacontroller_progress);
        if (mProgress != null) {
            if (mProgress instanceof SeekBar) {
                SeekBar seeker = (SeekBar) mProgress;
                seeker.setOnSeekBarChangeListener(mSeekListener);
            }
            mProgress.setMax(1000);
        }
        
        mEndTime = (TextView) v.findViewById(R.id.time);
        mCurrentTime = (TextView) v.findViewById(R.id.time_current);
        mFormatBuilder = new StringBuilder();
        mFormatter = new Formatter(mFormatBuilder, Locale.getDefault());
    }
    /**
     * Create the view that holds the widgets that control playback.
     * Derived classes can override this to create their own.
     * @return The controller view.
     * @hide This doesn't work as advertised
     */
    protected View makeControllerView() {
        LayoutInflater inflate = (LayoutInflater) mContext.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        mRoot = inflate.inflate(R.layout.media_controller, null);
  	
        initControllerView(mRoot);
                
        return mRoot;
    }

    private void initControllerView(View v) {
        mMediaControl = v.findViewById(R.id.media_control);
        mSettings = v.findViewById(R.id.settings);
        mSettings.setVisibility(View.GONE); 

        mPauseButton = (android.widget.ImageButton) v.findViewById(R.id.pause);
        if (mPauseButton != null) {
            mPauseButton.requestFocus();
            mPauseButton.setOnClickListener(mPauseListener);
        }

        mFfwdButton = (android.widget.ImageButton) v.findViewById(R.id.ffwd);
        if (mFfwdButton != null) {
            mFfwdButton.setOnClickListener(mFfwdListener);
        }
        mRewButton = (android.widget.ImageButton) v.findViewById(R.id.rew);
        if (mRewButton != null) {
            mRewButton.setOnClickListener(mRewListener);
        }

        mNextButton = (android.widget.ImageButton) v.findViewById(R.id.next);
        if (mNextButton != null) {
        	if(prevNextVisible){
        		mNextButton.setVisibility(VISIBLE);
        		mNextButton.setOnClickListener(mNextListener);        		
        	}
        }
        mPrevButton = (android.widget.ImageButton) v.findViewById(R.id.prev);
        if (mPrevButton != null) {
        	if(prevNextVisible){
        		mPrevButton.setVisibility(VISIBLE);
        		mPrevButton.setOnClickListener(mPrevListener);        		
        	}
        }
     
        mSetButton = (android.widget.ImageButton) v.findViewById(R.id.set);
        if (mSetButton != null) {
        	mSetButton.setOnClickListener(mSetListener);
        }
        mBackButton = (android.widget.ImageButton) v.findViewById(R.id.back);
        if (mBackButton != null) {
            mBackButton.setOnClickListener(mBackListener);
        }
        mVolumeIncButton = (android.widget.ImageButton) v.findViewById(R.id.vInc);
        if (mVolumeIncButton != null) {
            mVolumeIncButton.setOnClickListener(mVolumeIncListener);
        }
        mVolumeDecButton = (android.widget.ImageButton) v.findViewById(R.id.vDec);
        if (mVolumeDecButton != null) {
            mVolumeDecButton.setOnClickListener(mVolumeDecListener);
        }
        mSubGateButton = (android.widget.ImageButton) v.findViewById(R.id.gate);
        if (mSubGateButton != null) {
            mSubGateButton.setOnClickListener(mSubGateListener);
        }
        mSubSelectButton = (android.widget.ImageButton) v.findViewById(R.id.select);
        if (mSubSelectButton != null) {
            mSubSelectButton.setOnClickListener(mSubSelectListener);
        }
        mSubCharSetButton = (android.widget.ImageButton) v.findViewById(R.id.charset);
        if (mSubCharSetButton != null) {
            mSubCharSetButton.setOnClickListener(mSubCharSetListener);
        }
        mSubColorButton = (android.widget.ImageButton) v.findViewById(R.id.color);
        if (mSubColorButton != null) {
            mSubColorButton.setOnClickListener(mSubColorListener);
        }
        mSubCharSizeButton = (android.widget.ImageButton) v.findViewById(R.id.charsize);
        if (mSubCharSizeButton != null) {
            mSubCharSizeButton.setOnClickListener(mSubCharSizeListener);
        }
        mSubOffSetButton = (android.widget.ImageButton) v.findViewById(R.id.offset);
        if (mSubOffSetButton != null) {
            mSubOffSetButton.setOnClickListener(mSubOffSetListener);
        }
        mSubDelayButton = (android.widget.ImageButton) v.findViewById(R.id.delay);
        if (mSubDelayButton != null) {
            mSubDelayButton.setOnClickListener(mSubDelayListener);
        }
        mZoomButton = (android.widget.ImageButton) v.findViewById(R.id.zoom);
        if (mZoomButton != null) {
            mZoomButton.setOnClickListener(mZoomListener);
        }
        m3DButton = (android.widget.ImageButton) v.findViewById(R.id.mode3D);
        if (m3DButton != null) {
            m3DButton.setOnClickListener(m3DListener);
        }
        mTrackButton = (android.widget.ImageButton) v.findViewById(R.id.track);
        if (mTrackButton != null) {
            mTrackButton.setOnClickListener(mTrackListener);
        }
        mRepeatButton = (android.widget.ImageButton) v.findViewById(R.id.repeat);
        if (mRepeatButton != null) {
            mRepeatButton.setOnClickListener(mRepeatListener);
        }
        mUpButton = (android.widget.ImageButton) v.findViewById(R.id.up);
        if (mUpButton != null) {
        	mUpButton.setOnClickListener(mUpListener);
        }
        
        //init subtitle position
        Rect rect = new Rect();
        v.getWindowVisibleDisplayFrame(rect);
		
        Point outSize = new Point(800, 480);
        mWindowManager.getDefaultDisplay().getRealSize(outSize);
        int scnH = outSize.y;
        if(scnH > 0){
        	mUpSubPos = mInitSubPos = 100 - (rect.bottom * 100 / scnH);
            mRoot.measure(MeasureSpec.makeMeasureSpec(0, MeasureSpec.UNSPECIFIED),
            		MeasureSpec.makeMeasureSpec(0, MeasureSpec.UNSPECIFIED));
            int measuredH = mRoot.getMeasuredHeight();
            if(measuredH > 0) {
            	mUpSubPos = (measuredH * 100) / scnH + mInitSubPos;
            }
//            Log.d(TAG,"mInitSubPos = " + mInitSubPos + ", measuredH = " + measuredH + ", " + "mUpSubPos = " + mUpSubPos);
            if(mInitSubPos < 100){
            	mPlayer.setSubPosition(mInitSubPos);
            }
        }        
    }

    /**
     * Show the controller on screen. It will go away
     * automatically after 3 seconds of inactivity.
     */
    public void show() {
        show(sDefaultTimeout);
    }

//    /**
//     * Disable pause or seek buttons if the stream cannot be paused or seeked.
//     * This requires the control interface to be a MediaPlayerControlExt
//     */
//    private void disableUnsupportedButtons() {
//        try {
//            if (mPauseButton != null && !mPlayer.canPause()) {
//                mPauseButton.setEnabled(false);
//            }
//            if (mRewButton != null && !mPlayer.canSeekBackward()) {
//                mRewButton.setEnabled(false);
//            }
//            if (mFfwdButton != null && !mPlayer.canSeekForward()) {
//                mFfwdButton.setEnabled(false);
//            }
//        } catch (IncompatibleClassChangeError ex) {
//            // We were given an old version of the interface, that doesn't have
//            // the canPause/canSeekXYZ methods. This is OK, it just means we
//            // assume the media can be paused and seeked, and so we don't disable
//            // the buttons.
//        }
//    }
    
    /**
     * Show the controller on screen. It will go away
     * automatically after 'timeout' milliseconds of inactivity.
     * @param timeout The timeout in milliseconds. Use 0 to show
     * the controller until hide() is called.
     */
    public void show(int timeout) {

        if (!mShowing && mAnchor != null) {
            setProgress();
            if (mPauseButton != null) {
                mPauseButton.requestFocus();
            }
//            disableUnsupportedButtons();
            
            /* set subtitle position */
        	if(mUpSubPos < 100) {
        		mPlayer.setSubPosition(mUpSubPos);
        	}

            int [] anchorPos = new int[2];
            mAnchor.getLocationOnScreen(anchorPos);
            WindowManager.LayoutParams p = new WindowManager.LayoutParams();
            p.gravity = Gravity.CENTER;
            p.width = LayoutParams.MATCH_PARENT;
            p.height = LayoutParams.MATCH_PARENT;
            p.x = anchorPos[0];
            p.y = anchorPos[1];
            p.format = PixelFormat.TRANSLUCENT;
            p.type = WindowManager.LayoutParams.TYPE_APPLICATION_PANEL;
            p.flags |= WindowManager.LayoutParams.FLAG_ALT_FOCUSABLE_IM;
            p.token = null;
            p.windowAnimations = 0;
            mWindowManager.addView(mDecor, p);
            
        	/* show system time */
            String currentTime = mDateFormat.format(new Date());
            int index = currentTime.indexOf(' ');
            if(index >= 0) {
            	currentTime = currentTime.substring(index+1); 
            }
            mSytemTime.setText(currentTime);
			
            mRootView.setSystemUiVisibility(View.STATUS_BAR_VISIBLE | View.SYSTEM_UI_FLAG_LAYOUT_STABLE);

            mShowing = true;
        }
        updatePausePlay();
        
        // cause the progress bar to be updated even if mShowing
        // was already true.  This happens, for example, if we're
        // paused with the progress bar showing the user hits play.
        mHandler.sendEmptyMessage(SHOW_PROGRESS);

        Message msg = mHandler.obtainMessage(FADE_OUT);
        if (timeout != 0) {
            mHandler.removeMessages(FADE_OUT);
            mHandler.sendMessageDelayed(msg, timeout);
        }
        
//        Message msg2 = mHandler.obtainMessage(HIDE_SYSTEM_BAR);
//        if (timeout != 0) {
//        		mHandler.removeMessages(HIDE_SYSTEM_BAR);
//        		mHandler.sendMessageDelayed(msg2, timeout-1000);
//        }
        
    }
    
    public boolean isShowing() {
        return mShowing;
    }

    /* if the widget need keep state or not */
    public void setHolding(boolean hold) {
        mHolding = hold;
    }

    /**
     * Remove the controller from the screen.
     */
    public void hide() {
    	  
        if (mAnchor == null)
            return;
      
        if(mHolding) {
            Message msg = mHandler.obtainMessage(FADE_OUT);
            mHandler.removeMessages(FADE_OUT);
            mHandler.sendMessageDelayed(msg, sDefaultTimeout);
            return;
        }

        if (mShowing) {
            mMediaControl.setVisibility(View.VISIBLE);
            mSettings.setVisibility(View.GONE);
            
            try {
                mHandler.removeMessages(SHOW_PROGRESS);
                mWindowManager.removeView(mDecor);
                /* set subtitle position */
                if(mInitSubPos < 100){
                	mPlayer.setSubPosition(mInitSubPos);
                }
            } catch (IllegalArgumentException ex) {
                Log.w(TAG, "already removed");
            }

            mRootView.setSystemUiVisibility(View.STATUS_BAR_HIDDEN | View.SYSTEM_UI_FLAG_FULLSCREEN | View.SYSTEM_UI_FLAG_LAYOUT_STABLE | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION);

            mShowing = false;
        }
    }

    private Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            int pos;
            switch (msg.what) {
                case FADE_OUT:
                		//setSystemUiVisibility(SYSTEM_UI_FLAG_LOW_PROFILE);
                    hide();   
                    break;
                case SHOW_PROGRESS:
                    pos = setProgress();
                    //if (!mDragging && mShowing && mPlayer.isPlaying()) {
                    if(!mDragging && mShowing) {
                        msg = obtainMessage(SHOW_PROGRESS);
                        sendMessageDelayed(msg, 1000 - (pos % 1000));
                    }
                    break;
//                case HIDE_SYSTEM_BAR:
//                		setSystemUiVisibility(SYSTEM_UI_FLAG_HIDE_NAVIGATION);             
//                    break;
            }
        }
    };

    private String stringForTime(int timeMs) {
        int totalSeconds = timeMs / 1000;

        int seconds = totalSeconds % 60;
        int minutes = (totalSeconds / 60) % 60;
        int hours   = totalSeconds / 3600;

        mFormatBuilder.setLength(0);
        if (hours > 0) {
            return mFormatter.format("%d:%02d:%02d", hours, minutes, seconds).toString();
        } else {
            return mFormatter.format("%02d:%02d", minutes, seconds).toString();
        }
    }

    private int setProgress() {
        if (mPlayer == null || mDragging) {
            return 0;
        }
        int position = mPlayer.getCurrentPosition();
        int duration = mPlayer.getDuration();
        duration = duration == -1 ? 0 : duration;
        if (mProgress != null) {
            if (duration > 0) {
                // use long to avoid overflow
                long pos = 1000L * position / duration;
                mProgress.setProgress( (int) pos);
            }
            int percent = mPlayer.getBufferPercentage();
            mProgress.setSecondaryProgress(percent * 10);
        }

        if (mEndTime != null)
            mEndTime.setText(stringForTime(duration));
        if (mCurrentTime != null)
            mCurrentTime.setText(stringForTime(position));

    	/* show system time */
        String currentTime = mDateFormat.format(new Date());
        int index = currentTime.indexOf(' ');
        if(index >= 0) {
        	currentTime = currentTime.substring(index+1); 
        }
        mSytemTime.setText(currentTime);

        return position;
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
    	if(isShowing()) {
    		hide();
    		//setSystemUiVisibility(SYSTEM_UI_FLAG_LOW_PROFILE);
    	} else {
    		show(sDefaultTimeout);
    	}
        return false;
    }

    @Override
    public boolean onTrackballEvent(MotionEvent ev) {
        show(sDefaultTimeout);
        return false;
    }

    @Override
    public boolean dispatchKeyEvent(KeyEvent event) {
        int keyCode = event.getKeyCode();
        if (event.getRepeatCount() == 0 && event.getAction() == KeyEvent.ACTION_DOWN && (
                keyCode ==  KeyEvent.KEYCODE_HEADSETHOOK ||
                keyCode ==  KeyEvent.KEYCODE_MEDIA_PLAY_PAUSE ||
                keyCode ==  KeyEvent.KEYCODE_SPACE)) {
            doPauseResume();
            show(sDefaultTimeout);
            if (mPauseButton != null) {
                mPauseButton.requestFocus();
            }
            return true;
        } else if (keyCode ==  KeyEvent.KEYCODE_MEDIA_STOP) {
            if (mPlayer.isPlaying()) {
                mPlayer.pause();
                updatePausePlay();
            }
            return true;
        } else if (keyCode == KeyEvent.KEYCODE_VOLUME_DOWN ||
                keyCode == KeyEvent.KEYCODE_VOLUME_UP) {
            // don't show the controls for volume adjustment
            return super.dispatchKeyEvent(event);
        } else if (keyCode == KeyEvent.KEYCODE_BACK || keyCode == KeyEvent.KEYCODE_MENU) {
            hide();

            return true;
        } else {
            show(sDefaultTimeout);
        }
        return super.dispatchKeyEvent(event);
    }

    private View.OnClickListener mPauseListener = new View.OnClickListener() {
        public void onClick(View v) {
            doPauseResume();
            show(sDefaultTimeout);
        }
    };

    private void updatePausePlay() {
        if (mRoot == null || mPauseButton == null)
            return;

        if (mPlayer.isPlaying()) {
            mPauseButton.setImageResource(R.drawable.ic_media_pause);
        } else {
            mPauseButton.setImageResource(R.drawable.ic_media_play);
        }
    }

    private void doPauseResume() {
        if (mPlayer.isPlaying()) {
            mPlayer.pause();
        } else {
            mPlayer.start();
        }
        updatePausePlay();
    }

    // There are two scenarios that can trigger the seekbar listener to trigger:
    //
    // The first is the user using the touchpad to adjust the posititon of the
    // seekbar's thumb. In this case onStartTrackingTouch is called followed by
    // a number of onProgressChanged notifications, concluded by onStopTrackingTouch.
    // We're setting the field "mDragging" to true for the duration of the dragging
    // session to avoid jumps in the position in case of ongoing playback.
    //
    // The second scenario involves the user operating the scroll ball, in this
    // case there WON'T BE onStartTrackingTouch/onStopTrackingTouch notifications,
    // we will simply apply the updated position without suspending regular updates.
    private OnSeekBarChangeListener mSeekListener = new OnSeekBarChangeListener() {
        public void onStartTrackingTouch(SeekBar bar) {
            show(3600000);

            mDragging = true;

            // By removing these pending progress messages we make sure
            // that a) we won't update the progress while the user adjusts
            // the seekbar and b) once the user is done dragging the thumb
            // we will post one of these messages to the queue again and
            // this ensures that there will be exactly one message queued up.
            mHandler.removeMessages(SHOW_PROGRESS);
        }

        public void onProgressChanged(SeekBar bar, int progress, boolean fromuser) {
            if (!fromuser) {
                // We're not interested in programmatically generated changes to
                // the progress bar's position.
                return;
            }

            long duration = mPlayer.getDuration();
            long newposition = (duration * progress) / 1000L;
            mPlayer.seekTo( (int) newposition);
            if (mCurrentTime != null)
                mCurrentTime.setText(stringForTime( (int) newposition));
        }

        public void onStopTrackingTouch(SeekBar bar) {
            mDragging = false;
            setProgress();
            updatePausePlay();
            show(sDefaultTimeout);

            // Ensure that progress is properly updated in the future,
            // the call to show() does not guarantee this because it is a
            // no-op if we are already showing.
            mHandler.sendEmptyMessage(SHOW_PROGRESS);
        }
    };

    @Override
    public void setEnabled(boolean enabled) {
    	if(mMediaControl.getVisibility() != View.GONE)
    	{
    		if (mPauseButton != null) {
            	mPauseButton.setEnabled(enabled);
        	}
        	if (mFfwdButton != null) {
            	mFfwdButton.setEnabled(enabled);
        	}
        	if (mRewButton != null) {
            	mRewButton.setEnabled(enabled);
        	}
        	if (mNextButton != null) {
            	mNextButton.setEnabled(enabled);
        	}
        	if (mPrevButton != null) {
        		mPrevButton.setEnabled(enabled);
        	}
        	if (mProgress != null) {
        		mProgress.setEnabled(enabled);
        	}
//        	disableUnsupportedButtons();
    	}
    	
    	if(mSettings.getVisibility() != View.GONE) {
    		mSettings.setEnabled(enabled);
    	}
    	
        super.setEnabled(enabled);
    }

    private View.OnClickListener mRewListener = new View.OnClickListener() {
        public void onClick(View v) {
            int pos = mPlayer.getCurrentPosition();
            pos -= 5000; // milliseconds
            mPlayer.seekTo(pos);
            setProgress();

            show(sDefaultTimeout);
        }
    };

    private View.OnClickListener mFfwdListener = new View.OnClickListener() {
        public void onClick(View v) {
            int pos = mPlayer.getCurrentPosition();
            pos += 15000; // milliseconds
            mPlayer.seekTo(pos);
            setProgress();

            show(sDefaultTimeout);
        }
    };

    public void setFilePathTextView(String filePath) {
    	mFileNameText = filePath;
    	int index = filePath.lastIndexOf('/');
    	if(index >= 0) {
    		mFileNameText = filePath.substring(index+1);
    	}
    	if (mStatus != null && mFileName != null) {
    		mFileName.setText(mFileNameText);
    	}
    }
    
    public void setBatteryTextView(String battery) {
    	mBatteryText = battery;
    	if (mStatus != null && mBatteryRemain != null) {
    		mBatteryRemain.setText(mBatteryText);
    	}
    }
    
    public void setPrevNextVisible(boolean visible) {
    	prevNextVisible = visible;
    };
 
    public void setSetSettingsEnable() {
    	mMediaControl.setVisibility(View.GONE);
        mSettings.setVisibility(View.VISIBLE);
    }
    public void setSubsetEnabled(boolean enabled) {
    	mSubGateButton.setEnabled(enabled);
    	mSubCharSetButton.setEnabled(enabled);
    	mSubColorButton.setEnabled(enabled);
    	mSubCharSizeButton.setEnabled(enabled);
    	mSubOffSetButton.setEnabled(enabled);
    	mSubDelayButton.setEnabled(enabled);

    	Message msg = mHandler.obtainMessage(FADE_OUT);
        mHandler.removeMessages(FADE_OUT);
        mHandler.sendMessageDelayed(msg, sDefaultTimeout);
    }
    
    public void setSetListener(View.OnClickListener set) {
    	mSetListener = set;
        if (mRoot != null && mSettings != null) {
        	mSettings.setOnClickListener(mSetListener);
        }    	
    }
    private View.OnClickListener mUpListener = new View.OnClickListener() {
        public void onClick(View v) {
            mMediaControl.setVisibility(View.VISIBLE);
            mSettings.setVisibility(View.GONE);
        }
    };
    
    public void setPrevNextListeners(View.OnClickListener next, View.OnClickListener prev) {
        mNextListener = next;
        mPrevListener = prev;

        if (mRoot != null) {
            if (mNextButton != null) {
                mNextButton.setOnClickListener(mNextListener);
                mNextButton.setEnabled(mNextListener != null);
            }
    
            if (mPrevButton != null) {
                mPrevButton.setOnClickListener(mPrevListener);
                mPrevButton.setEnabled(mPrevListener != null);
            }
        }
    }

    public void setBackListener(View.OnClickListener back) {
        mBackListener = back;
        if (mRoot != null && mBackButton != null) {
            mBackButton.setOnClickListener(mBackListener);
        }
    }

    public void setVolumeIncListener(View.OnClickListener volumeinc) {
        mVolumeIncListener = volumeinc;
        if (mRoot != null && mVolumeIncButton != null) {
            mVolumeIncButton.setOnClickListener(mVolumeIncListener);
        }
    }
    
    public void setVolumeDecListener(View.OnClickListener volumedec) {
        mVolumeDecListener = volumedec;
        if (mRoot != null && mVolumeDecButton != null) {
            mVolumeDecButton.setOnClickListener(mVolumeDecListener);
        }
    }
    
    public void setSubGateListener(View.OnClickListener gate) {
        mSubGateListener = gate;
        if (mRoot != null && mSubGateButton != null) {
            mSubGateButton.setOnClickListener(mSubGateListener);
        }
    }
    
    public void setSubSelectListener(View.OnClickListener select) {
        mSubSelectListener = select;
        if (mRoot != null && mSubSelectButton != null) {
            mSubSelectButton.setOnClickListener(mSubSelectListener);
        }
    }
    
    public void setSubCharSetListener(View.OnClickListener charset) {
        mSubCharSetListener = charset;
        if (mRoot != null && mSubCharSetButton != null) {
            mSubCharSetButton.setOnClickListener(mSubCharSetListener);
        }
    }
    
    public void setSubColorListener(View.OnClickListener color) {
        mSubColorListener = color;
        if (mRoot != null && mSubColorButton != null) {
            mSubColorButton.setOnClickListener(mSubColorListener);
        }
    }
    
    public void setSubCharSizeListener(View.OnClickListener charsize) {
        mSubCharSizeListener = charsize;
        if (mRoot != null && mSubCharSizeButton != null) {
            mSubCharSizeButton.setOnClickListener(mSubCharSizeListener);
        }
    }

    public void setSubOffSetListener(View.OnClickListener offset) {
        mSubOffSetListener = offset;
        if (mRoot != null && mSubOffSetButton != null) {
            mSubOffSetButton.setOnClickListener(mSubOffSetListener);
        }
    }
    
    public void setSubDelayListener(View.OnClickListener delay) {
        mSubDelayListener = delay;
        if (mRoot != null && mSubDelayButton != null) {
            mSubDelayButton.setOnClickListener(mSubDelayListener);
        }
    }

    public void setZoomListener(View.OnClickListener zoom) {
        mZoomListener = zoom;
        if (mRoot != null && mZoomButton != null) {
            mZoomButton.setOnClickListener(mZoomListener);
        }
    }
    
    public void set3DListener(View.OnClickListener mode3D) {
        m3DListener = mode3D;
        if (mRoot != null && m3DButton != null) {
            m3DButton.setOnClickListener(m3DListener);
        }
    }
         
    public void setTrackListener(View.OnClickListener track) {
        mTrackListener = track;
        if (mRoot != null && mTrackButton != null) {
            mTrackButton.setOnClickListener(mTrackListener);
        }
    }
    
    public void setRepeatListener(View.OnClickListener repeat) {
        mRepeatListener = repeat;
        if (mRoot != null && mRepeatButton != null) {
            mRepeatButton.setOnClickListener(mRepeatListener);
        }
    }
    
    public interface MediaPlayerControl {
        void    start();
        void    pause();
        int     getDuration();
        int     getCurrentPosition();
        void    seekTo(int pos);
        boolean isPlaying();
        int     getBufferPercentage();
        boolean canPause();
        boolean canSeekBackward();
        boolean canSeekForward();
        
        int getSubPosition();
        int setSubPosition(int percent);
    }
}
