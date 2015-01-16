/*
 * Copyright (C) 2010 The Android-x86 Open Source Project
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
 *
 * Author: Yi Sun <beyounn@gmail.com>
 */

package com.android.server;

import java.net.UnknownHostException;
import android.net.ethernet.IEthernetManager;
import android.net.ethernet.EthernetManager;
import android.net.ethernet.EthernetDevInfo;
import android.net.ethernet.EthernetDataTracker;
import android.net.NetworkUtils;
import android.net.InterfaceConfiguration;
import android.net.DhcpInfoInternal;
import android.net.LinkProperties;
import android.net.LinkAddress;
import android.os.INetworkManagementService;
import android.os.Binder;
import android.os.Handler;
import android.os.IBinder;
import android.os.ServiceManager;
import android.os.RemoteException;
import android.provider.Settings;
import android.util.Log;
import android.util.Slog;
import android.util.Pair;
import android.content.BroadcastReceiver;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;

import java.io.BufferedInputStream;
import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.DataInputStream;
import java.io.File;
import java.io.FileDescriptor;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileWriter;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.PrintWriter;
import java.io.RandomAccessFile;
import java.io.UnsupportedEncodingException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.Collections;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.lang.String;

/**
 * EthernetService handles remote Ethernet operation requests by implementing
 * the IEthernetManager interface. It also creates a EtherentMonitor to listen
 * for Etherent-related events.
 *
 * @hide
 */
public class EthernetService extends IEthernetManager.Stub {
	private static final String TAG = "EthernetService";
	private static final int ETHERNET_HAS_CONFIG = 1;
	private static final boolean localLOGV = true;

	private int mEthState= EthernetManager.ETHERNET_STATE_DISABLED;
	private Context mContext;
	private EthernetDataTracker mTracker;
	private String[] DevName;
	private int isEnabled ;
	/*  the first String is now system_name, second String is boot_name  */
	private HashMap<String, EthernetDevInfo> mDeviceMap;
	private final INetworkManagementService mNMService;
    private final BroadcastReceiver mEthStateReceiver;
    private final IntentFilter mFilter;

	//private static final String ETH_CONFIG_FILE = "/data/misc/ethernet/eth.conf";
	private final String SYS_NET = "/sys/class/net/";
	private String ETH_USED = "";

	public EthernetService(Context context) {
		mContext = context;
		mDeviceMap = new HashMap<String, EthernetDevInfo>();

        IBinder b = ServiceManager.getService(Context.NETWORKMANAGEMENT_SERVICE);
        mNMService = INetworkManagementService.Stub.asInterface(b);
		mTracker = EthernetDataTracker.getInstance();
		mEthStateReceiver = new BroadcastReceiver(){
		@Override
		public void onReceive(Context context, Intent intent) {
				handleReceive(context, intent);
			}
		};
		mFilter = new IntentFilter();
        mFilter.addAction(EthernetManager.NETWORK_STATE_CHANGED_ACTION);

		scanDevice();

		if (localLOGV == true)
			Slog.i(TAG, "EthernetService Starting......\n");
        context.registerReceiver(mEthStateReceiver, mFilter);
	}

	/**
	 * check if the ethernet service has been configured.
	 * @return {@code true} if configured {@code false} otherwise
	 */
	public boolean isConfigured() {
		final ContentResolver cr = mContext.getContentResolver();
		return (Settings.Secure.getInt(cr, Settings.Secure.ETHERNET_CONF, 0) == ETHERNET_HAS_CONFIG);
	}

	/**
	 * Return the saved ethernet configuration
	 * @return ethernet interface configuration on success, {@code null} on failure
	 */
	public synchronized EthernetDevInfo getSavedConfig() {

		final ContentResolver cr = mContext.getContentResolver();
		if (!isConfigured())
			return null;

		EthernetDevInfo info = new EthernetDevInfo();
		try{
			info.setConnectMode(Settings.Secure.getInt(cr, Settings.Secure.ETHERNET_MODE));
		} catch (Settings.SettingNotFoundException e) {
		}
		info.setIfName(Settings.Secure.getString(cr, Settings.Secure.ETHERNET_IFNAME));
		info.setIpAddress(Settings.Secure.getString(cr, Settings.Secure.ETHERNET_IP));
		info.setDnsAddr(Settings.Secure.getString(cr, Settings.Secure.ETHERNET_DNS));
		info.setNetMask(Settings.Secure.getString(cr, Settings.Secure.ETHERNET_MASK));
		info.setGateWay(Settings.Secure.getString(cr, Settings.Secure.ETHERNET_ROUTE));

		return info;
	}

	/**
	 * Set the ethernet interface configuration mode
	 * @param mode {@code ETHERNET_CONN_MODE_DHCP} for dhcp {@code ETHERNET_CONN_MODE_MANUAL} for manual configure
	 */
	public synchronized void setMode(int mode) {
		final ContentResolver cr = mContext.getContentResolver();
		if (DevName != null) {
			Settings.Secure.putString(cr, Settings.Secure.ETHERNET_IFNAME, DevName[0]);
			Settings.Secure.putInt(cr, Settings.Secure.ETHERNET_CONF, 1);
			Settings.Secure.putInt(cr, Settings.Secure.ETHERNET_MODE, mode);
		}
	}

	/**
	 * update a ethernet interface information
	 * @param info  the interface infomation
	 */
	public synchronized void updateDevInfo(EthernetDevInfo info) {
		final ContentResolver cr = mContext.getContentResolver();
		Settings.Secure.putInt(cr, Settings.Secure.ETHERNET_CONF, 1);
		Settings.Secure.putString(cr, Settings.Secure.ETHERNET_IFNAME, info.getIfName());
		Settings.Secure.putString(cr, Settings.Secure.ETHERNET_IP, info.getIpAddress());
		Settings.Secure.putInt(cr, Settings.Secure.ETHERNET_MODE, info.getConnectMode());
		Settings.Secure.putString(cr, Settings.Secure.ETHERNET_DNS, info.getDnsAddr());
		Settings.Secure.putString(cr, Settings.Secure.ETHERNET_ROUTE, info.getGateWay());
		Settings.Secure.putString(cr, Settings.Secure.ETHERNET_MASK, info.getNetMask());
		if (mEthState == EthernetManager.ETHERNET_STATE_ENABLED) {
			mTracker.teardown();
			mTracker.reconnect();
			/*
			try {
				mNMService.setInterfaceUp(info.getIfName());
			} catch (RemoteException e) {
				Log.e(TAG, "setInterfaceUp is error: " + e);
			}
			*/
		}
		if ((info != null) && mDeviceMap.containsKey(info.getIfName())){
			mDeviceMap.put(info.getIfName(), info);
		}
	}


	/**
	 * get the number of ethernet interfaces in the system
	 * @return the number of ethernet interfaces
	 */
	public int getTotalInterface() {
		int total = 0;
		synchronized(mDeviceMap) {
			total = mDeviceMap.size();
		}
		return total;
	}


	private void scanDevice() {
		String[] Devices = null;
		try{
			Devices = mNMService.listInterfaces();
			for(String iface : Devices) {
				if(isEth(iface)){
					EthernetDevInfo value = new EthernetDevInfo();
					InterfaceConfiguration config = mNMService.getInterfaceConfig(iface);
					value.setIfName(iface);
					value.setHwaddr(config.getHardwareAddress());
					value.setIpAddress(config.getLinkAddress().getAddress().getHostAddress());
					synchronized(mDeviceMap){
						mDeviceMap.put(iface, value);
					}
				}
			}
		} catch (Exception e) {
            Log.e(TAG, "Could not get Config of interfaces " + e);
		}
	}
	
	private boolean isEth(String ifname) {
		if(ifname.startsWith("sit") || ifname.startsWith("lo") || ifname.startsWith("ppp")
				|| ifname.startsWith("ippp") || ifname.startsWith("tun")
				|| ifname.startsWith("gre") || ifname.startsWith("ip6"))
			return false;
		if(new File(SYS_NET + ifname + "/phy80211").exists())
			return false;
		if(new File(SYS_NET + ifname + "/wireless").exists())
			return false;
		if(new File(SYS_NET + ifname + "/wimax").exists())
			return false;
		if(new File(SYS_NET + ifname + "/bridge").exists())
			return false;

		return true;
	}

	private void handleReceive(Context context, Intent intent){
        String action = intent.getAction();
        if (EthernetManager.NETWORK_STATE_CHANGED_ACTION.equals(action)) {
			final LinkProperties linkProperties = (LinkProperties)
					intent.getParcelableExtra(EthernetManager.EXTRA_LINK_PROPERTIES);
			final int event = intent.getIntExtra(EthernetManager.EXTRA_ETHERNET_STATE,
												EthernetManager.EVENT_CONFIGURATION_SUCCEEDED);

			switch(event){
			case EthernetManager.EVENT_CONFIGURATION_SUCCEEDED:
			case EthernetManager.EVENT_CONFIGURATION_FAILED:
			case EthernetManager.EVENT_DISCONNECTED:
				break;
			default:
				break;
			}
        } 
	}

	/**
	 * get all the ethernet device names
	 * @return interface name list on success, {@code null} on failure
	 */
	public List<EthernetDevInfo> getDeviceNameList() {
		List<EthernetDevInfo> reDevs = new ArrayList<EthernetDevInfo>();

		synchronized(mDeviceMap){
			if(mDeviceMap.size() == 0)
				return null;
			for(EthernetDevInfo devinfo : mDeviceMap.values()){
				reDevs.add(devinfo);
			}
		}
		return reDevs;
	}

	private synchronized void persistEnabled(boolean enabled) {
		final ContentResolver cr = mContext.getContentResolver();
		Settings.Secure.putInt(cr, Settings.Secure.ETHERNET_ON,
				enabled ? EthernetManager.ETHERNET_STATE_ENABLED : EthernetManager.ETHERNET_STATE_DISABLED);
	}

	/**
	 * Enable or Disable a ethernet service
	 * @param enable {@code true} to enable, {@code false} to disable
	 */
	public synchronized void setState(int state) {

		if (mEthState != state) {
			mEthState = state;
			if (state == EthernetManager.ETHERNET_STATE_DISABLED) {
				persistEnabled(false);
				mTracker.teardown();
			} else if (state == EthernetManager.ETHERNET_STATE_ENABLED){
				persistEnabled(true);
				mTracker.reconnect();
			}
		}
	}

	/**
	 * Get ethernet service state
	 * @return the state of the ethernet service
	 */
	public int getState( ) {
		return mEthState = isOn() ? EthernetManager.ETHERNET_STATE_ENABLED
									: EthernetManager.ETHERNET_STATE_DISABLED;
	}

    private void sendChangedBroadcast(EthernetDevInfo info, int event) {
        Intent intent = new Intent(EthernetManager.ETHERNET_STATE_CHANGED_ACTION);
        intent.addFlags(Intent.FLAG_RECEIVER_REGISTERED_ONLY_BEFORE_BOOT
                | Intent.FLAG_RECEIVER_REPLACE_PENDING);
        intent.putExtra(EthernetManager.EXTRA_ETHERNET_INFO, info);
		intent.putExtra(EthernetManager.EXTRA_ETHERNET_STATE, event);
        mContext.sendStickyBroadcast(intent);
    }

	/**
	 * add the ethernet interface to Service Managing
	 * @param the name of ethernet interface
	 */
	public boolean addInterfaceToService(String iface) {
		if(!isEth(iface))
			return false;
		if(ETH_USED.isEmpty())
			ETH_USED = iface;
		if(!(new File(SYS_NET + iface + "/ifindex").exists()))
			return false;
		synchronized(mDeviceMap) {
			try{
				if(!mDeviceMap.containsKey(iface)){
					EthernetDevInfo value = new EthernetDevInfo();
					InterfaceConfiguration config = mNMService.getInterfaceConfig(iface);
					value.setIfName(iface);
					value.setHwaddr(config.getHardwareAddress());
					value.setIpAddress(config.getLinkAddress().getAddress().getHostAddress());
					mDeviceMap.put(iface, value);
					sendChangedBroadcast(value, EthernetManager.EVENT_NEWDEV);
				}
			} catch (RemoteException e) {
				Log.e(TAG, "Can't get the Interface Configure" + e);
			}
		}
		return true;
	}

	/**
	 * add the ethernet interface to Service Managing
	 * @param the name of ethernet interface
	 */
	public void  removeInterfaceFormService(String name) {
		if(!isEth(name))
			return ;
		synchronized(mDeviceMap) {
			if(mDeviceMap.containsKey(name)){
				sendChangedBroadcast(mDeviceMap.get(name), EthernetManager.EVENT_DEVREM);
				mDeviceMap.remove(name);
			}
		}
	}

	/**
	 * Checkout if the ethernet open
	 * @return the boolean of ethernet open 
	 */
	public synchronized boolean isOn() {
		final ContentResolver cr = mContext.getContentResolver();
		try{
			return Settings.Secure.getInt(cr, Settings.Secure.ETHERNET_ON) == 0 ? false : true;
		} catch (Settings.SettingNotFoundException e) {
			return false;
		}
	}

	/**
	 * Checkout if the ethernet dhcp
	 * @return the number of ethernet interfaces
	 */
	public synchronized boolean isDhcp() {
		final ContentResolver cr = mContext.getContentResolver();
		try{
			return Settings.Secure.getInt(cr, Settings.Secure.ETHERNET_MODE) == 0 ? false : true;
		} catch (Settings.SettingNotFoundException e) {
			return false;
		}
	}

	/**
	 * Checkout if the interface linkup or not.
	 */
	public int CheckLink(String ifname) {
		int ret = 0;
		File filefd = null;
		FileInputStream fstream = null;
		String s = null;
		try{
			if(!(new File(SYS_NET + ifname).exists()))
				return -1;
			fstream = new FileInputStream(SYS_NET + ifname + "/carrier");
			DataInputStream in = new DataInputStream(fstream);
			BufferedReader br = new BufferedReader(new InputStreamReader(in));

			s = br.readLine();
		} catch (IOException ex) {
		} finally {
			if (fstream != null) {
				try{
					fstream.close();
				} catch (IOException ex) {
				}
			}
		}
		if(s != null && s.equals("1"))
			ret = 1;
		return ret;
	}
}
