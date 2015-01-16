package com.android.gallery3d.app; 

import com.android.gallery3d.R;
import com.android.gallery3d.app.SlipSwitch.OnSwitchChangedListener;

import android.app.Dialog;
import android.content.Context;
import android.content.DialogInterface.OnCancelListener;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.LinearLayout;

public class SlipSwitchDialog{
    private OnSwitchResultListener mListener;
    private boolean mInitSwitch;
    private boolean switchResult;
    private Dialog mDialog;
    private View contentView;
    private SlipSwitch mSlipSwitch;	
	
    public interface OnSwitchResultListener {
        void OnSwitchResult(boolean switchOn);
    }

    public SlipSwitchDialog(Context context, OnSwitchResultListener listener, boolean initSwitch){
    	mListener = listener;
    	mInitSwitch = initSwitch;
    	switchResult = initSwitch;
    	
    	contentView = LayoutInflater.from(context).inflate(R.layout.slip_switch, null);
    	LinearLayout switchLayout = (LinearLayout)contentView.findViewById(R.id.slip_gate);
    	OnSwitchChangedListener switchListener = new OnSwitchChangedListener(){

			public void OnSwitchChanged(boolean switchOn) {
				// TODO Auto-generated method stub
				switchResult = switchOn;
			}
    		
    	};
    	mSlipSwitch = new SlipSwitch(context, switchListener, initSwitch);
        switchLayout.addView(mSlipSwitch);
    
        Button leftButton = (Button)contentView.findViewById(R.id.button1);
        Button rightButton = (Button)contentView.findViewById(R.id.button2);
        leftButton.setText(R.string.confirm_button);
        rightButton.setText(R.string.cancel);
        leftButton.setOnClickListener(new OnClickListener(){

			public void onClick(View v) {
				// TODO Auto-generated method stub
				mListener.OnSwitchResult(switchResult);				
				mDialog.dismiss();				
			}
        	
        });
        rightButton.setOnClickListener(new OnClickListener(){

			public void onClick(View v) {
				// TODO Auto-generated method stub
				mListener.OnSwitchResult(mInitSwitch);
				mDialog.dismiss();
			}
        	
        });
                
        mDialog = new Dialog(context,R.style.dialog);
        mDialog.setTitle(R.string.gate_title);
        mDialog.setContentView(contentView);
    }
    
    public void show(){
    	mDialog.show();
    }
    
    public void dismiss(){
    	mDialog.dismiss();
    }    

    public boolean isShowing(){
    	return mDialog.isShowing();
    }
    
    public void setCancelListener(OnCancelListener listener) {
        mDialog.setOnCancelListener(listener);
    }

}