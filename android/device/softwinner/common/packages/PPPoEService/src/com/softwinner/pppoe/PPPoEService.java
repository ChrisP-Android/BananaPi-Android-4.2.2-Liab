package com.softwinner.pppoe;

import android.app.AlarmManager;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.net.ethernet.EthernetDevInfo;
import android.net.ethernet.EthernetManager;
import android.os.Handler;
import android.os.IBinder;
import android.os.RemoteException;
import android.os.SystemProperties;
import android.provider.Settings;
import android.util.Log;
import android.widget.Toast;

import java.io.BufferedReader;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.regex.Pattern;

public class PPPoEService extends Service implements Runnable{

	private static final String TAG = "PPPoEService";
	private static final boolean DEBUG = false;
	public static final String ACTION_START_PPPOE = "com.softwinner.pppoe.START_PPPOE";
	private static Object sLock = new Object();
	private static boolean isThreadRun = false;

	static private final String PPPOE_INFO_SAVE_FILE = "/data/system/pap-secrets";
	static private final int MAX_INFO_LENGTH = 128;

	private static final String SVC_STATE_CMD_PREFIX = "init.svc.";
	private static final String SVC_START_CMD = "ctl.start";
	private static final String SVC_STOP_CMD = "ctl.stop";
	private static final String SVC_STATE_RUNNING = "running";
	private static final String SVC_STATE_STOPPED = "stopped";
	private static final String PPP_STATE_RUNNING = "running";
	private static final String PPP_STATE_GONE = "gone";
	private static final String PPPOE_PROTOCOL = "pppoe";

	private static final int MAX_CONN_TRY_TIMES = 3;
	private static final int CONN_TRY_TIME_OUT = 5 * 1000;

	private static final int STATE_STOPED = 0;
	private static final int STATE_STARTING = 1;
	private static final int STATE_STARTED = 2;
	private static final int STATE_RETRY = 3;
	private static final int STATE_EXIT = 4;
	private static final int STATE_UNKWON = 5;
	
	private static final int SUCCESS_CODE_ENTRY = 0x00;
	private static final int SUCCESS_CONNECT = SUCCESS_CODE_ENTRY + 1;
	
	private static final int ERROR_CODE_ENTRY = 0x100;
	private static final int ERROR_CODE_AUTH_FAILED = ERROR_CODE_ENTRY + 1;
	private static final int ERROR_CODE_USER_STOP = ERROR_CODE_ENTRY + 2;

	private int mState = STATE_STOPED;
	private int mTry = MAX_CONN_TRY_TIMES;

	private Handler mHandler;

	private String mIface;

	private final IBinder mBinder = new IPppoeBinder.Stub() {

		public void connect(String iface, String username) {

		}

		@Override
		public boolean isConnect() throws RemoteException {
			return false;
		}
	};

	@Override
	public IBinder onBind(Intent intent) {
		return mBinder;
	}

	public void run(){
		boolean enable = false; 
		boolean oldenable = false; 
		mState = STATE_UNKWON;
		Log.d(TAG,"start pppoe services");
check_labe:
		for(;;){
			enable = Settings.Secure.getInt(getContentResolver()
					,Settings.Secure.PPPOE_ENABLE,0) !=0 ? true : false;
			if(enable != oldenable){
				while(!enable && !stop()){
				}

				/*  when pppoe is starting we do nothing  */
				if(enable && (mState != STATE_STARTING)){
					String loginInfo[] = readLoginInfo().split(Pattern.quote("*"));
					String iface = Settings.Secure.getString(
							getContentResolver(),Settings.Secure.PPPOE_INTERFACE);
					mIface = iface;
					if(loginInfo != null && loginInfo.length == 2 && iface != null){
						loginInfo[0] = loginInfo[0].replace('\"', ' ');
						loginInfo[0] = loginInfo[0].trim();
						try{
							connect(iface ,loginInfo[0]);
						}catch(Exception e){
							e.printStackTrace();
						}
					}
				}
				oldenable = enable;
			}

			/*  Check State  */
			if(mState == getState()){
				try{
					Thread.sleep(CONN_TRY_TIME_OUT);
				}catch(Exception e){
					e.printStackTrace();
				}
				continue check_labe;
			} 

			mState = getState();
			switch(mState){
			case STATE_STOPED:
				if(DEBUG) Log.d(TAG, "STATE_STOPED------------------");
				if(enable && mTry != 0){
					String loginInfo[] = readLoginInfo().split(Pattern.quote("*"));
					String iface = Settings.Secure.getString(
							getContentResolver(),Settings.Secure.PPPOE_INTERFACE);
					mIface = iface;
					if(loginInfo != null && loginInfo.length == 2 && iface != null){
						loginInfo[0] = loginInfo[0].replace('\"', ' ');
						loginInfo[0] = loginInfo[0].trim();
						try{
							if(DEBUG)  Log.d(TAG, "try again------------------");
							connect(iface ,loginInfo[0]);
						}catch(Exception e){
							e.printStackTrace();
						}
					}
					mTry--;
				} else {
					isThreadRun = false;
					stopSelf();
					sendNotification(0,ERROR_CODE_USER_STOP);
                    if(isEthernetInterface(mIface)){
						Intent intent = new Intent(EthernetManager.PPPOE_STATE_CHANGED_ACTION);
						intent.putExtra(EthernetManager.EXTRA_PPPOE_STATE, EthernetManager.PPPOE_STATE_DISABLED);
						sendBroadcast(intent);
					}
					if(DEBUG)  Log.w(TAG,"exit pppoe services");
					return;
				}
				continue check_labe;

			case STATE_STARTING:
				if(DEBUG)  Log.d(TAG, "STATE_STARTING------------------");
				try{
					Thread.sleep(CONN_TRY_TIME_OUT);
				}catch(Exception e){
					e.printStackTrace();
				}
				continue check_labe;
			case STATE_STARTED:
				if(DEBUG)  Log.d(TAG, "STATE_STARTED------------------");
				mTry = MAX_CONN_TRY_TIMES;
				sendNotification(1,SUCCESS_CONNECT);
				if(isEthernetInterface(mIface)){
					Intent intent = new Intent(EthernetManager.PPPOE_STATE_CHANGED_ACTION);
					intent.putExtra(EthernetManager.EXTRA_PPPOE_STATE, EthernetManager.PPPOE_STATE_ENABLED);
					sendBroadcast(intent);
				}
				continue check_labe;
			case STATE_RETRY:
				continue check_labe;
			case STATE_EXIT:
				try{
				    int code = Integer.parseInt(SystemProperties.get("net.pppoe.ppp-exit"));
				    if(DEBUG)Log.d(TAG, "PPPoE  Exit  Code: " + code);
				    sendNotification(0,code);
				    if(isEthernetInterface(mIface)){
						Intent intent = new Intent(EthernetManager.PPPOE_STATE_CHANGED_ACTION);
						intent.putExtra(EthernetManager.EXTRA_PPPOE_STATE, EthernetManager.PPPOE_STATE_DISABLED);
						sendBroadcast(intent);
					}
				}catch (Exception e){
					
				}
				continue check_labe;
			}
		}
	}

/*****************************************
  STATE_EXIT, exit-number form "net.pppoe.ppp-exit"
		EXIT_OK				0
		EXIT_FATAL_ERROR	1
		EXIT_OPTION_ERROR	2
		EXIT_NOT_ROOT		3
		EXIT_NO_KERNEL_SUPPORT	4   *
		EXIT_USER_REQUEST	5
		EXIT_LOCK_FAILED	6
		EXIT_OPEN_FAILED	7
		EXIT_CONNECT_FAILED	8
		EXIT_PTYCMD_FAILED	9
		EXIT_NEGOTIATION_FAILED	10  * 
		EXIT_PEER_AUTH_FAILED	11
		EXIT_IDLE_TIMEOUT	12
		EXIT_CONNECT_TIME	13
		EXIT_CALLBACK		14
		EXIT_PEER_DEAD		15
		EXIT_HANGUP			16   * 
		EXIT_LOOPBACK		17
		EXIT_INIT_FAILED	18
		EXIT_AUTH_TOPEER_FAILED	19  * 
		EXIT_CNID_AUTH_FAILED	21
**************************************************/
	private int getState(){
		String cmd = SVC_STATE_CMD_PREFIX + PPPOE_PROTOCOL;

		if(SVC_STATE_STOPPED.equals(SystemProperties.get(cmd))
				&& PPP_STATE_GONE.equals(SystemProperties.get("net.pppoe.reason"))
				&& !SystemProperties.get("net.pppoe.ppp-exit").isEmpty())
			return STATE_EXIT;
		if(SVC_STATE_STOPPED.equals(SystemProperties.get(cmd)))
			return STATE_STOPED;
		if(SVC_STATE_RUNNING.equals(SystemProperties.get(cmd))
				&& SystemProperties.get("net.pppoe.reason").isEmpty())
			return STATE_STARTING;
		if(SVC_STATE_RUNNING.equals(SystemProperties.get(cmd))
				&& PPP_STATE_RUNNING.equals(SystemProperties.get("net.pppoe.reason")))
			return STATE_STARTED;
		return STATE_UNKWON;
	}

	private boolean isEthernetInterface(String iface){
		if(iface == null) return false;
		EthernetManager ethernetManager = EthernetManager.getInstance(); 
		List<EthernetDevInfo> devices = ethernetManager.getDeviceNameList();
		for(EthernetDevInfo device:devices){
			if(iface.equals(device.getIfName())){
				return true;
			}
		}
		return false;
	}

	private void sendNotification(final int type, final int code){

		final String[] str = getResources().getStringArray(R.array.notification_string);   
		int drawableId[] = {                
			R.drawable.internet_fail,
			R.drawable.internet_success,};

		final NotificationManager notificationManager = (NotificationManager) getSystemService(NOTIFICATION_SERVICE);
		Notification notification = 
			new Notification(drawableId[type], str[type], System.currentTimeMillis());
		PendingIntent pi = PendingIntent.getActivity(this, 
				0, new Intent(), 0); 
		notification.flags |= Notification.FLAG_AUTO_CANCEL;
		notification.setLatestEventInfo(this, "PPPoE", str[type], pi);
		notificationManager.notify(R.drawable.ic_launcher, notification);
		
		//show toast
		String tmpStr = str[type];
		if(type == 0 && code != ERROR_CODE_USER_STOP){
			tmpStr += " : " + getResources().getString(R.string.error_code);
			if(code == 19){
				tmpStr += " " + getResources().getString(R.string.error_code_auth_failed);
			}
		}
		final String toastStr = tmpStr;
		mHandler.post(new Runnable(){
			@Override
			public void run() {
				Toast.makeText(PPPoEService.this, toastStr, Toast.LENGTH_SHORT).show();
			}            
		});
		
	}

	static String readLoginInfo(){
		File file = new File(PPPOE_INFO_SAVE_FILE);
		char[] buf = new char[MAX_INFO_LENGTH];
		String loginInfo = new String();        
		FileReader in;
		try {
			in = new FileReader(file);
			BufferedReader bufferedreader = new BufferedReader(in);
			loginInfo = bufferedreader.readLine();            
			bufferedreader.close();
			in.close();            
		} catch (IOException e) {
			e.printStackTrace();
		}        
		return loginInfo != null? loginInfo:new String();
	}

	public static void connect(String iface,String username) throws InterruptedException{
		SystemProperties.set(SVC_START_CMD, PPPOE_PROTOCOL + 
				":" + iface + " " + username);
		if(DEBUG)  Log.d(TAG,"SystemProperties.set" + PPPOE_PROTOCOL + 
				":" + iface + " " + username);
	}

	public static boolean stop(){
		//SystemProperties.set(SVC_STOP_CMD, PPPOE_PROTOCOL);
		execCmd("pppoe-disconnect");
		return isStoped();
	}
	
	public static void execCmd(String cmd) {
		DataOutputStream dos = null;
		DataInputStream dis = null;
		try {
			Process p = Runtime.getRuntime().exec("su");
			dos = new DataOutputStream(p.getOutputStream());
			cmd += "\n";
			dos.writeBytes(cmd);
			dos.flush();
			dos.writeBytes("exit\n");
			dos.flush();
			p.waitFor();
		} catch (IOException e) {
			e.printStackTrace();
		} catch (InterruptedException e) {
			e.printStackTrace();
		} finally {
			try {
				if (dos != null)
					dos.close();
				if (dis != null)
					dis.close();
			} catch (IOException e) {
				e.printStackTrace();
			}
		}
	}

	public static boolean isConnect(){
		String cmd = SVC_STATE_CMD_PREFIX + PPPOE_PROTOCOL;
		return (SVC_STATE_RUNNING.equals(SystemProperties.get(cmd))
				&& SVC_STATE_RUNNING.equals(SystemProperties.get("net.pppoe.reason")));
	}

	private static boolean isStoped(){
		String cmd = SVC_STATE_CMD_PREFIX + PPPOE_PROTOCOL;
		return SVC_STATE_STOPPED.equals(SystemProperties.get(cmd));
	}    


	@Override
		public void onStart(Intent intent, int startId) {
			if(ACTION_START_PPPOE.equals(intent.getAction())){

			}
			mHandler = new Handler();
			synchronized (sLock) {
				if (!isThreadRun) {
					isThreadRun = true;
					new Thread(this).start();
				}
			}
		}
}
