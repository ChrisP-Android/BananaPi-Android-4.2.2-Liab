package com.softwinner.pppoe;

import java.util.List;
import java.util.regex.Pattern;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.net.NetworkInfo;
import android.net.ethernet.EthernetDevInfo;
import android.net.ethernet.EthernetManager;
import android.net.wifi.WifiManager;
import android.provider.Settings;
import android.util.Log;

public class Loader extends BroadcastReceiver {
    
	private static final String TAG = "PPPoE.Loader";
    public static final String ACTION_STATE_CHANGE = "com.softwinner.pppoe.ACTION_STATE_CHANGE";

    @Override
    public void onReceive(Context context, Intent intent) {
        String action = intent.getAction();
        Log.d(TAG,"receive : " + action);
        boolean autoConn = Settings.Secure.getInt(
                context.getContentResolver(),Settings.Secure.PPPOE_AUTO_CONN ,0)!=0 ? true : false;
        boolean enable = Settings.Secure.getInt(
                context.getContentResolver(),Settings.Secure.PPPOE_ENABLE, 0)!=0 ? true : false;
        
        if(!enable && ACTION_STATE_CHANGE.equals(action)){
			if (PPPoEService.isConnect()) {
        		PPPoEService.stop();
			}
		}

        if(enable && (autoConn || ACTION_STATE_CHANGE.equals(action))){
            Intent startIntent = new Intent(PPPoEService.ACTION_START_PPPOE);
            context.startService(startIntent);
        }
        
        String iface = Settings.Secure.getString(context.getContentResolver(), 
        		Settings.Secure.PPPOE_INTERFACE);
        
        if(iface == null){return;}
        
        boolean isEthernet = isEthernetInterface(context , iface);
        if(autoConn && !isEthernet && WifiManager.WIFI_STATE_CHANGED_ACTION.equals(action)){
        	final NetworkInfo networkInfo = (NetworkInfo)
                    intent.getParcelableExtra(WifiManager.EXTRA_NETWORK_INFO);
            boolean wifiConnected = networkInfo != null && networkInfo.isConnected(); 
            if(wifiConnected){
            	Log.d(TAG,"receive the wifi state change");
            	Intent startIntent = new Intent(PPPoEService.ACTION_START_PPPOE);
                context.startService(startIntent);
            }
        }
        
        if(enable && autoConn && isEthernet && EthernetManager.ETHERNET_LINKED_ACTION.equals(action)){
        	Intent startIntent = new Intent(PPPoEService.ACTION_START_PPPOE);
            context.startService(startIntent);
            Log.d(TAG,"receive the ethernet state change");
            
            String loginInfo[] = PPPoEService.readLoginInfo().split(Pattern.quote("*"));
			String ipppoeface = Settings.Secure.getString(
					context.getContentResolver(),Settings.Secure.PPPOE_INTERFACE);
			if(loginInfo != null && loginInfo.length == 2 && ipppoeface != null){
				loginInfo[0] = loginInfo[0].replace('\"', ' ');
				loginInfo[0] = loginInfo[0].trim();
				try{
					PPPoEService.connect(ipppoeface ,loginInfo[0]);
				}catch(Exception e){
					e.printStackTrace();
				}
			}
        }else if(isEthernet && EthernetManager.ETHERNET_DISLINKED_ACTION.equals(action)){
        	PPPoEService.stop();
        }
    }
    
	private boolean isEthernetInterface(Context context ,String iface){
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
}
