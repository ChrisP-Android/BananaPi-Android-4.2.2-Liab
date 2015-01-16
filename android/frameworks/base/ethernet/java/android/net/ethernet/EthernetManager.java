/*
 * Copyright (C) 2010 The Android-X86 Open Source Project
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

package android.net.ethernet;

import android.os.Handler;
import android.os.IBinder;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.util.Slog;
import android.content.Context;
import java.util.Iterator;
import java.util.List;

/**
 * This class provides the primary API for managing all aspects of Ethernet
 * connectivity. Get an instance of this class by calling
 * {@link android.content.Context#getSystemService(String) Context.getSystemService(Context.ETHERNET_SERVICE)}.
 *
 * This is the API to use when performing Ethernet specific operations. To
 * perform operations that pertain to network connectivity at an abstract
 * level, use {@link android.net.ConnectivityManager}.
 */
public class EthernetManager {
    public static final String TAG = "EthernetManager";
    public static final int ETHERNET_DEVICE_SCAN_RESULT_READY = 0;
    public static final String ETHERNET_STATE_CHANGED_ACTION =
							"android.net.ethernet.ETHERNET_STATE_CHANGED";
    public static final String NETWORK_STATE_CHANGED_ACTION =
							"android.net.ethernet.STATE_CHANGE";
    /**@hide*/
	public static final String ETHERNET_LINKED_ACTION = 
		                    "android.net.ethernet.LINKED_ACTION";
	/**@hide*/
	public static final String ETHERNET_DISLINKED_ACTION = 
		                    "android.net.ethernet.DISLINKED_ACTION";
    
    //add by celema ///////////////////////////////////////////////////
	/**@hide*/
    public static final String PPPOE_STATE_CHANGED_ACTION = 
                            "android.net.pppoe.STATE_CHANGE";
    /**@hide*/
    public static final String EXTRA_PPPOE_STATE = "pppoe_state";
    /**@hide*/
    public static final int PPPOE_STATE_ENABLED = 1;
    /**@hide*/
    public static final int PPPOE_STATE_DISABLED = 0;
    ////////////////////////////////////////////////////////////////////////

    public static final String EXTRA_ETHERNET_INFO		= "ethernetInfo";
    public static final String EXTRA_NETWORK_INFO		= "networkInfo";
    public static final String EXTRA_ETHERNET_STATE		= "ethernet_state";
    public static final String EXTRA_LINK_PROPERTIES	= "linkProperties";

    public static final int ETHERNET_STATE_DISABLED = 0;
    public static final int ETHERNET_STATE_ENABLED = 1;

	public static final int EVENT_DHCP_START                        = 0;
	public static final int EVENT_CONFIGURATION_SUCCEEDED			= 1;
	public static final int EVENT_CONFIGURATION_FAILED				= 2;
	public static final int EVENT_NEWDEV							= 3;
	public static final int EVENT_DISCONNECTED                      = 5;
	//public static final int EVENT_PHYCONNECTED                    = 6;
	//public static final int EVENT_DISPHYCONNECTED					= 7;
	public static final int EVENT_DEVREM							= 8;
	public static final int NOTIFY_ID								= 9;

    IEthernetManager mService = null;
	private static EthernetManager mEth = null;

	public static EthernetManager getInstance() {
		if (mEth == null)
			mEth = new EthernetManager();

		return mEth;
	}

    private EthernetManager() {
		IBinder b = ServiceManager.getService(Context.ETHERNET_SERVICE);

		if (mService == null)
			mService = IEthernetManager.Stub.asInterface(b);
    }

    /**
     * check if the ethernet service has been configured.
     * @return {@code true} if configured {@code false} otherwise
     */
    public boolean isConfigured() {
        try {
            return mService.isConfigured();
        } catch (RemoteException e) {
            Slog.i(TAG, "Can not check eth config state");
        }
        return false;
    }

    /**
     * Return the saved ethernet configuration
     * @return ethernet interface configuration on success, {@code null} on failure
     */
    public EthernetDevInfo getSavedConfig() {
        try {
            return mService.getSavedConfig();
        } catch (RemoteException e) {
            Slog.i(TAG, "Can not get eth config");
        }
        return null;
    }

    /**
     * update a ethernet interface information
     * @param info  the interface infomation
     */
    public void updateDevInfo(EthernetDevInfo info) {
        try {
            mService.updateDevInfo(info);
        } catch (RemoteException e) {
            Slog.i(TAG, "Can not update ethernet device info");
        }
    }

    /**
     * get all the ethernet device names
     * @return interface name list on success, {@code null} on failure
     */
    public List<EthernetDevInfo> getDeviceNameList() {
        try {
            return mService.getDeviceNameList();
        } catch (RemoteException e) {
            return null;
        }
    }

    /**
     * Enable or Disable a ethernet service
     * @param enable {@code true} to enable, {@code false} to disable
     * @hide
     */
    public void setEnabled(boolean enable) {
        try {
            mService.setState(enable ? ETHERNET_STATE_ENABLED:ETHERNET_STATE_DISABLED);
        } catch (RemoteException e) {
            Slog.i(TAG,"Can not set new state");
        }
    }

    /**
     * Get ethernet service state
     * @return the state of the ethernet service
     */
    public int getState( ) {
        try {
            return mService.getState();
        } catch (RemoteException e) {
            return 0;
        }
    }

    /**
     * get the number of ethernet interfaces in the system
     * @return the number of ethernet interfaces
     */
    public int getTotalInterface() {
        try {
            return mService.getTotalInterface();
        } catch (RemoteException e) {
            return 0;
        }
    }

	public boolean isOn() {
		try{
			return mService.isOn();
		} catch (RemoteException e) {
			return false;
		}
	}

	public boolean isDhcp() {
		try{
			return mService.isDhcp();
		} catch (RemoteException e) {
			return false;
		}
	}

	public void removeInterfaceFormService(String name) {
		try{
			mService.removeInterfaceFormService(name);
		} catch (RemoteException e) {
		}
	}

	public boolean addInterfaceToService(String name) {
		try{
			return mService.addInterfaceToService(name);
		} catch (RemoteException e) {
			return false;
		}
	}

    /**
     * @hide
     */
    public void setDefaultConf() {
        try {
            mService.setMode(EthernetDevInfo.ETHERNET_CONN_MODE_DHCP);
        } catch (RemoteException e) {
        }
    }

    public int CheckLink(String ifname) {
        try {
            return mService.CheckLink(ifname);
        } catch (RemoteException e) {
			return 0;
        }
    }
}
