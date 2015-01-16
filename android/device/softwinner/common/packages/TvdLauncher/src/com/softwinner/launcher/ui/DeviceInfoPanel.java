package com.softwinner.launcher.ui;

import com.softwinner.launcher.R;

import android.content.Context;
import android.util.AttributeSet;
import android.util.Log;
import android.widget.LinearLayout;
import android.widget.TextView;

public class DeviceInfoPanel extends LinearLayout{
	private static final String TAG = "DeviceInfoPanel";

    private TextView mWlanTv;
    private TextView mDeviceTv;
    private TextView mWlanPasswdTv;
    private final String deviceTitle;
    private final String wlanTitle;
    private String wlanPasswdTitle;
    private final static String WLNA_PASSWD = "12345678";
	
	public DeviceInfoPanel(Context context) {
		super(context);
		deviceTitle = context.getString(R.string.device_title);
		wlanTitle = context.getString(R.string.wlan_title);
	}
	
	public DeviceInfoPanel(Context context, AttributeSet attrs) {
		super(context, attrs);
		deviceTitle = context.getString(R.string.device_title);
		wlanTitle = context.getString(R.string.wlan_title);
	}
	
	public DeviceInfoPanel(Context context, AttributeSet attrs, int defStyle) {
		super(context, attrs, defStyle);
		deviceTitle = context.getString(R.string.device_title);
		wlanTitle = context.getString(R.string.wlan_title);
	}

	@Override
	protected void onFinishInflate() {
		super.onFinishInflate();		
		mDeviceTv = (TextView) findViewById(R.id.device_name_tv);
        mWlanTv = (TextView) findViewById(R.id.wlan_tv);
        mWlanPasswdTv = (TextView)findViewById(R.id.wlan_passwd_tv);
        setWlanPasswd(WLNA_PASSWD);
	}

	public void setDeviceName(String device){
		String nameStr = deviceTitle + (device == null? "" : device);
		mDeviceTv.setText(nameStr);
	}
	
	public void setWlanName(String wlan){
		String nameStr = wlanTitle + (wlan == null? getContext().getString(R.string.wlan_none) : wlan);
		mWlanTv.setText(nameStr);
	}
	
	public void setWlanPasswd(String passwd) {
		String passwdStr = getContext().getString(R.string.wlan_passwd_title) + passwd;
		mWlanPasswdTv.setText(passwdStr);
	}
}
