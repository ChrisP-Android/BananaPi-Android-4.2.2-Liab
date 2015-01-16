
package com.android.internal.policy.impl;


import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Paint.Style;
import android.graphics.Bitmap.Config;
import android.graphics.Matrix;
import android.util.AttributeSet;
import android.graphics.RectF;
import android.content.res.AssetManager;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Paint.FontMetricsInt;
import android.hardware.input.InputManager;
import android.hardware.input.InputManager.InputDeviceListener;
import android.os.SystemProperties;
import android.os.Handler;
import android.os.Message;
import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.VelocityTracker;
import android.view.View;
import android.view.ViewConfiguration;
import android.view.MotionEvent.PointerCoords;
import android.os.SystemClock;

import android.util.Log;
import android.util.Slog;
import android.util.DisplayMetrics;

import java.io.IOException;
import java.io.InputStream;
import java.io.FileInputStream;
import java.io.File;


public class BootAnimationView extends View{

	private static final boolean DEBUG = false;
	private static final String TAG = "BootAnimationView";
	private static final String BOOT_LOGO_PATH = "/system/media/bootlogo.bmp";
	private static final String INIT_LOGO_PATH = "/system/media/initlogo.bmp";

	private Bitmap mBootLogo;
	private Bitmap mInitLogo;
	private Bitmap [] mBat;

	private Bitmap mMemBitmap;
	private Canvas mMemCanvas = null;
	private Paint mMemPaint;

	private int mW,mH;

	private final static int nsize = 11;
	private int cframe;
	private int sframe;
	private final static int FRAME_RATE = 300;
	private final static int MSG_UPDATE_FRAME = 0;
	private final static int MSG_UPDATE = 1;


	private BootAnimationView mSlef;

	private int drawWhat;

	private Handler mHandler = new Handler(){
		@Override
        public void handleMessage(Message msg){
       		switch(msg.what){
				case MSG_UPDATE_FRAME:
					if(cframe>=nsize){
						cframe = sframe;
					}
					drawBatteryFrame();
					cframe++;
					mSlef.invalidate();
					sendEmptyMessageDelayed(MSG_UPDATE_FRAME,FRAME_RATE);
					break;
				case MSG_UPDATE:
					mSlef.invalidate();
					break;
					
       		}
		}
	};

	private Bitmap getImageFromAssetsFile(Context context,String filepath){
		Bitmap image = null;  
	    AssetManager am = context.getResources().getAssets();  
	    try{  
	          InputStream is = am.open(filepath);  
	          image = BitmapFactory.decodeStream(is);  
	          is.close();  
	      }catch (IOException e){  
	          e.printStackTrace();  
	      }  
	    return image;  
	}

	private Bitmap getImageFromSystemPath(Context context,String filepath){
		Bitmap image = null;
		File fp = new File(filepath);
		try{  
	          FileInputStream is = new FileInputStream(fp);  
	          image = BitmapFactory.decodeStream(is);  
	          is.close();  
	      }catch (IOException e){  
	          e.printStackTrace();  
	      }
		  return image;
	}
	
	public BootAnimationView(Context context){
		this(context,null);
	}

	public BootAnimationView(Context context, AttributeSet attrs) {
		super(context,attrs);
		mSlef = this;
		mBat = new Bitmap[nsize];
		mBootLogo = getImageFromSystemPath(context,BOOT_LOGO_PATH);
		mInitLogo = getImageFromSystemPath(context,INIT_LOGO_PATH);
		for(int i=0;i<mBat.length;i++){
			mBat[i] = getImageFromAssetsFile(context,"images/bat" + i + ".bmp");
		}
		Log.d(TAG,"BootAnimationView init");
		mW = mBat[0].getWidth();
		mH = mBat[0].getHeight();
		initMemCanvas();
	}

	public void startShowBatteryCharge(int percent){
		Log.d(TAG,"startShowBatteryCharge percent = " + percent);
		if(percent<=100&&percent>0)
			sframe = percent/10;
		else
			sframe = 0;
		cframe = sframe;
		if(DEBUG)
			Log.d(TAG,"cframe = " + cframe);
		drawWhat = 1;
		mHandler.removeMessages(MSG_UPDATE_FRAME);
		mHandler.sendEmptyMessage(MSG_UPDATE_FRAME);
	}

	private Bitmap zoomBitmap(Bitmap bmp, int w, int h){
		int width = bmp.getWidth();
		int height= bmp.getHeight();
		Matrix matrix = new Matrix();
		float scaleWidth = ((float)w / width);
		float scaleHeight = ((float)h / height);
		matrix.postScale(scaleWidth, scaleHeight);
		return Bitmap.createBitmap(bmp, 0, 0, width, height, matrix, true);
	}

	private void drawBatteryFrame(){
		mMemCanvas.drawBitmap(mBat[cframe],0,0,mMemPaint);
	}

	public void hideScreen(boolean enable){
		if(enable){
			drawWhat = 2;
		}else{
			drawWhat = 0;
		}
		mHandler.sendEmptyMessage(MSG_UPDATE);
	}
	public void showBootInitLogo(int logo){
		drawWhat = 3;
		mHandler.sendEmptyMessage(MSG_UPDATE);
	}

	public void showInitLogo(){
		drawWhat = 5;
		mHandler.sendEmptyMessage(MSG_UPDATE);
	}

	public void hideBootInitLogo(){
		drawWhat = 0;
		mHandler.sendEmptyMessage(MSG_UPDATE);
	}

	private void initMemCanvas(){
		Log.d(TAG,"initMemCanvas");
		mMemBitmap = Bitmap.createBitmap(mW,mH, Config.ARGB_8888);
		mMemCanvas = new Canvas();
		mMemCanvas.setBitmap(mMemBitmap);
		mMemPaint = new Paint();
		mMemPaint.setColor(Color.BLACK);
		mMemPaint.setStyle(Style.FILL);
	}

	public void showBootAnimation(){
		if(DEBUG)
			Log.d(TAG,"showBootAnimation");
		  	SystemProperties.set("service.bootanim.exit", "0");    //暂时用系统的代替一下，以后自己写
			SystemProperties.set("ctl.start", "bootanim");
	}

	public void hideBootAnimation(){
		if(DEBUG)
			Log.d(TAG,"hideBootAnimation");
			SystemProperties.set("service.bootanim.exit", "1");    
			SystemProperties.set("ctl.stop", "bootanim");
	}

	@Override
    protected void onDraw(Canvas canvas) {
    	super.onDraw(canvas);
		switch(drawWhat){
		case 1:
			canvas.drawRect(0,0,canvas.getWidth(), canvas.getHeight(), mMemPaint);
			canvas.drawBitmap(mMemBitmap,canvas.getWidth()/2 - mW/2,canvas.getHeight()/2 - mH/2,mMemPaint);
			break;
		case 5:
			canvas.drawBitmap(mInitLogo,0,0,mMemPaint);
			break;
		case 2:
			canvas.drawRect(0,0,canvas.getWidth(), canvas.getHeight(), mMemPaint);
			break;
		case 3:
			canvas.drawRect(0,0,canvas.getWidth(), canvas.getHeight(), mMemPaint);
			canvas.drawBitmap(mBootLogo,canvas.getWidth()/2 - mBootLogo.getWidth()/2,canvas.getHeight()/2 - mBootLogo.getHeight()/2,mMemPaint);
			break;
		}
	}
}










































