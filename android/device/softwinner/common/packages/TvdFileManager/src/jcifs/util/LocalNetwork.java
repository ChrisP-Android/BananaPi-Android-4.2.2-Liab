package jcifs.util;

import java.net.Inet4Address;
import java.net.UnknownHostException;
import java.util.ArrayList;

import jcifs.Config;

import android.util.Log;


public class LocalNetwork{
	public static final String IPV4 = "jcifs.ipv4";
	public static final String NET_MASK = "jcifs.netmask";
	public static final String B_ADDR = "jcifs.netbios.baddr";
	public static final int REQ_FAIL = -1;
	public static final int IP_TYPE_A = 0x00;
	public static final int IP_TYPE_B = 0x01;
	public static final int IP_TYPE_C = 0x02;
	public static final int IP_TYPE_D = 0x03;
	public static final int IP_TYPE_E = 0x04;
	public static final int IP_TYPE_UNKNOWN = 0x05;
	
	private static String curIPV4 = null;
	private static String curNetMask = null;
	private static String curBroadcast = null;
	
	public static int getCurIpv4(){
		String hostIp = curIPV4;
		Log.d("Samba", "hostip is " + hostIp);
		if(hostIp == null){
			return REQ_FAIL;
		}
		Inet4Address addr;
		try {
			addr = (Inet4Address) Inet4Address.getByName(hostIp);
			return byteToInt(addr.getAddress());
		} catch (UnknownHostException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
			return REQ_FAIL;
		}
	}
	
	public static String getSCurIpv4(){
		return curIPV4;
	}
	
	public static void setCurIpv4(String ipv4){
		//System.setProperty(IPV4, ipv4);
		curIPV4 = ipv4;
		int type = getIpType(ipv4);
		String mask = null;
		String broadcast = null;
		try{
			ArrayList<String> array = split(ipv4);
			
			switch(type){
			case IP_TYPE_A:
				mask = "255.0.0.0";
				broadcast = array.get(0) + ".255.255.255";
				break;
			case IP_TYPE_B:
				mask = "255.255.0.0";
				broadcast = array.get(0) + "." + array.get(1) + ".255.255";
				break;
			case IP_TYPE_C:
				mask = "255.255.255.0";
				broadcast = array.get(0) + "." + array.get(1) + "." + array.get(2) + ".255";
				break;
			case IP_TYPE_D:
			case IP_TYPE_E:
			case IP_TYPE_UNKNOWN:
				//do not use
				mask = "255.255.255.255";
				broadcast = "255.255.255.255";
				break;
			}
		}catch (Exception e) {
			Log.d("Samba", "setCurIpv4() exception");
			mask = "255.255.255.255";
			broadcast = "255.255.255.255";
		}
		Log.d("Samba", String.format("Set ip=%s, mask=%s, broadcast=%s", ipv4, mask, broadcast));
		curNetMask = mask;
		curBroadcast = broadcast;
	}
	
	public static int getCurNetMask(){
		String mask = curNetMask;
		Log.d("Samba", "netmask is " + mask);
		if(mask == null){
			return REQ_FAIL;
		}
		Inet4Address addr;
		try {
			addr = (Inet4Address) Inet4Address.getByName(mask);
			return byteToInt(addr.getAddress());
		} catch (UnknownHostException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
			return REQ_FAIL;
		}
	}
	
	public static String getCurBAddr(){
		return curBroadcast;
	}
	
	public static int byteToInt(byte[] b){
		int mask = 0xff;
		int tmp = 0;
		int n = 0;
		for(int i = 0; i < b.length; i++){
			n <<= 8;
			tmp = b[i] & mask;
			n |= tmp;
		}
		return n;
	}
	
	public static boolean sameNetSegment(int localIp, int compare, int netMask){
		if((localIp & netMask) == (compare & netMask)){
			return true;
		}
		return false;
	}
	
	public static int getIpType(String ipv4){
		int type = IP_TYPE_UNKNOWN;
		try{
			String ary0 = ipv4.substring(0, ipv4.indexOf("."));
			Integer itg = Integer.valueOf(ary0);
			int i = itg.intValue();
			if(i < 128 && i > 0){
				type = IP_TYPE_A;
			}else if(i < 192){
				type = IP_TYPE_B;
			}else if(i < 224){
				type = IP_TYPE_C;
			}else if(i < 240){
				type = IP_TYPE_D;
			}else if(i < 256){
				type = IP_TYPE_E;
			}else{
				type = IP_TYPE_UNKNOWN;
			}
		}catch (Exception e) {
			type = IP_TYPE_UNKNOWN;
		}
		return type;
	}
	
	private static ArrayList<String> split(String ip){
		//fix me:the String.split() method does not work for ip string
		ArrayList<String> list = new ArrayList<String>();
		while(ip.indexOf(".") >= 0){
			list.add(ip.substring(0, ip.indexOf(".")));
			ip = ip.substring(ip.indexOf(".") + 1);
			if(ip.indexOf(".") < 0){
				list.add(ip);
			}
		}
		return list;
	}
}