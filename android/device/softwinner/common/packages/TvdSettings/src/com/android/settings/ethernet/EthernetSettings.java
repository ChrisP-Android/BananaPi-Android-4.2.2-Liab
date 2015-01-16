/*
 * Copyright (C) 2011 The Android Open Source Project
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

package com.android.settings.ethernet;

import static android.net.ethernet.EthernetManager.ETHERNET_STATE_DISABLED;
import static android.net.ethernet.EthernetManager.ETHERNET_STATE_ENABLED;

import com.android.settings.R;

import android.app.Dialog;
import android.app.AlertDialog;
import android.app.Activity;
import android.content.Context;
import android.content.DialogInterface;
import android.content.BroadcastReceiver;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.net.NetworkInfo.DetailedState;
import android.net.LinkProperties;
import android.net.LinkAddress;
import android.net.ethernet.IEthernetManager;
import android.net.ethernet.EthernetManager;
import android.net.ethernet.EthernetDevInfo;
import android.net.InterfaceConfiguration;
import android.net.wifi.WifiManager;
import android.os.INetworkManagementService;
import android.net.RouteInfo;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.os.ServiceManager;
import android.preference.Preference;
import android.preference.CheckBoxPreference;
import android.preference.PreferenceGroup;
import android.preference.PreferenceCategory;
import android.preference.PreferenceScreen;
import android.util.Log;
import android.util.Slog;
import android.view.ContextMenu;
import android.view.ContextMenu.ContextMenuInfo;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.AdapterView.AdapterContextMenuInfo;

import com.android.settings.SettingsPreferenceFragment;

import java.net.Inet4Address;
import java.nio.charset.Charsets;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.ArrayList;
import android.os.AsyncTask;

public class EthernetSettings extends SettingsPreferenceFragment implements
        Preference.OnPreferenceClickListener, Preference.OnPreferenceChangeListener,
        DialogInterface.OnClickListener, DialogInterface.OnDismissListener {

    private static final String TAG = "EthernetSettings";

    private ConnectivityManager mService;

	//*******************************************************//
    private static final String KEY_TOGGLE = "eth_toggle";
    private static final String KEY_DEVICES_TITLE = "eth_device_title";
    private static final String KEY_ETH_CONFIG = "eth_configure";
    private static final String KEY_IP_PRE = "current_ip_address";
    private static final String KEY_MAC_PRE = "mac_address";

	private CheckBoxPreference mEthEnable;
	private PreferenceCategory mEthDevices;
    private Preference mMacPreference;
    private Preference mIpPreference;
    private Preference mEthConfigure;
    private EthPreference mSelected;
	private EthernetManager mEthManager;
    private final IntentFilter mFilter;
    private final BroadcastReceiver mEthStateReceiver;
	private List<EthernetDevInfo> mListDevices = new ArrayList<EthernetDevInfo>();
	private EthernetDialog mDialog;
	private boolean isClicked = false;

    /*  New a BroadcastReceiver for EthernetSettings  */
	public EthernetSettings() {
		mFilter = new IntentFilter();
        mFilter.addAction(EthernetManager.ETHERNET_STATE_CHANGED_ACTION);
        mFilter.addAction(EthernetManager.NETWORK_STATE_CHANGED_ACTION);

		mEthStateReceiver = new BroadcastReceiver() {
			@Override
			public void onReceive(Context context, Intent intent) {
				handleEvent(context, intent);
			}
		};
	}

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        addPreferencesFromResource(R.xml.ethernet_settings);
		mEthEnable = (CheckBoxPreference) findPreference(KEY_TOGGLE);
		mEthEnable.setOnPreferenceChangeListener(this);
        mEthDevices = (PreferenceCategory) findPreference(KEY_DEVICES_TITLE);
        mEthDevices.setOrderingAsAdded(false);
        mEthConfigure = findPreference(KEY_ETH_CONFIG);
		mIpPreference = findPreference(KEY_IP_PRE);
		mMacPreference= findPreference(KEY_MAC_PRE);

		/**  first, we must get the Service.  **/
		mService = (ConnectivityManager) getSystemService(Context.CONNECTIVITY_SERVICE);
		mEthManager = EthernetManager.getInstance();

		/**  Now, it should check the EthernetState.  **/
		if(mEthManager.getState() == EthernetManager.ETHERNET_STATE_ENABLED){
			mEthEnable.setChecked(true);
		}else{
			mEthEnable.setChecked(false);
		}
		if(mService != null){
			NetworkInfo networkinfo = mService.getNetworkInfo(ConnectivityManager.TYPE_ETHERNET);
			if(networkinfo.isConnected()){
				mEthEnable.setSummaryOn(getActivity().getString(R.string.eth_dev_summaryon));
			}else{
				mEthEnable.setSummaryOff(getActivity().getString(R.string.eth_dev_summaryoff));
			}
		}
		
		/**  Get the SaveConfig and update for Dialog.  **/
		EthernetDevInfo saveInfo = mEthManager.getSavedConfig();
		if(saveInfo != null){
			upDeviceList(saveInfo);
		}else{
		    upDeviceList(null);
		}
    }

    @Override
    public void onSaveInstanceState(Bundle savedState) {

    }

    @Override
    public void onStop() {
		super.onStop();
	}

    @Override
    public void onResume() {
        super.onResume();
        getActivity().registerReceiver(mEthStateReceiver, mFilter);
    }

    @Override
    public void onPause() {
        getActivity().unregisterReceiver(mEthStateReceiver);
        super.onPause();
    }

    @Override
    public void onDismiss(DialogInterface dialog) {
        // Here is the exit of a dialog.
        //mDialog = null;
    }

    @Override
    public void onClick(DialogInterface dialog, int whichbutton) {
		/**  get the information form dialog.  **/
		final EthernetDevInfo devIfo = mDialog.getDevInfo();
        if (whichbutton == DialogInterface.BUTTON_POSITIVE) {

			new AsyncTask<Void, Void, Void>(){
				protected void onPreExecute(){
					//Disable button
					mEthEnable.setEnabled(false);
					mEthConfigure.setEnabled(false);
					mEthDevices.setEnabled(false);
				}

				@Override
				protected Void doInBackground(Void... unused){
					try{
						mEthManager.updateDevInfo(devIfo);
						Thread.sleep(500);
					}catch(Exception e){
					}
					return null;
				}

				protected void onProgressUpdate(Void... unused){
				}

				protected void onPostExecute(Void unused) {
					EthPreference uppref = (EthPreference) mEthDevices.findPreference(devIfo.getIfName());
					if(uppref != null)
						uppref.update(devIfo);
					mEthConfigure.setEnabled(true);
					mEthEnable.setEnabled(true);
					mEthDevices.setEnabled(true);
				}
			}.execute();

        }else if(whichbutton == DialogInterface.BUTTON_NEUTRAL){
			mDialog.cancel();
			mDialog = new EthernetDialog(getActivity(), this, devIfo, true);
			mDialog.setOnDismissListener(this);
			mDialog.show();
		}
    }

    @Override
    public void onCreateContextMenu(ContextMenu menu, View view, ContextMenuInfo info) {

    }

    @Override
    public boolean onContextItemSelected(MenuItem item) {
        return false;
    }

    @Override
    public boolean onPreferenceClick(Preference preference) {
        if (preference instanceof EthPreference) {
			mDialog = new EthernetDialog(getActivity(), this, 
							((EthPreference)preference).getConfigure(), false);
			mDialog.setOnDismissListener(this);
			mDialog.show();
		}
        return true;
    }

	@Override
	public boolean onPreferenceChange(Preference preference, Object newValue){

		isClicked = true;
		setEthEnabled((Boolean)newValue);

		return false;
	}

	private void setEthEnabled(final boolean enable){

		new AsyncTask<Void, Void, Void>(){

			protected void onPreExecute(){
				//Disable button
				//mEthEnable.setSummary(R.string.eth_toggle_summary_opening);
				mEthEnable.setEnabled(false);
				mEthConfigure.setEnabled(false);
			}

			@Override
			protected Void doInBackground(Void... unused){
				try{
					if ((mEthManager.isConfigured() != true) && (enable == true)){
						publishProgress();
					}else{
						mEthManager.setEnabled(enable);
					}
					Thread.sleep(500);
				}catch(Exception e){
				}
				return null;
			}

			protected void onProgressUpdate(Void... unused){
				Preference tmpPre = mEthDevices.getPreference(0);
				if( tmpPre instanceof EthPreference){
					EthPreference tmpEthPre = (EthPreference)tmpPre;
					mEthManager.updateDevInfo(tmpEthPre.getConfigure());
					mEthManager.setEnabled(enable);
				}
			}

			protected void onPostExecute(Void unused) {
				mEthConfigure.setEnabled(true);
				mEthEnable.setChecked(enable);
				// Enable button
				mEthEnable.setEnabled(true);
			}
		}.execute();
	}

    @Override
    public boolean onPreferenceTreeClick(PreferenceScreen screen, Preference preference) {
        if (preference == mEthConfigure && mSelected != null) {
			mDialog = new EthernetDialog(getActivity(), this, mSelected.getConfigure(), true);
			mDialog.setOnDismissListener(this);
			mDialog.show();
		}
        return true;
    }

	private void upDeviceList(EthernetDevInfo DevIfo){
		String ifname = "";
		EthPreference upEthdevice = null;
		EthPreference tmpPreference = null;
		
		if(DevIfo != null)
			ifname = DevIfo.getIfName();

		if(mEthDevices != null)
			mEthDevices.removeAll();

		mListDevices = mEthManager.getDeviceNameList();
		if(mListDevices != null){
			for(EthernetDevInfo deviceinfo : mListDevices){
				if(!deviceinfo.getIfName().equals(ifname)){
					tmpPreference = new EthPreference(getActivity(), deviceinfo);
					mEthDevices.addPreference(tmpPreference);
					mSelected = tmpPreference;
				}else{
					DevIfo.setHwaddr(deviceinfo.getHwaddr());
					upEthdevice = new EthPreference(getActivity(), DevIfo);
				}
			}
			if(upEthdevice != null){
				mEthDevices.addPreference(upEthdevice);
				mSelected = upEthdevice;
			}
			if(mSelected != null)
				mMacPreference.setSummary(mSelected.getConfigure().getHwaddr().toUpperCase());
		}else{
			Preference tmpPre = new Preference(getActivity());
			tmpPre.setTitle(getActivity().getString(R.string.eth_dev_more));
			tmpPre.setEnabled(false);
			mEthDevices.addPreference(tmpPre);
			mMacPreference.setSummary("00:00:00:00:00:00");
			mIpPreference.setSummary("0.0.0.0");
			mSelected = null;
		}
	}

    private void handleEvent(Context context, Intent intent) {

        String action = intent.getAction();
        if (EthernetManager.ETHERNET_STATE_CHANGED_ACTION.equals(action)) {
			final EthernetDevInfo devinfo = (EthernetDevInfo)
								intent.getParcelableExtra(EthernetManager.EXTRA_ETHERNET_INFO);
			final int event = intent.getIntExtra(EthernetManager.EXTRA_ETHERNET_STATE,
																	EthernetManager.EVENT_NEWDEV);

			if(event == EthernetManager.EVENT_NEWDEV || event == EthernetManager.EVENT_DEVREM){
				if(mSelected != null){
					upDeviceList(mSelected.getConfigure());
				}else{
					upDeviceList(null);
				}
			}
        } else if (EthernetManager.NETWORK_STATE_CHANGED_ACTION.equals(action)) {
            final NetworkInfo networkInfo = (NetworkInfo)
                    intent.getParcelableExtra(EthernetManager.EXTRA_NETWORK_INFO);
			final LinkProperties linkProperties = (LinkProperties)
					intent.getParcelableExtra(EthernetManager.EXTRA_LINK_PROPERTIES);
			final int event = intent.getIntExtra(EthernetManager.EXTRA_ETHERNET_STATE,
												EthernetManager.EVENT_CONFIGURATION_SUCCEEDED);

			switch(event){
			case EthernetManager.EVENT_CONFIGURATION_SUCCEEDED:
				/*
				 * Every time open Eth Setting Activity, if eth connected, this
				 * will receive EVENT_CONFIGURATION_SUCCEEDED, there only handle
				 * Event result from user manual enable eth. 
				 * 
				 */
				if (isClicked) {
					/* 
					 * if eth state switch from close to  open && connected success,
					 * then close wifi 
					 */
					WifiManager wm = (WifiManager)getSystemService(Context.WIFI_SERVICE);
					wm.setWifiEnabled(false);
					isClicked = false;
				}

				for(LinkAddress l : linkProperties.getLinkAddresses()){
					mIpPreference.setSummary(l.getAddress().getHostAddress());
				}
				EthernetDevInfo saveInfo = mEthManager.getSavedConfig();
				if((mSelected != null) && (saveInfo != null)){
					upDeviceList(saveInfo);
					mEthEnable.setSummaryOn(context.getString(R.string.eth_dev_summaryon)
							+ mSelected.getConfigure().getIfName());
				}
				break;
			case EthernetManager.EVENT_CONFIGURATION_FAILED:
				mIpPreference.setSummary("0.0.0.0");
				break;
			case EthernetManager.EVENT_DISCONNECTED:
				if(mEthEnable.isChecked())
					mEthEnable.setSummaryOn(context.getString(R.string.eth_dev_summaryoff));
				else
					mEthEnable.setSummaryOn(context.getString(R.string.eth_dev_summaryoff));
				break;
			default:
				break;
			}
        } 
    }

	/*********************************************************/
	private class EthPreference extends Preference {
		private EthernetDevInfo mEthConf;
		private int mState = -1;

		EthPreference(Context context, EthernetDevInfo ethConfigure) {
			super(context);
			setPersistent(false);
			setOrder(0);
			setOnPreferenceClickListener(EthernetSettings.this);

			mEthConf = ethConfigure;
			update();
		}

		public EthernetDevInfo getConfigure() {
			return mEthConf;
		}

		public void update() {
			Context context = getContext();
			String mode = null;
			String hwaddr = null;

			if(mEthConf == null)
				return;

			setTitle(mEthConf.getIfName());
			if(mEthConf.getConnectMode() == EthernetDevInfo.ETHERNET_CONN_MODE_DHCP){
				mode = context.getString(R.string.eth_dhcp_mode);
			}else{
				mode = context.getString(R.string.eth_manual_mode);
			}
			hwaddr = mEthConf.getHwaddr().toUpperCase();
			setSummary("MAC: " + hwaddr + " -- " 
						+ context.getString(R.string.eth_ip_mode) + mode);
			setKey(mEthConf.getIfName());
		}

		public void update(EthernetDevInfo info){
			mEthConf = info;
			update();
		}

		@Override
		protected void onBindView(View view) {
			super.onBindView(view);
		}

		@Override
		public int compareTo(Preference preference) {
			int result = -1;
			if (preference instanceof EthPreference) {
				EthPreference another = (EthPreference) preference;
				EthernetDevInfo otherInfo = another.getConfigure();
				if (mEthConf.getIfName() == otherInfo.getIfName())
					result = 0;
				if (mEthConf.getHwaddr() == otherInfo.getHwaddr())
					result = 0;
			}
			return result;
		}
	}
}
