/*
 * Copyright (C) 2008 The Android Open Source Project
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

package com.softwinner.launcher.ui;

import com.softwinner.launcher.IAllAppsView;
import com.softwinner.launcher.ApplicationInfo;
import com.softwinner.launcher.LauncherApplication;
import com.softwinner.launcher.LauncherModel;

import android.app.Dialog;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.res.Resources;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.net.Uri;
import android.util.AttributeSet;
import android.util.Log;
import android.view.KeyEvent;
import android.view.ViewGroup;
import android.view.LayoutInflater;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.view.animation.AnimationUtils;
import android.widget.AdapterView;
import android.widget.TextView;
import android.widget.ArrayAdapter;
import android.widget.GridView;
import android.widget.RelativeLayout;

import java.util.ArrayList;
import java.util.Collections;

import com.softwinner.launcher.*;


public class AllApps2D
        extends RelativeLayout
        implements IAllAppsView,
        AdapterView.OnItemClickListener,
        View.OnKeyListener{

    private static final String TAG = "TvdLauncher";
    private static final String MODULE = "AllApp2D";
    private static final Boolean LOGD = false;

    protected Watcher mWatcher;

    protected GridView mGrid;
    
    private float mZoom;

    protected ArrayList<ApplicationInfo> mAllAppsList = new ArrayList<ApplicationInfo>();

    protected AppsAdapter mAppsAdapter;
    
    private View mAddBtn;
    private View mUninstallBtn;
    protected Dialog mMenuDialog;
    private View.OnClickListener mMenuEvent = new View.OnClickListener(){
        @Override
        public void onClick(View v) {
            View selected = mGrid.getSelectedView();
            if(selected == null){
                return;
            }
            int position = mGrid.getPositionForView(selected);
            ApplicationInfo app = (ApplicationInfo) mGrid.getItemAtPosition(position);
            if(v.equals(mAddBtn)){
                LauncherModel.setFavoritesApp(mContext ,app.componentName, true);
            }else if(v.equals(mUninstallBtn)){
                Uri packageURI = Uri.parse("package:" + app.componentName.getPackageName());
                Intent uninstallIntent = new Intent(Intent.ACTION_DELETE, packageURI);
                LauncherApplication.getLauncher().startActivity(uninstallIntent);
            } 
            mMenuDialog.dismiss();
        }
    };

    
    /**
     * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
     * Internal Class
     * Class name   : AppsAdapter
     * Public Method:
     *     Name      :
     *         public View getView(int position, View convertView, ViewGroup parent)
     *     Descriptor:To create text view with icon for the application 
     */

    public class AppsAdapter extends ArrayAdapter<ApplicationInfo> {
        private final LayoutInflater mInflater;

        public AppsAdapter(Context context, ArrayList<ApplicationInfo> apps) {
            super(context, 0, apps);
            mInflater = LayoutInflater.from(context);
        }

        @Override
        public View getView(int position, View convertView, ViewGroup parent) {
            final ApplicationInfo info = getItem(position);

            if (convertView == null) {
                convertView = mInflater.inflate(R.layout.application_boxed, parent, false);
            }

            final TextView textView = (TextView) convertView;
            if (LOGD) {
                Log.d(TAG, MODULE + "icon bitmap = " + info.iconBitmap
                    + " density = " + info.iconBitmap.getDensity());
            }
            info.iconBitmap.setDensity(Bitmap.DENSITY_NONE);
            textView.setCompoundDrawablesWithIntrinsicBounds(null, new BitmapDrawable(info.iconBitmap), null, null);
            textView.setText(info.title);

            return convertView;
        }
    }
    /**
     * Internal Class
     * -------------------------------------------------------------------------
     */

    public AllApps2D(Context context, AttributeSet attrs) {
        super(context, attrs);
        setVisibility(View.GONE);
        setSoundEffectsEnabled(false);

        mAppsAdapter = new AppsAdapter(getContext(), mAllAppsList);
        mAppsAdapter.setNotifyOnChange(false);
    }
    
    public AllApps2D(Context context, AttributeSet attrs, int defStyle) {
        this(context, attrs);
    }

    @Override
    protected void onFinishInflate() {
        try {
            mGrid = (GridView)findViewWithTag("all_apps_2d_grid");
            if (mGrid == null) throw new Resources.NotFoundException();
            mGrid.setOnItemClickListener(this);
            //mGrid.setBackgroundResource(R.drawable.background);
            mGrid.setCacheColorHint(Color.TRANSPARENT);
        } catch (Resources.NotFoundException e) {
            Log.e(TAG, MODULE + "Can't find necessary layout elements for AllApps2D");
        }
    }
    
    @Override
    protected void onFocusChanged(boolean gainFocus, int direction, android.graphics.Rect prev) {
        if (gainFocus) {
            mGrid.requestFocus();
        }
    }
    
    @Override
    public boolean onKey(View v, int keyCode, KeyEvent event) {
        if (!isVisible()) return false;

        switch (keyCode) {
            case KeyEvent.KEYCODE_BACK:
                break;
            default:
                return false;
        }

        return true;
    }
    
    @Override
    protected void onAnimationEnd() {
        if (!isVisible()) {
            setVisibility(View.GONE);
            mGrid.setAdapter(null);
            mZoom = 0.0f;
        } else {
            mZoom = 1.0f;
        }
        mWatcher.zoomed(mZoom);
    }
    
    
    /**
     * ++++++++++++++++++++++++++++++++++++++++++++++++++++++
     * AllAppsView Call back interface
     * 
     *     function name:public void setLauncher(Launcher launcher)
     *     description:
     *     
     *     function name:public boolean isVisible()
     *     description:return whether the app view is visible
     *     
     *     function name:public boolean isOpaque()
     *     description:
     *     
     *     function name:public void setApps(ArrayList<ApplicationInfo> list)
     *     description:set the app from a full copy of AllAppList.
     *     
     *     function name:public void addApps(ArrayList<ApplicationInfo> list)
     *     description:set the app from a added app copy of AllAppList.
     *     
     *     function name:public void updateApps(ArrayList<ApplicationInfo> list
     *     description:set the app from a updated app copy of AllAppList.
     *     
     *     function name:public void zoom(float zoom, boolean animate)
     *     description:start app view
     */
    @Override
    public void setWatcher(Watcher watcher) {
        mWatcher = watcher;
    }
    
    public boolean isVisible() {
        return mZoom > 0.001f;
    }

    @Override
    public boolean isOpaque() {
        return mZoom > 0.999f;
    }

    public void setApps(ArrayList<ApplicationInfo> list) {
        mAllAppsList.clear();
        addApps(list);
    }

    public void addApps(ArrayList<ApplicationInfo> list) {
        final int N = list.size();

        for (int i=0; i<N; i++) {
            final ApplicationInfo item = list.get(i);
            int index = Collections.binarySearch(mAllAppsList, item,
                    LauncherModel.APP_NAME_COMPARATOR);
            if (index < 0) {
                index = -(index+1);
            }
            mAllAppsList.add(index, item);
            if(LOGD) list.get(i).toString();
        }
        mAppsAdapter.notifyDataSetChanged();
    }

    public void removeApps(ArrayList<ApplicationInfo> list) {
        final int N = list.size();
        for (int i=0; i<N; i++) {
            final ApplicationInfo item = list.get(i);
            int index = findAppByComponent(mAllAppsList, item);
            if (index >= 0) {
                mAllAppsList.remove(index);
                //if(LOGD) item.dumpItemInfo(TAG, LOGD);
            } else {
                Log.w(TAG, MODULE + "couldn't find a match for item \"" + item + "\"");
                // Try to recover.  This should keep us from crashing for now.
            }
        }
        mAppsAdapter.notifyDataSetChanged();
    }

    public void updateApps(ArrayList<ApplicationInfo> list) {
        // Just remove and add, because they may need to be re-sorted.
        removeApps(list);
        addApps(list);
    }   
    
    private static int findAppByComponent(ArrayList<ApplicationInfo> list, ApplicationInfo item) {
        ComponentName component = item.intent.getComponent();
        final int N = list.size();
        for (int i=0; i<N; i++) {
            ApplicationInfo x = list.get(i);
            if (x.intent.getComponent().equals(component)) {
                return i;
            }
        }
        return -1;
    }
    
    /**
     * Zoom to the specifed level.
     *
     * @param zoom [0..1] 0 is hidden, 1 is open
     */
    public void zoom(float zoom, boolean animate) {
        cancelLongPress();

        mZoom = zoom;

        if (isVisible()) {
            getParent().bringChildToFront(this);
            setVisibility(View.VISIBLE);
            mGrid.setAdapter(mAppsAdapter);
            if (animate) {
                startAnimation(AnimationUtils.loadAnimation(getContext(), R.anim.all_apps_2d_fade_in));
            } else {
                onAnimationEnd();
            }
        } else {
            if (animate) {
                startAnimation(AnimationUtils.loadAnimation(getContext(), R.anim.all_apps_2d_fade_out));
            } else {
                onAnimationEnd();
            }
        }
    }

    public void dumpState() {
        
    }
    
    public void surrender() {
        
    }
    
    /**
     * -------------------------------------------------------------------
     */

    @Override
    public void onItemClick(AdapterView<?> parent, View v, int position, long id) {
        if(LOGD) Log.d(TAG,MODULE + "position=" + position + " " + "id=" + id);
        ApplicationInfo app = (ApplicationInfo) parent.getItemAtPosition(position);
        LauncherApplication.getLauncher().startActivitySafely(app.intent, TAG);
    }
    
    public Dialog createMenuDialog(Context context,ApplicationInfo app){
        Dialog dialog = new Dialog(context,R.style.menu_dialog);
        Window win = dialog.getWindow();
        WindowManager.LayoutParams params = new WindowManager.LayoutParams();
        params.y = 99999;
        params.x = 0;
        win.setAttributes(params);
        dialog.setCanceledOnTouchOutside(true);
        LayoutInflater inflater = dialog.getLayoutInflater();
        View content = inflater.inflate(R.layout.all_apps_menu, null);
        mAddBtn = content.findViewById(R.id.add_to_favorites);
        mUninstallBtn = content.findViewById(R.id.uninstall_app);
        if(mAddBtn != null){
            mAddBtn.setOnClickListener(mMenuEvent);
        }
        if(mUninstallBtn != null){
            mUninstallBtn.setOnClickListener(mMenuEvent);
        }
        dialog.setContentView(content);
        return dialog;
    }
    
    @Override
    public void onCreateMenu() {
        // TODO Auto-generated method stub
        View selected = mGrid.getSelectedView();
        if(selected == null){
            return;
        }
        int position = mGrid.getPositionForView(selected);
        ApplicationInfo app = (ApplicationInfo) mGrid.getItemAtPosition(position);
        mMenuDialog = createMenuDialog(getContext(),app);
        mMenuDialog.show();
        Window win = mMenuDialog.getWindow();
        win.setWindowAnimations(R.style.menuDialogWindowAnim);
    }
}
