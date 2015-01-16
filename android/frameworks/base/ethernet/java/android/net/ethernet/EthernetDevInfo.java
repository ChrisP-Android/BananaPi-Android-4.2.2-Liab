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

//import android.net.ethernet.EthernetDevInfo;
import android.os.Parcel;
import android.os.Parcelable;
import android.os.Parcelable.Creator;

/**
 * Describes the state of any Ethernet connection that is active or
 * is in the process of being set up.
 */

public class EthernetDevInfo implements Parcelable {
    /**
     * The ethernet interface is configured by dhcp
     */
    public static final int ETHERNET_CONN_MODE_DHCP= 1;
    /**
     * The ethernet interface is configured manually
     */
    public static final int ETHERNET_CONN_MODE_MANUAL = 0;

    private String dev_name;
    private String ipaddr;
    private String netmask;
    private String gw;
    private String dns;
    private int mode;
	private String hwaddr;

    public EthernetDevInfo () {
        dev_name = null;
        ipaddr = null;
        dns = null;
        gw = null;
        netmask = null;
        mode = ETHERNET_CONN_MODE_DHCP;
		hwaddr = null;
    }

    /**
     * save interface name into the configuration
     */
    public void setIfName(String ifname) {
        this.dev_name = ifname;
    }

    /**
     * Returns the interface name from the saved configuration
     * @return interface name
     */
    public String getIfName() {
        return this.dev_name;
    }

    public void setIpAddress(String ip) {
        this.ipaddr = ip;
    }

    public String getIpAddress( ) {
        return this.ipaddr;
    }

    public void setNetMask(String ip) {
        this.netmask = ip;
    }

    public String getNetMask( ) {
        return this.netmask;
    }

    public void setGateWay(String gw) {
        this.gw = gw;
    }

    public String getGateWay() {
        return this.gw;
    }

    public void setDnsAddr(String dns) {
        this.dns = dns;
    }

    public String getDnsAddr( ) {
        return this.dns;
    }

    public void setHwaddr(String hwaddr) {
        this.hwaddr = hwaddr;
    }

    public String getHwaddr( ) {
        return this.hwaddr;
    }

    /**
     * Set ethernet configuration mode
     * @param mode {@code ETHERNET_CONN_MODE_DHCP} for dhcp {@code ETHERNET_CONN_MODE_MANUAL} for manual configure
     * @return
     */
    public void setConnectMode(int mode) {
            this.mode = mode;
    }

    public int getConnectMode() {
        return this.mode;
    }

    public int describeContents() {
        return 0;
    }

    public void writeToParcel(Parcel dest, int flags) {
        dest.writeString(this.dev_name);
        dest.writeString(this.ipaddr);
        dest.writeString(this.netmask);
        dest.writeString(this.gw);
        dest.writeString(this.dns);
        dest.writeInt(this.mode);
        dest.writeString(this.hwaddr);
    }

    /** Implement the Parcelable interface {@hide} */
    public static final Creator<EthernetDevInfo> CREATOR = new Creator<EthernetDevInfo>() {
        public EthernetDevInfo createFromParcel(Parcel in) {
            EthernetDevInfo info = new EthernetDevInfo();
            info.setIfName(in.readString());
            info.setIpAddress(in.readString());
            info.setNetMask(in.readString());
            info.setGateWay(in.readString());
            info.setDnsAddr(in.readString());
            info.setConnectMode(in.readInt());
            info.setHwaddr(in.readString());
            return info;
        }

        public EthernetDevInfo[] newArray(int size) {
            return new EthernetDevInfo[size];
        }
    };
}
