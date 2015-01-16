package com.softwinner.launcher.ui;

import com.softwinner.launcher.R;

import android.content.Context;
import android.content.Intent;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.net.wifi.WifiManager;
import android.util.AttributeSet;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.ImageView;
import android.widget.LinearLayout;

import java.util.ArrayList;

public class StatusbarRight extends LinearLayout{
    
    private ArrayList<View> mViews = new ArrayList<View>();
   
    private final int LEFT_BACKGROUND_ID = R.drawable.statusbar_l;
    private final int MIDDLE_BACKGROUND_ID = R.drawable.statusbar_m;
    private final int RIGHT_BACKGROUND_ID = R.drawable.statusbar_r;
    
    private final int [] WIFI_SIGNAL_ID = {
            R.drawable.wifi_signal_1,
            R.drawable.wifi_signal_2,
            R.drawable.wifi_signal_3,
            R.drawable.wifi_signal_4,
            R.drawable.wifi_signal_5,            
        };
    
    private final int [] SETTINGS_ID = {
            R.drawable.ssettings,
            R.drawable.settings,
        };
    
    private final String WIFI_TAG = "wifi_icon";
    private final String MID_VIEW_TAG = "_mid";
    
    private LayoutInflater mInflater;
    private ImageView mLeftView ; 
    private ImageView mWifiSignal;
    private ImageView mSettings;
    
    private int mLastWifiSignalLevel;
    private boolean mIsWifiConnected;

    public StatusbarRight(Context context, AttributeSet attrs) {
        super(context, attrs);
        mInflater = LayoutInflater.from(context);
        mLeftView = new ImageView(context, attrs);
        mLeftView.setImageResource(LEFT_BACKGROUND_ID);
        View clock = mInflater.inflate(R.layout.digital_clock,null);
        addViewForStatusbar(clock,null);
        mSettings = new ImageView(context, attrs);
        mSettings.setImageResource(SETTINGS_ID[0]);
        //addViewForStatusbar(mSettings,null);
        mWifiSignal = new ImageView(context,attrs);
        mWifiSignal.setImageResource(WIFI_SIGNAL_ID[0]);
        addViewForStatusbar(mWifiSignal,WIFI_TAG);
        setTagViewVisible(WIFI_TAG,false);
        setOrientation(HORIZONTAL);
    }
    
    private void addViewForLayout(){
        detachAllViewsFromParent();
        for(View childs:mViews){
            addView(childs);
        }
        invalidate();
    }
    
    private View createMiddleView(){
        ImageView v = new ImageView(mContext);
        v.setImageResource(MIDDLE_BACKGROUND_ID);
        return v;
    }
    
    public void addViewForStatusbar(View child,Object TAG){
        if(mViews.contains(child)){
            return;
        }
        if(mViews.size() == 0){
            mViews.add(mLeftView);
        }
        if(mViews.size() == 1){
            mViews.add(child);
            child.setBackgroundResource(RIGHT_BACKGROUND_ID);
        }else{
            mViews.add(1, child);
            child.setBackgroundResource(RIGHT_BACKGROUND_ID);
            View v = createMiddleView();
            if(TAG != null ){
                child.setTag(TAG);
                v.setTag(TAG + MID_VIEW_TAG);
            }
            mViews.add(2,v);
        }
        addViewForLayout();
    }
    public void setTagViewVisible(Object o , boolean isVisible){
        View v = findViewWithTag(o);
        View vm = findViewWithTag(o + MID_VIEW_TAG);
        if(v==null || vm==null){
            return; 
        }
        if(!isVisible){
            v.setVisibility(GONE);
            vm.setVisibility(GONE);
        }else{
            v.setVisibility(VISIBLE);
            vm.setVisibility(VISIBLE);
        }
    }
    public void updateWifi(Intent intent){
        final String action = intent.getAction();
        Log.d("status","0");
        if(action.equals(ConnectivityManager.CONNECTIVITY_ACTION) ||
                action.equals(ConnectivityManager.INET_CONDITION_ACTION)){
            NetworkInfo info = (NetworkInfo)(intent.getParcelableExtra(
                    ConnectivityManager.EXTRA_NETWORK_INFO));
            Log.d("status","t"+info.getType());
            switch(info.getType()){
                case ConnectivityManager.TYPE_WIFI:
                    if (info.isConnected()) {
                        mIsWifiConnected = true;
                        int iconId;
                        if (mLastWifiSignalLevel == -1) {
                            iconId = WIFI_SIGNAL_ID[0];
                        } else {
                            iconId = WIFI_SIGNAL_ID[mLastWifiSignalLevel];
                        }
                        mWifiSignal.setImageResource(iconId);
                        // Show the icon since wi-fi is connected
                        Log.d("status","1");
                        setTagViewVisible(WIFI_TAG,true);
                    } else {
                        mLastWifiSignalLevel = -1;
                        mIsWifiConnected = false;
                        int iconId = WIFI_SIGNAL_ID[0];
                        Log.d("status","2");
                        mWifiSignal.setImageResource(iconId);
                        setTagViewVisible(WIFI_TAG,false);
                    }
                break;                    
            }
        }
        if (action.equals(WifiManager.WIFI_STATE_CHANGED_ACTION)) {

            final boolean enabled = intent.getIntExtra(WifiManager.EXTRA_WIFI_STATE,
                    WifiManager.WIFI_STATE_UNKNOWN) == WifiManager.WIFI_STATE_ENABLED;

            if (!enabled) {
                Log.d("status","3");
                setTagViewVisible(WIFI_TAG,false);
            }

        } else if (action.equals(WifiManager.SUPPLICANT_CONNECTION_CHANGE_ACTION)) {
            final boolean enabled = intent.getBooleanExtra(WifiManager.EXTRA_SUPPLICANT_CONNECTED,
                                                           false);
            if (!enabled) {
                Log.d("status","4");
                setTagViewVisible(WIFI_TAG,false);
            }
        } else if (action.equals(WifiManager.RSSI_CHANGED_ACTION)) {
            int iconId;
            final int newRssi = intent.getIntExtra(WifiManager.EXTRA_NEW_RSSI, -200);
            int newSignalLevel = WifiManager.calculateSignalLevel(newRssi,WIFI_SIGNAL_ID.length);
            if (newSignalLevel != mLastWifiSignalLevel) {
                mLastWifiSignalLevel = newSignalLevel;
                if (mIsWifiConnected) {
                    iconId = WIFI_SIGNAL_ID[newSignalLevel];
                }else{
                    iconId = WIFI_SIGNAL_ID[0];
                }
                Log.d("status","5");
                mWifiSignal.setImageResource(iconId);
            }
        }
    }
}
