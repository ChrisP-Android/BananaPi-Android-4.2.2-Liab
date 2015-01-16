package com.softwinner.tvremoteserver;

import java.io.IOException;
import java.net.Socket;

import javax.net.ssl.KeyManager;
import javax.net.ssl.SSLContext;
import javax.net.ssl.SSLServerSocket;
import javax.net.ssl.SSLServerSocketFactory;
import javax.net.ssl.TrustManager;

import android.content.Context;
import android.os.ServiceManager;
import android.util.Log;
import android.view.IWindowManager;

import com.google.anymote.common.AnymoteFactory;
import com.google.anymote.common.ErrorListener;
import com.google.anymote.server.ServerAdapter;

public class AnymoteSever extends Thread {     
	private boolean DEBUG = true;
	private static final String TAG	= "AnymoteSever";
	private volatile boolean isCancel = false;
	private int mPort;
	private KeyStoreManager mKeyStoreManager;
	private SSLServerSocket server;
	private ServerAdapter mServerAdapter;
	private CommandDispatcher dispatcher;   
	private ErrorListener listener = new ErrorListener() {
		
		@Override
		public void onIoError(String arg0, Throwable arg1) {
			if(DEBUG)Log.d(TAG, "---io error !!!--");
			
		}
	};
	
	public AnymoteSever(int port, KeyStoreManager manager)
	{
		mPort = port;
		mKeyStoreManager = manager;
		dispatcher = new CommandDispatcher(IWindowManager.Stub.asInterface(ServiceManager.getService(Context.WINDOW_SERVICE)));
	}

	@Override
	public void run() {
		try {
			server = getServerSocker(mPort);   
		} catch (Exception e) {
			if(DEBUG)Log.d(TAG, "  -- get server socket failed !!!");
			e.printStackTrace();
		}
		while (!isCancel){
			try {
				Socket socket = server.accept();
				if(DEBUG)Log.d(TAG, "-----anymote accept client : " + socket.getInetAddress());
				mServerAdapter = AnymoteFactory.getServerAdapter(dispatcher, socket.getInputStream(), socket.getOutputStream(), listener);
			} catch (Exception e) {
				e.printStackTrace();
				if(DEBUG)Log.d(TAG, " accept exception !!");
			}
			
		}
		dispatcher.closeVmouse();
		if(DEBUG)Log.d(TAG, "  thread exit safely ");
	}
	
	public void cancle()
	{
		isCancel = true;
		try {
			server.close();
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
	}
	
	public ServerAdapter getServerAdapter()
	{
		return mServerAdapter;
	}
	
	private SSLServerSocket getServerSocker(int port) throws Exception
	{
		SSLServerSocket mServersocket;
		SSLContext mSSLContext = SSLContext.getInstance("TLS");
		KeyManager[] keyMangers = mKeyStoreManager.getKeyManagers();
		TrustManager[] trustMangers = mKeyStoreManager.getTrustManagers();
		mSSLContext.init(keyMangers, trustMangers, null);
		SSLServerSocketFactory factory = mSSLContext.getServerSocketFactory();
		mServersocket = (SSLServerSocket)factory.createServerSocket(port);
		mServersocket.setNeedClientAuth(true);
		return mServersocket;
	}
}
