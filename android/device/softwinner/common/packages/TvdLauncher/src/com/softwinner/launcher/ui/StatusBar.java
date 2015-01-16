package com.softwinner.launcher.ui;

import com.softwinner.launcher.IStatusBar;
import com.softwinner.launcher.R;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.ConnectivityManager;
import android.net.wifi.WifiManager;
import android.util.AttributeSet;
import android.view.View;
import android.widget.ImageView;
import android.widget.RelativeLayout;


public class StatusBar extends RelativeLayout implements IStatusBar{   
        
    private ImageView mHome;
    private StatusbarRight mStatusbarRight;
    private final int [] HOME_ID = {
            R.drawable.shome,
            R.drawable.home,
    };
    
    private BroadcastReceiver mBr = new BroadcastReceiver(){
        @Override
        public void onReceive(Context context, Intent intent) {
            processReceiver(intent);
        }        
    };

    public StatusBar(Context context) {
        super(context);

    }
    
    public StatusBar(Context context, AttributeSet attrs) {
        super(context, attrs);        
    }
    
    public void Zoomed(boolean sel){
        if(sel){
            mHome.setImageResource(HOME_ID[1]);
        }else{
            mHome.setImageResource(HOME_ID[0]);
        }
    }

    @Override
    protected void onFinishInflate() {
        super.onFinishInflate();
        mHome = (ImageView)findViewById(R.id.home);
        mHome.setImageResource(HOME_ID[0]);
        mHome.setOnClickListener(new OnClickListener() {            
            @Override
            public void onClick(View arg0) {
                Intent intent = new Intent();
                intent.setClassName("com.softwinner.launcher", "com.softwinner.launcher.Launcher");
                mContext.startActivity(intent);
            }
        });
        mStatusbarRight = (StatusbarRight)findViewById(R.id.statusbar_right);
    }

    public void registerReceiver() {
        IntentFilter filter = new IntentFilter();
        filter.addAction(WifiManager.WIFI_STATE_CHANGED_ACTION);
        filter.addAction(WifiManager.RSSI_CHANGED_ACTION);
        filter.addAction(WifiManager.SUPPLICANT_CONNECTION_CHANGE_ACTION);
        filter.addAction(ConnectivityManager.CONNECTIVITY_ACTION);
        filter.addAction(ConnectivityManager.INET_CONDITION_ACTION);
        mContext.registerReceiver(mBr,filter);
    }
    
    public void unregisterReceiver(){
        mContext.unregisterReceiver(mBr);
    }
    
    public void processReceiver(Intent intent) {
        if(mStatusbarRight != null){
            mStatusbarRight.updateWifi(intent);
        }
    }    
}
