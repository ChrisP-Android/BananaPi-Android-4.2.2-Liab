package com.softwinner.launcher.ui;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Rect;
import android.util.AttributeSet;
import android.widget.Button;
import android.util.Log;

public class MenuItem extends Button
{
	private int height = 6;
	private String TAG = "MenuItem";
	
	public MenuItem(Context context)
	{
		super(context);
	}
	
	public MenuItem(Context context, AttributeSet attrs)
	{
		super(context,attrs);
	}
	
	public MenuItem(Context context, AttributeSet attrs, int defStyle)
	{
		super(context, attrs, defStyle);
	}
	
	@Override
	protected void onDraw (Canvas canvas)
	{
		super.onDraw(canvas);
		if(this.isFocused())
		{
			Paint mPaint = new Paint();
			mPaint.setColor(0xFFF8B62D);
			Rect rect = new Rect(0, this.getHeight() - height, this.getWidth(), this.getHeight());
			canvas.drawRect(rect, mPaint);
			this.setTextColor(0xFFF8B62D);
		}
		else
		{
			Paint mPaint = new Paint();
			mPaint.setColor(0xFFFFFFFF);
			Rect rect = new Rect(0, this.getHeight() - height, this.getWidth(), this.getHeight());
			canvas.drawRect(rect, mPaint);
			this.setTextColor(Color.WHITE);
		}
	}
}