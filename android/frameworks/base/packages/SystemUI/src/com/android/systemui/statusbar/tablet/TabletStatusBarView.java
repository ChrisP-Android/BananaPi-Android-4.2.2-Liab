/*
 * Copyright (C) 2010 The Android Open Source Project
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
 */

package com.android.systemui.statusbar.tablet;

import com.android.systemui.R;
import com.android.systemui.statusbar.BaseStatusBar;
import com.android.systemui.statusbar.DelegateViewHelper;
import com.android.systemui.statusbar.policy.KeyButtonView;
import android.content.res.Configuration;

import android.content.Context;
import android.os.Handler;
import android.util.AttributeSet;
import android.util.Slog;
import android.view.View;
import android.view.MotionEvent;
import android.widget.FrameLayout;

public class TabletStatusBarView extends FrameLayout {
	private static final boolean DEBUG = false;
    private Handler mHandler;	
    private boolean displayLock = true;
    private final int PORT_WIDTH_DP = 650;
    private final int MAX_PANELS = 5;
    private final View[] mIgnoreChildren = new View[MAX_PANELS];
    private final View[] mPanels = new View[MAX_PANELS];
    private final int[] mPos = new int[2];
    private DelegateViewHelper mDelegateHelper;

    public TabletStatusBarView(Context context) {
        this(context, null);
    }

    public TabletStatusBarView(Context context, AttributeSet attrs) {
        super(context, attrs);
        mDelegateHelper = new DelegateViewHelper(this);
    }

    public void setDelegateView(View view) {
        mDelegateHelper.setDelegateView(view);
    }

    public void setBar(BaseStatusBar phoneStatusBar) {
        mDelegateHelper.setBar(phoneStatusBar);
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        if (mDelegateHelper != null) {
            mDelegateHelper.onInterceptTouchEvent(event);
        }
        return true;
    }

    @Override
    protected void onLayout(boolean changed, int left, int top, int right, int bottom) {
		if (DEBUG)
			Slog.i("onLayout", "changed:" +  changed + right + "," + left);
        // super.onLayout(changed, left, top, right, bottom);
        // Find the view we wish to grab events from in order to detect search gesture.
        // Depending on the device, this will be one of the id's listed below.
        // If we don't find one, we'll use the view provided in the constructor above (this view).
        View view = findViewById(R.id.navigationArea);
        if (view == null) {
            view = findViewById(R.id.nav_buttons);
        }
/*
		//modified by Derek for display of virtual volume keys when TabletStatusBarView layout 
		//or relayout(e.g. orientation of screen changed). 2012.10.24
		Configuration config = mContext.getResources().getConfiguration();
		KeyButtonView volume_up=(KeyButtonView)findViewById(R.id.volume_up);
		KeyButtonView volume_down=(KeyButtonView)findViewById(R.id.volume_down);
		KeyButtonView menu =(KeyButtonView)findViewById(R.id.menu);
		KeyButtonView screenshot =(KeyButtonView)findViewById(R.id.screenshot);
		if(displayLock)
		{
			if(config.screenWidthDp>PORT_WIDTH_DP)
			{
			    volume_up.setVisibility(View.VISIBLE);
			    volume_down.setVisibility(View.VISIBLE);
			    screenshot.setVisibility(View.VISIBLE);
                // menu.setVisibility(View.VISIBLE);
			}
			else
			{
			    volume_up.setVisibility(View.GONE);
			    volume_down.setVisibility(View.GONE);
			    screenshot.setVisibility(View.GONE);
                // menu.setVisibility(View.GONE);
			}
            menu.setVisibility(View.VISIBLE);
		}
		else
		{
			volume_up.setVisibility(View.GONE);
			volume_down.setVisibility(View.GONE);
			screenshot.setVisibility(View.GONE);
            menu.setVisibility(View.GONE);
		}//end modify
		if (DEBUG) {
			Slog.i("onLayout", "displayLock:" +  displayLock + ",width:" + config.screenWidthDp);
			Slog.i("onLayout", "visible:" + volume_up.getVisibility() + "," + screenshot.getVisibility() + "," + menu.getVisibility());
		}*/

        mDelegateHelper.setSourceView(view);
        mDelegateHelper.setInitialTouchRegion(view);
        super.onLayout(changed, left, top, right, bottom);
    }

    @Override
    public boolean onInterceptTouchEvent(MotionEvent ev) {
        if (ev.getAction() == MotionEvent.ACTION_DOWN) {
            if (TabletStatusBar.DEBUG) {
                Slog.d(TabletStatusBar.TAG, "TabletStatusBarView intercepting touch event: " + ev);
            }
            // do not close the recents panel here- the intended behavior is that recents is dismissed
            // on touch up when clicking on status bar buttons
            // TODO: should we be closing the notification panel and input methods panel?
            mHandler.removeMessages(TabletStatusBar.MSG_CLOSE_NOTIFICATION_PANEL);
            mHandler.sendEmptyMessage(TabletStatusBar.MSG_CLOSE_NOTIFICATION_PANEL);
            mHandler.removeMessages(TabletStatusBar.MSG_CLOSE_INPUT_METHODS_PANEL);
            mHandler.sendEmptyMessage(TabletStatusBar.MSG_CLOSE_INPUT_METHODS_PANEL);
            mHandler.removeMessages(TabletStatusBar.MSG_STOP_TICKER);
            mHandler.sendEmptyMessage(TabletStatusBar.MSG_STOP_TICKER);

            for (int i=0; i < mPanels.length; i++) {
                if (mPanels[i] != null && mPanels[i].getVisibility() == View.VISIBLE) {
                    if (eventInside(mIgnoreChildren[i], ev)) {
                        if (TabletStatusBar.DEBUG) {
                            Slog.d(TabletStatusBar.TAG,
                                    "TabletStatusBarView eating event for view: "
                                    + mIgnoreChildren[i]);
                        }
                        return true;
                    }
                }
            }
        }
        if (TabletStatusBar.DEBUG) {
            Slog.d(TabletStatusBar.TAG, "TabletStatusBarView not intercepting event");
        }
        if (mDelegateHelper != null && mDelegateHelper.onInterceptTouchEvent(ev)) {
            return true;
        }
        return super.onInterceptTouchEvent(ev);
    }

	
	@Override
	protected void onSizeChanged(int w, int h, int oldw, int oldh){
		/*KeyButtonView volume_up=(KeyButtonView)findViewById(R.id.volume_up);
		KeyButtonView volume_down=(KeyButtonView)findViewById(R.id.volume_down);
		KeyButtonView menu =(KeyButtonView)findViewById(R.id.menu);
		KeyButtonView screenshot =(KeyButtonView)findViewById(R.id.screenshot);
		View navigationArea=(View)findViewById(R.id.navigationArea);
		View mNotificationArea = (View)findViewById(R.id.notificationArea);
		if(displayLock){
			if(w<navigationArea.getWidth()+mNotificationArea.getWidth()){
				volume_up.setVisibility(View.GONE);
				volume_down.setVisibility(View.GONE);
				screenshot.setVisibility(View.GONE);   //GONE
                // menu.setVisibility(View.GONE);
			}else{
				volume_up.setVisibility(View.VISIBLE);
				volume_down.setVisibility(View.VISIBLE);
				screenshot.setVisibility(View.VISIBLE);
				// menu.setVisibility(View.VISIBLE);
			}
            menu.setVisibility(View.VISIBLE);
		} else {
			volume_up.setVisibility(View.GONE);
			volume_down.setVisibility(View.GONE);
			screenshot.setVisibility(View.GONE);
            menu.setVisibility(View.GONE);
        }*/
		super.onSizeChanged(w, h, oldw, oldh);
	}
		
    private boolean eventInside(View v, MotionEvent ev) {
        // assume that x and y are window coords because we are.
        final int x = (int)ev.getX();
        final int y = (int)ev.getY();

        final int[] p = mPos;
        v.getLocationInWindow(p);

        final int l = p[0];
        final int t = p[1];
        final int r = p[0] + v.getWidth();
        final int b = p[1] + v.getHeight();

        return x >= l && x < r && y >= t && y < b;
    }

    public void setHandler(Handler h) {
        mHandler = h;
    }


    public void setShowVolume(boolean show,Context mContext){
        displayLock=show;
        Configuration config = mContext.getResources().getConfiguration();
        KeyButtonView volume_up=(KeyButtonView)findViewById(R.id.volume_up);
        KeyButtonView volume_down=(KeyButtonView)findViewById(R.id.volume_down);
        KeyButtonView menu =(KeyButtonView)findViewById(R.id.menu);
        KeyButtonView screenshot =(KeyButtonView)findViewById(R.id.screenshot);
        if (DEBUG) {
            Slog.i("setShowVolume", "displayLock:" +  show);
            Slog.i("setShowVolume", "width" + config.screenWidthDp);
        }

        //modified by Derek for display of virtual volume keys when screen lock and unlock, 2012.10.24
        if(show)
        {
            if(config.screenWidthDp>PORT_WIDTH_DP)
            {
                volume_up.setVisibility(View.VISIBLE);
                volume_down.setVisibility(View.VISIBLE);
                //screenshot.setVisibility(View.VISIBLE);
                // menu.setVisibility(View.VISIBLE);
            }
            else
            {
                volume_up.setVisibility(View.GONE);
                volume_down.setVisibility(View.GONE);
                //screenshot.setVisibility(View.GONE);
                // menu.setVisibility(View.GONE);
            }
            menu.setVisibility(View.VISIBLE);
        }
        else
        {
            volume_up.setVisibility(View.GONE);
            volume_down.setVisibility(View.GONE);
            //screenshot.setVisibility(View.GONE);
            menu.setVisibility(View.GONE);
        }//end modify
        /*if(config.screenWidthDp>480&&show){
          volume_up.setVisibility(View.VISIBLE);
          volume_down.setVisibility(View.VISIBLE);
        //menu.setVisibility(View.VISIBLE);
        screenshot.setVisibility(View.VISIBLE);
        }else{
        volume_up.setVisibility(View.INVISIBLE);
        volume_down.setVisibility(View.INVISIBLE);
        menu.setVisibility(View.INVISIBLE);
        screenshot.setVisibility(View.INVISIBLE);
        }*/
    }

    /**
     * Let the status bar know that if you tap on ignore while panel is showing, don't do anything.
     *
     * Debounces taps on, say, a popup's trigger when the popup is already showing.
     */
    public void setIgnoreChildren(int index, View ignore, View panel) {
        mIgnoreChildren[index] = ignore;
        mPanels[index] = panel;
    }
}
