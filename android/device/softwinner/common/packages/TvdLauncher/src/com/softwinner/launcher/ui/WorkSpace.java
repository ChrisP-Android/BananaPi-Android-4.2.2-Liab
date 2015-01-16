package com.softwinner.launcher.ui;


import com.softwinner.launcher.ActivityRuntimeInterface;
import com.softwinner.launcher.ApplicationInfo;
import com.softwinner.launcher.FolderInfo;
import com.softwinner.launcher.IAllAppsView;
import com.softwinner.launcher.IStatusBar;
import com.softwinner.launcher.ItemInfo;
import com.softwinner.launcher.Launcher;
import com.softwinner.launcher.LauncherAppWidgetInfo;
import com.softwinner.launcher.LauncherApplication;
import com.softwinner.launcher.LauncherModel;
import com.softwinner.launcher.R;
import com.softwinner.launcher.ui.OptionMenu.TextAdapter;

import android.app.Dialog;
import android.appwidget.AppWidgetProviderInfo;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.res.Configuration;
import android.content.res.Resources;
import android.graphics.Rect;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.net.Uri;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.os.Bundle;
import android.os.SystemClock;
import android.os.SystemProperties;
import android.util.AttributeSet;
import android.util.Log;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.view.WindowManager;
import android.view.animation.AnimationUtils;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemSelectedListener;
import android.widget.ArrayAdapter;
import android.widget.BaseAdapter;
import android.widget.ListAdapter;
import android.widget.ListView;
import android.widget.RelativeLayout;
import android.widget.TextView;
import android.widget.Toast;

import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.preference.PreferenceManager;
import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Random;

import java.util.ArrayList;
import java.util.HashMap;

public class WorkSpace extends RelativeLayout 
            implements LauncherModel.Callbacks,ActivityRuntimeInterface,IAllAppsView.Watcher{
    class WidgetAdapter extends BaseAdapter{
        ArrayList<LauncherAppWidgetInfo> mWidgetsInfo = new ArrayList<LauncherAppWidgetInfo>();
         @Override
        public int getCount() {
            // TODO Auto-generated method stub
            return mWidgetsInfo.size();
        }
        @Override
        public LauncherAppWidgetInfo getItem(int id) {
            // TODO Auto-generated method stub
            return mWidgetsInfo.get(id);
        }
        public void setWidgetInfo(ArrayList<ItemInfo> items){
            for(int i=0;i<items.size();i++){
                if(items.get(i) instanceof LauncherAppWidgetInfo){
                    LauncherAppWidgetInfo widgetInfo = (LauncherAppWidgetInfo) items.get(i);
                    mWidgetsInfo.add(widgetInfo);
                }
            }
        }
        @Override
        public long getItemId(int position) {
            // TODO Auto-generated method stub
            return position;
        }
        @Override
        public View getView(int position, View convert, ViewGroup parent) {
            if(convert == null || convert.findViewById(R.id.widget_title) == null){
                convert = LayoutInflater.from(getContext()).inflate(R.layout.del_widget_dialog_item, null);
            }
            TextView tv = (TextView) convert.findViewById(R.id.widget_title);
            CharSequence str;
            try{
                str = "".equals(mWidgetsInfo.get(position).title) ? mContext.getString(R.string.unknown):
                        mWidgetsInfo.get(position).title;
            }catch(Exception e){
                
            }finally{
                str = mContext.getString(R.string.unknown);
            }
            tv.setText(str);
            return convert;
        }            
    };
    
    class WidgetOptionMenu extends OptionMenu{
        public WidgetOptionMenu(Context context) {
            super(context);
        }
        @Override
        public void dismiss(){
            mWidgetsLayout.disableDrawRect();
            super.dismiss();
        }
    }
    
    private final String TAG = "Laucher.MainViewContainer";
    
    
    //UI widgets
    private IAllAppsView      mAllAppsView;
    private IAllAppsView      mFavoritesAppsView;
    private CellLayout        mWidgetsLayout;    
    private QuicklyStartupBar mQsb;
    private StatusBar         mStatusBar;
    private View              mAddWidgetsBtn;
    private View              mDelWidgetsBtn;
    private View              mWallpaperBtn;
    private DeviceInfoPanel mDeviceInfoPanel;
    
    Launcher          mLauncher;
    
    private static boolean DEBUG_WIDGETS = false;

    public WorkSpace(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

	//added by JM.
	private static final char[] DEVICE_LABELS = {'0','1','2','3','4','5','6','7','8','9',
												 'a','b','c','d','e','f','g','h','i','j',
												 'k','l','m','n','o','p','q','r','s','t',
												 'u','v','w','x','y','z'};
	
	private static final String getPrivateId(){
		Random rand = new Random();
		char[] chars = new char[4];
		for(int i = 0; i < 4; i++){
			chars[i] = DEVICE_LABELS[rand.nextInt(DEVICE_LABELS.length - 1)];
		}
		return String.valueOf(chars);
	}
	
	private static final String DEVICE_TITLE = SystemProperties.get("persist.sys.device_name", "MiniMax");
	private static final String DEVICE_NAME = DEVICE_TITLE + "-" +getPrivateId();
	
	public String getDeviceId(){
		SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(mContext);
		String devId = prefs.getString("deviceid", null);
		if(devId == null){
			devId = DEVICE_NAME;
			Editor editor = prefs.edit();
			editor.putString("deviceid", DEVICE_NAME);
			editor.commit();
			SystemProperties.set("persist.sys.device_name", devId);
		}
//		Log.e(TAG, "########getDeviceId()..devId = " + devId);
		return devId;
	}

    @Override
    protected void onFinishInflate() {
        super.onFinishInflate();
        mDeviceInfoPanel = (DeviceInfoPanel)findViewById(R.id.device_info);
				mDeviceInfoPanel.setDeviceName(getDeviceId());
					WifiManager wifiManager = (WifiManager) mContext.getSystemService(Context.WIFI_SERVICE);
					WifiInfo wifiInfo = wifiManager.getConnectionInfo();
				//mDeviceInfoPanel.setWlanName(wifiInfo != null? removeDoubleQuotes(wifiInfo.getSSID()): "");
				String wlanName = mContext.getString(R.string.disconnected);
                                 if(wifiInfo != null
                                                 && !"0x".equals(removeDoubleQuotes(wifiInfo.getSSID()))
                                                 && !removeDoubleQuotes(wifiInfo.getSSID()).equals("<unknown ssid>")) {
                                         wlanName = removeDoubleQuotes(wifiInfo.getSSID());
                                 }
				 mDeviceInfoPanel.setWlanName(wlanName);    
		
        mWidgetsLayout = (CellLayout)findViewById(R.id.widgets);
        mQsb           = (QuicklyStartupBar)findViewById(R.id.quickly_start_bar);
        mAllAppsView   = (IAllAppsView)findViewById(R.id.all_apps_view);        
        mFavoritesAppsView = (IAllAppsView)findViewById(R.id.apps_favorites_view);
        mStatusBar = (StatusBar)findViewById(R.id.statusbar);
        mAllAppsView.setWatcher(this);
        
        onAppViewFocusChange(false);
    }
    /**
     * Adds the specified child in the specified screen. The position and dimension of
     * the child are defined by x, y, spanX and spanY.
     *
     * @param child The child to add in one of the workspace's screens.
     * @param x The X position of the child in the screen's grid.
     * @param y The Y position of the child in the screen's grid.
     * @param spanX The number of cells spanned horizontally by the child.
     * @param spanY The number of cells spanned vertically by the child.
     */
    public void addInScreen(View child, int x, int y, int spanX, int spanY) {
        addInScreen(child, x, y, spanX, spanY, false);
    }

    /**
     * Adds the specified child in the specified screen. The position and dimension of
     * the child are defined by x, y, spanX and spanY.
     *
     * @param child The child to add in one of the workspace's screens.
     * @param x The X position of the child in the screen's grid.
     * @param y The Y position of the child in the screen's grid.
     * @param spanX The number of cells spanned horizontally by the child.
     * @param spanY The number of cells spanned vertically by the child.
     * @param insert When true, the child is inserted at the beginning of the children list.
     */
    public void addInScreen(View child , int x, int y, int spanX, int spanY, boolean insert) {
        final CellLayout group = mWidgetsLayout;
        CellLayout.LayoutParams lp = (CellLayout.LayoutParams) child.getLayoutParams();
        if (lp == null) {
            lp = new CellLayout.LayoutParams(x, y, spanX, spanY);
        } else {
            lp.cellX = x;
            lp.cellY = y;
            lp.cellHSpan = spanX;
            lp.cellVSpan = spanY;
        }
        group.addView(child, insert ? 0 : -1, lp);
    }
    
    public void addFocusables(ArrayList<View> views, int direction, int focusableMode){
        if(!isAllAppsVisible()&&!isAppsFavoritesVisible()){
            mQsb.addFocusables(views, direction);
            mWidgetsLayout.addFocusables(views, direction);
        }
    }
    
    @Override
    public View focusSearch(View view, int direction){
        View v = super.focusSearch(direction);
        if(v == null && !isAllAppsVisible() && !isAppsFavoritesVisible()){
            if(direction == View.FOCUS_DOWN){
                mQsb.requestFocus();
            }else if(direction == View.FOCUS_UP && mWidgetsLayout.getChildCount() > 0){
                mWidgetsLayout.requestFocus();
            }
            return super.focusSearch(view, direction);
        }
        return v;        
    }

    /**
     * Key event
     * 
     */
    @Override
    public boolean dispatchKeyEvent(KeyEvent event) {
        if(event.getKeyCode() == KeyEvent.KEYCODE_MENU){
            return event.dispatch(this, null, null);
        }          
        return super.dispatchKeyEvent(event);
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        if(keyCode == KeyEvent.KEYCODE_MENU){
            if(isAllAppsVisible()){
                mAllAppsView.onCreateMenu();
            }else if(isAppsFavoritesVisible()){
                mFavoritesAppsView.onCreateMenu();
            }else{
                createMainMenu();
            }
        }
        return super.onKeyDown(keyCode, event);
    }
    
    public void setLauncher(Launcher launcher){
        mLauncher = launcher;
        mQsb.setMainLayout(this);
    }
    
    public CellLayout getWidgetLayout(){
        return mWidgetsLayout;
    }

    public void onAppViewFocusChange(boolean appVisiable){
        if(appVisiable){
            mQsb.setFocusable(false);
            mWidgetsLayout.setFocusable(false);
        }else{            
            mQsb.setFocusable(true);
            mWidgetsLayout.setFocusable(true);
            mQsb.requestFocus();
        }        
    }
    
    private void showDesktopByAnimated(boolean isShow){        
        if(isShow){
            mQsb.startAnimation(
                    AnimationUtils.loadAnimation(getContext(), R.anim.all_apps_2d_fade_in));
            mWidgetsLayout.startAnimation(
                    AnimationUtils.loadAnimation(getContext(), R.anim.all_apps_2d_fade_in));
        }else{
            mQsb.startAnimation(
                    AnimationUtils.loadAnimation(getContext(), R.anim.all_apps_2d_fade_out));
            mWidgetsLayout.startAnimation(
                    AnimationUtils.loadAnimation(getContext(), R.anim.all_apps_2d_fade_out));
        }
    }
    
    /**
     * -----------------------------------------------------
     * API  for  view
     */
    
    public void showAllApps(boolean animated) {
        mAllAppsView.zoom(1.0f, animated);

        ((View) mAllAppsView).setFocusable(true);
        ((View) mAllAppsView).requestFocus();
        if(animated) showDesktopByAnimated(false);
    }
    public void showFavoritesApps(boolean animated){
        mFavoritesAppsView.zoom(1.0f, animated);
        mFavoritesAppsView.setWatcher(this);
        ((View) mFavoritesAppsView).setFocusable(true);
        ((View) mFavoritesAppsView).requestFocus();
        if(animated) showDesktopByAnimated(false);
    }
    
    public void closeAllApps(boolean animated) {
        if (mAllAppsView.isVisible()) {
            mAllAppsView.zoom(0.0f, animated);
            ((View)mAllAppsView).setFocusable(false);
            if(animated) showDesktopByAnimated(true);
        }        
    }
    
    public void closeAppsFavorites(boolean animated){
        if (mFavoritesAppsView.isVisible()) {
            mFavoritesAppsView.zoom(0.0f, animated);
            ((View)mFavoritesAppsView).setFocusable(false);
            if(animated) showDesktopByAnimated(true);
        }        
    }
    
    
    /**
     * -----------------------------------------------------------------
     */

    @Override
    public boolean setLoadOnResume() {
        // TODO Auto-generated method stub
        return false;
    }

    @Override
    public int getCurrentWorkspaceScreen() {
        // TODO Auto-generated method stub
        return 0;
    }

    @Override
    public void startBinding() {
        // TODO Auto-generated method stub
        mWidgetsLayout.removeAllViewsInLayout();
    }

    @Override
    public void bindItems(ArrayList<ItemInfo> shortcuts, int start, int end) {
        // TODO Auto-generated method stub
        
    }

    @Override
    public void bindFolders(HashMap<Long, FolderInfo> folders) {
        // TODO Auto-generated method stub
        
    }

    @Override
    public void finishBindingItems() {
        // TODO Auto-generated method stub
        
    }

    @Override
    public void bindAppWidget(LauncherAppWidgetInfo item) {
        setLoadOnResume();
        
        final long start = DEBUG_WIDGETS ? SystemClock.uptimeMillis() : 0;
        if (DEBUG_WIDGETS) {
            Log.d(TAG, "bindAppWidget: " + item);
        }

        final int appWidgetId = item.appWidgetId;
        final AppWidgetProviderInfo appWidgetInfo = 
            mLauncher.getAppWidgetManager().getAppWidgetInfo(appWidgetId);
        if (DEBUG_WIDGETS) {
            Log.d(TAG, "bindAppWidget: id=" + item.appWidgetId + 
                    " belongs to component " + appWidgetInfo.provider);
        }

        item.hostView = mLauncher.getAppWidgetHost().createView(mLauncher, appWidgetId, appWidgetInfo);

        item.hostView.setAppWidget(appWidgetId, appWidgetInfo);
        item.hostView.setTag(item);

        addInScreen(item.hostView, item.cellX,
                item.cellY, item.spanX, item.spanY, false);

        requestLayout();

        if(!mLauncher.mDesktopItems.contains(item)){
            mLauncher.mDesktopItems.add(item);
        }
        if (DEBUG_WIDGETS) {
            Log.d(TAG, "bound widget id="+item.appWidgetId+" in "
                    + (SystemClock.uptimeMillis()-start) + "ms");
        }
    }

    @Override
    public CellLayout getCellLayout(int reservation) {
        return mWidgetsLayout;
    }

    @Override
    public void bindAllApplications(ArrayList<ApplicationInfo> apps) {
        mAllAppsView.setApps(apps);
        mFavoritesAppsView.setApps(apps);
    }

    @Override
    public void bindAppsAdded(ArrayList<ApplicationInfo> apps) {
        mAllAppsView.addApps(apps);
        mFavoritesAppsView.addApps(apps);
    }

    @Override
    public void bindAppsUpdated(ArrayList<ApplicationInfo> apps) {
        mAllAppsView.updateApps(apps);
        mFavoritesAppsView.updateApps(apps);
    }

    @Override
    public void bindAppsRemoved(ArrayList<ApplicationInfo> apps, boolean permanent) {
        mAllAppsView.removeApps(apps);
        mFavoritesAppsView.removeApps(apps);
    }
    
    public boolean isAppsFavoritesVisible(){
        return (mFavoritesAppsView != null)?mFavoritesAppsView.isVisible():false;
    }

    @Override
    public boolean isAllAppsVisible() {
        // TODO Auto-generated method stub
        return (mAllAppsView != null) ? mAllAppsView.isVisible() : false;
    }
    
    /**
     * ---------------------------------------------------------------------------
     */

//    @Override
//    public void onResume() {
//        //mStatusBar.registerReceiver();
//    }

    @Override
    public void onDestroy() {
        //mStatusBar.unregisterReceiver();
    }
    
    @Override
    public void onNewIntent(Intent intent) {
        Log.d(TAG, ""+intent);
        if(intent.getCategories()!= null && 
                intent.getCategories().contains(Launcher.LAUNCHER_CATEGORY_START_APP)){
        	closeAppsFavorites(false);
            showAllApps(false);
        }else{
            closeAllApps(false);
            closeAppsFavorites(false);
        }
    }

    @Override
    public void onBackPressed() {
        if (isAllAppsVisible()) {
            closeAllApps(true);
        }else if(isAppsFavoritesVisible()){
            closeAppsFavorites(true);
        }
    }

    @Override
    public void onSaveInstanceState(Bundle outState) {

    }
    @Override
    public void onConfigurationChanged(Configuration newConfig){
        mQsb.reflashLocal();
    }
    
    public void createMainMenu(){
        final Context context = getContext();
        Resources r = context.getResources();
        
        final int height = getHeight() - 
                r.getDimensionPixelSize(R.dimen.widgets_layout_shortaxis_start_padding) - 
                r.getDimensionPixelSize(R.dimen.widgets_layout_shortaxis_end_padding);
        final int width = r.getDimensionPixelSize(R.dimen.widgets_layout_longaxis_start_padding) - 20;
        final int y =  r.getDimensionPixelSize(
                R.dimen.widgets_layout_shortaxis_start_padding) - getHeight()/2 + height/2;
        int x = 99999;
        final OptionMenu om = new OptionMenu(context);
        final OptionMenu.TextAdapter adapter = (TextAdapter) OptionMenu.createTextAdapter(context);
        adapter.mTextIds.add(R.string.add_widgets);
        adapter.mTextIds.add(R.string.del_widget);
        adapter.mTextIds.add(R.string.set_the_wallpaper);
        om.setAdapter(adapter);
        om.setTitle(R.string.main_option_menu_title);
        om.setClickCallback(new AdapterView.OnItemClickListener() {
            @Override
            public void onItemClick(AdapterView<?> adapterView, View child, int position, long id) {
                Integer item = (Integer) adapterView.getItemAtPosition(position);
                if(item.equals(R.string.add_widgets)){
                    mLauncher.pickAppWidget();
                    om.dismiss();
                }else if(item.equals(R.string.del_widget)){
                    final OptionMenu widgetMenu = new WidgetOptionMenu(context);
                    
                    //set widget delete dialog selected callback
                    widgetMenu.setSelectedCallback(new OnItemSelectedListener() {
                        @Override
                        public void onItemSelected(AdapterView<?> adapterView, View arg1, int position, long arg3) {
                            LauncherAppWidgetInfo item = 
                                (LauncherAppWidgetInfo) adapterView.getItemAtPosition(position);
                            mWidgetsLayout.drawCellRect(item.cellX, item.cellY, item.spanX, item.spanY);
                        }
                        @Override
                        public void onNothingSelected(AdapterView<?> arg0) {
                            mWidgetsLayout.disableDrawRect();
                    }});
                    
                    //set widget delete dialog item click callback
                    widgetMenu.setClickCallback(new AdapterView.OnItemClickListener() {
                        @Override
                        public void onItemClick(AdapterView<?> adapterView, View child, int position, long id) {
                            LauncherAppWidgetInfo item = (LauncherAppWidgetInfo) adapterView.getItemAtPosition(position);
                            mLauncher.removeAppWidget(item);
                            mWidgetsLayout.removeViewFromCell(item.cellX,item.cellY);
                            widgetMenu.dismiss();
                        }
                    });
                    final WidgetAdapter adapter = new WidgetAdapter();
                    adapter.setWidgetInfo(mLauncher.mDesktopItems);
                    widgetMenu.setTitle(R.string.del_widget_dialog_instruct);
                    widgetMenu.setAdapter(adapter);
                    widgetMenu.show(-99999, y, width, height, true);
                }else if(item.equals(R.string.set_the_wallpaper)){
                    final Intent pickWallpaper = new Intent(Intent.ACTION_SET_WALLPAPER);
                    Intent chooser = Intent.createChooser(pickWallpaper,
                    mContext.getText(R.string.chooser_wallpaper));
                    mLauncher.startActivitySafely(chooser,TAG);
                    om.dismiss();
                }
            }
        });
        om.show(x,y,width,height,false);
    }

    @Override
    public void onRestoreInstanceState(Bundle savedInstanceState) {
    }

    @Override
    public void zoomed(float zoom) {
        if(isAllAppsVisible() || isAppsFavoritesVisible()){
        	mDeviceInfoPanel.setVisibility(View.GONE);
            mQsb.setVisibility(View.GONE);
            mWidgetsLayout.setVisibility(View.GONE);
            onAppViewFocusChange(true);
            mStatusBar.Zoomed(true);
        }else{
        	mDeviceInfoPanel.setVisibility(View.VISIBLE);
            mQsb.setVisibility(View.VISIBLE);
            mWidgetsLayout.setVisibility(View.VISIBLE);
            onAppViewFocusChange(false);
            mStatusBar.Zoomed(false);
        }
    }
        
		private BroadcastReceiver mReceiver = new BroadcastReceiver(){
	
			@Override
			public void onReceive(Context context, Intent intent) {
					ConnectivityManager connManager = (ConnectivityManager) mContext.getSystemService(Context.CONNECTIVITY_SERVICE);
					NetworkInfo networkInfo = connManager.getNetworkInfo(ConnectivityManager.TYPE_WIFI);
					WifiManager wifiManager = (WifiManager) mContext.getSystemService(Context.WIFI_SERVICE);
					WifiInfo wifiInfo = wifiManager.getConnectionInfo();
					
					if(intent != null){
		        String action = intent.getAction();
						if(WifiManager.NETWORK_STATE_CHANGED_ACTION.equals(action)){
		        	networkInfo = (NetworkInfo) intent.getParcelableExtra(WifiManager.EXTRA_NETWORK_INFO);
		        }
					}
					
					if(networkInfo != null && mDeviceInfoPanel != null) {
						mDeviceInfoPanel.setDeviceName(getDeviceId());
						switch(networkInfo.getState()){
						case CONNECTED:
							mDeviceInfoPanel.setWlanName(wifiInfo != null? removeDoubleQuotes(wifiInfo.getSSID()): "");
							break;
						case CONNECTING:
							mDeviceInfoPanel.setWlanName(getContext().getString(R.string.connecting));
							break;
						default:
							mDeviceInfoPanel.setWlanName(getContext().getString(R.string.disconnected));
							break;
						}
					}
			}
			
		};
		    
	@Override
	public void onResume() {
		IntentFilter filter = new IntentFilter();
		filter.addAction(WifiManager.NETWORK_STATE_CHANGED_ACTION);
		getContext().registerReceiver(mReceiver, filter);
	}

	@Override
	public void onPause() {
		getContext().unregisterReceiver(mReceiver);
	}
	
  static String removeDoubleQuotes(String string) {
    int length = string.length();
    if ((length > 1) && (string.charAt(0) == '"')
            && (string.charAt(length - 1) == '"')) {
        return string.substring(1, length - 1);
    }
    return string;
	}

}
