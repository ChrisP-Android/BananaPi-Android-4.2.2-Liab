package com.android.gallery3d.app; 

import com.android.gallery3d.R;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Matrix;
import android.graphics.Paint;
import android.graphics.Rect;
import android.util.AttributeSet;
import android.util.Log;
import android.view.MotionEvent;
import android.view.View;
import android.view.View.OnTouchListener;

public class SlipSwitch extends View implements OnTouchListener{
	private final static String TAG = "SlipSwitch";

	private boolean mInitSwitch;
	private boolean NowChoose = false;//记录当前按钮是否打开,true为打开,flase为关闭
	private boolean OnSlip = false;//记录用户是否在滑动的变量
	private float DownX,NowX;//按下时的x,当前的x,
	private Rect Btn_On,Btn_Off;//打开和关闭状态下,游标的Rect
	private OnSwitchChangedListener ChgLsn;
	
	private Bitmap backgroud, slip_btn;
	
	public interface OnSwitchChangedListener {  
	    abstract void OnSwitchChanged(boolean switchOn);  
	}	
	
	public SlipSwitch(Context context) {
		super(context);
		// TODO Auto-generated constructor stub
		init();
	}

	public SlipSwitch(Context context, AttributeSet attrs) {
		super(context, attrs);
		// TODO Auto-generated constructor stub
		init();
	}

	public SlipSwitch(Context context, OnSwitchChangedListener listener, boolean initSwitch){
		super(context);
		ChgLsn = listener;
		mInitSwitch = initSwitch;
		NowChoose = mInitSwitch;		
		init();
	}
	
	private void init(){//初始化
		//载入图片资源
		backgroud = BitmapFactory.decodeResource(getResources(), R.drawable.slipswitch_bg);
		if(mInitSwitch){
			slip_btn = BitmapFactory.decodeResource(getResources(), R.drawable.slipswitch_uf_on);
		}
		else{
			slip_btn = BitmapFactory.decodeResource(getResources(), R.drawable.slipswitch_uf_off);
		}
		//获得需要的Rect数据
		Btn_On = new Rect(0,0,slip_btn.getWidth(),slip_btn.getHeight());
		Btn_Off = new Rect(backgroud.getWidth()-slip_btn.getWidth(), 0, backgroud.getWidth(), slip_btn.getHeight());
		setOnTouchListener(this);//设置监听器,也可以直接复写OnTouchEvent
	}
	
	@Override
	protected void onDraw(Canvas canvas) {//绘图函数
		// TODO Auto-generated method stub
		super.onDraw(canvas);
		Matrix matrix = new Matrix();
		Paint paint = new Paint();
		float x;
		
		{
			canvas.drawBitmap(backgroud,matrix, paint);
			
			if(OnSlip)//是否是在滑动状态,
			{
				if(NowX >= backgroud.getWidth())//是否划出指定范围,不能让游标跑到外头,必须做这个判断
					x = backgroud.getWidth()-slip_btn.getWidth()/2;//减去游标1/2的长度...
				else
					x = NowX - slip_btn.getWidth()/2;
			}else{//非滑动状态
				if(NowChoose)//根据现在的开关状态设置画游标的位置
					x = Btn_On.left;
				else
					x = Btn_Off.left;
			}
		if(x<0)//对游标位置进行异常判断...
			x = 0;
		else if(x > backgroud.getWidth() - slip_btn.getWidth())
			x = backgroud.getWidth() - slip_btn.getWidth();
		canvas.drawBitmap(slip_btn,x, 0, paint);//画出游标.
		}
	}

	@Override
	protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
		// TODO Auto-generated method stub
		setMeasuredDimension(backgroud.getWidth(), backgroud.getHeight());
	}

	public boolean onTouch(View v, MotionEvent event) {
		// TODO Auto-generated method stub
		switch(event.getAction())//根据动作来执行代码
		{
			case MotionEvent.ACTION_MOVE://滑动
				NowX = event.getX();
				invalidate();//重画控件
				break;
			case MotionEvent.ACTION_DOWN://按下
				if(event.getX() > backgroud.getWidth()||event.getY()>backgroud.getHeight())
					return false;
				slip_btn = BitmapFactory.decodeResource(getResources(), R.drawable.slipswitch_f);
				OnSlip = true;
				DownX = event.getX();
				NowX = DownX;
				invalidate();//重画控件
				break;
			case MotionEvent.ACTION_UP://松开
				OnSlip = false;
				if(event.getX() >= (backgroud.getWidth()/2)){
					NowChoose = false;
					slip_btn = BitmapFactory.decodeResource(getResources(), R.drawable.slipswitch_uf_off);
				}
				else{
					NowChoose = true;
					slip_btn = BitmapFactory.decodeResource(getResources(), R.drawable.slipswitch_uf_on);
				}
				invalidate();//重画控件
				Log.e(TAG, "ACTION_UP, NowChoose = " + NowChoose);
				if(ChgLsn != null){
					ChgLsn.OnSwitchChanged(NowChoose);
				}
				break;
			default:
		
		}
		return true;
	}
		
}