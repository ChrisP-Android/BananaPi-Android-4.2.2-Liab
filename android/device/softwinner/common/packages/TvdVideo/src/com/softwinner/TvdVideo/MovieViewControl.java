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

package com.softwinner.TvdVideo;

import java.io.File;
import java.io.UnsupportedEncodingException;
import java.util.ArrayList;
import java.util.Collections;

import android.app.Dialog;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.IContentProvider;
import android.content.Intent;
import android.content.DialogInterface.OnCancelListener;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.content.res.Resources;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;
import android.hardware.input.InputManager;
import android.media.MediaPlayer;
import android.media.MediaPlayer.SubInfo;
import android.media.MediaPlayer.TrackInfoVendor;
import android.net.Uri;
import android.os.Handler;
import android.os.Message;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.os.SystemClock;
import android.provider.MediaStore;
import android.provider.MediaStore.Video;
import android.util.Log;
import android.view.DisplayManagerAw;
import android.view.Gravity;
import android.view.IWindowManager;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.TextView;
import android.widget.Toast;

import com.softwinner.TvdVideo.JumpView.OnTimeConfirmListener;

public class MovieViewControl implements MediaPlayer.OnErrorListener,
		MediaPlayer.OnCompletionListener, MediaController.OnListDataChanged,
		VideoView.OnSubFocusItems {

	@SuppressWarnings("unused")
	private static final String TAG = "MovieViewControl";

	private static final String STORE_NAME = "VideoSetting";
	private static final String EDITOR_SUBGATE = "MovieViewControl.SUBGATE";
	private static final String EDITOR_SUBSELECT = "MovieViewControl.SUBSELECT";
	private static final String EDITOR_SUBCHARSET = "MovieViewControl.SUBCHARSET";
	private static final String EDITOR_SUBCOLOR = "MovieViewControl.SUBCOLOR";
	private static final String EDITOR_SUBCHARSIZE = "MovieViewControl.SUBCHARSIZE";
	private static final String EDITOR_SUBOFFSET = "MovieViewControl.SUBOFFSET";
	private static final String EDITOR_SUBDELAY = "MovieViewControl.SUBDELAY";
	private static final String EDITOR_ZOOM = "MovieViewControl.MODEZOOM";
	private static final String EDITOR_MODE3D = "MovieViewControl.MODE3D";
	private static final String EDITOR_MODEREPEAT = "MovieViewControl.MODEREPEAT";
	private static final String EDITOR_TRACK = "MovieViewControl.TRACK";

	private static final int DISMISS_DIALOG = 0x10;
	private static final int DISMISS_DELAY = 5000; // 5 s

	private BookmarkService mBookmarkService;
	private JumpView mJumpView;
	private final VideoView mVideoView;
	private final View mProgressView;
	private Dialog mReplayDialog;
	private Message mDismissMsg;
	private Uri mUri;
	private Context mContext;
	private boolean mToQuit = false;
	private boolean mOnPause, mFinishOnCompletion;
	private String mPlayListType;
	private final SharedPreferences sp;
	private final SharedPreferences.Editor editor;
	private Resources mRes;
	private int mCurrentIndex = 0, mRepeatMode = 0;
	private int mVideoPosition = 0;
	private ArrayList<String> mPlayList;
	private MediaController mMediaController;
	private Toast mJumpToast;
	private MyToast mSubtitleToast, mAudioToast, mPrevToast, mNextToast,
			mStopToast, mRepeatToast;
	private String[] mTransformSubtitle, mTransformTrack;

	private BroadcastReceiver mBroadcastReceiver;

	// SAVE sub and track, add by maizirong
	private int mCurrentTrackSave = -1;
	private int mCurrentSubSave = -1;
	// SAVE subcolor and subfontsize, add by maizirong
	private int mCurrentSubColorSave = -256;
	private int mCurrentSubSizeSave = 32;

	Handler mHandler = new Handler() {

		@Override
		public void handleMessage(Message msg) {
			// TODO Auto-generated method stub
			if (msg.what == DISMISS_DIALOG && mReplayDialog.isShowing()) {
				mReplayDialog.dismiss();
			}
		}

	};

	Runnable mMode3DRunnable = null;
	Runnable mRepeatRunnable = null;
	Runnable mTrackRunnable = null;
	Runnable mZoomRunnable = null;
	Runnable mSubsetRunnable = null;
	Runnable mPlayingChecker = new Runnable() {
		public void run() {
			if (mVideoView.isPlaying()) {
				mProgressView.setVisibility(View.GONE);
			} else {
				mHandler.postDelayed(mPlayingChecker, 250);
			}
		}
	};

	Runnable mPlayRunnable = new Runnable() {
		@Override
		public void run() {
			setImageButtonListener();
			mVideoView.setVideoURI(mUri);
			mMediaController.setFilePathTextView(mUri.getPath());
			mVideoView.requestFocus();
			mVideoView.start();

			mHandler.removeCallbacks(mPlayRunnable);
		}
	};

	class MyToast extends Toast {
		View mNextView;
		int mDuration;

		public MyToast(Context context) {
			super(context);
			// TODO Auto-generated constructor stub
			LayoutInflater inflate = (LayoutInflater) mContext
					.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
			mNextView = inflate.inflate(R.layout.transient_notification, null);
			mDuration = MyToast.LENGTH_SHORT;
			super.setView(mNextView);
			super.setDuration(mDuration);
			super.setGravity(Gravity.LEFT | Gravity.TOP, 100, 0);
		}

		@Override
		public void onHide() {
			// TODO Auto-generated method stub
			super.onHide();

			if (mSubtitleToast.isShowing() && mTransformSubtitle != null) {
				String newSub = (String) ((TextView) mSubtitleToast.getView()
						.findViewById(R.id.message)).getText();
				for (int i = 0; i < mTransformSubtitle.length; i++) {
					if (newSub.equals(mTransformSubtitle[i])) {
						mVideoView.switchSub(i);
						break;
					}
				}
			} else if (mAudioToast.isShowing() && mTransformTrack != null) {
				String newTrack = (String) ((TextView) mAudioToast.getView()
						.findViewById(R.id.message)).getText();
				for (int i = 0; i < mTransformTrack.length; i++) {
					if (newTrack.equals(mTransformTrack[i])) {
						mVideoView.switchTrack(i);
						break;
					}
				}
			}
		}
	}

	public static String formatDuration(final Context context, int durationMs) {
		int duration = durationMs / 1000;
		int h = duration / 3600;
		int m = (duration - h * 3600) / 60;
		int s = duration - (h * 3600 + m * 60);
		String durationValue;
		if (h == 0) {
			durationValue = String.format("%02d:%02d", m, s);
		} else {
			durationValue = String.format("%d:%02d:%02d", h, m, s);
		}
		return durationValue;
	}

	public MovieViewControl(View rootView, Context context, Intent intent,
			String path000) {
		mVideoView = (VideoView) rootView.findViewById(R.id.surface_view);
		mProgressView = rootView.findViewById(R.id.progress_indicator);
		Log.v(TAG, "path000=" + path000);
		mContext = context;
		mUri = Uri2File2Uri(intent.getData());
		mRes = mContext.getResources();
		sp = mContext.getSharedPreferences(STORE_NAME, Context.MODE_PRIVATE);
		editor = sp.edit();
		initToast();

		// For streams that we expect to be slow to start up, show a
		// progress spinner until playback starts.
		String scheme = mUri.getScheme();
		if ("http".equalsIgnoreCase(scheme) || "rtsp".equalsIgnoreCase(scheme)) {
			mHandler.postDelayed(mPlayingChecker, 250);
		} else {
			mProgressView.setVisibility(View.GONE);
		}

		/* create playlist */
		mFinishOnCompletion = intent.getBooleanExtra(
				MediaStore.EXTRA_FINISH_ON_COMPLETION, true);
		mPlayListType = intent.getStringExtra(MediaStore.PLAYLIST_TYPE);
		mPlayList = new ArrayList<String>();
		if (mPlayListType != null) {
			if (mPlayListType
					.equalsIgnoreCase(MediaStore.PLAYLIST_TYPE_CUR_FOLDER)) {
				/* create playlist from current folder */
				createFolderDispList();
			} else if (mPlayListType
					.equalsIgnoreCase(MediaStore.PLAYLIST_TYPE_MEDIA_PROVIDER)) {
				/* create playlist from mediaprovider */
				createMediaProviderDispList(mUri, mContext);
			}
		} else {
			Log.w(TAG,
					"*********** scheme is null or scheme != file, create playlist failed *************");
		}

		mVideoView.setBDFolderPlayMode(intent.getBooleanExtra(
				MediaStore.EXTRA_BD_FOLDER_PLAY_MODE, false));
		mVideoView.setOnSubFocusItems(this);
		mVideoView.setOnErrorListener(this);
		mVideoView.setOnCompletionListener(this);
		mVideoView.setVideoURI(mUri, path000, 1);
		mMediaController = new MediaController(context);
		setImageButtonListener();
		mVideoView.setMediaController(mMediaController);
		mMediaController.setFilePathTextView(mUri.getPath());
		// make the video view handle keys for seeking and pausing
		mVideoView.requestFocus();
		mMediaController.setOnListDataChanged(this);

		mBookmarkService = new BookmarkService(mContext);
		final int bookmark = getBookmark();
		// SAVE sub and track, add by maizirong
		mCurrentSubSave = getSubSave();
		mCurrentTrackSave = getTrackSave();
		mCurrentSubColorSave = getSubColorSave();
		mCurrentSubSizeSave = getSubSizeSave();
		Log.v("Mzizirong",
				"_subBookMark______" + Integer.toString(mCurrentSubSave));
		if (bookmark > 0) {
			replayVideoDialog();
			mVideoView.seekTo(bookmark);
			// deleteBookmark();
		}

		mVideoView.start();
		// setSaves();
	}

	private Uri Uri2File2Uri(Uri videoUri) {
		String path = null;
		Cursor c = null;
		IContentProvider mMediaProvider = mContext.getContentResolver()
				.acquireProvider("media");
		String[] VIDEO_PROJECTION = new String[] { Video.Media.DATA };

		/* get video file */
		try {
			c = mMediaProvider.query(videoUri, VIDEO_PROJECTION, null, null,
					null, null);
		} catch (RemoteException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		if (c != null) {
			try {
				while (c.moveToNext()) {
					path = c.getString(0);
				}
			} finally {
				c.close();
				c = null;
			}
		}

		if (path != null) {
			return Uri.fromFile(new File(path));
		} else {
			Log.w(TAG, "************ Uri2File2Uri failed ***************");
			return videoUri;
		}
	}

	private void replayVideoDialog() {
		LayoutInflater inflate = (LayoutInflater) mContext
				.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
		View replayView = inflate.inflate(R.layout.dialog_replay, null);
		((Button) replayView.findViewById(R.id.replay_confirm))
				.setOnClickListener(new View.OnClickListener() {
					public void onClick(View arg0) {
						mHandler.removeMessages(DISMISS_DIALOG);
						mReplayDialog.dismiss();
						Boolean i = deleteBookmark();
						mCurrentSubSave = 0;
						mCurrentTrackSave = 0;
						mCurrentSubColorSave = -256;
						mCurrentSubSizeSave = 32;
						Log.v("Maizirong",
								"___deleteBookmark()_____"
										+ Boolean.toString(i));
						mVideoView.setVideoURI(mUri);
						mVideoView.start();
						setSaves();
					}
				});
		((Button) replayView.findViewById(R.id.replay_cancel))
				.setOnClickListener(new View.OnClickListener() {
					public void onClick(View arg0) {
						mHandler.removeMessages(DISMISS_DIALOG);
						mReplayDialog.dismiss();
					}
				});
		mReplayDialog = new Dialog(mContext, R.style.dialog);
		mReplayDialog.setContentView(replayView);
		mReplayDialog.show();

		mDismissMsg = mHandler.obtainMessage(DISMISS_DIALOG);
		mHandler.sendMessageDelayed(mDismissMsg, DISMISS_DELAY);
	}

	private void initToast() {
		mJumpToast = Toast.makeText(mContext, "", Toast.LENGTH_SHORT);
		mJumpToast.setGravity(Gravity.CENTER, 0, 0);
		mSubtitleToast = new MyToast(mContext);
		mAudioToast = new MyToast(mContext);
		mPrevToast = new MyToast(mContext);
		mNextToast = new MyToast(mContext);
		mStopToast = new MyToast(mContext);
		mRepeatToast = new MyToast(mContext);

		ImageView imageView;
		imageView = (ImageView) mSubtitleToast.getView().findViewById(
				R.id.message_image);
		imageView.setImageResource(R.drawable.button_subtitle);
		imageView = (ImageView) mAudioToast.getView().findViewById(
				R.id.message_image);
		imageView.setImageResource(R.drawable.button_track);
		imageView = (ImageView) mPrevToast.getView().findViewById(
				R.id.message_image);
		imageView.setImageResource(R.drawable.button_prev);
		imageView = (ImageView) mNextToast.getView().findViewById(
				R.id.message_image);
		imageView.setImageResource(R.drawable.button_next);
		imageView = (ImageView) mStopToast.getView().findViewById(
				R.id.message_image);
		imageView.setImageResource(R.drawable.button_stop);
		imageView = (ImageView) mRepeatToast.getView().findViewById(
				R.id.message_image);
		imageView.setImageResource(R.drawable.button_repeat);
		TextView textView;
		textView = (TextView) mPrevToast.getView().findViewById(R.id.message);
		textView.setVisibility(View.GONE);
		textView = (TextView) mPrevToast.getView().findViewById(R.id.message);
		textView.setVisibility(View.GONE);
	}

	public void subFocusItems() {
		/* repeat mode */
		mRepeatMode = sp.getInt(EDITOR_MODEREPEAT, 0);

		/* sub gate */
		boolean gate = sp.getBoolean(EDITOR_SUBGATE, true);
		mVideoView.setSubGate(gate);

		/* sub color */
		int[] listColor = mRes.getIntArray(R.array.screen_color_values);
		int clorIndex = sp.getInt(EDITOR_SUBCOLOR, 0);
		mVideoView.setSubColor(listColor[clorIndex]);

		/* sub char size */
		int[] listCharsize = mRes.getIntArray(R.array.screen_charsize_values);
		int charsizeIndex = sp.getInt(EDITOR_SUBCHARSIZE, 2);
		mVideoView.setSubFontSize(listCharsize[charsizeIndex]);

		/* zoom mode */
		int zoom = sp.getInt(EDITOR_ZOOM, 0);
		mVideoView.setZoomMode(zoom);
	}

	private static boolean uriSupportsBookmarks(Uri uri) {
		if (uri.getScheme() == null) {
			return false;
		}
		return ("file".equalsIgnoreCase(uri.getScheme()));
	}

	private int getBookmark() {
		return mBookmarkService.findByPathReturnSeek(mUri.getPath());
	}

	// SAVE sub and track, add by maizirong
	private int getSubSave() {
		return mBookmarkService.findByPathReturnSubSave(mUri.getPath());
	}

	private int getTrackSave() {
		return mBookmarkService.findByPathReturnTrackSave(mUri.getPath());
	}

	// SAVE subColor and subFontSize, add by maizirong
	private int getSubColorSave() {
		return mBookmarkService.findByPathReturnSubColorSave(mUri.getPath());
	}

	private int getSubSizeSave() {
		return mBookmarkService.findByPathReturnSubSizeSave(mUri.getPath());
	}

	private boolean deleteBookmark() {
		return mBookmarkService.delete(mUri.getPath());
	}

	private void setBookmark(int bookmark) {
		if (!uriSupportsBookmarks(mUri)) {
			return;
		}

		String path = mUri.getPath();
		if (mBookmarkService.findByPathReturnSeek(path) != 0) {
			mBookmarkService.update(path, bookmark, mCurrentSubSave,
					mCurrentTrackSave, mCurrentSubColorSave,
					mCurrentSubSizeSave);
		} else {
			mBookmarkService.save(path, bookmark, mCurrentSubSave,
					mCurrentTrackSave, mCurrentSubColorSave,
					mCurrentSubSizeSave);
		}

	}

	@Override
	public void initSubAndTrackInfo() {
		// TODO Auto-generated method stub

		/* get track info */
		mTransformTrack = null;
		TrackInfoVendor[] trackList = mVideoView.getTrackList();
		if (trackList != null) {
			int trackCount = trackList.length;
			mTransformTrack = new String[trackList.length];
			for (int i = 0; i < trackCount; i++) {
				try {
					if (trackList[i].charset
							.equals(MediaPlayer.CHARSET_UNKNOWN)) {
						mTransformTrack[i] = new String(trackList[i].name,
								"UTF-8");
					} else {
						mTransformTrack[i] = new String(trackList[i].name,
								trackList[i].charset);
					}
				} catch (UnsupportedEncodingException e) {
					// TODO Auto-generated catch block
					Log.w(TAG, "*********** unsupported encoding: "
							+ trackList[i].charset);
					e.printStackTrace();
				}
			}
		}

		/* get sub info */
		mTransformSubtitle = null;
		SubInfo[] subList = mVideoView.getSubList();

		if (subList != null) {
			int subCount = subList.length;
			mTransformSubtitle = new String[subList.length];
			for (int i = 0; i < subCount; i++) {
				try {
					if (subList[i].charset.equals(MediaPlayer.CHARSET_UNKNOWN)) {
						mTransformSubtitle[i] = new String(subList[i].name,
								"UTF-8");
					} else {
						mTransformSubtitle[i] = new String(subList[i].name,
								subList[i].charset);
					}

				} catch (UnsupportedEncodingException e) {
					// TODO Auto-generated catch block
					Log.w(TAG, "*********** unsupported encoding: "
							+ subList[i].charset);
					e.printStackTrace();
				}
			}
		}
	}

	private void sendKeyIntent(int keycode) {
		final int keyCode = keycode;
		// to avoid deadlock, start a thread to perform operations
		Thread sendKeyDelay = new Thread() {
			public void run() {
				try {
					int count = 1;

					IWindowManager wm = IWindowManager.Stub
							.asInterface(ServiceManager.getService("window"));
					for (int i = 0; i < count; i++) {
						Thread.sleep(100);
						long now = SystemClock.uptimeMillis();
						if (!mOnPause) {
							KeyEvent keyDown = new KeyEvent(now, now,
									KeyEvent.ACTION_DOWN, keyCode, 0);
							InputManager.getInstance().injectInputEvent(
									keyDown,
									InputManager.INJECT_INPUT_EVENT_MODE_ASYNC);
							// wm.injectKeyEvent(keyDown, false);
							KeyEvent keyUp = new KeyEvent(now, now,
									KeyEvent.ACTION_UP, keyCode, 0);
							InputManager.getInstance().injectInputEvent(keyUp,
									InputManager.INJECT_INPUT_EVENT_MODE_ASYNC);
							// wm.injectKeyEvent(keyUp, false);
						}
					}
				} catch (InterruptedException e) {
					e.printStackTrace();
				}/*
				 * catch (RemoteException e) { e.printStackTrace(); }
				 */
			}
		};
		sendKeyDelay.start();
	}

	private void createFolderDispList() {
		String fileNameText, filePathText;
		File filePath;

		String[] fileEndingVideo = mRes.getStringArray(R.array.fileEndingVideo);
		fileNameText = mUri.getPath();
		int index = fileNameText.lastIndexOf('/');
		if (index >= 0) {
			filePathText = fileNameText.substring(0, index);
			filePath = new File(filePathText);
			File[] fileList = filePath.listFiles();
			if (fileList != null && filePath.isDirectory()) {
				for (File currenFile : fileList) {
					String fileName = currenFile.getName();
					int indexPoint = fileName.lastIndexOf('.');
					if (indexPoint > 0 && currenFile.isFile()) {
						String fileEnd = fileName.substring(indexPoint + 1);
						for (int i = 0; i < fileEndingVideo.length; i++) {
							if (fileEnd.equalsIgnoreCase(fileEndingVideo[i])) {
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
		for (int i = 0; i < mPlayList.size(); i++) {
			if (mCurrentPath.equalsIgnoreCase(mPlayList.get(i))) {
				mCurrentIndex = i;
				break;
			}
		}
	}

	private void createMediaProviderDispList(Uri uri, Context mContext) {
		Cursor c = null;
		IContentProvider mMediaProvider = mContext.getContentResolver()
				.acquireProvider("media");
		Uri mVideoUri = Video.Media.getContentUri("external");
		String[] VIDEO_PROJECTION = new String[] { Video.Media.DATA };

		/* get playlist */
		try {
			c = mMediaProvider.query(mVideoUri, VIDEO_PROJECTION, null, null,
					null, null);
		} catch (RemoteException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		if (c != null) {
			try {
				while (c.moveToNext()) {
					String path = c.getString(0);
					if (new File(path).exists()) {
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
			for (int i = 0; i < mPlayList.size(); i++) {
				if (mCurrentPath.equalsIgnoreCase(mPlayList.get(i))) {
					mCurrentIndex = i;
					break;
				}
			}
		}
	}

	private void updateDispList(String mediaPath) {
		int size = mPlayList.size();
		String path;
		if (size > 0) {
			for (int i = size - 1; i >= 0; i--) {
				path = mPlayList.get(i);
				if (path.startsWith(mediaPath)) {
					mPlayList.remove(i);
				}
			}
		}
	}

	private void setImageButtonListener() {
		String scheme = mUri.getScheme();
		if (scheme != null && scheme.equalsIgnoreCase("file")) {
			mMediaController.setPrevNextListeners(mNextListener, mPrevListener);
		} else {
			mMediaController.setPrevNextListeners(null, null);
		}
		if (mPlayList.size() > 0) {
			mMediaController.setRepeatListener(mRepeatModeListener);
		}
		mMediaController.set3DListener(mMode3DListener);
		mMediaController.setTrackListener(mTrackListener);
		mMediaController.setZoomListener(mZoomListener);
		mMediaController.setSubsetListener(mSubsetListener);
		mMediaController.setJumpListener(mJumpClickListener);
	}

	View.OnClickListener mNextListener = new View.OnClickListener() {
		public void onClick(View v) {
			PlayNext();
		}
	};
	View.OnClickListener mPrevListener = new View.OnClickListener() {
		public void onClick(View v) {
			PlayPrev();
		}
	};

	@Override
	public void OnMediaPrevKeyClickListener() {
		// TODO Auto-generated method stub
		mPrevToast.show();
		PlayPrev();
	}

	@Override
	public void OnMediaNextKeyClickListener() {
		// TODO Auto-generated method stub
		mNextToast.show();
		PlayNext();
	}

	@Override
	public void OnMediaStopKeyClickListener() {
		// TODO Auto-generated method stub
		mStopToast.show();
		sendKeyIntent(KeyEvent.KEYCODE_BACK);
	}

	@Override
	public void OnMediaRepeatKeyClickListener() {
		// TODO Auto-generated method stub
		if (mPlayList.size() > 0) {
			String[] listModeRepeat = mRes
					.getStringArray(R.array.screen_repeat_entries);
			mRepeatMode = (mRepeatMode + 1) % listModeRepeat.length;
			editor.putInt(EDITOR_MODEREPEAT, mRepeatMode);
			editor.commit();

			TextView tv = (TextView) mRepeatToast.getView().findViewById(
					R.id.message);
			tv.setText(listModeRepeat[mRepeatMode]);
			mRepeatToast.show();
		} else {
			TextView tv = (TextView) mRepeatToast.getView().findViewById(
					R.id.message);
			tv.setText(R.string.playlist_miss);
			mRepeatToast.show();
		}
	}

	@Override
	public void OnMediaSubtitleKeyClickListener() {
		// TODO Auto-generated method stub

		TextView tv = (TextView) mSubtitleToast.getView().findViewById(
				R.id.message);

		if (mTransformSubtitle != null) {
			int focus;
			int subCount = mTransformSubtitle.length;
			focus = mVideoView.getCurSub();
			/* if(mSubtitleToast.isShowing()) */{
				String currentSub = tv.getText().toString();
				for (int i = 0; i < mTransformSubtitle.length; i++) {
					if (currentSub.equals(mTransformSubtitle[i])) {
						focus = i;
						break;
					}
				}
				focus = (focus + 1) % subCount;
			}
			if (focus < subCount) {
				tv.setText(mTransformSubtitle[focus]);
			}
		} else {
			String[] SubList = mRes
					.getStringArray(R.array.screen_select_entries);
			tv.setText(SubList[0]);
		}
		mSubtitleToast.show();
	}

	@Override
	public void OnMediaAudioKeyClickListener() {
		// TODO Auto-generated method stub
		TextView tv = (TextView) mAudioToast.getView().findViewById(
				R.id.message);

		if (mTransformTrack != null) {
			int trackCount = mTransformTrack.length;
			int focus = mVideoView.getCurTrack();

			/* if(mAudioToast.isShowing()) */{
				String currentTrack = tv.getText().toString();
				for (int i = 0; i < mTransformTrack.length; i++) {
					if (currentTrack.equals(mTransformTrack[i])) {
						focus = i;
						break;
					}
				}
				focus = (focus + 1) % trackCount;
			}
			if (focus < trackCount) {
				tv.setText(mTransformTrack[focus]);
			}
		} else {
			String[] TrackList = mRes
					.getStringArray(R.array.screen_track_entries);
			tv.setText(TrackList[0]);
		}
		mAudioToast.show();
	}

	private void PlayPrev() {
		int size = mPlayList.size();
		if (mCurrentIndex >= 0 && size > 0) {
			if (size == 1) {
				mCurrentIndex = 0;
			} else if (mCurrentIndex == 0) {
				mCurrentIndex = size - 1;
			} else {
				mCurrentIndex = (mCurrentIndex - 1) % size;
			}
			mUri = Uri.fromFile(new File(mPlayList.get(mCurrentIndex)));
			playFile();
		}
	}

	private void PlayNext() {
		int size = mPlayList.size();
		if (mCurrentIndex >= 0 && size > 0) {
			mCurrentIndex = (mCurrentIndex + 1) % size;
			mUri = Uri.fromFile(new File(mPlayList.get(mCurrentIndex)));
			playFile();
		}
	}

	private void playFile() {
		// mWakeLock.acquire();

		mHandler.postDelayed(mPlayRunnable, 500);

		// mWakeLock.release();
	}

	View.OnClickListener mJumpClickListener = new View.OnClickListener() {

		@Override
		public void onClick(View arg0) {
			// TODO Auto-generated method stub
			mMediaController.setHolding(true);
			OnTimeConfirmListener confirmListener = new OnTimeConfirmListener() {

				@Override
				public void onTimeConfirm(int time) {
					// TODO Auto-generated method stub
					Log.i("jump", "*************time: " + time);
					int duration = mVideoView.getDuration() / 1000;
					if (time <= duration) {
						mVideoView.seekTo(time * 1000);
					} else {
						String string = mContext.getResources().getString(
								R.string.overflow_toast);
						string += String.format("%02d:%02d:%02d",
								duration / 3600, duration / 60 % 60,
								duration % 60);
						mJumpToast.setText(string);
						mJumpToast.show();
					}
					mJumpView.jumpViewDismiss();
					mMediaController.setHolding(false);
				}

			};
			OnCancelListener cancelListener = new OnCancelListener() {

				@Override
				public void onCancel(DialogInterface dialog) {
					// TODO Auto-generated method stub
					mJumpView.jumpViewDismiss();
					mMediaController.setHolding(false);
				}
			};
			int duration = mVideoView.getDuration() / 1000;
			mJumpView = new JumpView(mContext, duration, confirmListener);
			mJumpView.setOnCancelListener(cancelListener);
		}
	};

	View.OnFocusChangeListener mMode3DListener = new View.OnFocusChangeListener() {
		@Override
		public void onFocusChange(View v, boolean hasFocus) {
			// TODO Auto-generated method stub

			if (mMediaController.getMediaControlFocusId() == R.id.mode3D) {
				return;
			}

			if (hasFocus) {
				mMode3DRunnable = new Runnable() {

					@Override
					public void run() {
						// TODO Auto-generated method stub
						Log.w(TAG, "Mode3D has focused");
						// int currentMode =
						// mVideoView.getOutputDimensionType();
						String[] listMode3D = mRes
								.getStringArray(R.array.screen_3d_entries);
						mMediaController.setListViewData(R.id.mode3D, 0,
								listMode3D);
					}

				};
				mHandler.postDelayed(mMode3DRunnable, 200);
			} else {
				if (mMode3DRunnable != null) {
					mHandler.removeCallbacks(mMode3DRunnable);
					mMode3DRunnable = null;
				}
			}
		}
	};

	View.OnFocusChangeListener mRepeatModeListener = new View.OnFocusChangeListener() {
		@Override
		public void onFocusChange(View v, boolean hasFocus) {
			// TODO Auto-generated method stub

			if (mMediaController.getMediaControlFocusId() == R.id.repeat) {
				return;
			}

			if (hasFocus) {
				mRepeatRunnable = new Runnable() {

					@Override
					public void run() {
						// TODO Auto-generated method stub
						Log.w(TAG, "Mode repeat has focused");
						int currentMode = sp.getInt(EDITOR_MODEREPEAT, 0);
						String[] listModeRepeat = mRes
								.getStringArray(R.array.screen_repeat_entries);
						mMediaController.setListViewData(R.id.repeat,
								currentMode, listModeRepeat);
					}
				};
				mHandler.postDelayed(mRepeatRunnable, 200);
			} else {
				if (mRepeatRunnable != null) {
					mHandler.removeCallbacks(mRepeatRunnable);
					mRepeatRunnable = null;
				}
			}
		}
	};

	View.OnFocusChangeListener mTrackListener = new View.OnFocusChangeListener() {
		@Override
		public void onFocusChange(View v, boolean hasFocus) {
			// TODO Auto-generated method stub

			if (mMediaController.getMediaControlFocusId() == R.id.track) {
				return;
			}

			if (hasFocus) {
				mTrackRunnable = new Runnable() {

					@Override
					public void run() {
						// TODO Auto-generated method stub
						if (mTransformTrack != null) {
							int currentTrack = mVideoView.getCurTrack();
							mMediaController.setListViewData(R.id.track,
									currentTrack, mTransformTrack);
						} else {
							mMediaController
									.setListViewData(
											R.id.track,
											0,
											mRes.getStringArray(R.array.screen_track_entries));
						}
					}
				};
				mHandler.postDelayed(mTrackRunnable, 200);
			} else {
				if (mTrackRunnable != null) {
					mHandler.removeCallbacks(mTrackRunnable);
					mTrackRunnable = null;
				}
			}
		}
	};

	View.OnFocusChangeListener mZoomListener = new View.OnFocusChangeListener() {
		@Override
		public void onFocusChange(View v, boolean hasFocus) {
			// TODO Auto-generated method stub

			if (mMediaController.getMediaControlFocusId() == R.id.zoom) {
				return;
			}

			if (hasFocus) {
				mZoomRunnable = new Runnable() {

					@Override
					public void run() {
						// TODO Auto-generated method stub
						int currentMode = sp.getInt(EDITOR_ZOOM, 0);
						String[] listModeZoom = mRes
								.getStringArray(R.array.screen_zoom_entries);
						mMediaController.setListViewData(R.id.zoom,
								currentMode, listModeZoom);
					}
				};
				mHandler.postDelayed(mZoomRunnable, 200);
			} else {
				if (mZoomRunnable != null) {
					mHandler.removeCallbacks(mZoomRunnable);
					mZoomRunnable = null;
				}
			}
		}
	};

	View.OnFocusChangeListener mSubsetListener = new View.OnFocusChangeListener() {
		@Override
		public void onFocusChange(View v, boolean hasFocus) {
			// TODO Auto-generated method stub

			if (mMediaController.getMediaControlFocusId() == R.id.subset) {
				return;
			}

			if (hasFocus) {
				mSubsetRunnable = new Runnable() {

					@Override
					public void run() {
						// TODO Auto-generated method stub
						String[] listSubset = mRes
								.getStringArray(R.array.screen_subset_entries);
						mMediaController.setListViewData(R.id.subset, 0,
								listSubset);
					}
				};
				mHandler.postDelayed(mSubsetRunnable, 200);
			} else {
				if (mSubsetRunnable != null) {
					mHandler.removeCallbacks(mSubsetRunnable);
					mSubsetRunnable = null;
				}
			}
		}
	};

	public void onCompletion(MediaPlayer arg0) {
		if (mJumpView != null) {
			mJumpView.jumpViewDismiss();
		}

		onCompletion();
	}

	public void onCompletion() {
		// TODO Auto-generated method stub

		mVideoView.setOnErrorListener(this);
		int size = mPlayList.size();
		Log.i(TAG, "************************ in onCompletion, mToQuit: "
				+ mToQuit + ", mPlayList size: " + size + ", mRepeatMode"
				+ mRepeatMode);
		if (mCurrentIndex >= 0 && size > 0) {
			switch (mRepeatMode) {
			case 0: // repeat all
			{
				mCurrentIndex = (mCurrentIndex + 1) % size;
				break;
			}
			case 1: // sequence play
			{
				if (mCurrentIndex + 1 < size) {
					mCurrentIndex++;
				} else {
					mToQuit = true;
					return;
				}
				break;
			}
			case 2: // repeat one
			{
				break;
			}
			case 3: // random play
			{
				mCurrentIndex = (int) (Math.random() * size);
				break;
			}
			default:
				break;
			}

			File nextFile = new File(mPlayList.get(mCurrentIndex));
			if (!nextFile.exists()) {
				mToQuit = true;
			} else {
				mUri = Uri.fromFile(nextFile);
				playFile();
			}
		}

		if (size == 0 && mFinishOnCompletion) {
			mToQuit = true;
		}
	}

	public void onPause() {
		mOnPause = true;

		mContext.unregisterReceiver(mBroadcastReceiver);

		mVideoPosition = mVideoView.getCurrentPosition();
		int duration = mVideoView.getDuration();

		// current time > 10s and save current position
		if (mVideoPosition > 10 * 1000 && duration - mVideoPosition > 10 * 1000) {
			setBookmark(mVideoPosition - 3 * 1000);
		} else {
			deleteBookmark();
		}

		if (mJumpView != null) {
			mJumpView.jumpViewDismiss();
		}
		mHandler.removeCallbacksAndMessages(null);
		mVideoView.suspend();
	}

	public void onResume() {
		if (mOnPause) {
			mVideoView.seekTo(mVideoPosition);
			mVideoView.resume();
			mOnPause = false;
		}

		/* receive the device eject message */
		mBroadcastReceiver = new BroadcastReceiver() {
			@Override
			public void onReceive(Context context, Intent intent) {
				// TODO Auto-generated method stub
				final String action = intent.getAction();
				final String mediaPath = intent.getData().getPath();

				if (action.equals(Intent.ACTION_MEDIA_EJECT)) {
					String path = mUri.getPath();
					Log.i(TAG, "*********** media eject **********");
					/* the current video's media was eject */
					if (path.startsWith(mediaPath)) {
						Toast toast = Toast.makeText(mContext,
								R.string.play_finish, Toast.LENGTH_SHORT);
						toast.setGravity(Gravity.CENTER, 0, 0);
						toast.show();
						onCompletion();
					} else if (mPlayList.size() > 0) {
						updateDispList(mediaPath);
					}
				}
			}
		};

		IntentFilter iFilter = new IntentFilter();
		iFilter.addAction(Intent.ACTION_MEDIA_EJECT);
		iFilter.addDataScheme("file");
		mContext.registerReceiver(mBroadcastReceiver, iFilter);
		setSaves();
	}

	public void setSaves() {
		new Handler().postDelayed(new Runnable() {
			@Override
			public void run() {
				Log.v("maizirong",
						"___run__" + Boolean.toString(mVideoView.isPlaying()));
				mVideoView.switchSub(mCurrentSubSave);
				mVideoView.switchTrack(mCurrentTrackSave);
				mVideoView.setSubColor(mCurrentSubColorSave);
				mVideoView.setSubFontSize(mCurrentSubSizeSave);
				Log.v("maizirong", "___setSaves___mCurrentSubSave:"
						+ mCurrentSubSave + "___mCurrentTrackSave"
						+ mCurrentTrackSave + "___mCurrentSubColorSave"
						+ mCurrentSubColorSave + "___mCurrentSubSizeSave"
						+ mCurrentSubSizeSave);
				Log.v("maizirong",
						"___run__" + Boolean.toString(mVideoView.isPlaying()));
			}
		}, 1000);
	}

	public void onDestroy() {
		mVideoView.stopPlayback();
		mBookmarkService.close();
	}

	@Override
	public boolean onError(MediaPlayer arg0, int arg1, int arg2) {
		// TODO Auto-generated method stub
		mHandler.removeCallbacksAndMessages(null);
		mProgressView.setVisibility(View.GONE);
		mToQuit = true;

		return false;
	}

	public boolean toQuit() {
		return mToQuit;
	}

	@Override
	public void OnListDataChangedListener(int mediaControlFocusId,
			int selectedIterm) {
		// TODO Auto-generated method stub
		switch (mediaControlFocusId) {
		case R.id.mode3D: {
			if (selectedIterm > 1) {
				setSublistData(selectedIterm);
			} else {
				mMediaController.setGridViewData(selectedIterm, 0);
			}
			/*
			 * int focusItem = selectedIterm; int[] list =
			 * mRes.getIntArray(R.array.screen_3d_values); int currentMode =
			 * mVideoView.getOutputDimensionType(); if(currentMode ==
			 * list[focusItem]) { break; } focusItem = focusItem%list.length;
			 * if( mVideoView.setOutputDimensionType(focusItem) != 0 ) {
			 * Log.w(TAG, "*********** change the 3D mode failed !"); }
			 */

			break;
		}
		case R.id.repeat: {
			mRepeatMode = selectedIterm;
			editor.putInt(EDITOR_MODEREPEAT, selectedIterm);
			editor.commit();

			break;
		}
		case R.id.track: {
			if (selectedIterm == mVideoView.getCurTrack()) {
				break;
			}
			if (mVideoView.switchTrack(selectedIterm) != 0) {
				Log.w(TAG, "*********** change the sub track failed !");
			} else {
				mCurrentTrackSave = selectedIterm;
			}

			break;
		}
		case R.id.zoom: {
			if (selectedIterm == mVideoView.getZoomMode()) {
				break;
			}
			mVideoView.setZoomMode(selectedIterm);
			editor.putInt(EDITOR_ZOOM, selectedIterm);
			editor.commit();

			break;
		}
		case R.id.subset: {
			setSublistData(selectedIterm);

			break;
		}
		default:
			break;
		}
	}

	private void setSublistData(int selectedIterm) {
		switch (selectedIterm) {
		case 0: {
			// set sub gate
			if (mMediaController.getMediaControlFocusId() == R.id.subset) {
				boolean gate = sp.getBoolean(EDITOR_SUBGATE, true);
				int focus = (gate == true) ? 0 : 1;

				String[] listGate = mRes
						.getStringArray(R.array.screen_gate_entries);
				mMediaController.setSublistViewData(0, focus, listGate);
			}

			break;
		}
		case 1: {
			// set sub select
			if (mMediaController.getMediaControlFocusId() == R.id.subset) {
				int focus = 0;
				if (mTransformSubtitle != null) {
					int subCount = mTransformSubtitle.length;
					focus = mVideoView.getCurSub();
					mMediaController.setSublistViewData(1, focus,
							mTransformSubtitle);
				} else {
					String[] transformSub = mRes
							.getStringArray(R.array.screen_select_entries);
					mMediaController.setSublistViewData(1, focus, transformSub);
				}
			}
			break;
		}
		case 2: {
			// set sub color
			if (mMediaController.getMediaControlFocusId() == R.id.subset) {
				int focus = sp.getInt(EDITOR_SUBCOLOR, 0);
				String[] listColor = mRes
						.getStringArray(R.array.screen_color_entries);
				mMediaController.setSublistViewData(2, focus, listColor);
			} else if (mMediaController.getMediaControlFocusId() == R.id.mode3D) {
				/* set the 3D anaglagh */
				int[] listAnaglaghValue = mRes
						.getIntArray(R.array.screen_3d_anaglagh_values);
				int currentAnaglagh = mVideoView.getAnaglaghType();
				int focus = 0;
				for (int i = 0; i < listAnaglaghValue.length; i++) {
					if (currentAnaglagh == listAnaglaghValue[i]) {
						focus = i;
						break;
					}
				}
				String[] listAnaglaghEntry = mRes
						.getStringArray(R.array.screen_3d_anaglagh_entries);
				mMediaController
						.setSublistViewData(2, focus, listAnaglaghEntry);
			}

			break;
		}
		case 3: {
			// set sub charsize
			if (mMediaController.getMediaControlFocusId() == R.id.subset) {
				int focus = sp.getInt(EDITOR_SUBCHARSIZE, 2);
				String[] listcharsize = mRes
						.getStringArray(R.array.screen_charsize_entries);
				mMediaController.setSublistViewData(3, focus, listcharsize);
			} else if (mMediaController.getMediaControlFocusId() == R.id.mode3D) {
				/* set the 3D anaglagh */
				int[] listAnaglaghValue = mRes
						.getIntArray(R.array.screen_3d_anaglagh_values);
				int currentAnaglagh = mVideoView.getAnaglaghType();
				int focus = 0;
				for (int i = 0; i < listAnaglaghValue.length; i++) {
					if (currentAnaglagh == listAnaglaghValue[i]) {
						focus = i;
						break;
					}
				}
				String[] listAnaglaghEntry = mRes
						.getStringArray(R.array.screen_3d_anaglagh_entries);
				mMediaController
						.setSublistViewData(3, focus, listAnaglaghEntry);
			}

			break;
		}
		case 4: // set sub charset
		{
			int focus = 0;
			String currentCharset = mVideoView.getSubCharset();
			String[] charsetValue = mRes
					.getStringArray(R.array.screen_charset_values);
			String[] listCharset = mRes
					.getStringArray(R.array.screen_charset_entries);
			for (int i = 0; i < charsetValue.length; i++) {
				if (currentCharset.equalsIgnoreCase(charsetValue[i])) {
					focus = i;
					break;
				}
			}
			mMediaController.setSublistViewData(4, focus, listCharset);

			break;
		}
		case 5: // set sub delay
		{
			int focus = 3;
			int subDelay = mVideoView.getSubDelay();
			int[] listValue = mRes.getIntArray(R.array.screen_delay_values);
			for (int i = 0; i < listValue.length; i++) {
				if (subDelay == listValue[i]) {
					focus = i;
					break;
				}
			}
			String[] listDelay = mRes
					.getStringArray(R.array.screen_delay_entries);
			mMediaController.setSublistViewData(5, focus, listDelay);

			break;
		}
		case 6: {
			String[] listOffset = mRes
					.getStringArray(R.array.screen_offset_entries);
			mMediaController.setSublistViewData(6, 0, listOffset);

			break;
		}
		}
	}

	@Override
	public void OnSublistDataChangedListener(int listFoucsIndex,
			int selectedIterm) {
		// TODO Auto-generated method stub
		if (mMediaController.getMediaControlFocusId() == R.id.subset) {
			setSubsetListData(listFoucsIndex, selectedIterm);
		} else if (mMediaController.getMediaControlFocusId() == R.id.mode3D) {
			setMode3DListData(listFoucsIndex, selectedIterm);
		}
	}

	private void setSubsetListData(int listFoucsIndex, int selectedIterm) {
		switch (listFoucsIndex) {
		case 0: // set sub gate
		{
			boolean switchOn;
			if (selectedIterm == 0) {
				switchOn = true;
			} else {
				switchOn = false;
			}
			mVideoView.setSubGate(switchOn);

			editor.putBoolean(EDITOR_SUBGATE, switchOn);
			editor.commit();

			break;
		}
		case 1: // set sub select
		{
			if (mVideoView.switchSub(selectedIterm) != 0) {
				Log.w(TAG, "*********** change the sub select failed !");
			} else {
				mCurrentSubSave = selectedIterm;
				Log.v("Maizirong",
						"______setSubsetListData_mCurrentSubSave:______"
								+ Integer.toString(selectedIterm));
			}

			break;
		}
		case 2: // set sub color
		{
			Log.v("maizirong",
					"set_sub_color________"
							+ Boolean.toString(mVideoView.isPlaying()));
			int[] listColor = mRes.getIntArray(R.array.screen_color_values);
			if (mVideoView.setSubColor(listColor[selectedIterm]) != 0) {
				Log.w(TAG, "*********** change the sub color failed !");
			} else {
				mCurrentSubColorSave = listColor[selectedIterm];
				Log.v("Maizirong",
						"______setSubsetListData_mCurrentSubColorSave:______"
								+ Integer.toString(listColor[selectedIterm]));
				editor.putInt(EDITOR_SUBCOLOR, selectedIterm);
				editor.commit();
			}

			break;
		}
		case 3: // set sub charsize
		{
			int[] listCharsize = mRes
					.getIntArray(R.array.screen_charsize_values);
			if (mVideoView.setSubFontSize(listCharsize[selectedIterm]) != 0) {
				Log.w(TAG, "*********** change the sub font size failed !");
			} else {
				mCurrentSubSizeSave = listCharsize[selectedIterm];
				Log.v("Maizirong",
						"______setSubsetListData_mCurrentSubSizeSave:______"
								+ Integer.toString(listCharsize[selectedIterm]));
				editor.putInt(EDITOR_SUBCHARSIZE, selectedIterm);
				editor.commit();
			}

			break;
		}
		case 4: // set sub charset
		{
			String[] listCharSet = mRes
					.getStringArray(R.array.screen_charset_values);
			if (mVideoView.setSubCharset(listCharSet[selectedIterm]) != 0) {
				Log.w(TAG, "*********** change the sub charset failed !");
			} else {
				editor.putInt(EDITOR_SUBCHARSET, selectedIterm);
				editor.commit();
			}

			break;
		}
		case 5: // set sub delay
		{
			int[] listDelay = mRes.getIntArray(R.array.screen_delay_values);
			if (mVideoView.setSubDelay(listDelay[selectedIterm]) != 0) {
				Log.w(TAG, "*********** change the sub delay failed !");
			}

			break;
		}
		case 6: // set sub offset
		{
			int[] listOffset = mRes.getIntArray(R.array.screen_offset_values);
			if (mVideoView.setSubPosition(listOffset[selectedIterm]) != 0) {
				Log.w(TAG, "*********** change the sub offset failed !");
			}

			break;
		}
		}
	}

	private void setMode3DListData(int listFoucsIndex, int selectedIterm) {
		mVideoView.setZoomMode(1);
		// anaglagh mode
		if (listFoucsIndex >= 2) {
			if (listFoucsIndex == 2) {
				mVideoView
						.setInputDimensionType(MediaPlayer.PICTURE_3D_MODE_SIDE_BY_SIDE);
			} else if (listFoucsIndex == 3) {
				mVideoView
						.setInputDimensionType(MediaPlayer.PICTURE_3D_MODE_TOP_TO_BOTTOM);
			}
			switch (selectedIterm) {
			case 0: {
				mVideoView.setAnaglaghType(MediaPlayer.ANAGLAGH_RED_BLUE);

				break;
			}
			case 1: {
				mVideoView.setAnaglaghType(MediaPlayer.ANAGLAGH_RED_GREEN);

				break;
			}
			case 2: {
				mVideoView.setAnaglaghType(MediaPlayer.ANAGLAGH_RED_CYAN);

				break;
			}
			case 3: {
				mVideoView.setAnaglaghType(MediaPlayer.ANAGLAGH_COLOR);

				break;
			}
			case 4: {
				mVideoView.setAnaglaghType(MediaPlayer.ANAGLAGH_HALF_COLOR);

				break;
			}
			case 5: {
				mVideoView.setAnaglaghType(MediaPlayer.ANAGLAGH_OPTIMIZED);

				break;
			}
			case 6: {
				mVideoView.setAnaglaghType(MediaPlayer.ANAGLAGH_YELLOW_BLUE);

				break;
			}
			}
			mVideoView
					.setOutputDimensionType(MediaPlayer.DISPLAY_3D_MODE_ANAGLAGH);
		} else { // 2D | 3D
			if (listFoucsIndex == 0) { // 2D
				switch (selectedIterm) {
				case 0: {
					mVideoView
							.setInputDimensionType(MediaPlayer.PICTURE_3D_MODE_SIDE_BY_SIDE);
					mVideoView
							.setOutputDimensionType(MediaPlayer.DISPLAY_3D_MODE_HALF_PICTURE);

					break;
				}
				case 1: {
					mVideoView
							.setInputDimensionType(MediaPlayer.PICTURE_3D_MODE_TOP_TO_BOTTOM);
					mVideoView
							.setOutputDimensionType(MediaPlayer.DISPLAY_3D_MODE_HALF_PICTURE);

					break;
				}
				case 2: {
					mVideoView
							.setOutputDimensionType(MediaPlayer.DISPLAY_3D_MODE_2D);

					break;
				}
				}
			} else if (listFoucsIndex == 1) { // 3D
				/* check Mode 3D enable */
				DisplayManagerAw displayManager = (DisplayManagerAw) mContext
						.getSystemService(Context.DISPLAY_SERVICE_AW);
				int displayType = displayManager.getDisplayOutputType(0);
				if (displayType != DisplayManagerAw.DISPLAY_OUTPUT_TYPE_HDMI
						|| displayType == DisplayManagerAw.DISPLAY_OUTPUT_TYPE_HDMI
						&& displayManager.isSupport3DMode() <= 0) {
					int id = R.string.not_hdmi;
					if (displayType == DisplayManagerAw.DISPLAY_OUTPUT_TYPE_HDMI) {
						id = R.string.not_support;
					}
					Toast toast = Toast.makeText(mContext, id,
							Toast.LENGTH_SHORT);
					toast.setGravity(Gravity.CENTER, 0, 0);
					toast.show();
					return;
				}

				switch (selectedIterm) {
				case 0: {
					mVideoView
							.setInputDimensionType(MediaPlayer.PICTURE_3D_MODE_SIDE_BY_SIDE);

					break;
				}
				case 1: {
					mVideoView
							.setInputDimensionType(MediaPlayer.PICTURE_3D_MODE_TOP_TO_BOTTOM);

					break;
				}
				case 2: {
					mVideoView
							.setInputDimensionType(MediaPlayer.PICTURE_3D_MODE_DOUBLE_STREAM);

					break;
				}
				case 3: {
					mVideoView
							.setInputDimensionType(MediaPlayer.PICTURE_3D_MODE_LINE_INTERLEAVE);

					break;
				}
				case 4: {
					mVideoView
							.setInputDimensionType(MediaPlayer.PICTURE_3D_MODE_COLUME_INTERLEAVE);

					break;
				}
				}
				mVideoView
						.setOutputDimensionType(MediaPlayer.DISPLAY_3D_MODE_3D);
			}
		}
	}
}

class BookmarkService {
	private final int MAXRECORD = 100;
	private MangerDatabase dbmanger;

	public BookmarkService(Context context) {
		dbmanger = new MangerDatabase(context);
	}

	public void save(String path, int bookmark, int subsave, int tracksave,
			int subcolorsave, int subsizesave) {
		long time = System.currentTimeMillis();

		SQLiteDatabase database = dbmanger.getWritableDatabase();
		if (getCount() == MAXRECORD) {
			long oldestTime = time;
			Cursor cursor = database.query(MangerDatabase.NAME, null, null,
					null, null, null, null);
			if (cursor != null) {
				try {
					while (cursor.moveToNext()) {
						long recordTime = cursor.getLong(2);
						if (recordTime < oldestTime) {
							oldestTime = recordTime;
						}
					}
				} finally {
					cursor.close();
				}
			}
			if (oldestTime < time) {
				database.execSQL("delete from " + MangerDatabase.NAME
						+ " where " + MangerDatabase.TIME + "=?",
						new Object[] { oldestTime });
			}
		}

		// database.execSQL("insert into "+MangerDatabase.NAME+"("+MangerDatabase.PATH+","+MangerDatabase.BOOKMARK+","+MangerDatabase.TIME+") values(?,?,?)",
		// new Object[] {path, bookmark, time});
		// SAVE sub,track,color,size ,add by maizirong
		Log.v("Maizirong",
				"______BookmarkService_save()______"
						+ Integer.toString(subsave) + "______"
						+ Integer.toString(tracksave) + "______"
						+ Integer.toString(subcolorsave) + "______"
						+ Integer.toString(subsizesave));
		database.execSQL("insert into " + MangerDatabase.NAME + "("
				+ MangerDatabase.PATH + "," + MangerDatabase.BOOKMARK + ","
				+ MangerDatabase.SUBSAVE + "," + MangerDatabase.TRACKSAVE + ","
				+ MangerDatabase.SUBCOLORSAVE + ","
				+ MangerDatabase.SUBSIZESAVE + "," + MangerDatabase.TIME
				+ ") values(?,?,?,?,?,?,?)", new Object[] { path, bookmark,
				subsave, tracksave, subcolorsave, subsizesave, time });
	}

	public boolean delete(String path) {
		boolean ret = false;
		SQLiteDatabase database = dbmanger.getWritableDatabase();

		Cursor cursor = database.rawQuery("select * from "
				+ MangerDatabase.NAME + " where " + MangerDatabase.PATH + "=?",
				new String[] { path });
		if (cursor != null) {
			database.execSQL("delete from " + MangerDatabase.NAME + " where "
					+ MangerDatabase.PATH + "=?", new Object[] { path });
			cursor.close();

			ret = true;
		}

		return ret;
	}

	public void update(String path, int bookmark, int subsave, int tracksave,
			int subcolorsave, int subsizesave) {
		long time = System.currentTimeMillis();
		SQLiteDatabase database = dbmanger.getWritableDatabase();

		// SAVE sub,track,color,size ,add by maizirong
		Log.v("Maizirong",
				"______BookmarkService_update()______"
						+ Integer.toString(subsave) + "______"
						+ Integer.toString(tracksave) + "______"
						+ Integer.toString(subcolorsave) + "______"
						+ Integer.toString(subsizesave));
		database.execSQL("update " + MangerDatabase.NAME + " set "
				+ MangerDatabase.BOOKMARK + "=?," + MangerDatabase.TIME + "=?,"
				+ MangerDatabase.SUBSAVE + "=?," + MangerDatabase.TRACKSAVE
				+ "=?," + MangerDatabase.SUBCOLORSAVE + "=?,"
				+ MangerDatabase.SUBSIZESAVE + "=? where "
				+ MangerDatabase.PATH + "=?", new Object[] { bookmark, time,
				subsave, tracksave, subcolorsave, subsizesave, path });

	}

	public int findByPathReturnSeek(String path) {
		int ret = 0;
		SQLiteDatabase database = dbmanger.getWritableDatabase();

		Cursor cursor = database.rawQuery("select * from "
				+ MangerDatabase.NAME + " where " + MangerDatabase.PATH + "=?",
				new String[] { path });
		if (cursor != null) {
			try {
				if (cursor.moveToNext()) {
					ret = cursor.getInt(1);
				}
			} finally {
				cursor.close();
			}
		}

		Log.v("Maizirong", "_findByPath_path___" + path);
		Log.v("Maizirong", "_findByPath___" + Integer.toString(ret));
		return ret;
	}

	public int findByPathReturnSubSave(String path) {
		int sub = 0;
		SQLiteDatabase database = dbmanger.getWritableDatabase();

		Cursor cursor = database.rawQuery("select * from "
				+ MangerDatabase.NAME + " where " + MangerDatabase.PATH + "=?",
				new String[] { path });
		if (cursor != null) {
			try {
				if (cursor.moveToNext()) {
					sub = cursor.getInt(cursor.getColumnIndex("subsave"));
				}
			} finally {
				cursor.close();
			}
		}
		Log.v("Maizirong", "_findReturn__SubSave______" + Integer.toString(sub));
		return sub;
	}

	public int findByPathReturnTrackSave(String path) {
		int track = 0;
		SQLiteDatabase database = dbmanger.getWritableDatabase();
		Cursor cursor = database.rawQuery("select * from "
				+ MangerDatabase.NAME + " where " + MangerDatabase.PATH + "=?",
				new String[] { path });
		if (cursor != null) {
			try {
				if (cursor.moveToNext()) {
					track = cursor.getInt(cursor.getColumnIndex("tracksave"));
				}
			} finally {
				cursor.close();
			}
		}
		Log.v("Maizirong",
				"_findByPathReturn__TrackSave______" + Integer.toString(track));
		return track;
	}

	public int findByPathReturnSubColorSave(String path) {
		int subColor = -256;
		SQLiteDatabase database = dbmanger.getWritableDatabase();
		Cursor cursor = database.rawQuery("select * from "
				+ MangerDatabase.NAME + " where " + MangerDatabase.PATH + "=?",
				new String[] { path });
		if (cursor != null) {
			try {
				if (cursor.moveToNext()) {
					subColor = cursor.getInt(cursor
							.getColumnIndex("subcolorsave"));
				}
			} finally {
				cursor.close();
			}
		}
		Log.v("Maizirong",
				"_findByPathReturn__SubColorSave______"
						+ Integer.toString(subColor));
		return subColor;
	}

	public int findByPathReturnSubSizeSave(String path) {
		int subSize = 32;
		SQLiteDatabase database = dbmanger.getWritableDatabase();
		Cursor cursor = database.rawQuery("select * from "
				+ MangerDatabase.NAME + " where " + MangerDatabase.PATH + "=?",
				new String[] { path });
		if (cursor != null) {
			try {
				if (cursor.moveToNext()) {
					subSize = cursor.getInt(cursor
							.getColumnIndex("subsizesave"));
				}
			} finally {
				cursor.close();
			}
		}
		Log.v("Maizirong",
				"_findByPathReturn__SubSizeSave______"
						+ Integer.toString(subSize));
		return subSize;
	}

	public int getCount() {
		long count = 0;

		SQLiteDatabase database = dbmanger.getWritableDatabase();

		Cursor cursor = database.rawQuery("select count(*) from "
				+ MangerDatabase.NAME, null);
		if (cursor != null) {
			try {
				if (cursor.moveToLast()) {
					count = cursor.getLong(0);
				}
			} finally {
				cursor.close();
			}
		}

		return (int) count;
	}

	public void close() {
		dbmanger.close();
	}
}

class MangerDatabase extends SQLiteOpenHelper {
	public static final String NAME = "breakpoint";
	public static final String PATH = "path";
	public static final String BOOKMARK = "bookmark";
	public static final String TIME = "time";

	// SAVE sub and track, add by maizirong
	public static final String SUBSAVE = "subsave";
	public static final String TRACKSAVE = "tracksave";
	public static final String SUBCOLORSAVE = "subcolorsave";
	public static final String SUBSIZESAVE = "subsizesave";

	private static final int version = 1;

	public MangerDatabase(Context context) {
		super(context, NAME, null, version);
		// TODO Auto-generated constructor stub
	}

	@Override
	public void onCreate(SQLiteDatabase arg0) {
		// TODO Auto-generated method stub
		Log.w("MangerDatabase", "*********** create MangerDatabase !");

		// arg0.execSQL("CREATE TABLE IF NOT EXISTS "+NAME+" ("+PATH+" varchar PRIMARY KEY, "+BOOKMARK+" INTEGR, "+TIME+" LONG)");
		// SAVE sub and track, add by maizirong
		arg0.execSQL("CREATE TABLE IF NOT EXISTS " + NAME + " (" + PATH
				+ " varchar PRIMARY KEY, " + BOOKMARK + " INTEGR, " + SUBSAVE
				+ " INTEGR, " + TRACKSAVE + " INTEGR, " + SUBCOLORSAVE
				+ " INTEGR, " + SUBSIZESAVE + " INTEGR, " + TIME + " LONG)");
	}

	@Override
	public void onUpgrade(SQLiteDatabase arg0, int arg1, int arg2) {
		// TODO Auto-generated method stub
		arg0.execSQL("DROP TABLE IF EXISTS " + NAME);
		onCreate(arg0);
	}
}
