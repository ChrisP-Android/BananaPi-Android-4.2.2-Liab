package com.android.gallery3d.app; 

import com.android.gallery3d.R;

import android.app.AlertDialog;
import android.app.Dialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.DialogInterface.OnCancelListener;
import android.graphics.*;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup.LayoutParams;
import android.widget.Button;
import android.widget.LinearLayout;

public class ColorPickerDialog{

    public interface OnColorChangedListener {
        void colorChanged(int color);
    }

    private OnColorChangedListener mListener;
    private int mInitialColor;
    private Dialog mDialog;
    private View contentView;
    private ColorPickerView mColorPickerView;

    public ColorPickerDialog(Context context,OnColorChangedListener listener,int initialColor){
        mListener = listener;
        mInitialColor = initialColor;
    	
        contentView = LayoutInflater.from(context).inflate(R.layout.color_picker, null);
    	mColorPickerView = new ColorPickerView(context, listener, mInitialColor);
        
        LinearLayout colorLayout = (LinearLayout)contentView.findViewById(R.id.color_pan);
        colorLayout.addView(mColorPickerView);
        Button leftButton = (Button)contentView.findViewById(R.id.button1);
        Button rightButton = (Button)contentView.findViewById(R.id.button2);
        leftButton.setText(R.string.confirm_button);
        rightButton.setText(R.string.cancel);        
        leftButton.setOnClickListener(new OnClickListener(){

			public void onClick(View v) {
				// TODO Auto-generated method stub
				mListener.colorChanged(mColorPickerView.getCenterColor());
				mDialog.dismiss();
			}
        	
        });
        rightButton.setOnClickListener(new OnClickListener(){

			public void onClick(View v) {
				// TODO Auto-generated method stub
				mListener.colorChanged(mInitialColor);
				mDialog.dismiss();
			}
        	
        });
        
        mDialog = new Dialog(context,R.style.dialog);
        mDialog.setTitle(R.string.color_title);
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
    public int getCurrentColor(){
    	return mColorPickerView.getCenterColor();
    }
}
