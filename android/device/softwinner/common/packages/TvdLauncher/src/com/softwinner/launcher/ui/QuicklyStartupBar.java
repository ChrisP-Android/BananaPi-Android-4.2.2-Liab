package com.softwinner.launcher.ui;

import com.softwinner.launcher.Launcher;
import com.softwinner.launcher.R;
import com.softwinner.launcher.Utilities;

import android.content.Context;
import android.content.Intent;
import android.graphics.Color;
import android.graphics.Rect;
import android.util.AttributeSet;
import android.util.Log;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.RelativeLayout;
import android.widget.TextView;

import java.util.ArrayList;

public class QuicklyStartupBar extends RelativeLayout implements View.OnClickListener{
    
    private final String TAG = "Launcher.QuicklyStartupBar";
    
    private final int ITEM_NUM = 5;
    
    private Launcher mLauncher;
    
    private WorkSpace mMainLayout;
    
    private int [] up_image_unselected_list = {
          R.drawable.up1,
          R.drawable.up2,
          R.drawable.up3,
          R.drawable.up4,
          R.drawable.up5,
    };
    
    private int [] up_image_selected_list = {
            R.drawable.sup1,
            R.drawable.sup2,
            R.drawable.sup3,
            R.drawable.sup4,
            R.drawable.sup5,
    };
    private int [] mid_image_selected_list = {
            R.drawable.m1,
            R.drawable.m2,
            R.drawable.m3,
            R.drawable.m4,
            R.drawable.m5,
          };
    private int [] down_image_unselected_list = {
            R.drawable.down1,
            R.drawable.down2,
            R.drawable.down3,
            R.drawable.down4,
            R.drawable.down5,
          };
    private int [] down_image_selected_list = {
            R.drawable.sdown1,
            R.drawable.sdown2,
            R.drawable.sdown3,
            R.drawable.sdown4,
            R.drawable.sdown5,
          };
    private int [] startupbar_text = {
            R.string.startup_bar_title_1,
            R.string.startup_bar_title_2,
            R.string.startup_bar_title_4,
            R.string.startup_bar_title_3,
            R.string.startup_bar_title_5
    };
    private int [] text_selected_color = {
            0xffc0e259,
            0xffb08047,
            0xffef86d7,
            0xff6ae4d0,
            0xfff19600
    };
    private int mSelectedPosition = Integer.MAX_VALUE/2-1;
    
    private LinearLayout mUp;
    private ArrayList<ImageView> mUpImage;
    
    private RelativeLayout mMiddle;
    
    private LinearLayout mDown;
    private ArrayList<ImageView> mDownImage;
    
    private LinearLayout mTitle;
    private ArrayList<TextView> mText; 
    
    private LayoutInflater mInflater;
    
    private View.OnClickListener mClick[] = {
      new OnClickListener(){
        @Override
        public void onClick(View arg0) {
            mSelectedPosition = Integer.MAX_VALUE/2-3;
            setImage(mUpImage, up_image_selected_list,up_image_unselected_list);
            setImage(mDownImage, down_image_selected_list,down_image_unselected_list);
            setViewForMiddle();
            setTitle(mText,startupbar_text);
            onKeyDown(KeyEvent.KEYCODE_DPAD_CENTER,
                    new KeyEvent(KeyEvent.ACTION_DOWN,KeyEvent.KEYCODE_DPAD_CENTER));
        }},
      new OnClickListener(){
        @Override
        public void onClick(View arg0) {
            mSelectedPosition = Integer.MAX_VALUE/2-2;
            setImage(mUpImage, up_image_selected_list,up_image_unselected_list);
            setImage(mDownImage, down_image_selected_list,down_image_unselected_list);
            setViewForMiddle();
            setTitle(mText,startupbar_text);
            onKeyDown(KeyEvent.KEYCODE_DPAD_CENTER,
                    new KeyEvent(KeyEvent.ACTION_DOWN,KeyEvent.KEYCODE_DPAD_CENTER));
        }},
     new OnClickListener(){
        @Override
        public void onClick(View arg0) {
            mSelectedPosition = Integer.MAX_VALUE/2-1;
            setImage(mUpImage, up_image_selected_list,up_image_unselected_list);
            setImage(mDownImage, down_image_selected_list,down_image_unselected_list);
            setViewForMiddle();
            setTitle(mText,startupbar_text);
            onKeyDown(KeyEvent.KEYCODE_DPAD_CENTER,
                    new KeyEvent(KeyEvent.ACTION_DOWN,KeyEvent.KEYCODE_DPAD_CENTER));
        }},
     new OnClickListener(){
        @Override
        public void onClick(View arg0) {
            mSelectedPosition = Integer.MAX_VALUE/2;
            setImage(mUpImage, up_image_selected_list,up_image_unselected_list);
            setImage(mDownImage, down_image_selected_list,down_image_unselected_list);
            setViewForMiddle();
            setTitle(mText,startupbar_text);
            onKeyDown(KeyEvent.KEYCODE_DPAD_CENTER,
                    new KeyEvent(KeyEvent.ACTION_DOWN,KeyEvent.KEYCODE_DPAD_CENTER));
        }},
     new OnClickListener(){
        @Override
        public void onClick(View arg0) {
            mSelectedPosition = Integer.MAX_VALUE/2 + 1;
            setImage(mUpImage, up_image_selected_list,up_image_unselected_list);
            setImage(mDownImage, down_image_selected_list,down_image_unselected_list);
            setViewForMiddle();
            setTitle(mText,startupbar_text);
            onKeyDown(KeyEvent.KEYCODE_DPAD_CENTER,
                    new KeyEvent(KeyEvent.ACTION_DOWN,KeyEvent.KEYCODE_DPAD_CENTER));
        }        
    }};

    public QuicklyStartupBar(Context context, AttributeSet attrs) {
        super(context, attrs);
        mInflater = LayoutInflater.from(context);
    }

    @Override
    protected void onFinishInflate() {
        super.onFinishInflate();
        
        mUp = (LinearLayout)findViewById(R.id.quickly_start_bar_up);
        mMiddle = (RelativeLayout)findViewById(R.id.quickly_start_bar_middle);
        mDown = (LinearLayout)findViewById(R.id.quickly_start_bar_down);
        mTitle = (LinearLayout)findViewById(R.id.quickly_start_bar_title);
        
        addViewForUp(getContext() , null);
        addViewForDown(getContext(), null);
        setViewForMiddle();
        addTitle();
        setOnClickListener(this);
        invalidate();
    }
    
    private void addViewForUp(Context context, AttributeSet attrs){
        mUpImage = new ArrayList<ImageView>(ITEM_NUM);
        ImageView v;
        for(int i=0;i<ITEM_NUM;i++){
            v = (ImageView) mUp.getChildAt(i);
            v.setOnClickListener(mClick[i]);
            mUpImage.add(v);
        }
        setImage(mUpImage, up_image_selected_list,up_image_unselected_list);
    }
    
    private void setViewForMiddle(){
        ImageView v = (ImageView) mMiddle.findViewById(R.id.long_bar);
        v.setImageResource(mid_image_selected_list[mSelectedPosition%mid_image_selected_list.length]);
        int t = v.getTop();
        int height = v.getHeight();
        int l = mSelectedPosition%ITEM_NUM*240;
        int width = v.getWidth();
        RelativeLayout.LayoutParams lp = (LayoutParams) v.getLayoutParams();
        lp.leftMargin = mSelectedPosition%ITEM_NUM*240;
        v.setLayoutParams(lp);
    }
    
    private void setTitle(ArrayList<TextView> text,int [] id){
        LinearLayout.LayoutParams lp;
        for(int i=0;i<ITEM_NUM;i++){
            text.get(i).setText(id[i]);
            text.get(i).setTextSize(getResources().getDimensionPixelSize(R.dimen.quickly_start_bar_title_normal));
            lp = (android.widget.LinearLayout.LayoutParams) text.get(i).getLayoutParams();
            lp.width = getResources().getDimensionPixelSize(R.dimen.quickly_start_bar_title_normal_width);
            text.get(i).setLayoutParams(lp);
            text.get(i).setTextColor(Color.WHITE);
            if(i==mSelectedPosition%ITEM_NUM){
                text.get(i).setTextColor(text_selected_color[mSelectedPosition%text_selected_color.length]);
                text.get(i).setTextSize(getResources().getDimensionPixelSize(R.dimen.quickly_start_bar_title_sel));
                lp = (android.widget.LinearLayout.LayoutParams) text.get(i).getLayoutParams();
                lp.width = getResources().getDimensionPixelSize(R.dimen.quickly_start_bar_title_sel_width);
                text.get(i).setLayoutParams(lp);
            }
        }
    }
    
    private void addViewForDown(Context context, AttributeSet attrs){
        mDownImage = new ArrayList<ImageView>(ITEM_NUM);
        ImageView v;
        for(int i=0;i<ITEM_NUM;i++){
            v = (ImageView)mDown.getChildAt(i);
            v.setOnClickListener(mClick[i]);
            mDownImage.add(v);
        }
        setImage(mDownImage, down_image_selected_list,down_image_unselected_list);
    }
    private void addTitle(){
        mText = new ArrayList<TextView>(ITEM_NUM);
        TextView v;
        for(int i=0;i<ITEM_NUM;i++){
            v = (TextView)mTitle.getChildAt(i);
            v.setTextColor(Color.WHITE);
            v.setShadowLayer(1, 1, 1, Color.BLACK);
            v.setOnClickListener(mClick[i]);
            mText.add(v);
        }
        setTitle(mText,startupbar_text);
    }
    private void setImage(ArrayList<ImageView> list,int [] sel,int [] unsel){
        for(int i=0;i<ITEM_NUM;i++){
            if(i==mSelectedPosition%ITEM_NUM){
                list.get(i).setImageResource(sel[i]);
            }else{
                list.get(i).setImageResource(unsel[i%unsel.length]);
            }
        }
    }
    
    /**
     * ----------------------------------------------------------------
     * public method
     * setPosition:set the current selected position.
     * moveToLeft:move quickly start bar to left.
     * moveToRight:move quickly start bar to right.
     */
    
    /**
     * set the current selected position and flush the view.
     * @param position the position you want to set ,range 0 ~ Integer.MAX_VALUE
     */
    public void setPosition(int position){
        mSelectedPosition = position;
        setImage(mUpImage, up_image_selected_list,up_image_unselected_list);
        setImage(mDownImage, down_image_selected_list,down_image_unselected_list);
        setViewForMiddle();
    }
    /**
     * move quickly start bar to left.
     */
    
    public void moveToLeft(){
        mSelectedPosition--;
        setImage(mUpImage, up_image_selected_list,up_image_unselected_list);
        setImage(mDownImage, down_image_selected_list,down_image_unselected_list);
        setViewForMiddle();
        setTitle(mText,startupbar_text);
    }
    
    /**
     * move quickly start bar to right.
     */
    public void moveToRight(){
        mSelectedPosition++;
        setImage(mUpImage, up_image_selected_list,up_image_unselected_list);
        setImage(mDownImage, down_image_selected_list,down_image_unselected_list);
        setViewForMiddle();
        setTitle(mText,startupbar_text);
    }
    
    /**
     * Set the launcher, launcher can provide part of the functions.    
     * @param launcher the launcher
     */
    public void setMainLayout(WorkSpace mainLayout){
        mMainLayout = mainLayout;
    }   
    
    @Override
    protected void onFocusChanged(boolean gainFocus, int direction, Rect previouslyFocusedRect) {
        if(gainFocus){
            mText.get(mSelectedPosition%ITEM_NUM).setTextColor(text_selected_color[mSelectedPosition%text_selected_color.length]);
        }else{
            mText.get(mSelectedPosition%ITEM_NUM).setTextColor(Color.WHITE);
        }
    }

    /**
     * ---------------------------------------------------------
     * 
     */
    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        if(keyCode == KeyEvent.KEYCODE_DPAD_LEFT){
            moveToLeft();
        }else if(keyCode == KeyEvent.KEYCODE_DPAD_RIGHT){
            moveToRight();
        }else if(keyCode == KeyEvent.KEYCODE_DPAD_CENTER){
            Intent intent;
            switch(mSelectedPosition%Utilities.ACTION_COUNT){
                case Utilities.ACTION_START_ALL_APP_VIEW:
                    mMainLayout.showAllApps(true);
                    break;
                case Utilities.ACTION_START_FILEMANAGER:
                    intent = new Intent();
                    intent.setClassName(Utilities.FILEMANAGER_PACKAGE,Utilities.FILEMANAGER_NAME);
                    intent.putExtra("media_type", Utilities.FILEMANAGER_TYPE_ALL);
                    mMainLayout.mLauncher.startActivitySafely(intent,TAG);
                    break;
                case Utilities.ACTION_START_WEBBROWSER:
                    intent = new Intent();
                    intent.setClassName("com.android.browser", "com.android.browser.BrowserActivity");
                    mMainLayout.mLauncher.startActivitySafely(intent,TAG);
                    break;
                case Utilities.ACTION_START_FAVORITES_APPS:
                    mMainLayout.showFavoritesApps(true);
                    break;
                case Utilities.ACTION_START_SETTINGS:
                    intent = new Intent();
                    intent.setClassName("com.android.settings", "com.android.settings.Settings");
                    mMainLayout.mLauncher.startActivitySafely(intent,TAG);
                    break;
            }
        }
        return super.onKeyDown(keyCode, event);
    }
    
    public void reflashLocal(){
        setTitle(mText,startupbar_text);
    }

    @Override
    public void onClick(View v) {        
        mText.get(mSelectedPosition%ITEM_NUM).setTextColor(text_selected_color[mSelectedPosition%text_selected_color.length]);
    }   
}
