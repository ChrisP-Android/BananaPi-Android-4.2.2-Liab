package com.softwinner.tvremoteserver;

import android.app.Service;
import android.content.Intent;
import android.os.AsyncTask;
import android.os.IBinder;
import android.provider.Settings;
import android.util.Log;

public class TvRemoteService extends Service {
	public static final String TVREMOTE_ACTION="com.softwinner.tvremoteserver";

	private boolean DEBUG = true;
	private static final String TAG	= "TvRemoteService";
	private static final int ANYMOTE_SERVER_PORT = 9000;
	private KeyStoreManager mKeyStoreManager;
	private BroadcastRspThread mBroadcastRspThread;
	private AnymoteSever mAnymoteSever;
	private ParingServer mParingServer;
	static{
		System.loadLibrary("tvremote");
		}
	@Override
	public IBinder onBind(Intent intent) {
		// TODO Auto-generated method stub
		return null;
	}

	@Override
	public void onCreate() {
		if(DEBUG)Log.d(TAG," on create tv remote service");
		mKeyStoreManager = new KeyStoreManager(this);
		mBroadcastRspThread = new BroadcastRspThread(ANYMOTE_SERVER_PORT);
		mAnymoteSever = new AnymoteSever(ANYMOTE_SERVER_PORT, mKeyStoreManager);
		mParingServer = new ParingServer(this,this, ANYMOTE_SERVER_PORT +1, mKeyStoreManager);
	}

	@Override
	public int onStartCommand(Intent intent, int flags, int startId) {
		startServer();
		return Service.START_STICKY;
	}
	
	
	public void startServer()
	{
		if (!mKeyStoreManager.hasServerIdentityAlias()){
			new KeystoreInitializerTask(getUniqueId()).execute(mKeyStoreManager);
		} else {
			mBroadcastRspThread.start();
			mAnymoteSever.start();
			mParingServer.start();
		}
	}
	
	public void restartAnymote()
	{
		mAnymoteSever.cancle();
		mAnymoteSever = new AnymoteSever(ANYMOTE_SERVER_PORT, mKeyStoreManager);
		mAnymoteSever.start();
	}
	
	private String getUniqueId() {
	    String id = Settings.Secure.getString(getContentResolver(),
	        Settings.Secure.ANDROID_ID);
	    // null ANDROID_ID is possible on emulator
	    return id != null ? id : "emulator";
	  }
	
	class KeystoreInitializerTask extends AsyncTask<KeyStoreManager, Void, Void>
	{
		
		private final String id;

	    public KeystoreInitializerTask(String id) {
	      this.id = id;
	    }

		@Override
		protected Void doInBackground(KeyStoreManager... keyStoreManagers) {
			keyStoreManagers[0].initializeKeyStore(id);
			return null;
		}

		@Override
		protected void onPostExecute(Void result) {
			startServer();
		}
		
		
	}
	

}
