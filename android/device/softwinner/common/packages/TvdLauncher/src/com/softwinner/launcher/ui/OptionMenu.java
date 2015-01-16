package com.softwinner.launcher.ui;

import com.softwinner.launcher.R;

import android.app.Dialog;
import android.content.Context;
import android.graphics.Bitmap;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.view.WindowManager;
import android.widget.Adapter;
import android.widget.AdapterView;
import android.widget.BaseAdapter;
import android.widget.ImageView;
import android.widget.ListAdapter;
import android.widget.ListView;
import android.widget.TextView;

import java.util.ArrayList;

public class OptionMenu extends Dialog {
    
    protected ImageView mIcon;
    protected TextView mTitle;
    protected ListView mContentList;
    protected Adapter mAdapter;
    protected View mView;
    protected LayoutInflater mInflater;
    
    static public class IconAdapter extends BaseAdapter{
        
        ArrayList<Integer> mImageIds = new ArrayList<Integer>();
        ArrayList<Integer> mTextIds = new ArrayList<Integer>();
        protected LayoutInflater mInflater;
        
        public IconAdapter(Context context){
            mInflater = LayoutInflater.from(context);
        }
        @Override
        public int getCount() {
            return mTextIds.size();
        }

        @Override
        public Object getItem(int id) {
            return mTextIds.get(id);
        }

        @Override
        public long getItemId(int position) {
            return position;
        }

        @Override
        public View getView(int position, View convert, ViewGroup parent) {
            if(convert == null){
                convert = mInflater.inflate(R.layout.option_menu_item_by_icon, parent ,false);
            }
            ImageView ic = (ImageView) convert.findViewById(R.id.item_icon);
            TextView content = (TextView) convert.findViewById(R.id.item_content);
            ic.setImageResource(mImageIds.get(position));
            content.setText(mTextIds.get(position));
            return convert;
        }
        
    }
    
    static public class TextAdapter extends BaseAdapter{

        ArrayList<Integer> mTextIds = new ArrayList<Integer>();        
        protected LayoutInflater mInflater;
        
        public TextAdapter(Context context){
            mInflater = LayoutInflater.from(context);
        }
        @Override
        public int getCount() {
            return mTextIds.size();
        }

        @Override
        public Object getItem(int id) {
            return mTextIds.get(id);
        }

        @Override
        public long getItemId(int position) {
            return position;
        }

        @Override
        public View getView(int position, View convert, ViewGroup parent) {
            if(convert == null){
                convert = mInflater.inflate(R.layout.option_menu_item_by_text, parent ,false);
            }
            TextView content=(TextView)convert.findViewById(R.id.item_text_content);
            content.setText(mTextIds.get(position));
            return convert;
        }
        
    }
    
    public OptionMenu(Context context) {
        this(context,R.style.optionMenuDialog);
        LayoutInflater inflater = getLayoutInflater();
        mView = inflater.inflate(R.layout.option_menu, null);
        mIcon = (ImageView)mView.findViewById(R.id.icon);
        mTitle = (TextView)mView.findViewById(R.id.title);
        mContentList = (ListView)mView.findViewById(R.id.content_list);
        
    }

    public OptionMenu(Context context, int theme) {
        super(context, theme);
    }

    public OptionMenu(Context context, boolean cancelable, OnCancelListener cancelListener) {
        super(context, cancelable, cancelListener);
    }

    @Override
    public void dismiss() {
        super.dismiss();
    }

    public void show(int x, int y, int width ,int height, boolean isLeft) {
        Window window = getWindow();
        WindowManager.LayoutParams params = new WindowManager.LayoutParams();
        params.x = x;
        params.y = y;
        window.setAttributes(params);
        setContentView(mView);
        super.show();        
        window.setLayout(width, height);
        int animId = isLeft?R.style.optionMenuZoomInLeft:R.style.optionMenuZoomInRight;
        window.setWindowAnimations(animId);
    }
    
    public void setContentView(Adapter adapterView , Adapter adapter){
        
    }
    
    public void setTitle(String title){
        mTitle.setText(title);
    }
    
    public void setTitle(int resId){
        mTitle.setText(resId);
    }
    
    public void setIcon(Bitmap icon){
        mIcon.setImageBitmap(icon);
    }
    
    public void setIcon(int resId){
        mIcon.setImageResource(resId);
    }
    
    public void setClickCallback(AdapterView.OnItemClickListener callback){
        mContentList.setOnItemClickListener(callback);
    }
    
    public void setSelectedCallback(AdapterView.OnItemSelectedListener callback){
        mContentList.setOnItemSelectedListener(callback);
    }
    
    static public ListAdapter createIconAdapter(Context context){
        return new IconAdapter(context);
    }
    
    static public ListAdapter createTextAdapter(Context context){
        return new TextAdapter(context);
    }
    
    public void setAdapter(ListAdapter adapter){
        mAdapter = adapter;
        mContentList.setAdapter(adapter);
    }
}
