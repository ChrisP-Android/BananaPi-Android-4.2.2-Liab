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

package com.softwinner.TvdVideo;

import android.app.Dialog;
import android.content.Context;
import android.content.Intent;
import android.content.res.Resources;
import android.media.AudioManager;
import android.media.MediaPlayer;
import android.media.MediaPlayer.SubInfo;
import android.media.MediaPlayer.TrackInfoVendor;
import android.media.Metadata;
import android.media.MediaPlayer.OnCompletionListener;
import android.media.MediaPlayer.OnErrorListener;
import android.net.Uri;
import android.util.AttributeSet;
import android.util.Log;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.widget.Button;
import android.widget.RelativeLayout;
import android.widget.TextView;
import java.util.HashMap;
import java.util.Map;

import java.io.IOException;
import java.util.Map;

import com.softwinner.TvdVideo.MediaController.MediaPlayerControl;

/**
 * Displays a video file. The VideoView class can load images from various
 * sources (such as resources or content providers), takes care of computing its
 * measurement from the video so that it can be used in any layout manager, and
 * provides various display options such as scaling and tinting.
 */
public class VideoView extends SurfaceView implements MediaPlayerControl {
	private String TAG = "VideoView";
	// settable by the client
	private Uri mUri;
	private Map<String, String> mHeaders;
	private int mDuration;
	private int mZoomMode = ZOOM_FULL_SCREEN_VIDEO_RATIO;

	// all video zoom mode
	public static final int ZOOM_FULL_SCREEN_VIDEO_RATIO = 0;
	public static final int ZOOM_FULL_SCREEN_SCREEN_RATIO = 1;
	public static final int ZOOM_ORIGIN_SIZE = 2;
	public static final int ZOOM_4R3 = 3;
	public static final int ZOOM_16R9 = 4;

	// all possible internal states
	private static final int STATE_ERROR = -1;
	private static final int STATE_IDLE = 0;
	private static final int STATE_PREPARING = 1;
	private static final int STATE_PREPARED = 2;
	private static final int STATE_PLAYING = 3;
	private static final int STATE_PAUSED = 4;
	private static final int STATE_PLAYBACK_COMPLETED = 5;

	// mCurrentState is a VideoView object's current state.
	// mTargetState is the state that a method caller intends to reach.
	// For instance, regardless the VideoView object's current state,
	// calling pause() intends to bring the object to a target state
	// of STATE_PAUSED.
	private int mCurrentState = STATE_IDLE;
	private int mTargetState = STATE_IDLE;

	// All the stuff we need for playing and showing a video
	private SurfaceHolder mSurfaceHolder = null;
	private MediaPlayer mMediaPlayer = null;
	private int mVideoWidth;
	private int mVideoHeight;
	// private int mSurfaceWidth;
	// private int mSurfaceHeight;
	private MediaController mMediaController;
	private OnCompletionListener mOnCompletionListener;
	private MediaPlayer.OnPreparedListener mOnPreparedListener;
	private int mCurrentBufferPercentage;
	private OnErrorListener mOnErrorListener;
	private int mSeekWhenPrepared; // recording the seek position while
									// preparing
	private boolean mCanPause;
	private boolean mCanSeekBack;
	private boolean mCanSeekForward;
	private int mStateWhenSuspended; // state before calling suspend()
	private boolean mBDFolderPlayMode = false;

	Dialog mErrorDialog;

	public VideoView(Context context) {
		super(context);
		initVideoView();
	}

	public VideoView(Context context, AttributeSet attrs) {
		this(context, attrs, 0);
		initVideoView();
	}

	public VideoView(Context context, AttributeSet attrs, int defStyle) {
		super(context, attrs, defStyle);
		initVideoView();
	}

	@Override
	protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
		int width = getDefaultSize(mVideoWidth, widthMeasureSpec);
		int height = getDefaultSize(mVideoHeight, heightMeasureSpec);
		if (mVideoWidth > 0 && mVideoHeight > 0) {
			setvideoSize(width, height);
		} else {
			setMeasuredDimension(width, height);
		}
	}

	private void setvideoSize(int width, int height) {
		switch (mZoomMode) {
		case ZOOM_FULL_SCREEN_VIDEO_RATIO: {
			if (mVideoWidth * height > width * mVideoHeight) {
				height = width * mVideoHeight / mVideoWidth;
			} else if (mVideoWidth * height < width * mVideoHeight) {
				width = height * mVideoWidth / mVideoHeight;
			}
			break;
		}
		case ZOOM_FULL_SCREEN_SCREEN_RATIO: {

			break;
		}
		case ZOOM_ORIGIN_SIZE: {
			if (width > mVideoWidth) {
				width = mVideoWidth;
			}
			if (height > mVideoHeight) {
				height = mVideoHeight;
			}

			break;
		}
		case ZOOM_4R3: {
			if (width * 3 > 4 * height) {
				width = height * 4 / 3;
			} else {
				height = width * 3 / 4;
			}
			break;
		}
		case ZOOM_16R9: {
			if (width * 9 > 16 * height) {
				width = height * 16 / 9;
			} else {
				height = width * 9 / 16;
			}
			break;
		}
		default:
			break;
		}
		Log.d(TAG, "#################setvideoSize(), result: width = " + width
				+ ", height = " + height);
		setMeasuredDimension(width, height);
	}

	public int resolveAdjustedSize(int desiredSize, int measureSpec) {
		int result = desiredSize;
		int specMode = MeasureSpec.getMode(measureSpec);
		int specSize = MeasureSpec.getSize(measureSpec);

		switch (specMode) {
		case MeasureSpec.UNSPECIFIED:
			/*
			 * Parent says we can be as big as we want. Just don't be larger
			 * than max size imposed on ourselves.
			 */
			result = desiredSize;
			break;

		case MeasureSpec.AT_MOST:
			/*
			 * Parent says we can be as big as we want, up to specSize. Don't be
			 * larger than specSize, and don't be larger than the max size
			 * imposed on ourselves.
			 */
			result = Math.min(desiredSize, specSize);
			break;

		case MeasureSpec.EXACTLY:
			// No choice. Do what we are told.
			result = specSize;
			break;
		}
		return result;
	}

	private void initVideoView() {
		getHolder().addCallback(mSHCallback);
		// getHolder().setType(SurfaceHolder.SURFACE_TYPE_PUSH_BUFFERS);
		setFocusable(true);
		setFocusableInTouchMode(true);
		requestFocus();
	}

	public void setVideoPath(String path) {
		setVideoURI(Uri.parse(path));
	}

    public void setVideoURI(Uri uri) {
		Log.w(TAG, "1 setVideoURI: " );

	   setVideoURI(uri, null);
    }

    /**
     * @hide
     */
    public void setVideoURI(Uri uri, Map<String, String> headers) {
        mUri = uri;
        mHeaders = headers;
        mSeekWhenPrepared = 0;
        openVideo();
        requestLayout();
        invalidate();
		 Log.w(TAG, "2setVideoURI:  uri= "+uri);
    }
     

	public String mSourcePath = null;
	public void setVideoURI(Uri uri, String sourcePath,int i){
		mSourcePath = sourcePath;
		 Log.w(TAG, "0setVideoURI:   "+sourcePath);
		setVideoURI(uri);
		
	}

	public void stopPlayback() {
		if (mMediaPlayer != null) {
			mMediaPlayer.stop();
			mMediaPlayer.release();
			mMediaPlayer = null;
			mCurrentState = STATE_IDLE;
			mTargetState = STATE_IDLE;
		}
	}

	public void setBDFolderPlayMode(boolean bdMode) {
		mBDFolderPlayMode = bdMode;
	}
	
    private void openVideo() {
        if (mUri == null || mSurfaceHolder == null) {
            // not ready for playback just yet, will try again later
            return;
        }
        // Tell the music playback service to pause
        // TODO: these constants need to be published somewhere in the framework.
        Intent i = new Intent("com.android.music.musicservicecommand");
        i.putExtra("command", "pause");
        mContext.sendBroadcast(i);
		Log.w(TAG, "openVideo 0");

        // we shouldn't clear the target state, because somebody might have
        // called start() previously
        release(false);
	     Log.w(TAG, "openVideo 1");
        try {
            mMediaPlayer = new MediaPlayer();
            mMediaPlayer.setOnPreparedListener(mPreparedListener);
            mMediaPlayer.setOnVideoSizeChangedListener(mSizeChangedListener);
            mDuration = -1;
		Log.w(TAG, "openVideo 3");
            mMediaPlayer.setOnCompletionListener(mCompletionListener);
            mMediaPlayer.setOnErrorListener(mErrorListener);
            mMediaPlayer.setOnBufferingUpdateListener(mBufferingUpdateListener);
            mCurrentBufferPercentage = 0;
		Log.w(TAG, "openVideo 4");
		Log.w(TAG, "sean : 2mSourcePath: " + mSourcePath);
		Log.v(TAG, "sean : 2mSourcePath: " + mSourcePath);
		if(mSourcePath!=null)
		{
			mHeaders=new HashMap<String, String>();
		//	mHeaders.put(new String(mSourcePath),"BD_SUB");
			mHeaders.put("BD_SUB",new String(mSourcePath));
		}
		Log.v("sean", "coming to set data source ");
            mMediaPlayer.setDataSource(mContext, mUri, mHeaders);
           	Log.w(TAG, "openVideo 5");
		mMediaPlayer.setDisplay(mSurfaceHolder);
		Log.w(TAG, "openVideo 6");
            mMediaPlayer.setAudioStreamType(AudioManager.STREAM_MUSIC);
            mMediaPlayer.setScreenOnWhilePlaying(true);
		Log.w(TAG, "openVideo 7");
            mMediaPlayer.setBdFolderPlayMode(mBDFolderPlayMode);
            mMediaPlayer.prepareAsync();
		Log.w(TAG, "openVideo 8");
            // we don't set the target state here either, but preserve the
            // target state that was there before.
            mCurrentState = STATE_PREPARING;
            if( mOnSubFocusItems != null ) {
                mOnSubFocusItems.subFocusItems();
            }
            if (mMediaController != null) {
                mMediaController.setMediaPlayer(this);
            }
            //attachMediaController();
             Log.w(TAG, "openVideo 9");
        } catch (IOException ex) {
            Log.w(TAG, "Unable to open content: " + mUri, ex);
            mCurrentState = STATE_ERROR;
            mTargetState = STATE_ERROR;
            mErrorListener.onError(mMediaPlayer, MediaPlayer.MEDIA_ERROR_UNKNOWN, 0);
            return;
        } catch (IllegalArgumentException ex) {
            Log.w(TAG, "Unable to open content: " + mUri, ex);
            mCurrentState = STATE_ERROR;
            mTargetState = STATE_ERROR;
            mErrorListener.onError(mMediaPlayer, MediaPlayer.MEDIA_ERROR_UNKNOWN, 0);
            return;
        }
    }

	public void setMediaController(MediaController controller) {
		if (mMediaController != null) {
			mMediaController.hide();
		}
		mMediaController = controller;
		attachMediaController();
	}

	private void attachMediaController() {
		if (mMediaController != null) {
			// if (mMediaPlayer != null && mMediaController != null) {
			// mMediaController.setMediaPlayer(this);
			View anchorView = this.getParent() instanceof View ? (View) this
					.getParent() : this;
			mMediaController.setAnchorView(anchorView);
			mMediaController.setEnabled(isInPlaybackState());
		}
	}

	MediaPlayer.OnVideoSizeChangedListener mSizeChangedListener = new MediaPlayer.OnVideoSizeChangedListener() {
		public void onVideoSizeChanged(MediaPlayer mp, int width, int height) {
			mVideoWidth = mp.getVideoWidth();
			mVideoHeight = mp.getVideoHeight();
			if (mVideoWidth > 0 && mVideoHeight > 0) {
				getHolder().setFixedSize(mVideoWidth, mVideoHeight);
			}
		}
	};

	MediaPlayer.OnPreparedListener mPreparedListener = new MediaPlayer.OnPreparedListener() {
		public void onPrepared(MediaPlayer mp) {
			mCurrentState = STATE_PREPARED;

			// Get the capabilities of the player for this stream
			Metadata data = mp.getMetadata(MediaPlayer.METADATA_ALL,
					MediaPlayer.BYPASS_METADATA_FILTER);

			if (data != null) {
				mCanPause = !data.has(Metadata.PAUSE_AVAILABLE)
						|| data.getBoolean(Metadata.PAUSE_AVAILABLE);
				mCanSeekBack = !data.has(Metadata.SEEK_BACKWARD_AVAILABLE)
						|| data.getBoolean(Metadata.SEEK_BACKWARD_AVAILABLE);
				mCanSeekForward = !data.has(Metadata.SEEK_FORWARD_AVAILABLE)
						|| data.getBoolean(Metadata.SEEK_FORWARD_AVAILABLE);
			} else {
				mCanPause = mCanSeekBack = mCanSeekForward = true;
			}

			if (mOnPreparedListener != null) {
				mOnPreparedListener.onPrepared(mMediaPlayer);
			}
			if (mMediaController != null) {
				mMediaController.setEnabled(true);
			}
			if (mOnSubFocusItems != null) {
				mOnSubFocusItems.initSubAndTrackInfo();
			}
			mVideoWidth = mp.getVideoWidth();
			mVideoHeight = mp.getVideoHeight();

			int seekToPosition = mSeekWhenPrepared; // mSeekWhenPrepared may be
													// changed after seekTo()
													// call
			if (seekToPosition != 0) {
				seekTo(seekToPosition);
			}
			if (mVideoWidth > 0 && mVideoHeight > 0) {
				getHolder().setFixedSize(mVideoWidth, mVideoHeight);

				if (mTargetState == STATE_PLAYING) {
					start();
					if (mMediaController != null) {
						mMediaController.show();
					}
				} else if (!isPlaying()
						&& (seekToPosition != 0 || getCurrentPosition() > 0)) {
					if (mMediaController != null) {
						// Show the media controls when we're paused into a
						// video and make 'em stick.
						mMediaController.show(0);
					}
				}
			} else {
				// We don't know the video size yet, but should start anyway.
				// The video size might be reported to us later.
				if (mTargetState == STATE_PLAYING) {
					start();
				}
			}
		}
	};

	private MediaPlayer.OnCompletionListener mCompletionListener = new MediaPlayer.OnCompletionListener() {
		public void onCompletion(MediaPlayer mp) {
			mCurrentState = STATE_PLAYBACK_COMPLETED;
			mTargetState = STATE_PLAYBACK_COMPLETED;
			if (mMediaController != null) {
				mMediaController.hide();
			}
			if (mOnCompletionListener != null) {
				mOnCompletionListener.onCompletion(mMediaPlayer);
			}
		}
	};

	private MediaPlayer.OnErrorListener mErrorListener = new MediaPlayer.OnErrorListener() {
		public boolean onError(MediaPlayer mp, int framework_err, int impl_err) {
			Log.d(TAG, "Error: " + framework_err + "," + impl_err);
			mCurrentState = STATE_ERROR;
			mTargetState = STATE_ERROR;
			if (mMediaController != null) {
				mMediaController.hide();
			}

			/* If an error handler has been supplied, use it and finish. */
			if (mOnErrorListener != null) {
				if (mOnErrorListener.onError(mMediaPlayer, framework_err,
						impl_err)) {
					return true;
				}
			}

			/*
			 * Otherwise, pop up an error dialog so the user knows that
			 * something bad has happened. Only try and pop up the dialog if
			 * we're attached to a window. When we're going away and no longer
			 * have a window, don't bother showing the user an error.
			 */
			if (getWindowToken() != null) {
				Resources r = mContext.getResources();
				int messageId;

				if (framework_err == MediaPlayer.MEDIA_ERROR_NOT_VALID_FOR_PROGRESSIVE_PLAYBACK) {
					messageId = com.android.internal.R.string.VideoView_error_text_invalid_progressive_playback;
				} else {
					messageId = com.android.internal.R.string.VideoView_error_text_unknown;
				}

				LayoutInflater inflate = (LayoutInflater) mContext
						.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
				View errorView = inflate.inflate(R.layout.dialog_replay, null);
				/* error title */
				TextView v = (TextView) errorView
						.findViewById(R.id.replay_title);
				v.setText(com.android.internal.R.string.VideoView_error_title);
				/* error message */
				v = (TextView) errorView.findViewById(R.id.replay_msg);
				v.setText(messageId);
				((Button) errorView.findViewById(R.id.replay_confirm))
						.setOnClickListener(new View.OnClickListener() {
							public void onClick(View arg0) {
								if (mOnCompletionListener != null) {
									mOnCompletionListener
											.onCompletion(mMediaPlayer);
								}
								mErrorDialog.dismiss();
							}
						});
				((Button) errorView.findViewById(R.id.replay_cancel))
						.setVisibility(GONE);
				mErrorDialog = new Dialog(mContext, R.style.dialog);
				mErrorDialog.setContentView(errorView);
				mErrorDialog.setCancelable(false);
				mErrorDialog.show();
			}
			return true;
		}
	};

	private MediaPlayer.OnBufferingUpdateListener mBufferingUpdateListener = new MediaPlayer.OnBufferingUpdateListener() {
		public void onBufferingUpdate(MediaPlayer mp, int percent) {
			mCurrentBufferPercentage = percent;
		}
	};

	/**
	 * Register a callback to be invoked when the media file is loaded and ready
	 * to go.
	 * 
	 * @param l
	 *            The callback that will be run
	 */
	public void setOnPreparedListener(MediaPlayer.OnPreparedListener l) {
		mOnPreparedListener = l;
	}

	/**
	 * Register a callback to be invoked when the end of a media file has been
	 * reached during playback.
	 * 
	 * @param l
	 *            The callback that will be run
	 */
	public void setOnCompletionListener(OnCompletionListener l) {
		mOnCompletionListener = l;
	}

	/**
	 * Register a callback to be invoked when an error occurs during playback or
	 * setup. If no listener is specified, or if the listener returned false,
	 * VideoView will inform the user of any errors.
	 * 
	 * @param l
	 *            The callback that will be run
	 */
	public void setOnErrorListener(OnErrorListener l) {
		mOnErrorListener = l;
	}

	SurfaceHolder.Callback mSHCallback = new SurfaceHolder.Callback() {
		public void surfaceChanged(SurfaceHolder holder, int format, int w,
				int h) {
			// mSurfaceWidth = w;
			// mSurfaceHeight = h;
			boolean isValidState = (mTargetState == STATE_PLAYING);
			boolean hasValidSize = (mVideoWidth == w && mVideoHeight == h);
			if (mMediaPlayer != null && isValidState && hasValidSize) {
				if (mSeekWhenPrepared != 0) {
					seekTo(mSeekWhenPrepared);
				}
				start();
				if (mMediaController != null) {
					if (mMediaController.isShowing()) {
						// ensure the controller will get repositioned later
						mMediaController.hide();
					}
					mMediaController.show();
				}
			}
		}

		public void surfaceCreated(SurfaceHolder holder) {
			mSurfaceHolder = holder;
			openVideo();
		}

		public void surfaceDestroyed(SurfaceHolder holder) {
			// after we return from this we can't use the surface any more
			mSurfaceHolder = null;
			if (mMediaController != null) {
				mMediaController.hide();
			}
			release(true);
		}
	};

	/*
	 * release the media player in any state
	 */
	private void release(boolean cleartargetstate) {
		if (mMediaPlayer != null) {
			mMediaPlayer.reset();
			mMediaPlayer.release();
			mMediaPlayer = null;
			mCurrentState = STATE_IDLE;
			if (cleartargetstate) {
				mTargetState = STATE_IDLE;
			}
		}
	}

	@Override
	public boolean onTouchEvent(MotionEvent ev) {
		if (isInPlaybackState() && mMediaController != null) {
			toggleMediaControlsVisiblity();
		}
		return false;
	}

	@Override
	public boolean onTrackballEvent(MotionEvent ev) {
		if (isInPlaybackState() && mMediaController != null) {
			toggleMediaControlsVisiblity();
		}
		return false;
	}

	@Override
	public boolean onKeyDown(int keyCode, KeyEvent event) {
		boolean isKeyCodeSupported = keyCode != KeyEvent.KEYCODE_BACK
				&& keyCode != KeyEvent.KEYCODE_VOLUME_UP
				&& keyCode != KeyEvent.KEYCODE_VOLUME_DOWN
				&& keyCode != KeyEvent.KEYCODE_MENU
				&& keyCode != KeyEvent.KEYCODE_CALL
				&& keyCode != KeyEvent.KEYCODE_ENDCALL;
		if (isInPlaybackState() && isKeyCodeSupported
				&& mMediaController != null) {
			if (keyCode == KeyEvent.KEYCODE_HEADSETHOOK
					|| keyCode == KeyEvent.KEYCODE_MEDIA_PLAY_PAUSE) {
				if (mMediaPlayer.isPlaying()) {
					pause();
					mMediaController.show();
				} else {
					start();
					mMediaController.hide();
				}
				return true;
			} else if (keyCode == KeyEvent.KEYCODE_SUBTITLE) { // switch
																// subtitle
				mMediaController.OnMediaSubtitleKeyListener();
				return true;
			} else if (keyCode == KeyEvent.KEYCODE_AUDIO) { // switch audio
				mMediaController.OnMediaAudioKeyListener();
				return true;
			} else if (keyCode == KeyEvent.KEYCODE_LOOP) { // repeat
				mMediaController.OnMediaRepeatClickListener();
				return true;
			} else if (keyCode == KeyEvent.KEYCODE_MEDIA_PREVIOUS) { // prev
				mMediaController.OnMediaPrevClickListener();
				return true;
			} else if (keyCode == KeyEvent.KEYCODE_MEDIA_NEXT) { // next
				mMediaController.OnMediaNextClickListener();
				return true;
			} else if (keyCode == KeyEvent.KEYCODE_MEDIA_STOP
					&& mMediaPlayer.isPlaying()) {
				mMediaController.OnMediaStopClickListener();
				// pause();
				// mMediaController.show();
			} else {
				toggleMediaControlsVisiblity();
			}
		}

		return super.onKeyDown(keyCode, event);
	}

	private void toggleMediaControlsVisiblity() {
		if (mMediaController.isShowing()) {
			mMediaController.hide();
		} else {
			mMediaController.show();
		}
	}

	public void start() {
		if (isInPlaybackState()) {
			mMediaPlayer.start();
			mCurrentState = STATE_PLAYING;
		}
		mTargetState = STATE_PLAYING;
	}

	public void pause() {
		if (isInPlaybackState()) {
			if (mMediaPlayer.isPlaying()) {
				mMediaPlayer.pause();
				mCurrentState = STATE_PAUSED;
			}
		}
		mTargetState = STATE_PAUSED;
	}

	public void suspend() {
		release(false);
	}

	public void resume() {
		openVideo();
	}

	// cache duration as mDuration for faster access
	public int getDuration() {
		if (isInPlaybackState()) {
			if (mDuration > 0) {
				return mDuration;
			}
			mDuration = mMediaPlayer.getDuration();
			return mDuration;
		}
		mDuration = -1;
		return mDuration;
	}

	public int getCurrentPosition() {
		if (isInPlaybackState()) {
			return mMediaPlayer.getCurrentPosition();
		}
		return 0;
	}

	public void seekTo(int msec) {
		if (isInPlaybackState()) {
			mMediaPlayer.seekTo(msec);
			mSeekWhenPrepared = 0;
		} else {
			mSeekWhenPrepared = msec;
		}
	}

	public boolean isPlaying() {
		return isInPlaybackState() && mMediaPlayer.isPlaying();
	}

	public int getBufferPercentage() {
		if (mMediaPlayer != null) {
			return mCurrentBufferPercentage;
		}
		return 0;
	}

	private boolean isInPlaybackState() {
		return (mMediaPlayer != null && mCurrentState != STATE_ERROR
				&& mCurrentState != STATE_IDLE && mCurrentState != STATE_PREPARING);
	}

	public boolean canPause() {
		return mCanPause;
	}

	public boolean canSeekBackward() {
		return mCanSeekBack;
	}

	public boolean canSeekForward() {
		return mCanSeekForward;
	}

	/**
	 * Get the subtitle list of the current playing video.
	 * <p>
	 * 
	 * @return subtitle list. null means there is no subtitle.
	 */
	public SubInfo[] getSubList() {
		if (mMediaPlayer == null) {
			return null;
		}
		return mMediaPlayer.getSubList();
	}

	/**
	 * get the index of the current showing subtitle in the subtitle list.
	 * <p>
	 * 
	 * @return the index of the current showing subtitle in the subtitle list.
	 *         <0 means no subtitle.
	 */
	public int getCurSub() {
		if (mMediaPlayer == null) {
			return -1;
		}
		return mMediaPlayer.getCurSub();
	}

	/**
	 * switch another subtitle to show.
	 * <p>
	 * 
	 * @param index
	 *            the subtitle’s index in the subtitle list。
	 * @return ==0 means successful, !=0 means failed.
	 */
	public int switchSub(int index) {
		if (mMediaPlayer == null) {
			return -1;
		}
		Log.v("Maizirong","___mMediaPlayer.switchSub___" + Integer.toString(index));
		return mMediaPlayer.switchSub(index);
	}

	/**
	 * show or hide a subitle.
	 * <p>
	 * 
	 * @param showSub
	 *            whether to show subtitle or not
	 * @return ==0 means successful, !=0 means failed.
	 */
	public int setSubGate(boolean showSub) {
		if (mMediaPlayer == null) {
			return -1;
		}
		return mMediaPlayer.setSubGate(showSub);
	}

	/**
	 * check whether subtitles is allowed showing.
	 * <p>
	 * 
	 * @return true if subtitles is allowed showing, false otherwise.
	 */
	public boolean getSubGate() {
		if (mMediaPlayer == null) {
			return false;
		}
		return mMediaPlayer.getSubGate();
	}

	/**
	 * Set the subtitle’s color.
	 * <p>
	 * 
	 * @param color
	 *            subtitle’s color.
	 * @return ==0 means successful, !=0 means failed.
	 */
	public int setSubColor(int color) {
		if (mMediaPlayer == null) {
			return -1;
		}
		Log.v("Maizirong","___mMediaPlayer.setSubColor___" + Integer.toString(color));
		return mMediaPlayer.setSubColor(color);
	}

	/**
	 * Get the subtitle’s color.
	 * <p>
	 * 
	 * @return the subtitle’s color.
	 */
	public int getSubColor() {
		if (mMediaPlayer == null) {
			return -1;
		}
		return mMediaPlayer.getSubColor();
	}

	/**
	 * Set the subtitle frame’s color.
	 * <p>
	 * 
	 * @param color
	 *            subtitle frame’s color.
	 * @return ==0 means successful, !=0 means failed.
	 */
	public int setSubFrameColor(int color) {
		if (mMediaPlayer == null) {
			return -1;
		}
		return mMediaPlayer.setSubFrameColor(color);
	}

	/**
	 * Get the subtitle frame’s color.
	 * <p>
	 * 
	 * @return the subtitle frame’s color.
	 */
	public int getSubFrameColor() {
		if (mMediaPlayer == null) {
			return -1;
		}
		return mMediaPlayer.getSubFrameColor();
	}

	/**
	 * Set the subtitle’s font size.
	 * <p>
	 * 
	 * @param size
	 *            font size in pixel.
	 * @return ==0 means successful, !=0 means failed.
	 */
	public int setSubFontSize(int size) {
		if (mMediaPlayer == null) {
			return -1;
		}
		Log.v("Maizirong","___mMediaPlayer.setSubFontSize___" + Integer.toString(size));
		return mMediaPlayer.setSubFontSize(size);
	}

	/**
	 * Get the subtitle’s font size.
	 * <p>
	 * 
	 * @return the subtitle’s font size. <0 means failed.
	 */
	public int getSubFontSize() {
		if (mMediaPlayer == null) {
			return -1;
		}
		return mMediaPlayer.getSubFontSize();
	}

	/**
	 * Set the subtitle’s charset. If the underlying mediaplayer can absolutely
	 * parse the charset of the subtitles, still use the parsed charset;
	 * otherwise, use the charset argument.
	 * <p>
	 * 
	 * @param charset
	 *            the canonical name of a charset.
	 * @return ==0 means successful, !=0 means failed.
	 */
	public int setSubCharset(String charset) {
		if (mMediaPlayer == null) {
			return -1;
		}
		return mMediaPlayer.setSubCharset(charset);
	}

	/**
	 * Get the subtitle’s charset.
	 * <p>
	 * 
	 * @return the canonical name of a charset.
	 */
	public String getSubCharset() {
		if (mMediaPlayer == null) {
			return null;
		}
		return mMediaPlayer.getSubCharset();
	}

	/**
	 * Set the subtitle’s position vertically in the screen.
	 * <p>
	 * 
	 * @param percent
	 *            字幕下沿距离屏幕下沿的距离占整个屏幕高度的百分比。这是一个整数值，譬如说，10%，参数应该填10.
	 * @return ==0 means successful, !=0 means failed.
	 */
	public int setSubPosition(int percent) {
		if (mMediaPlayer == null) {
			return -1;
		}
		return mMediaPlayer.setSubPosition(percent);
	}

	/**
	 * Get the subtitle’s position vertically in the screen.
	 * <p>
	 * 
	 * @return percent 字幕下沿距离屏幕下沿的距离占整个屏幕高度的百分比。这是一个整数值，譬如说，10%，参数返回10.
	 */
	public int getSubPosition() {
		if (mMediaPlayer == null) {
			return -1;
		}
		return mMediaPlayer.getSubPosition();
	}

	/**
	 * Set the subtitle’s delay time.
	 * <p>
	 * 
	 * @param time
	 *            delay time in milliseconds. It can be <0.
	 * @return ==0 means successful, !=0 means failed.
	 */
	public int setSubDelay(int time) {
		if (mMediaPlayer == null) {
			return -1;
		}
		return mMediaPlayer.setSubDelay(time);
	}

	/**
	 * Get the subtitle’s delay time.
	 * <p>
	 * 
	 * @return delay time in milliseconds.
	 */
	public int getSubDelay() {
		if (mMediaPlayer == null) {
			return -1;
		}
		return mMediaPlayer.getSubDelay();
	}

	/**
	 * Get the track list of the current playing video.
	 * <p>
	 * 
	 * @return track list. null means there is no track.
	 */
	public TrackInfoVendor[] getTrackList() {
		if (mMediaPlayer != null) {
			return mMediaPlayer.getTrackList();
		}
		return null;
	}

	/**
	 * get the index of the current track in the track list.
	 * <p>
	 * 
	 * @return the index of the current track in the track list. <0 means no
	 *         track.
	 */
	public int getCurTrack() {
		if (mMediaPlayer != null) {
			return mMediaPlayer.getCurTrack();
		}
		return -1;
	}

	/**
	 * set the anaglagh type of the output.
	 * <p>
	 * 
	 * @param type
	 *            the anaglagh type of the output
	 * @return ==0 means successful, !=0 means failed.
	 */
	public int setAnaglaghType(int type) {
		if (mMediaPlayer == null) {
			return -1;
		}
		return mMediaPlayer.setAnaglaghType(type);
	}

	/**
	 * get the anaglagh type of the output.
	 * <p>
	 * 
	 * @return the anaglagh type of the output. -1 means failed.
	 */
	public int getAnaglaghType() {
		if (mMediaPlayer == null) {
			return -1;
		}
		return mMediaPlayer.getAnaglaghType();
	}

	/**
	 * set the dimension type of the source file.
	 * <p>
	 * 
	 * @param type
	 *            the 3D picture format of the source file
	 * @return ==0 means successful, !=0 means failed.
	 */
	public int setInputDimensionType(int type) {
		if (mMediaPlayer == null) {
			return -1;
		}
		return mMediaPlayer.setInputDimensionType(type);
	}

	/**
	 * get the dimension type of the source file.
	 * <p>
	 * 
	 * @return the 3D picture format of the source file. -1 means failed.
	 */
	public int getInputDimensionType() {
		if (mMediaPlayer == null) {
			return -1;
		}
		return mMediaPlayer.getInputDimensionType();
	}

	/**
	 * set the dimension type of the output.
	 * <p>
	 * 
	 * @param type
	 *            the dimension type of the output
	 * @return ==0 means successful, !=0 means failed.
	 */
	public int setOutputDimensionType(int type) {
		if (mMediaPlayer == null) {
			return -1;
		}
		return mMediaPlayer.setOutputDimensionType(type);
	}

	/**
	 * get the dimension type of the output.
	 * <p>
	 * 
	 * @return the dimension type of the output. -1 means failed.
	 */
	public int getOutputDimensionType() {
		if (mMediaPlayer == null) {
			return -1;
		}
		return mMediaPlayer.getOutputDimensionType();
	}

	/**
	 * switch another track to play.
	 * <p>
	 * 
	 * @param index
	 *            the track’s index in the track list。
	 * @return ==0 means successful, !=0 means failed.
	 */
	public int switchTrack(int index) {
		if (mMediaPlayer == null) {
			return -1;
		}
		return mMediaPlayer.switchTrack(index);
	}

	/**
	 * switch another screen size to show video.
	 * <p>
	 * 
	 * @param mode
	 *            the zoom’s index in the zoom list。
	 */
	public void setZoomMode(int mode) {
		if (mode == mZoomMode) {
			return;
		}
		mZoomMode = mode;
		if (mVideoWidth > 0 && mVideoHeight > 0) {
			getHolder().setFixedSize(mVideoWidth, mVideoHeight);
			requestLayout();
		}
	}

	/**
	 * get screen size to show video.
	 * <p>
	 * 
	 */
	public int getZoomMode() {
		return mZoomMode;
	}

	/**
	 * Register a callback to be invoked when the media source is ready for
	 * playback.
	 * 
	 */
	public interface OnSubFocusItems {
		void subFocusItems();

		void initSubAndTrackInfo();
	}

	public void setOnSubFocusItems(OnSubFocusItems SubFocusItems) {
		mOnSubFocusItems = SubFocusItems;
	}

	private OnSubFocusItems mOnSubFocusItems;
}
