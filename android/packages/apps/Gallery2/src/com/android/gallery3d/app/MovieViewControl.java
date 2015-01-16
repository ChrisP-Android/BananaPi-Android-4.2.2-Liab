/*
 * Copyright (C) 2009 The Android Open Source Project
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

import java.io.File;
import java.io.UnsupportedEncodingException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Random;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.BroadcastReceiver;
import android.content.ContentResolver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.IContentProvider;
import android.content.Intent;
import android.content.DialogInterface.OnCancelListener;
import android.content.DialogInterface.OnClickListener;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.content.res.Resources;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;
import android.graphics.Color;
import android.media.MediaPlayer;
import android.media.MediaPlayer.SubInfo;
import android.media.MediaPlayer.TrackInfoVendor;
import android.net.Uri;
import android.os.Handler;
import android.os.PowerManager;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.os.SystemClock;
import android.provider.MediaStore;
import android.provider.MediaStore.Video;
import android.util.DisplayMetrics;
import android.util.Log;
import android.hardware.input.InputManager;
import android.view.IWindowManager;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.View.OnTouchListener;
import android.view.DisplayManagerAw;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.ListView;
import android.widget.SeekBar;
import android.widget.TextView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.SeekBar.OnSeekBarChangeListener;

import com.android.gallery3d.R;
import com.android.gallery3d.app.ColorPickerDialog.OnColorChangedListener;
import com.android.gallery3d.app.SlipSwitchDialog.OnSwitchResultListener;

public class MovieViewControl implements MediaPlayer.OnErrorListener, MediaPlayer.OnCompletionListener,
VideoView.OnSubFocusItems{

    private static final String TAG = "MovieViewControl";
    private static final String STORE_NAME = "SubtitleSetting";

    private static final String EDITOR_SUBGATE = "MovieViewControl.SUBGATE";
    private static final String EDITOR_SUBSELECT = "MovieViewControl.SUBSELECT";
    private static final String EDITOR_SUBCHARSET = "MovieViewControl.SUBCHARSET";
    private static final String EDITOR_SUBCOLOR = "MovieViewControl.SUBCOLOR";
    private static final String EDITOR_SUBCHARSIZE = "MovieViewControl.SUBCHARSIZE";
    private static final String EDITOR_SUBOFFSET = "MovieViewControl.SUBOFFSET";
    private static final String EDITOR_SUBDELAY = "MovieViewControl.SUBDELAY";
    private static final String EDITOR_ZOOM = "MovieViewControl.MODEZOOM";
    private static final String EDITOR_MODE3D = "MovieViewControl.MODE3D";
    private static final String EDITOR_TRACK = "MovieViewControl.TRACK";
    private static final String EDITOR_REPEAT = "MovieViewControl.REPEAT";
    
    // If we resume the acitivty with in RESUMEABLE_TIMEOUT, we will keep playing.
    // Otherwise, we pause the player.
    private static final long RESUMEABLE_TIMEOUT = 3 * 60 * 1000; // 3 mins

    // Copied from MediaPlaybackService in the Music Player app. Should be
    // public, but isn't.
    private static final String SERVICECMD = "com.android.music.musicservicecommand";
    private static final String CMDNAME = "command";
    private static final String CMDPAUSE = "pause";

    private final VideoView mVideoView;
    private final View mProgressView;
    private Uri mUri;
    private final ContentResolver mContentResolver;
	private final SharedPreferences sp;
	private final SharedPreferences.Editor editor;  
	
    private long mResumeableTime = Long.MAX_VALUE;
    private int mVideoPosition = 0;
    private int mCurrentTrackSave, mCurrentSubSave;		//used for video onPause/onResume
	private boolean mOnPause = false;
	int mBookMark, mDuration;
    private int mCurrentIndex = 0;
    private String mPlayListType;
    private ArrayList<String> mPlayList;

    private BookmarkService mBookmarkService;
    private boolean mToQuit = false;
    private Context mContext; 
    private Resources mRes;
    private PowerManager.WakeLock mWakeLock;
    private MediaController mMediaController;
    private Dialog mListDialog, mCharsizeDialog, mDelayDialog;
    private View mDialogView;
    private TextView mDialogTitle;
    private ListView mListView;
    private String mControlFocus = null;
    private int mListFocus = 0, m3DFocus = 2;
    private int mLastSystemUiVis = 0;

	private final AwHdmiPluggedReceiver mAwHdmiPluggedReceiver;
	
    private boolean mHdmiPluggedFirstFlags = true;
	private boolean mCurHdmiPlugged = false;
	private boolean mPreHdmiPlugged = false;
    
    private static final int PLAYMODE_REPEAT_ALL = 0;
    private static final int PLAYMODE_SEQUENCE = 1;
    private static final int PLAYMODE_REPEAT_ONE = 2;
    private static final int PLAYMODE_RANDOM = 3;
    private int mRepeatMode = PLAYMODE_REPEAT_ALL;
    
    /* seek bar param */
	private TextView mCharsizeScheduleText, mDelayScheduleText;
	private int mCharsizePaddingLeft, mDelayPaddingLeft;
	private SeekBar mListSeekBar, mCharsizeSeekBar, mDelaySeekBar;
	private int mMinValue;
	/* sub switch */
	private SlipSwitchDialog mSlipSwitch;
	/* sub color */
	private ColorPickerDialog mColorPicker;

    Handler mHandler = new Handler();

    Runnable mPlayingChecker = new Runnable() {
        public void run() {
            if (mVideoView.isPlaying()) {
                mProgressView.setVisibility(View.GONE);
            } else {
                mHandler.postDelayed(mPlayingChecker, 250);
            }
        }
    };

    public static String formatDuration(final Context context, int durationMs) {
        int duration = durationMs / 1000;
        int h = duration / 3600;
        int m = (duration - h * 3600) / 60;
        int s = duration - (h * 3600 + m * 60);
        String durationValue;
        if (h == 0) {
            durationValue = String.format(context.getString(R.string.details_ms), m, s);
        } else {
            durationValue = String.format(context.getString(R.string.details_hms), h, m, s);
        }
        return durationValue;
    }

	public MovieViewControl(View rootView, Context context, Intent intent) {
        mContentResolver = context.getContentResolver();
        mVideoView = (VideoView) rootView.findViewById(R.id.surface_view);
        mProgressView = rootView.findViewById(R.id.progress_indicator);

        mContext = context;
        mUri = Uri2File2Uri(intent.getData());
        mRes = mContext.getResources();
        sp = mContext.getSharedPreferences(STORE_NAME, Context.MODE_PRIVATE);
		editor = sp.edit();
		
        // For streams that we expect to be slow to start up, show a
        // progress spinner until playback starts.
        String scheme = mUri.getScheme();
        if ("http".equalsIgnoreCase(scheme) || "rtsp".equalsIgnoreCase(scheme)) {
            mHandler.postDelayed(mPlayingChecker, 250);
        } else {
            mProgressView.setVisibility(View.GONE);
        }

        SetListDialogParam();
        setCharsizeDialogParam();
        setDelayDialogParam();
        
        PowerManager pm = (PowerManager)context.getSystemService(Context.POWER_SERVICE);
        mWakeLock = pm.newWakeLock(PowerManager.SCREEN_BRIGHT_WAKE_LOCK|PowerManager.ON_AFTER_RELEASE, TAG);
        mWakeLock.setReferenceCounted(false);
		
		mAwHdmiPluggedReceiver = new AwHdmiPluggedReceiver();
		mAwHdmiPluggedReceiver.register();

        mVideoView.setOnSubFocusItems(this);
        mVideoView.setOnErrorListener(this);
        mVideoView.setOnCompletionListener(this);
        mVideoView.setVideoURI(mUri);
        mMediaController = new MediaController(context, rootView);
        setImageButtonListener(mMediaController);
        mVideoView.setMediaController(mMediaController);
        mMediaController.setFilePathTextView(mUri.getPath());

        rootView.setOnTouchListener(new OnTouchListener(){

			@Override
			public boolean onTouch(View v, MotionEvent me) {
				// TODO Auto-generated method stub
	            if (me.getAction() == MotionEvent.ACTION_DOWN) {
	                if (mMediaController.isShowing()) {
	                	mMediaController.hide();
	                }
	                else{
	                	mMediaController.show();
	                }
	            }				
				return false;
			}
        	
        });

        rootView.setOnSystemUiVisibilityChangeListener(
			new View.OnSystemUiVisibilityChangeListener() {
            @Override
            public void onSystemUiVisibilityChange(int visibility) {
                int diff = mLastSystemUiVis ^ visibility;
                mLastSystemUiVis = visibility;
                if ((diff & View.SYSTEM_UI_FLAG_HIDE_NAVIGATION) != 0
                        && (visibility & View.SYSTEM_UI_FLAG_HIDE_NAVIGATION) == 0) {
                    mMediaController.show();
                }
            }
        });
        rootView.setSystemUiVisibility(View.STATUS_BAR_HIDDEN | View.SYSTEM_UI_FLAG_FULLSCREEN | View.SYSTEM_UI_FLAG_LAYOUT_STABLE | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION);


        /* create playlist */
        mPlayListType =  intent.getStringExtra(MediaStore.PLAYLIST_TYPE);
        mPlayList = new ArrayList<String>();
        if(mPlayListType != null) {
        	if(mPlayListType.equalsIgnoreCase(MediaStore.PLAYLIST_TYPE_CUR_FOLDER)) {
         		/* create playlist from current folder */
         		createFolderDispList();
        	} else if(mPlayListType.equalsIgnoreCase(MediaStore.PLAYLIST_TYPE_MEDIA_PROVIDER)) {
        		/* create playlist from mediaprovider */
        		createMediaProviderDispList(mUri, mContext);
        	}
        	
        	if(scheme != null && scheme.equalsIgnoreCase("file")) {
	         	mMediaController.setPrevNextVisible(true);		// make prev/next button visible
        	}
        	else{
        		Log.w(TAG,"############scheme = " + scheme);
        	}
        }        
        
        mMediaController.setVolumeIncListener(new View.OnClickListener() {
			public void onClick(View arg0) {
				// TODO Auto-generated method stub
    			sendKeyIntent(KeyEvent.KEYCODE_VOLUME_UP);
			}
    	});
        mMediaController.setVolumeDecListener(new View.OnClickListener() {
			public void onClick(View arg0) {
				// TODO Auto-generated method stub
    			sendKeyIntent(KeyEvent.KEYCODE_VOLUME_DOWN);
			}
    	});
        mMediaController.setBackListener(new View.OnClickListener() {
			public void onClick(View arg0) {
				// TODO Auto-generated method stub
    			sendKeyIntent(KeyEvent.KEYCODE_BACK);
			}
        }); 
        mMediaController.setPrevNextListeners(new View.OnClickListener() {
            public void onClick(View v) {
            	int size = mPlayList.size();
            	if(mCurrentIndex >= 0 && size > 0)
            	{
            		mCurrentIndex = (mCurrentIndex+1)%size;
            		String path = mPlayList.get(mCurrentIndex);
            		mUri = Uri.fromFile(new File(path));
            		playFile();
            	}
            }
        }, 
        new View.OnClickListener() {
            public void onClick(View v) {
            	int size = mPlayList.size();
            	if(mCurrentIndex >= 0 && size > 0)
            	{
            		mCurrentIndex = (mCurrentIndex - 1 + size) % size;
            		String path = mPlayList.get(mCurrentIndex);
            		mUri = Uri.fromFile(new File(path));
            		playFile();
            	}
            }
        }); 
        
        // make the video view handle keys for seeking and pausing
        mVideoView.requestFocus();
        
        Intent i = new Intent(SERVICECMD);
        i.putExtra(CMDNAME, CMDPAUSE);
        context.sendBroadcast(i);

        mBookmarkService = new BookmarkService(mContext);
        final int bookmark = getBookmark();
        if (bookmark > 0) {
            AlertDialog.Builder builder = new AlertDialog.Builder(context);
            builder.setTitle(R.string.resume_playing_title);
            builder.setMessage(String.format(context.getString(R.string.resume_playing_message), 
            		formatDuration(context, bookmark)));
            builder.setOnCancelListener(new OnCancelListener() {
                public void onCancel(DialogInterface dialog) {
                    onCompletion();
                }
            });
            builder.setPositiveButton(R.string.resume_playing_resume, new OnClickListener() {
                public void onClick(DialogInterface dialog, int which) {
                    mVideoView.seekTo(bookmark);
                    mVideoView.start();
                }
            });
            builder.setNegativeButton(R.string.resume_playing_restart, new OnClickListener() {
                public void onClick(DialogInterface dialog, int which) {
                    mVideoView.start();
                }
            });
            builder.show();
        } else {
            mVideoView.start();
        }
    }
        
    private Uri Uri2File2Uri(Uri videoUri) {
    	String path = null;
    	Cursor c = null;
        IContentProvider mMediaProvider = mContext.getContentResolver().acquireProvider("media");
        String[] VIDEO_PROJECTION = new String[] { Video.Media.DATA };
        
        /* get video file */
        try {
			c = mMediaProvider.query(videoUri, VIDEO_PROJECTION, null, null, null, null);
		} catch (RemoteException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
        if(c != null)
        {
            try {
                while (c.moveToNext()) {
                    path = c.getString(0);
                }
            } finally {
                c.close();
                c = null;
            }
        }
        
        if(path != null) {
        	return Uri.fromFile(new File(path));
        }else {
        	Log.w(TAG,"************ Uri2File2Uri failed ***************");
        	return videoUri;
        }
    }
	
    //* ACTION_HDMISTATUS_CHANGED
	private class AwHdmiPluggedReceiver extends BroadcastReceiver {
		public void register() {
			mContext.registerReceiver(this,
					new IntentFilter(Intent.ACTION_HDMISTATUS_CHANGED));
		}

		public void unregister() {
			mContext.unregisterReceiver(this);
		}

		@Override
		public void onReceive(Context context, Intent intent) {
			int wasPlugged = mCurHdmiPlugged?1:0;
			mCurHdmiPlugged = intent.getIntExtra(DisplayManagerAw.EXTRA_HDMISTATUS, wasPlugged)>0?true:false;

			if (mHdmiPluggedFirstFlags)
			{
			    mPreHdmiPlugged = mCurHdmiPlugged;
			    mHdmiPluggedFirstFlags = false;
			}
			if (mPreHdmiPlugged != mCurHdmiPlugged)
			{
			    int rotation = ((Activity)mContext).getWindowManager().getDefaultDisplay().getRotation();
				if (rotation != 0)
				{
					onPause();
					onResume();	
				}
			}
			mPreHdmiPlugged = mCurHdmiPlugged;
		}
	}
    
    private void createFolderDispList() {
    	String fileNameText, filePathText;
    	File filePath;
    	
    	String[] fileEndingVideo = mRes.getStringArray(R.array.fileEndingVideo);
    	fileNameText = mUri.getPath();
    	int index = fileNameText.lastIndexOf('/');
    	if(index >= 0) {
    		filePathText = fileNameText.substring(0, index);
    		filePath = new File(filePathText);
    		File[] fileList = filePath.listFiles();
    		if(fileList != null && filePath.isDirectory()) {
    			for(File currenFile : fileList) {
    				String fileName = currenFile.getName();
    				int indexPoint = fileName.lastIndexOf('.');
    				if(indexPoint > 0 && currenFile.isFile()) {
    					String fileEnd = fileName.substring(indexPoint+1);
    					for(int i = 0;i < fileEndingVideo.length; i++) {
        					if(fileEnd.equalsIgnoreCase(fileEndingVideo[i])) {
        						mPlayList.add(currenFile.getPath());
        						break;
        					}
        				}
    				}
    			}
    		}
    	}
    	Collections.sort(mPlayList);

        /* get current index */
        mCurrentIndex = 0;
        String mCurrentPath = mUri.getPath();
        for(int i = 0;i < mPlayList.size();i++) {
        	if( mCurrentPath.equalsIgnoreCase(mPlayList.get(i)) ) {
        		mCurrentIndex = i;
        		break;
        	}
        }
    }

    private void createMediaProviderDispList(Uri uri, Context mContext) {
        Cursor c = null;
        IContentProvider mMediaProvider = mContext.getContentResolver().acquireProvider("media");
        Uri mVideoUri = Video.Media.getContentUri("external");
        String[] VIDEO_PROJECTION = new String[] { Video.Media.DATA };
        
        /* get playlist */
        try {
			c = mMediaProvider.query(mVideoUri, VIDEO_PROJECTION, null, null, null, null);
		} catch (RemoteException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
        if(c != null)
        {
            try {
                while (c.moveToNext()) {
                    String path = c.getString(0);
                    if(new File(path).exists()){
                    	mPlayList.add(path);
                    }
                }
            } finally {
                c.close();
                c = null;
            }
            
            /* get current index */
            mCurrentIndex = 0;
            String mCurrentPath = mUri.getPath();
            for(int i = 0;i < mPlayList.size();i++) {
            	if( mCurrentPath.equalsIgnoreCase(mPlayList.get(i)) ) {
            		mCurrentIndex = i;
            		break;
            	}
            }
        }
    }

    private void SetListDialogParam() {
        mDialogView = View.inflate(mContext, R.layout.dialog_list, null);
        mDialogTitle = (TextView) mDialogView.findViewById(R.id.list_title);
        mListView = (ListView) mDialogView.findViewById(R.id.list);
        mListView.setItemsCanFocus(true);
        mListView.setChoiceMode(ListView.CHOICE_MODE_SINGLE);
        mListView.setOnItemClickListener(mItemClickListener);
        Button confirm = (Button) mDialogView.findViewById(R.id.list_confirm);
        confirm.setOnClickListener(new View.OnClickListener(){
			public void onClick(View arg0) {
                mMediaController.setHolding(false);
				mListDialog.hide();
			}
        });
        
        mListDialog =  new Dialog(mContext,R.style.dialog);
    	mListDialog.setContentView(mDialogView);
    	mListDialog.setOnCancelListener(new OnCancelListener(){
			public void onCancel(DialogInterface arg0) {
                mMediaController.setHolding(false);
				mListDialog.hide();
			}
        });    	
    }

    private void setCharsizeDialogParam() {
    	View dialogView = View.inflate(mContext, R.layout.dialog_charsize, null);
        mCharsizeSeekBar = (SeekBar) dialogView.findViewById(R.id.seekbar_charsize_progress);
        //mCharsizeSeekBar.setBackgroundResource(android.R.id.progress);
        mCharsizeScheduleText = (TextView) dialogView.findViewById(R.id.charsize_schedule);
//        final int paddingLeft = mCharsizeScheduleText.getPaddingLeft();
        mCharsizeSeekBar.setOnSeekBarChangeListener(new OnSeekBarChangeListener() {

			public void onProgressChanged(SeekBar seekBar, int progress,
					boolean fromUser) {
				// TODO Auto-generated method stub
				Log.i("SeekBarTest","proggress changed");
				int position = mCharsizeSeekBar.getProgress();
				SetCharsizeText(position);
	    	}

			public void onStartTrackingTouch(SeekBar seekBar) {
				// TODO Auto-generated method stub
				
				Log.i("SeekBarTest","proggress changed");
				int position = mCharsizeSeekBar.getProgress();
				SetCharsizeText(position);
			}

			public void onStopTrackingTouch(SeekBar seekBar) {
				// TODO Auto-generated method stub
				mListFocus = mCharsizeSeekBar.getProgress() + mMinValue;
				mVideoView.setSubFontSize(mListFocus);
				editor.putInt(mControlFocus, mListFocus);
				editor.commit();
			}
        	
        });
        
        Button confirm = (Button) dialogView.findViewById(R.id.charsize_confirm);
        confirm.setOnClickListener(new View.OnClickListener(){
			public void onClick(View arg0) {
                mMediaController.setHolding(false);
                mCharsizeDialog.hide();
			}
        });
        
        mCharsizeDialog =  new Dialog(mContext,R.style.dialog);
        mCharsizeDialog.setContentView(dialogView);
        mCharsizeDialog.setOnCancelListener(new OnCancelListener(){
			public void onCancel(DialogInterface arg0) {
                mMediaController.setHolding(false);
                mCharsizeDialog.hide();
			}
        });
    }

    private void setDelayDialogParam() {
    	View dialogView = View.inflate(mContext, R.layout.dialog_delay, null);
        mDelaySeekBar = (SeekBar) dialogView.findViewById(R.id.seekbar_delay_progress);
        //mDelaySeekBar.setBackgroundResource(android.R.id.progress);
        mDelayScheduleText = (TextView) dialogView.findViewById(R.id.delay_schedule);
        mCharsizePaddingLeft = mDelayScheduleText.getPaddingLeft();
        mDelaySeekBar.setOnSeekBarChangeListener(new OnSeekBarChangeListener() {

			public void onProgressChanged(SeekBar seekBar, int progress,
					boolean fromUser) {
				// TODO Auto-generated method stub
				Log.i("SeekBarTest","proggress changed");
				int position = mDelaySeekBar.getProgress();
				SetDelayText(position*50);
	    	}

			public void onStartTrackingTouch(SeekBar seekBar) {
				// TODO Auto-generated method stub
				
			}

			public void onStopTrackingTouch(SeekBar seekBar) {
				// TODO Auto-generated method stub
				mListFocus = 50 * mDelaySeekBar.getProgress() + mMinValue;
				mVideoView.setSubDelay(mListFocus);
			}
        	
        });
        
        Button confirm = (Button) dialogView.findViewById(R.id.delay_confirm);
        confirm.setOnClickListener(new View.OnClickListener(){
			public void onClick(View arg0) {
                mMediaController.setHolding(false);
                mDelayDialog.hide();
			}
        });
        
        mDelayDialog =  new Dialog(mContext,R.style.dialog);
        mDelayDialog.setContentView(dialogView);
        mDelayDialog.setOnCancelListener(new OnCancelListener(){
			public void onCancel(DialogInterface arg0) {
                mMediaController.setHolding(false);
                mDelayDialog.hide();
			}
        });
    }
    
    private void SetCharsizeText(int offset) {
		int max = mCharsizeSeekBar.getMax();
		int layout_width =  mCharsizeSeekBar.getLayoutParams().width;
		layout_width = diptopx(layout_width);
		int textPos = offset * layout_width / max;
		mCharsizeScheduleText.setPadding(mCharsizePaddingLeft+textPos, 0, 0, 0);
		mCharsizeScheduleText.setText(String.valueOf(offset+mMinValue));
    }
    
    private void SetDelayText(int offset) {
		int max = 50 * mDelaySeekBar.getMax();
		int layout_width =  mDelaySeekBar.getLayoutParams().width;
		layout_width = diptopx(layout_width);
		int textPos = offset * layout_width / max;
		mDelayScheduleText.setPadding(mDelayPaddingLeft+textPos, 0, 0, 0);
		mDelayScheduleText.setText(String.valueOf(offset+mMinValue));
    }
    
    private int diptopx(int dipValue) {
        DisplayMetrics metrics = new DisplayMetrics(); 
        ((Activity)mContext).getWindowManager().getDefaultDisplay().getMetrics(metrics);
        float density = metrics.density;
        float pxValue = dipValue * density;
        
        return (int)pxValue;
    }
    
	private void sendKeyIntent(int keycode){
		final int keyCode = keycode;
		// to avoid deadlock, start a thread to perform operations
        Thread sendKeyDelay = new Thread(){   
            public void run() {
                try {
                    int count = 1;
                    if(keyCode == KeyEvent.KEYCODE_BACK)
                        count = 2;
                    
                    IWindowManager wm = IWindowManager.Stub.asInterface(ServiceManager.getService("window"));
                    for(int i = 0; i < count; i++){
                        Thread.sleep(100);
                        long now = SystemClock.uptimeMillis();
                        if(!mOnPause) {
	                        KeyEvent keyDown = new KeyEvent(now, now, KeyEvent.ACTION_DOWN, keyCode, 0);
        					InputManager.getInstance().injectInputEvent(keyDown, InputManager.INJECT_INPUT_EVENT_MODE_ASYNC);
//	                        wm.injectKeyEvent(keyDown, false);   
	            
	                        KeyEvent keyUp = new KeyEvent(now, now, KeyEvent.ACTION_UP, keyCode, 0);
	                        InputManager.getInstance().injectInputEvent(keyUp, InputManager.INJECT_INPUT_EVENT_MODE_ASYNC);   
//	                        wm.injectKeyEvent(keyUp, false);
                        }
                    }  
                } catch (InterruptedException e) {
                    e.printStackTrace();   
                }   
            }   
        };
        sendKeyDelay.start();
    }

    private void setImageButtonListener(MediaController mediaController) {
    	mediaController.setSetListener(mSetListener);
    	mediaController.setSubGateListener(mSubGateListener);
    	mediaController.setSubSelectListener(mSubSelectListener);
    	mediaController.setSubCharSetListener(mSubCharSetListener);
    	mediaController.setSubColorListener(mSubColorListener);
    	mediaController.setSubCharSizeListener(mSubCharSizeListener);
    	mediaController.setSubOffSetListener(mSubOffSetListener);
    	mediaController.setSubDelayListener(mSubDelayListener);
    	mediaController.setZoomListener(mZoomListener);
    	mediaController.set3DListener(m3DListener);
    	mediaController.setTrackListener(mTrackListener);
    	mediaController.setRepeatListener(mRepeatListener);
    }
    
    private View.OnClickListener mSetListener = new View.OnClickListener() {
        public void onClick(View v) {
        	mMediaController.setSetSettingsEnable();
        	SubInfo[] subList = mVideoView.getSubList();
        	if(subList != null && subList.length > 0) {
    			mMediaController.setSubsetEnabled(true);
    		} else {
    			mMediaController.setSubsetEnabled(false);
    		}
        }
    };
    
    private OnItemClickListener mItemClickListener = new OnItemClickListener() {

		public void onItemClick(AdapterView<?> arg0, View arg1, int position, long id) {
			if(position == mListFocus) {
				/* the same item */
				return;
			}
			
			int ret;	// mVideoView set the sub param's return value
			if(mControlFocus.equals(EDITOR_SUBSELECT)) {
					/* sub select */
				ret = mVideoView.switchSub(position); 
        		if(ret == 0) {
        			mListFocus = position;
        		} else {
        			Log.w(TAG, "*********** change the sub select failed !");
        		}
        	} else if(mControlFocus.equals(EDITOR_SUBCHARSET)) {
        		/* sub charset */
        		String[] listCharSet = mRes.getStringArray(R.array.screen_charset_values);
        		ret = mVideoView.setSubCharset(listCharSet[position]); 
        		if(ret == 0) {
        			mListFocus = position;
        		} else {
        			Log.w(TAG, "*********** change the sub charset failed !");
        		}
        	} else if(mControlFocus.equals(EDITOR_TRACK)) {
        		/* track */
        		ret = mVideoView.switchTrack(position); 
        		if(ret == 0) {
        			mListFocus = position;
        		} else {
        			Log.w(TAG, "*********** change the sub track failed !");
        		}
        	}
        	else if(mControlFocus.equals(EDITOR_REPEAT)){
        		/* repeat */
				mRepeatMode = position;
	        	editor.putInt(EDITOR_REPEAT, mRepeatMode);
				editor.commit();
        	}
        	else if(mControlFocus.equals(EDITOR_MODE3D)) {
        		/* 3D mode */
    			mListFocus = position;
    			m3DFocus = position;
        		set3DMode(position);
        	}
        	else if(mControlFocus.equals(EDITOR_ZOOM)) {
        		/* zoom mode */
        		mListFocus = position;
        		mVideoView.setZoomMode(position); 
	        	editor.putInt(EDITOR_ZOOM, position);
				editor.commit();
        	}
		}
    };
    
    private void set3DMode(int position) {
    	int[] listValue = mRes.getIntArray(R.array.screen_3d_values);
    	int crrentValue = listValue[position];
    	if(crrentValue == 0) {
    		mVideoView.setInputDimensionType(MediaPlayer.PICTURE_3D_MODE_SIDE_BY_SIDE);
			mVideoView.setOutputDimensionType(MediaPlayer.DISPLAY_3D_MODE_HALF_PICTURE);
    	} else if(crrentValue == 1) {
    		mVideoView.setInputDimensionType(MediaPlayer.PICTURE_3D_MODE_TOP_TO_BOTTOM);
			mVideoView.setOutputDimensionType(MediaPlayer.DISPLAY_3D_MODE_HALF_PICTURE);
    	} else if(crrentValue == 2) {
    		mVideoView.setOutputDimensionType(MediaPlayer.DISPLAY_3D_MODE_2D);
    	} else if(crrentValue == 3) {
    		mVideoView.setInputDimensionType(MediaPlayer.PICTURE_3D_MODE_SIDE_BY_SIDE);
    		mVideoView.setOutputDimensionType(MediaPlayer.DISPLAY_3D_MODE_3D);
    	} else if(crrentValue == 4) {
    		mVideoView.setInputDimensionType(MediaPlayer.PICTURE_3D_MODE_TOP_TO_BOTTOM);
    		mVideoView.setOutputDimensionType(MediaPlayer.DISPLAY_3D_MODE_3D);
    	} else if(crrentValue == 5) {
    		mVideoView.setInputDimensionType(MediaPlayer.PICTURE_3D_MODE_DOUBLE_STREAM);
    		mVideoView.setOutputDimensionType(MediaPlayer.DISPLAY_3D_MODE_3D);
    	} else if(crrentValue == 6) {
    		mVideoView.setInputDimensionType(MediaPlayer.PICTURE_3D_MODE_LINE_INTERLEAVE);
    		mVideoView.setOutputDimensionType(MediaPlayer.DISPLAY_3D_MODE_3D);
    	}/* else if(crrentValue == 7) {
    		mVideoView.setOutputDimensionType(MediaPlayer.DISPLAY_3D_MODE_3D);
    		mVideoView.setInputDimensionType(MediaPlayer.PICTURE_3D_MODE_COLUME_INTERLEAVE);
    	}*/ else if(crrentValue == 8) {
    		mVideoView.setInputDimensionType(MediaPlayer.PICTURE_3D_MODE_SIDE_BY_SIDE);
    		mVideoView.setAnaglaghType(MediaPlayer.ANAGLAGH_RED_BLUE);
    		mVideoView.setOutputDimensionType(MediaPlayer.DISPLAY_3D_MODE_ANAGLAGH);
    	} else if(crrentValue == 9) {
    		mVideoView.setInputDimensionType(MediaPlayer.PICTURE_3D_MODE_SIDE_BY_SIDE);
    		mVideoView.setAnaglaghType(MediaPlayer.ANAGLAGH_RED_GREEN);
    		mVideoView.setOutputDimensionType(MediaPlayer.DISPLAY_3D_MODE_ANAGLAGH);
    	} else if(crrentValue == 10) {
    		mVideoView.setInputDimensionType(MediaPlayer.PICTURE_3D_MODE_SIDE_BY_SIDE);
    		mVideoView.setAnaglaghType(MediaPlayer.ANAGLAGH_RED_CYAN);
    		mVideoView.setOutputDimensionType(MediaPlayer.DISPLAY_3D_MODE_ANAGLAGH);
    	} else if(crrentValue == 11) {
    		mVideoView.setInputDimensionType(MediaPlayer.PICTURE_3D_MODE_SIDE_BY_SIDE);
    		mVideoView.setAnaglaghType(MediaPlayer.ANAGLAGH_COLOR);
    		mVideoView.setOutputDimensionType(MediaPlayer.DISPLAY_3D_MODE_ANAGLAGH);
    	} else if(crrentValue == 12) {
    		mVideoView.setInputDimensionType(MediaPlayer.PICTURE_3D_MODE_SIDE_BY_SIDE);
    		mVideoView.setAnaglaghType(MediaPlayer.ANAGLAGH_HALF_COLOR);
    		mVideoView.setOutputDimensionType(MediaPlayer.DISPLAY_3D_MODE_ANAGLAGH);
    	} else if(crrentValue == 13) {
    		mVideoView.setInputDimensionType(MediaPlayer.PICTURE_3D_MODE_SIDE_BY_SIDE);
    		mVideoView.setAnaglaghType(MediaPlayer.ANAGLAGH_OPTIMIZED);
    		mVideoView.setOutputDimensionType(MediaPlayer.DISPLAY_3D_MODE_ANAGLAGH);
    	} else if(crrentValue == 14) {
    		mVideoView.setInputDimensionType(MediaPlayer.PICTURE_3D_MODE_SIDE_BY_SIDE);
    		mVideoView.setAnaglaghType(MediaPlayer.ANAGLAGH_YELLOW_BLUE);
    		mVideoView.setOutputDimensionType(MediaPlayer.DISPLAY_3D_MODE_ANAGLAGH);
    	} else if(crrentValue == 15) {
    		mVideoView.setInputDimensionType(MediaPlayer.PICTURE_3D_MODE_TOP_TO_BOTTOM);
    		mVideoView.setAnaglaghType(MediaPlayer.ANAGLAGH_RED_BLUE);
    		mVideoView.setOutputDimensionType(MediaPlayer.DISPLAY_3D_MODE_ANAGLAGH);
    	} else if(crrentValue == 16) {
    		mVideoView.setInputDimensionType(MediaPlayer.PICTURE_3D_MODE_TOP_TO_BOTTOM);
    		mVideoView.setAnaglaghType(MediaPlayer.ANAGLAGH_RED_GREEN);
    		mVideoView.setOutputDimensionType(MediaPlayer.DISPLAY_3D_MODE_ANAGLAGH);
    	} else if(crrentValue == 17) {
    		mVideoView.setInputDimensionType(MediaPlayer.PICTURE_3D_MODE_TOP_TO_BOTTOM);
    		mVideoView.setAnaglaghType(MediaPlayer.ANAGLAGH_RED_CYAN);
    		mVideoView.setOutputDimensionType(MediaPlayer.DISPLAY_3D_MODE_ANAGLAGH);
    	} else if(crrentValue == 18) {
    		mVideoView.setInputDimensionType(MediaPlayer.PICTURE_3D_MODE_TOP_TO_BOTTOM);
    		mVideoView.setAnaglaghType(MediaPlayer.ANAGLAGH_COLOR);
    		mVideoView.setOutputDimensionType(MediaPlayer.DISPLAY_3D_MODE_ANAGLAGH);
    	} else if(crrentValue == 19) {
    		mVideoView.setInputDimensionType(MediaPlayer.PICTURE_3D_MODE_TOP_TO_BOTTOM);
    		mVideoView.setAnaglaghType(MediaPlayer.ANAGLAGH_HALF_COLOR);
    		mVideoView.setOutputDimensionType(MediaPlayer.DISPLAY_3D_MODE_ANAGLAGH);
    	} else if(crrentValue == 20) {
    		mVideoView.setInputDimensionType(MediaPlayer.PICTURE_3D_MODE_TOP_TO_BOTTOM);
    		mVideoView.setAnaglaghType(MediaPlayer.ANAGLAGH_OPTIMIZED);
    		mVideoView.setOutputDimensionType(MediaPlayer.DISPLAY_3D_MODE_ANAGLAGH);
    	} else if(crrentValue == 21) {
    		mVideoView.setInputDimensionType(MediaPlayer.PICTURE_3D_MODE_TOP_TO_BOTTOM);
    		mVideoView.setAnaglaghType(MediaPlayer.ANAGLAGH_YELLOW_BLUE);
    		mVideoView.setOutputDimensionType(MediaPlayer.DISPLAY_3D_MODE_ANAGLAGH);
    	}
    }
    private View.OnClickListener mSubGateListener = new View.OnClickListener() {
        public void onClick(View v) {
            mMediaController.setHolding(true);
            OnSwitchResultListener l = new OnSwitchResultListener(){

    			public void OnSwitchResult(boolean switchOn) {
    				// TODO Auto-generated method stub
    				if(mSlipSwitch.isShowing()){
    					mSlipSwitch.dismiss();
    				}
    				mVideoView.setSubGate(switchOn); 
                    mMediaController.setHolding(false);
                    
    	        	editor.putBoolean(EDITOR_SUBGATE, switchOn);
    				editor.commit();
    			}
            };
            boolean curSwitch = mVideoView.getSubGate();
            mSlipSwitch = new SlipSwitchDialog(mContext, l, curSwitch);
            mSlipSwitch.setCancelListener(new OnCancelListener(){
    			public void onCancel(DialogInterface arg0) {
                mMediaController.setHolding(false);
    			}
            });
            mSlipSwitch.show();
        }
    };
    
    private View.OnClickListener mSubSelectListener = new View.OnClickListener() {
        public void onClick(View v) {
            mMediaController.setHolding(true);
        	mControlFocus = EDITOR_SUBSELECT;
        	mListFocus = 0;

        	mDialogTitle.setText(R.string.select_title);
        	SubInfo[] subList = mVideoView.getSubList();
        	if(subList != null) {
        		int subCount = subList.length;
        		mListFocus =  mVideoView.getCurSub();
        		String[] transformSub = new String[subList.length];
        		for(int i = 0;i < subCount;i++) {
        			try {
        				if(subList[i].charset.equals(MediaPlayer.CHARSET_UNKNOWN)) {
        					transformSub[i] = new String(subList[i].name, "UTF-8");
        				} else {
        					transformSub[i] = new String(subList[i].name, subList[i].charset);
        				}
					} catch (UnsupportedEncodingException e) {
						// TODO Auto-generated catch block
						Log.w(TAG, "*********** unsupported encoding: "+subList[i].charset);
						e.printStackTrace();
					}
        		}
               	ArrayAdapter<String> adapter = new ArrayAdapter<String>(mContext, 
            			R.layout.simple_list_item_single_choice, 
            			transformSub);
               	mListView.setAdapter(adapter);
               	mListView.setItemChecked(mListFocus, true);
               	mListView.smoothScrollToPosition(mListFocus);
        	} else {
        		ArrayAdapter<String> adapter = new ArrayAdapter<String>(mContext, 
            			R.layout.simple_list_item_single_choice, 
            			mRes.getStringArray(R.array.screen_select_entries));
               	mListView.setAdapter(adapter);
                mListView.setItemChecked(mListFocus, true);
        	}
        	mListDialog.show();
        }
    };

    private View.OnClickListener mSubCharSetListener = new View.OnClickListener() {
        public void onClick(View v) {
            mMediaController.setHolding(true);
        	mControlFocus = EDITOR_SUBCHARSET;
        	
        	mDialogTitle.setText(R.string.charset_title);
        	mListFocus = 0;
        	String currentCharset = mVideoView.getSubCharset();
        	String[] CharsetList = mRes.getStringArray(R.array.screen_charset_values);
        	for(int i = 0; i < CharsetList.length; i++) {
        		if(currentCharset.equalsIgnoreCase(CharsetList[i])) {
        			mListFocus = i;
        			break;
        		}
        	}
        	
        	ArrayAdapter<String> adapter = new ArrayAdapter<String>(mContext, 
        			R.layout.simple_list_item_single_choice, 
        			mRes.getStringArray(R.array.screen_charset_entries));
        	mListView.setAdapter(adapter);
            mListView.setItemChecked(mListFocus, true);
            mListView.smoothScrollToPosition(mListFocus);
        	mListDialog.show();
        }
    };

    private View.OnClickListener mSubColorListener = new View.OnClickListener() {
        public void onClick(View v) {
            mMediaController.setHolding(true);
            OnColorChangedListener l = new OnColorChangedListener(){
                
    			public void colorChanged(int color) {
    				// TODO Auto-generated method stub
    				if(mColorPicker.isShowing()){
    					mColorPicker.dismiss();
    				}
    				mVideoView.setSubColor(color);
    				mMediaController.setHolding(false);
    				
    	        	editor.putInt(EDITOR_SUBCOLOR, color);
    				editor.commit(); 
    			}	
            };
            int curColor = mVideoView.getSubColor();
            mColorPicker = new ColorPickerDialog(mContext, l, curColor);
            mColorPicker.setCancelListener(new OnCancelListener(){
    			public void onCancel(DialogInterface arg0) {
                    mMediaController.setHolding(false);
    			}
            });
            mColorPicker.show();
        }
    };

    private View.OnClickListener mSubCharSizeListener = new View.OnClickListener() {
        public void onClick(View v) {
            mMediaController.setHolding(true);
        	mControlFocus = EDITOR_SUBCHARSIZE;

        	int currentCharSize =  mVideoView.getSubFontSize();
			mDialogTitle.setText(R.string.charsize_title);
			mMinValue = mRes.getInteger(R.integer.min_value_charsize);
			mCharsizeSeekBar.setMax(mRes.getInteger(R.integer.max_step_charsize));
			mCharsizeSeekBar.setProgress(currentCharSize-mMinValue);
			SetCharsizeText(currentCharSize-mMinValue);
        	mCharsizeDialog.show();
        }
    };

    private View.OnClickListener mSubOffSetListener = new View.OnClickListener() {
        public void onClick(View v) {
            mMediaController.setHolding(true);
        	mControlFocus = EDITOR_SUBOFFSET;

        	int currentOffset =  mVideoView.getSubPosition();
			mDialogTitle.setText(R.string.offset_title);
			mMinValue = mRes.getInteger(R.integer.min_value_suboffset);
			mListSeekBar.setProgressDrawable(mContext.getResources().getDrawable(R.drawable.seek_charsize_progress));
			mListSeekBar.setMax(mRes.getInteger(R.integer.max_step_suboffset));
			mListSeekBar.setProgress(currentOffset-mMinValue);
        	//SetSeekText(currentOffset-mMinValue);
        	mListDialog.show();
        }
    };

    private View.OnClickListener mSubDelayListener = new View.OnClickListener() {
        public void onClick(View v) {
            mMediaController.setHolding(true);
        	mControlFocus = EDITOR_SUBDELAY;

        	int currentDelay =  mVideoView.getSubDelay();
			mMinValue = mRes.getInteger(R.integer.min_value_subdelay);
			mDelaySeekBar.setMax(mRes.getInteger(R.integer.max_step_subdelay)/50);
			mDelaySeekBar.setProgress((currentDelay-mMinValue)/50);
			SetDelayText(currentDelay-mMinValue);
        	mDelayDialog.show();
        }
    };


    private View.OnClickListener mZoomListener = new View.OnClickListener() {
        public void onClick(View v) {
            mMediaController.setHolding(true);
        	mControlFocus = EDITOR_ZOOM;
        	
        	mDialogTitle.setText(R.string.zoom_title);
        	int currentMode = sp.getInt(EDITOR_ZOOM, 0);
        	int[] list = mRes.getIntArray(R.array.screen_zoom_values);
        	for(mListFocus = 0; mListFocus < list.length; mListFocus++) {
        		if(currentMode == list[mListFocus]) {
        			break;
        		}
        	}
        	mListFocus = mListFocus%(list.length);
        	ArrayAdapter<String> adapter = new ArrayAdapter<String>(mContext, 
        			R.layout.simple_list_item_single_choice, 
        			mRes.getStringArray(R.array.screen_zoom_entries));
        	mListView.setAdapter(adapter);
            mListView.setItemChecked(mListFocus, true);
            mListView.smoothScrollToPosition(mListFocus);
        	mListDialog.show();
        }
    };
    

    private View.OnClickListener m3DListener = new View.OnClickListener() {
        public void onClick(View v) {
            mMediaController.setHolding(true);
        	mControlFocus = EDITOR_MODE3D;
        	
        	mDialogTitle.setText(R.string.mode3d_title);
        	//int currentMode = mVideoView.getOutputDimensionType();
        	//int[] list = mRes.getIntArray(R.array.screen_3d_values);
        	//for(mListFocus = 0; mListFocus < list.length; mListFocus++) {
        	//	if(currentMode == list[mListFocus]) {
        	//		break;
        	//	}
        	//}
        	//mListFocus = mListFocus%(list.length);
        	mListFocus = m3DFocus;
        	ArrayAdapter<String> adapter = new ArrayAdapter<String>(mContext, 
        			R.layout.simple_list_item_single_choice, 
        			mRes.getStringArray(R.array.screen_3d_entries));
        	mListView.setAdapter(adapter);
            mListView.setItemChecked(mListFocus, true);
            mListView.smoothScrollToPosition(mListFocus);
        	mListDialog.show();
        }
    };
    
    private View.OnClickListener mTrackListener = new View.OnClickListener() {
        public void onClick(View v) {
            mMediaController.setHolding(true);
        	mControlFocus = EDITOR_TRACK;
        	mListFocus = 0;

        	mDialogTitle.setText(R.string.track_title);
        	TrackInfoVendor[] trackList = mVideoView.getTrackList();
        	if(trackList != null) {
        		int trackCount = trackList.length;
        		mListFocus =  mVideoView.getCurTrack();
        		String[] transformTrack = new String[trackList.length];
        		for(int i = 0;i < trackCount;i++) {
        			try {
        				if(trackList[i].charset.equals(MediaPlayer.CHARSET_UNKNOWN)) {
        					transformTrack[i] = new String(trackList[i].name, "UTF-8");
        				} else {
        					transformTrack[i] = new String(trackList[i].name, trackList[i].charset);
        				}
					} catch (UnsupportedEncodingException e) {
						// TODO Auto-generated catch block
						Log.w(TAG, "*********** unsupported encoding: "+trackList[i].charset);
						e.printStackTrace();
					}
        		}
               	ArrayAdapter<String> adapter = new ArrayAdapter<String>(mContext, 
            			R.layout.simple_list_item_single_choice, 
            			transformTrack);
               	mListView.setAdapter(adapter);
               	mListView.setItemChecked(mListFocus, true);
        	} else {
        		ArrayAdapter<String> adapter = new ArrayAdapter<String>(mContext, 
            			R.layout.simple_list_item_single_choice, 
            			mRes.getStringArray(R.array.screen_track_entries));
        		mListView.setAdapter(adapter);
               	mListView.setItemChecked(mListFocus, true);
        	}
        	mListDialog.show();
        }
    };
    
    private View.OnClickListener mRepeatListener = new View.OnClickListener() {
        public void onClick(View v) {
            mMediaController.setHolding(true);
        	mControlFocus = EDITOR_REPEAT;
        	mListFocus = mRepeatMode;

        	mDialogTitle.setText(R.string.repeat_title);
        	ArrayAdapter<String> adapter = new ArrayAdapter<String>(mContext, 
        			R.layout.simple_list_item_single_choice, 
        			mRes.getStringArray(R.array.screen_repeat_entries));
        	mListView.setAdapter(adapter);
            mListView.setItemChecked(mListFocus, true);
            mListView.smoothScrollToPosition(mListFocus);
        	mListDialog.show();
        }
    };

    public void subFocusItems() {
    	/* sub gate */
    	boolean gate = sp.getBoolean(EDITOR_SUBGATE, true);
    	mVideoView.setSubGate(gate);

    	/* sub color */
		int clor = sp.getInt(EDITOR_SUBCOLOR, Color.WHITE);
		mVideoView.setSubColor(clor);

    	/* sub char size */
		int charsize = sp.getInt(EDITOR_SUBCHARSIZE, 24);
		mVideoView.setSubFontSize(charsize);
		
		/* sub offset */
		int offset = sp.getInt(EDITOR_SUBOFFSET, 0);
		mVideoView.setSubPosition(offset);

		/* zoom mode */
		int zoom = sp.getInt(EDITOR_ZOOM, 0);
		mVideoView.setZoomMode(zoom);
		
		/* play mode */
		mRepeatMode = sp.getInt(EDITOR_REPEAT, 0);
		
		/* resume the current sub & track */
		if(mOnPause) {
			mOnPause = false;
			mVideoView.switchSub(mCurrentSubSave);			
			mVideoView.switchTrack(mCurrentTrackSave);
		}
}
    
    private static boolean uriSupportsBookmarks(Uri uri) {
    	if (uri.getScheme() == null)
    	{
    		return false;
    	}
    	return ("file".equalsIgnoreCase(uri.getScheme()));
    }
    
    private int getBookmark() {
        return mBookmarkService.findByPath(mUri.getPath());
    }

    private boolean deleteBookmark() {
    	return mBookmarkService.delete(mUri.getPath());
    }
    
    private void setBookmark(int bookmark) {
        if (!uriSupportsBookmarks(mUri)) {
            return;
        }
        
        String path = mUri.getPath();
        if( mBookmarkService.findByPath(path) > 0 ) {
        	mBookmarkService.update(path, bookmark);
        } else {
        	mBookmarkService.save(path, bookmark);
        }
    }

    public void onPause() {
        mOnPause = true;
        mHandler.removeCallbacksAndMessages(null);
        mCurrentTrackSave = mVideoView.getCurTrack();
        mCurrentSubSave = mVideoView.getCurSub();
        mResumeableTime = System.currentTimeMillis() + RESUMEABLE_TIMEOUT;
        mVideoPosition = mVideoView.getCurrentPosition();
        int duration = mVideoView.getDuration();
        // current time > 10s and save current position
        if(mVideoPosition > 10 * 1000 && duration - mVideoPosition > 10 * 1000) {
        	setBookmark(mVideoPosition - 3 * 1000);
        }
        else{
        	deleteBookmark();
        }
        if(mListDialog != null) {
        	mListDialog.dismiss();
        }
        if(mCharsizeDialog != null) {
        	mCharsizeDialog.dismiss();
        }
        if(mDelayDialog != null) {
        	mDelayDialog.dismiss();
        }

        mVideoView.suspend();
        mContext.unregisterReceiver(mBatteryReceiver);
    }

    public void onResume() {
        if (mOnPause) {
            mVideoView.seekTo(mVideoPosition);
            mVideoView.resume();

            // If we have slept for too long, pause the play
            if (System.currentTimeMillis() > mResumeableTime) {
            	mVideoView.pause();
            }
        }
        mContext.registerReceiver(mBatteryReceiver, new IntentFilter(Intent.ACTION_BATTERY_CHANGED)); 
    }

    public void onDestroy() {
        mVideoView.stopPlayback();
        mBookmarkService.close();
		mAwHdmiPluggedReceiver.unregister();	
    }

    public boolean onError(MediaPlayer player, int arg1, int arg2) {
        mHandler.removeCallbacksAndMessages(null);
        mProgressView.setVisibility(View.GONE);
		mToQuit = true;
        return false;
    }

    
    public boolean toQuit() {
        return mToQuit;
    }
    
    private BroadcastReceiver mBatteryReceiver = new BroadcastReceiver() {

		@Override
		public void onReceive(Context context, Intent intent) {
			// TODO Auto-generated method stub
			if(intent.getAction().equals(Intent.ACTION_BATTERY_CHANGED)) {
				int level = intent.getIntExtra("level", 0);
				mMediaController.setBatteryTextView(mRes.getString(R.string.system_battery)+level+"%");
			}
		}
  	
    };
    public void onCompletion(MediaPlayer mp) {
    	/* hide the dialog */
    	if(mListDialog != null && mListDialog.isShowing()) {
            mMediaController.setHolding(false);
    		mListDialog.hide();
    	}
    	if(mSlipSwitch != null && mSlipSwitch.isShowing()) {
            mMediaController.setHolding(false);
    		mSlipSwitch.dismiss();
    	}
    	if(mColorPicker != null && mColorPicker.isShowing()) {
            mMediaController.setHolding(false);
    		mColorPicker.dismiss();
    	}
    	if(mCharsizeDialog != null && mCharsizeDialog.isShowing()) {
    		mMediaController.setHolding(false);
    		mCharsizeDialog.hide();
    	}
    	if(mDelayDialog != null && mDelayDialog.isShowing()) {
    		mMediaController.setHolding(false);
    		mDelayDialog.hide();
    	}
    	
        onCompletion();
    }

    public void onCompletion() {
        mVideoView.setOnErrorListener(this);
        int size = mPlayList.size();
        
        if(mToQuit == true)
        	return;
        
        if(mCurrentIndex >= 0 && size > 0){
        	switch(mRepeatMode){
        		case PLAYMODE_REPEAT_ALL:{
        			int index = (mCurrentIndex+1)%size;
        			File nextFile = new File(mPlayList.get(index));
        			if (!nextFile.exists()){
        				   mToQuit = true;
        			}else {
        				mCurrentIndex = index;
        				mUri = Uri.fromFile(nextFile);
        				mMediaController.setFilePathTextView(mUri.getPath());
        				playFile();
        			}
    				break;
    			}	
        		case PLAYMODE_SEQUENCE:{
        			int index;
        			if(mCurrentIndex < size - 1){
        				index = mCurrentIndex + 1;
        				File nextFile = new File(mPlayList.get(index));
        				if (!nextFile.exists()){
        				   	mToQuit = true;
        				}else {
        					mCurrentIndex = index;
        					mUri = Uri.fromFile(nextFile);
        					mMediaController.setFilePathTextView(mUri.getPath());
        					playFile();
        				}
        			}
        			else{
        				mToQuit = true;
        			}
        			break;
        		}
        		case PLAYMODE_REPEAT_ONE:{
        			playFile();
        			break;
        		}
        		case PLAYMODE_RANDOM:{
        			int index = (new Random()).nextInt(size);
        			File nextFile = new File(mPlayList.get(index));
        			if (!nextFile.exists()){
        				   mToQuit = true;
        			}else {
        				mCurrentIndex = index;
        				mUri = Uri.fromFile(nextFile);
        				mMediaController.setFilePathTextView(mUri.getPath());
        				playFile();
        			}
        			break;
        		}
        		default:
        			break;
        	}
        }else{
        	mToQuit = true;
        }
    }
    
    private void playFile() {
        mWakeLock.acquire();

        m3DFocus = 2;
		mVideoView.setVideoURI(mUri);
		mMediaController.setFilePathTextView(mUri.getPath());
        mVideoView.requestFocus();
		mVideoView.start();

        mWakeLock.release();        
    }
}    
    


class BookmarkService {
	private final int MAXRECORD = 100;
    private MangerDatabase dbmanger;

    public BookmarkService(Context context) {
    	dbmanger=new MangerDatabase(context);
    }
    
    public void save(String path, int bookmark) {
    	long time = System.currentTimeMillis();
    	
    	SQLiteDatabase database= dbmanger.getWritableDatabase();
    	if(getCount() == MAXRECORD) {
    		long oldestTime = time;
    		Cursor cursor = database.query(MangerDatabase.NAME, null, null, null, null, null, null);
        	if(cursor != null) {
        		try {
    		    	while(cursor.moveToNext()) {
    		    		long recordTime = cursor.getLong(2);
    		    		if(recordTime < oldestTime) {
    		    			oldestTime = recordTime;
    		    		}
    		    	}
        		} finally {
        			cursor.close();
        		}
        	}
        	if(oldestTime < time) {
        		database.execSQL("delete from "+MangerDatabase.NAME+" where "+MangerDatabase.TIME+"=?"
        				,new Object[] {oldestTime});
        	}
    	}
    	
    	database.execSQL("insert into "+MangerDatabase.NAME+"("+
    			MangerDatabase.PATH+","+MangerDatabase.BOOKMARK+","+MangerDatabase.TIME+
    			") values(?,?,?)", new Object[] {path, bookmark, time});
    }

    public boolean delete(String path) {
    	boolean ret = false;
        SQLiteDatabase database= dbmanger.getWritableDatabase();

        Cursor cursor = database.rawQuery("select * from "+MangerDatabase.NAME+" where "+MangerDatabase.PATH+"=?"
        		, new String[]{path});
    	if(cursor != null) {
    		database.execSQL("delete from "+MangerDatabase.NAME+" where "+MangerDatabase.PATH+"=?"
    			,new Object[] {path});
    		cursor.close();
    		
    		ret = true;
    	}
    	
    	return ret;
    }
    
    public void update(String path, int bookmark) {
    	long time = System.currentTimeMillis();
    	SQLiteDatabase database= dbmanger.getWritableDatabase();
    	database.execSQL("update "+MangerDatabase.NAME+" set "+
    			MangerDatabase.BOOKMARK+"=?,"+MangerDatabase.TIME+"=? where "+MangerDatabase.PATH+"=?"
    			, new Object[] {bookmark, time, path});	
    }

    public int findByPath(String path) {
    	int ret = 0;
    	
        SQLiteDatabase database= dbmanger.getWritableDatabase();

        Cursor cursor = database.rawQuery("select * from "+MangerDatabase.NAME+" where "+MangerDatabase.PATH+"=?"
        		, new String[]{path});
    	if(cursor != null) {
    		try {
		    	if(cursor.moveToNext()) {
		    		ret = cursor.getInt(1);
		    	}
    		} finally {
    			cursor.close();
    		}
    	}
    	
    	return ret;
    }
    
    public int getCount() {
    	long count = 0;
    	
    	SQLiteDatabase database= dbmanger.getWritableDatabase();

    	Cursor cursor = database.rawQuery("select count(*) from "+MangerDatabase.NAME, null);
    	if(cursor != null) {
    		try {
		    	if(cursor.moveToLast()) {
		    		count = cursor.getLong(0);
		    	}
    		} finally {
    			cursor.close();
    		}
    	}
    	
    	return (int)count;
    }
    
    public void close() {
    	dbmanger.close();
    }
}



class MangerDatabase extends SQLiteOpenHelper {
	public static final String NAME="breakpoint";
	public static final String PATH="path";
	public static final String BOOKMARK="bookmark";
	public static final String TIME="time";
	
	private static final int version = 1;
	
	public MangerDatabase(Context context) {
		super(context, NAME, null, version);
		// TODO Auto-generated constructor stub
	}
	
	@Override
	public void onCreate(SQLiteDatabase arg0) {
		// TODO Auto-generated method stub
		Log.w("MangerDatabase", "*********** create MangerDatabase !");
		arg0.execSQL("CREATE TABLE IF NOT EXISTS "+NAME+" ("+PATH+" varchar PRIMARY KEY, "+BOOKMARK+" INTEGR, "+TIME+" LONG)");
	}

	@Override
	public void onUpgrade(SQLiteDatabase arg0, int arg1, int arg2) {
		// TODO Auto-generated method stub
		arg0.execSQL("DROP TABLE IF EXISTS "+NAME);  
        onCreate(arg0);  
	}
}

