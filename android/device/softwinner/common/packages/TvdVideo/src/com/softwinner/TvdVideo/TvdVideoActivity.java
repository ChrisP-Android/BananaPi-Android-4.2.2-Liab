package com.softwinner.TvdVideo;

import android.app.Activity;
import android.content.Intent;
import android.media.MediaPlayer;
import android.os.Bundle;
import android.provider.MediaStore;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.widget.Toast;
import android.util.Log;

public class TvdVideoActivity extends Activity {
	private static final String TAG = "TvdVideoActivity";

	private MovieViewControl mControl;
	private boolean mBDFolderPlayMode = false;

	/** Called when the activity is first created. */
	@Override
	public void onCreate(Bundle bundle) {
		super.onCreate(bundle);

		requestWindowFeature(Window.FEATURE_NO_TITLE);
		getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,
				WindowManager.LayoutParams.FLAG_FULLSCREEN);
		String path000 = getIntent().getStringExtra("VideoPath000");
		Log.v(TAG, "path=" + path000);
		setContentView(R.layout.movie_view);
		View rootView = findViewById(R.id.root);
		mBDFolderPlayMode = getIntent().getBooleanExtra(
				MediaStore.EXTRA_BD_FOLDER_PLAY_MODE, false);
		mControl = new MovieViewControl(rootView, this, getIntent(), path000) {
			// add by maizirong, show insufficient memory error,then finish it.
			@Override
			public boolean onError(MediaPlayer arg0, int arg1, int arg2) {
				Log.e(TAG, "MeaidPlay onError : arg1=" + Integer.toString(arg1));
				if (arg1 == MediaPlayer.MEDIA_ERROR_UNKNOWN) {
					Toast.makeText(getApplicationContext(),
							R.string.error_memeory, Toast.LENGTH_LONG).show();
					finish();
				}
				return false;
			}

			@Override
			public void onCompletion() {
				if (mBDFolderPlayMode) {
					finish();
				} else {
					super.onCompletion();
					if (super.toQuit()) {
						finish();
					}
				}
			}
		};
	}

	@Override
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
	}
}