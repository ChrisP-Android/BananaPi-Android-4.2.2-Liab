package com.softwinner.tvremoteserver;


import java.net.Socket;
import javax.net.ssl.SSLServerSocket;
import javax.net.ssl.SSLServerSocketFactory;
import javax.net.ssl.SSLSocket;

import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.DialogInterface.OnClickListener;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Message;
import android.util.Log;
import android.view.LayoutInflater;  
import android.view.View;
import android.view.Gravity;
import android.view.WindowManager;
import android.widget.LinearLayout;
import android.widget.LinearLayout.LayoutParams;
import android.widget.TextView;

import com.google.polo.pairing.PairingContext;
import com.google.polo.pairing.PairingListener;
import com.google.polo.pairing.PairingSession;
import com.google.polo.pairing.ServerPairingSession;
import com.google.polo.pairing.message.EncodingOption;
import com.google.polo.ssl.DummySSLServerSocketFactory;
import com.google.polo.wire.PoloWireInterface;
import com.google.polo.wire.WireFormat;

public class ParingServer extends Thread {
	private boolean DEBUG = true;
	private static final String TAG	= "ParingServer";
	private static final String SERVER_NAME = "AnymoteServer";
	private static final int SHOW_DIALOG = 1;
	private static final int CANCEL_DIALOG = 2;
	private TvRemoteService remoteService;
	private Context mContext;
	private int mPort;
	private KeyStoreManager mKeyStoreManager;
	private SSLServerSocket server;
	private AlertDialog paringDialog;
	private String secretCode;
	private String clientName;
	
	private Handler mHandler;
	
	public ParingServer(TvRemoteService service, Context context, int port, KeyStoreManager manager)
	{
		remoteService = service;
		mContext = context;
		mPort = port;
		mKeyStoreManager = manager;
		HandlerThread handlerThread = new HandlerThread("paringserver");
		handlerThread.start();
		mHandler = new Handler(handlerThread.getLooper()){

			@Override
			public void handleMessage(Message msg) {
				if (msg.what == SHOW_DIALOG)
				{	
					showParingDialog(clientName, secretCode);
				} else if (msg.what == CANCEL_DIALOG) {
					paringDialog.cancel();
				}
			}
			
		};
	}

	@Override
	public void run() {
		if(DEBUG) Log.d(TAG, "----------paring server start ----------");
		try {
			server = getServerSocket();
		} catch (Exception e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		while (true){
			Socket socket;
			ServerPairingSession session = null;
			try {
				socket = server.accept();
				if(DEBUG)Log.d(TAG, "--------paring server accept a client : " + socket.getInetAddress());
				PairingContext context;
				context = PairingContext.fromSslSocket((SSLSocket)socket, true);
				PoloWireInterface polowire = WireFormat.PROTOCOL_BUFFERS.getWireInterface(context);
				session = new ServerPairingSession(polowire, context, SERVER_NAME);
				EncodingOption hexEnc =
		            new EncodingOption(
		                EncodingOption.EncodingType.ENCODING_HEXADECIMAL, 4);
				session.addInputEncoding(hexEnc);
		        session.addOutputEncoding(hexEnc);
		        boolean ret = session.doPair(listener);
		        if (ret){
		        	mKeyStoreManager.storeCertificate(context.getClientCertificate());
		        	remoteService.restartAnymote();
		        }
		        if (paringDialog != null)
		        {
		        	mHandler.obtainMessage(CANCEL_DIALOG).sendToTarget();
		        }
			} catch (Exception e) {
				e.printStackTrace();
				if (session != null){
					session.teardown();
				}
				break;
			}
		}
	}
	
	private SSLServerSocket getServerSocket() throws Exception
	{
		SSLServerSocket mServerSocket;
		SSLServerSocketFactory factory = DummySSLServerSocketFactory.fromKeyManagers(mKeyStoreManager.getKeyManagers());
		mServerSocket = (SSLServerSocket)factory.createServerSocket(mPort);
		mServerSocket.setNeedClientAuth(true);
		if(DEBUG)Log.d(TAG, "-------create serversocket sucess port " + mPort);
		return mServerSocket;
	}
	
	private void showParingDialog(String clientName, String code)
	{
		LayoutInflater inflater = LayoutInflater.from(mContext);
		View dialogView = inflater.inflate(R.layout.paring_dialog, null);
		TextView message = (TextView)dialogView.findViewById(R.id.paring_message);
		message.setText(String.format(mContext.getResources().getString(R.string.device_message), clientName));
		TextView codeText = (TextView)dialogView.findViewById(R.id.paring_code);
		codeText.setText(code);
		AlertDialog.Builder builder = new AlertDialog.Builder(mContext);
		builder.setView(dialogView);
		builder.setCancelable(true);
		builder.setPositiveButton(R.string.ignore_requests, new OnClickListener() {
			
			@Override
			public void onClick(DialogInterface arg0, int arg1) {
				mHandler.obtainMessage(CANCEL_DIALOG).sendToTarget();
			}
		});
		builder.setNegativeButton(android.R.string.cancel, new OnClickListener() {
			
			@Override
			public void onClick(DialogInterface arg0, int arg1) {
				mHandler.obtainMessage(CANCEL_DIALOG).sendToTarget();
			}
		});
		paringDialog = builder.create();
		paringDialog.getWindow().setType(WindowManager.LayoutParams.TYPE_SYSTEM_ALERT);
		paringDialog.getWindow().setFlags(WindowManager.LayoutParams.FLAG_ALT_FOCUSABLE_IM,
                WindowManager.LayoutParams.FLAG_ALT_FOCUSABLE_IM);
		paringDialog.show();
	}
	
	PairingListener listener = new PairingListener() {
        public void onSessionEnded(PairingSession session) {
          if(DEBUG)Log.d(TAG, "onSessionEnded: " + session);
        }

        public void onSessionCreated(PairingSession session) {
        	if(DEBUG)Log.d(TAG, "onSessionCreated: " + session);
        }

        public void onPerformOutputDeviceRole(PairingSession session,
            byte[] gamma) {
        	final PairingSession mSession = session;
        	secretCode = session.getEncoder().encodeToString(gamma);
        	clientName = mSession.getPeerName();
        	mHandler.obtainMessage(SHOW_DIALOG).sendToTarget();
        	if(DEBUG)Log.d(TAG, "onPerformOutputDeviceRole: " + session + ", "
              + secretCode);
        }

        public void onPerformInputDeviceRole(PairingSession session) {
        	if(DEBUG)Log.d(TAG, "--------onPerformInputDeviceRole----------");
        }

        public void onLogMessage(LogLevel level, String message) {
        	if(DEBUG)Log.d(TAG, "Log: " + message + " (" + level + ")");
        }
      };
	
}
