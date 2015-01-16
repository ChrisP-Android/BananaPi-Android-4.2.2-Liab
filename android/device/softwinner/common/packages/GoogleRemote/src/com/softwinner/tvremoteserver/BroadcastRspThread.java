package com.softwinner.tvremoteserver;

import java.io.IOException;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.SocketException;
import android.os.Build;
import android.os.SystemProperties;
import android.util.Log;

public class BroadcastRspThread extends Thread {
	private boolean DEBUG = true;
	private static final String TAG = "BroadcastRspThread";
	private static final String DESIRED_SERVICE = "_anymote._tcp";
	private static final String COMMAND_DISCOVER = "discover";
	private static final int BROADCAST_SERVER_PORT = 9101;
	private int anymotePort;
	private DatagramSocket mSocket;
	
	public BroadcastRspThread(int port)	   
	{
		anymotePort = port;
		try {
			mSocket = new DatagramSocket(BROADCAST_SERVER_PORT);
		} catch (SocketException e) {
			Log.d(TAG, " create socket failed !!!!");
			e.printStackTrace();
		}
	}

	@Override
	public void run() {
		if(DEBUG) Log.d(TAG, "----------------broadcastReciever thread start -----------");
		byte[] buffer = new byte[256];
		
		while (true){
			DatagramPacket reviever = new DatagramPacket(buffer, buffer.length);
			try {
				mSocket.receive(reviever);
				handleBroadcast(reviever);
			} catch (IOException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
		}
		
	}
	
	private void handleBroadcast(DatagramPacket packet)
	{
		String str = new String(packet.getData(), 0, packet.getLength());
		Log.d(TAG, " get packet data " + str);
		String tokens[] = str.trim().split("\\s+");
		if (tokens.length != 3){
			Log.d(TAG, "-----get tokens less than 3");
		}
		if (tokens[0].equals(COMMAND_DISCOVER) && tokens[1].equals(DESIRED_SERVICE))
		{
		    String deviceName = SystemProperties.get("persist.sys.device_name", "MiniMax");
			String rsp = DESIRED_SERVICE + " " + deviceName + " " + anymotePort + "\n";
			byte[] buffer = rsp.getBytes();
			packet.setData(buffer, 0, buffer.length);
			try {
				mSocket.send(packet);
			} catch (IOException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
		}
	}
	
}
