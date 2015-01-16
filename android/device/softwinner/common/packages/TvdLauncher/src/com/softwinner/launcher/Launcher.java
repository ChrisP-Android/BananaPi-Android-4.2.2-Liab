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

package com.softwinner.launcher;

import android.app.Activity;

import android.app.WallpaperManager;
import android.content.ActivityNotFoundException;
import android.content.BroadcastReceiver;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.res.Configuration;
import android.database.ContentObserver;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.Handler;
import android.os.SystemClock;
import android.text.method.TextKeyListener;
import android.util.Log;
import android.view.Display;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;
import android.widget.Toast;
import android.appwidget.AppWidgetManager;
import android.appwidget.AppWidgetProviderInfo;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.io.DataOutputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.DataInputStream;
import java.lang.ref.WeakReference;

import com.softwinner.launcher.R;
import com.softwinner.launcher.ui.CellLayout;
import com.softwinner.launcher.ui.CellLayout.CellInfo;
import com.softwinner.launcher.ui.DeviceInfoPanel;
import com.softwinner.launcher.ui.WorkSpace;
/**
 * Default launcher application.
 */
public final class Launcher extends Activity{
    static final String TAG = "Launcher";
    static final boolean LOGD = true;

    static final boolean PROFILE_STARTUP = false;
    static final boolean DEBUG_WIDGETS = false;
    static final boolean DEBUG_USER_INTERFACE = false;

    private static final int MENU_GROUP_ADD = 1;
    private static final int MENU_GROUP_WALLPAPER = MENU_GROUP_ADD + 1;

    private static final int MENU_ADD = Menu.FIRST + 1;
    private static final int MENU_MANAGE_APPS = MENU_ADD + 1;
    private static final int MENU_WALLPAPER_SETTINGS = MENU_MANAGE_APPS + 1;
    private static final int MENU_SEARCH = MENU_WALLPAPER_SETTINGS + 1;
    private static final int MENU_NOTIFICATIONS = MENU_SEARCH + 1;
    private static final int MENU_SETTINGS = MENU_NOTIFICATIONS + 1;

    private static final int REQUEST_CREATE_SHORTCUT = 1;
    private static final int REQUEST_CREATE_LIVE_FOLDER = 4;
    private static final int REQUEST_CREATE_APPWIDGET = 5;
    private static final int REQUEST_PICK_APPLICATION = 6;
    private static final int REQUEST_PICK_SHORTCUT = 7;
    private static final int REQUEST_PICK_LIVE_FOLDER = 8;
    private static final int REQUEST_PICK_APPWIDGET = 9;
    private static final int REQUEST_PICK_WALLPAPER = 10;

    static final String EXTRA_SHORTCUT_DUPLICATE = "duplicate";

    static final int SCREEN_COUNT = 1;
    static final int DEFAULT_SCREEN = 1;
    static final int NUMBER_CELLS_X = 6;
    static final int NUMBER_CELLS_Y = 4;

    static final int DIALOG_CREATE_SHORTCUT = 1;
    static final int DIALOG_RENAME_FOLDER = 2;

    private static final String PREFERENCES = "launcher.preferences";

    // Type: int
    private static final String RUNTIME_STATE_CURRENT_SCREEN = "launcher.current_screen";
    // Type: boolean
    private static final String RUNTIME_STATE_ALL_APPS_FOLDER = "launcher.all_apps_folder";
    // Type: long
    private static final String RUNTIME_STATE_USER_FOLDERS = "launcher.user_folder";
    // Type: int
    private static final String RUNTIME_STATE_PENDING_ADD_SCREEN = "launcher.add_screen";
    // Type: int
    private static final String RUNTIME_STATE_PENDING_ADD_CELL_X = "launcher.add_cellX";
    // Type: int
    private static final String RUNTIME_STATE_PENDING_ADD_CELL_Y = "launcher.add_cellY";
    // Type: int
    private static final String RUNTIME_STATE_PENDING_ADD_SPAN_X = "launcher.add_spanX";
    // Type: int
    private static final String RUNTIME_STATE_PENDING_ADD_SPAN_Y = "launcher.add_spanY";
    // Type: int
    private static final String RUNTIME_STATE_PENDING_ADD_COUNT_X = "launcher.add_countX";
    // Type: int
    private static final String RUNTIME_STATE_PENDING_ADD_COUNT_Y = "launcher.add_countY";
    // Type: int[]
    private static final String RUNTIME_STATE_PENDING_ADD_OCCUPIED_CELLS = "launcher.add_occupied_cells";
    // Type: boolean
    private static final String RUNTIME_STATE_PENDING_FOLDER_RENAME = "launcher.rename_folder";
    // Type: long
    private static final String RUNTIME_STATE_PENDING_FOLDER_RENAME_ID = "launcher.rename_folder_id";

    static final int APPWIDGET_HOST_ID = 1024;

    private static final Object sLock = new Object();
    private static int sScreen = DEFAULT_SCREEN;

    private final BroadcastReceiver mCloseSystemDialogsReceiver
            = new CloseSystemDialogsIntentReceiver();
    private final ContentObserver mWidgetObserver = new AppWidgetResetObserver();

    private LayoutInflater mInflater;

    private AppWidgetManager mAppWidgetManager;
    private LauncherAppWidgetHost mAppWidgetHost;

    private CellLayout.CellInfo mAddItemCellInfo;
    private final int[] mCellCoordinates = new int[2];

    private IAllAppsView mAllAppsGrid;
    private IAllAppsView mAppsFavorites;

    private Bundle mSavedState;

    private boolean mWorkspaceLoading = true;

    private boolean mPaused = true;
    private boolean mRestoring;
    private boolean mWaitingForResult;
    private boolean mOnResumeNeedsLoad;

    private Bundle mSavedInstanceState;

    private LauncherModel mModel;
    
    private IconCache mIconCache;

    private static LocaleConfiguration sLocaleConfiguration = null;

    public ArrayList<ItemInfo> mDesktopItems = new ArrayList<ItemInfo>();
    private static HashMap<Long, FolderInfo> sFolders = new HashMap<Long, FolderInfo>();
    
    public static final String LAUNCHER_CATEGORY_START_APP = "com.softwinner.category.app";

    private WeakReference<ActivityRuntimeInterface> mView;
    /**
     * -------------------------------------------------------------------------
     * 1.life time affair
     */
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        
        if (PROFILE_STARTUP) {
            android.os.Debug.startMethodTracing("/sdcard/launcher");
        }
        
        //Get application instance
        mAppWidgetHost = new LauncherAppWidgetHost(this, APPWIDGET_HOST_ID);
        mAppWidgetHost.startListening();
        //Set Launcher model 
        mModel = LauncherApplication.getModel();
        LauncherApplication.setLauncher(this);
        //mModel.setCallbacks(this);
        //get the icon cache 
        mIconCache = LauncherApplication.getIconCache();
        //get the app widget manager
        mAppWidgetManager = AppWidgetManager.getInstance(this);
        //set the wall paper the suit self window
        setWallpaperDimension();
        
        //load base view to window
        setupViews();
        
        //Model start load workspace item & applist
        if (!mRestoring) {
            mModel.startLoader(this, true);
        }
        
        restoreState(savedInstanceState);
        
        //register the close system dialogs intent filters
        IntentFilter filter = new IntentFilter(Intent.ACTION_CLOSE_SYSTEM_DIALOGS);
        registerReceiver(mCloseSystemDialogsReceiver, filter);
        
        if (PROFILE_STARTUP) {
            android.os.Debug.stopMethodTracing();
        }
        
        List<AppWidgetProviderInfo> installed = mAppWidgetManager.getInstalledProviders();
        if(LOGD){
            for(AppWidgetProviderInfo widget:installed){
                int id[] = mAppWidgetManager.getAppWidgetIds(widget.provider);
                for(int i=0;i<id.length;i++){
                    Log.w(TAG, "" + id[i]);
                }
            }
        }
    }
    
    @Override
    protected void onResume() {
        super.onResume();
        mPaused = false;
        
        if (mRestoring || mOnResumeNeedsLoad ) {
            mWorkspaceLoading = true;
            mModel.startLoader(this, true);
            mRestoring = false;
            mOnResumeNeedsLoad = false;
        }
        ActivityRuntimeInterface callbacks = mView.get();
        if(callbacks != null){
            callbacks.onResume();
        }
    }
    
    @Override
    protected void onPause() {
        super.onPause();
        mPaused = true;
        
    }
    
    @Override
    protected void onDestroy() {
        try {
            mAppWidgetHost.stopListening();
        } catch (NullPointerException ex) {
            Log.w(TAG, "problem while stopping AppWidgetHost during Launcher destruction", ex);
        }

        TextKeyListener.getInstance().release();

        mModel.stopLoader();

        getContentResolver().unregisterContentObserver(mWidgetObserver);

        unregisterReceiver(mCloseSystemDialogsReceiver);
        ActivityRuntimeInterface callbacks = mView.get();
        if(callbacks != null){
            callbacks.onDestroy();
        }
        
        super.onDestroy();
    }
    
    /**
     * -----------------------------------------------------------------
     * 2.communicate between activity affair
     */
    @Override
    protected void onNewIntent(Intent intent) {
        if(LOGD)  Log.d(TAG,"receive a new intent " + intent);
        super.onNewIntent(intent);
        ActivityRuntimeInterface callbacks = mView!=null ? mView.get():null;
        if(callbacks != null){
            callbacks.onNewIntent(intent);
        }
    }   

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        if (resultCode == RESULT_OK) {
            switch(requestCode){
            case REQUEST_PICK_APPWIDGET:
                addAppWidget(data);
                break;
            case REQUEST_CREATE_APPWIDGET:
                completeAddAppWidget(data, mAddItemCellInfo);
                break;
            }
        }
    }
    
    /**
     * -----------------------------------------------------------------
     * 3.runtime instance state store
     */
    @Override
    protected void onSaveInstanceState(Bundle outState) {
        /*
        if (isAllAppsVisible()) {
            outState.putBoolean(RUNTIME_STATE_ALL_APPS_FOLDER, true);
        }

        if (mAddItemCellInfo != null && mAddItemCellInfo.valid && mWaitingForResult) {
            final CellLayout.CellInfo addItemCellInfo = mAddItemCellInfo;
            final CellLayout layout = (CellLayout) mMainLayout.getWidgetLayout();

            outState.putInt(RUNTIME_STATE_PENDING_ADD_SCREEN, addItemCellInfo.screen);
            outState.putInt(RUNTIME_STATE_PENDING_ADD_CELL_X, addItemCellInfo.cellX);
            outState.putInt(RUNTIME_STATE_PENDING_ADD_CELL_Y, addItemCellInfo.cellY);
            outState.putInt(RUNTIME_STATE_PENDING_ADD_SPAN_X, addItemCellInfo.spanX);
            outState.putInt(RUNTIME_STATE_PENDING_ADD_SPAN_Y, addItemCellInfo.spanY);
            outState.putInt(RUNTIME_STATE_PENDING_ADD_COUNT_X, layout.getCountX());
            outState.putInt(RUNTIME_STATE_PENDING_ADD_COUNT_Y, layout.getCountY());
            outState.putBooleanArray(RUNTIME_STATE_PENDING_ADD_OCCUPIED_CELLS,
                   layout.getOccupiedCells());
        }
        */
    }
    
    @Override
    protected void onRestoreInstanceState(Bundle savedInstanceState) {
        // Do not call super here
        mSavedInstanceState = savedInstanceState;
    }
    
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
        checkForLocaleChange();
        mModel.setAllAppsLoaded(false); // Set force load all apps list;
        mModel.startLoader(this, true); // Reload apps list
        ActivityRuntimeInterface callback = mView != null?mView.get():null;
        if(callback != null){
            callback.onConfigurationChanged(newConfig);
        }
    }
    
    @Override
    public Object onRetainNonConfigurationInstance() {
        // Flag the loader to stop early before switching
        //Log.d(TAG,"Rumtime" + mModel);
        //mModel.stopLoader();
        return Boolean.TRUE;
    }
    /**
     * --------------------------------------------------------------
     * 4.globle key event
     */
    @Override
    public void onBackPressed() {
        ActivityRuntimeInterface callback = mView !=null ? mView.get() : null;
        if(callback != null){
            callback.onBackPressed();
        }
    }
    
    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        return super.onCreateOptionsMenu(menu);
    }
    
    @Override
    public boolean onSearchRequested() {
        startSearch(null, false, null, true);
        return true;
    }
    
    /**
     * ----------------------------------------------------------------
     */

    private void checkForLocaleChange() {
        if (sLocaleConfiguration == null) {
            new AsyncTask<Void, Void, LocaleConfiguration>() {
                @Override
                protected LocaleConfiguration doInBackground(Void... unused) {
                    LocaleConfiguration localeConfiguration = new LocaleConfiguration();
                    readConfiguration(Launcher.this, localeConfiguration);
                    return localeConfiguration;
                }

                @Override
                protected void onPostExecute(LocaleConfiguration result) {
                    sLocaleConfiguration = result;
                    checkForLocaleChange();  // recursive, but now with a locale configuration
                }
            }.execute();
            return;
        }

        final Configuration configuration = getResources().getConfiguration();

        final String previousLocale = sLocaleConfiguration.locale;
        final String locale = configuration.locale.toString();

        final int previousMcc = sLocaleConfiguration.mcc;
        final int mcc = configuration.mcc;

        final int previousMnc = sLocaleConfiguration.mnc;
        final int mnc = configuration.mnc;

        boolean localeChanged = !locale.equals(previousLocale) || mcc != previousMcc || mnc != previousMnc;

        if (localeChanged) {
            sLocaleConfiguration.locale = locale;
            sLocaleConfiguration.mcc = mcc;
            sLocaleConfiguration.mnc = mnc;

            mIconCache.flush();

            final LocaleConfiguration localeConfiguration = sLocaleConfiguration;
            new Thread("WriteLocaleConfiguration") {
                public void run() {
                    writeConfiguration(Launcher.this, localeConfiguration);
                }
            }.start();
        }
    }

    private static class LocaleConfiguration {
        public String locale;
        public int mcc = -1;
        public int mnc = -1;
    }

    private static void readConfiguration(Context context, LocaleConfiguration configuration) {
        DataInputStream in = null;
        try {
            in = new DataInputStream(context.openFileInput(PREFERENCES));
            configuration.locale = in.readUTF();
            configuration.mcc = in.readInt();
            configuration.mnc = in.readInt();
        } catch (FileNotFoundException e) {
            // Ignore
        } catch (IOException e) {
            // Ignore
        } finally {
            if (in != null) {
                try {
                    in.close();
                } catch (IOException e) {
                    // Ignore
                }
            }
        }
    }

    private static void writeConfiguration(Context context, LocaleConfiguration configuration) {
        DataOutputStream out = null;
        try {
            out = new DataOutputStream(context.openFileOutput(PREFERENCES, MODE_PRIVATE));
            out.writeUTF(configuration.locale);
            out.writeInt(configuration.mcc);
            out.writeInt(configuration.mnc);
            out.flush();
        } catch (FileNotFoundException e) {
            // Ignore
        } catch (IOException e) {
            //noinspection ResultOfMethodCallIgnored
            context.getFileStreamPath(PREFERENCES).delete();
        } finally {
            if (out != null) {
                try {
                    out.close();
                } catch (IOException e) {
                    // Ignore
                }
            }
        }
    }

    private void setWallpaperDimension() {
        WallpaperManager wpm = (WallpaperManager)getSystemService(WALLPAPER_SERVICE);
        Display display = getWindowManager().getDefaultDisplay();

        final int width = display.getWidth();
        final int height = display.getHeight();
        /*
        try {
            //wpm.setResource(R.drawable.background);
            wpm.setResource(R.drawable.wallpaper_fireair);
        } catch (IOException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }*/
        
        wpm.suggestDesiredDimensions(width, height);  
        
    }

    /**
     * Restores the previous state, if it exists.
     *
     * @param savedState The previous state.
     */
    private void restoreState(Bundle savedState) {
        /*
        if (savedState == null) {
            return;
        }
        //restore app state
        final boolean allApps = savedState.getBoolean(RUNTIME_STATE_ALL_APPS_FOLDER, false);
        if (allApps) {
            showAllApps(false);
        }
        
        //restore widgets info
        mAddItemCellInfo = new CellLayout.CellInfo();
        final CellLayout.CellInfo addItemCellInfo = mAddItemCellInfo;
        addItemCellInfo.valid = true;
        addItemCellInfo.cellX = savedState.getInt(RUNTIME_STATE_PENDING_ADD_CELL_X);
        addItemCellInfo.cellY = savedState.getInt(RUNTIME_STATE_PENDING_ADD_CELL_Y);
        addItemCellInfo.spanX = savedState.getInt(RUNTIME_STATE_PENDING_ADD_SPAN_X);
        addItemCellInfo.spanY = savedState.getInt(RUNTIME_STATE_PENDING_ADD_SPAN_Y);
        addItemCellInfo.findVacantCellsFromOccupied(
                savedState.getBooleanArray(RUNTIME_STATE_PENDING_ADD_OCCUPIED_CELLS),
                savedState.getInt(RUNTIME_STATE_PENDING_ADD_COUNT_X),
                savedState.getInt(RUNTIME_STATE_PENDING_ADD_COUNT_Y));
        mRestoring = true;
        */
    }

    /**
     * Finds all the views we need and configure them properly.
     */
    private void setupViews() {
        setContentView(R.layout.launcher);
        //mAllAppsGrid = (IAllAppsView)findViewById(R.id.all_apps_view);
        WorkSpace mainLayout = (WorkSpace)findViewById(R.id.mainlayout);
        mainLayout.setLauncher(this);
        mModel.setCallbacks(mainLayout);
        mView = new WeakReference<ActivityRuntimeInterface>(mainLayout);
    }
    /**
     * -----------------------------------------------------------------
     * API For app widgets
     */
    
    /**
     * Receives notifications whenever the appwidgets are reset.
     */
    private class AppWidgetResetObserver extends ContentObserver {
        public AppWidgetResetObserver() {
            super(new Handler());
        }

        @Override
        public void onChange(boolean selfChange) {
            onAppWidgetReset();
        }
    }
    
    public AppWidgetManager getAppWidgetManager(){
        return mAppWidgetManager;        
    }

    public void pickAppWidget(){
        int appWidgetId = Launcher.this.mAppWidgetHost.allocateAppWidgetId();
    
        Intent pickIntent = new Intent(Utilities.ACTION_TVD_WIDGETS_PICKER);
        pickIntent.putExtra(AppWidgetManager.EXTRA_APPWIDGET_ID, appWidgetId);
        // start the pick activity
        startActivityForResult(pickIntent, REQUEST_PICK_APPWIDGET);
    }
    
    public void addAppWidget(Intent data) {
        // TODO: catch bad widget exception when sent
        int appWidgetId = data.getIntExtra(AppWidgetManager.EXTRA_APPWIDGET_ID, -1);
        AppWidgetProviderInfo appWidget = mAppWidgetManager.getAppWidgetInfo(appWidgetId);

        if (appWidget.configure != null) {
            // Launch over to configure widget, if needed
            Intent intent = new Intent(AppWidgetManager.ACTION_APPWIDGET_CONFIGURE);
            intent.setComponent(appWidget.configure);
            intent.putExtra(AppWidgetManager.EXTRA_APPWIDGET_ID, appWidgetId);

            startActivityForResultSafely(intent, REQUEST_CREATE_APPWIDGET);
        } else {
            // Otherwise just add it
            onActivityResult(REQUEST_CREATE_APPWIDGET, Activity.RESULT_OK, data);
        }
    }
    
    public void removeAppWidget(LauncherAppWidgetInfo launcherInfo) {
        mDesktopItems.remove(launcherInfo);
        launcherInfo.hostView = null;
        
        final LauncherAppWidgetInfo launcherAppWidgetInfo = launcherInfo;
        final LauncherAppWidgetHost appWidgetHost = getAppWidgetHost();
        if (appWidgetHost != null) {
            final int appWidgetId = launcherAppWidgetInfo.appWidgetId;
            // Deleting an app widget ID is a void call but writes to disk before returning
            // to the caller...
            new Thread("deleteAppWidgetId") {
                public void run() {
                    appWidgetHost.deleteAppWidgetId(launcherAppWidgetInfo.appWidgetId);
                }
            }.start();
        }
        LauncherModel.deleteItemFromDatabase(this, launcherInfo);
    }

    public LauncherAppWidgetHost getAppWidgetHost() {
        return mAppWidgetHost;
    }
    
    /**
     * Re-listen when widgets are reset.
     */
    private void onAppWidgetReset() {
        mAppWidgetHost.startListening();
    }
    
    /**
     * Add a widget to the workspace.
     *
     * @param data The intent describing the appWidgetId.
     * @param cellInfo The position on screen where to create the widget.
     */
    private void completeAddAppWidget(Intent data, CellLayout.CellInfo cellInfo) {
        Bundle extras = data.getExtras();
        int appWidgetId = extras.getInt(AppWidgetManager.EXTRA_APPWIDGET_ID, -1);

        if (LOGD) Log.d(TAG, "dumping extras content=" + extras.toString());

        AppWidgetProviderInfo appWidgetInfo = mAppWidgetManager.getAppWidgetInfo(appWidgetId);
        
        LauncherAppWidgetInfo launcherInfo = new LauncherAppWidgetInfo(appWidgetId);
        launcherInfo.title = appWidgetInfo.label;
        launcherInfo.hostView = mAppWidgetHost.createView(this, appWidgetId, appWidgetInfo);
        launcherInfo.hostView.setAppWidget(appWidgetId, appWidgetInfo);
        launcherInfo.hostView.setTag(launcherInfo);
        
        if(!mModel.addWidgetRumtime(appWidgetInfo, appWidgetId, launcherInfo)){            
            mAppWidgetHost.deleteAppWidgetId(appWidgetId);
            launcherInfo.hostView = null;
            return;
        }
    }
    
    /**
     * API For app widgets
     * -----------------------------------------------------------------
     */

    public void closeSystemDialogs() {
        getWindow().closeAllPanels();

        try {
            dismissDialog(DIALOG_CREATE_SHORTCUT);
            // Unlock the workspace if the dialog was showing
        } catch (Exception e) {
            // An exception is thrown if the dialog is not visible, which is fine
        }

        try {
            dismissDialog(DIALOG_RENAME_FOLDER);
            // Unlock the workspace if the dialog was showing
        } catch (Exception e) {
            // An exception is thrown if the dialog is not visible, which is fine
        }

        // Whatever we were doing is hereby canceled.
        mWaitingForResult = false;
    }
    

    /**
     * Indicates that we want global search for this activity by setting the globalSearch
     * argument for {@link #startSearch} to true.
     */


    public boolean isWorkspaceLocked() {
        return mWorkspaceLoading || mWaitingForResult;
    }
    /**
     * Registers various content observers. The current implementation registers
     * only a favorites observer to keep track of the favorites applications.
     */
    private void registerContentObservers() {
        ContentResolver resolver = getContentResolver();
        resolver.registerContentObserver(LauncherProvider.CONTENT_APPWIDGET_RESET_URI,
                true, mWidgetObserver);
    }
    
    @Override
    public void startActivityForResult(Intent intent, int requestCode) {
        if (requestCode >= 0) mWaitingForResult = true;
        super.startActivityForResult(intent, requestCode);
    }

    public void startActivitySafely(Intent intent, Object tag) {
        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        try {
            startActivity(intent);
        } catch (ActivityNotFoundException e) {
            Toast.makeText(this, R.string.activity_not_found, Toast.LENGTH_SHORT).show();
            Log.e(TAG, "Unable to launch. tag=" + tag + " intent=" + intent, e);
        } catch (SecurityException e) {
            Toast.makeText(this, R.string.activity_not_found, Toast.LENGTH_SHORT).show();
            Log.e(TAG, "Launcher does not have the permission to launch " + intent +
                    ". Make sure to create a MAIN intent-filter for the corresponding activity " +
                    "or use the exported attribute for this activity. "
                    + "tag="+ tag + " intent=" + intent, e);
        }
    }   
    
    public void startActivityForResultSafely(Intent intent, int requestCode) {
        try {
            startActivityForResult(intent, requestCode);
        } catch (ActivityNotFoundException e) {
            Toast.makeText(this, R.string.activity_not_found, Toast.LENGTH_SHORT).show();
        } catch (SecurityException e) {
            Toast.makeText(this, R.string.activity_not_found, Toast.LENGTH_SHORT).show();
            Log.e(TAG, "Launcher does not have the permission to launch " + intent +
                    ". Make sure to create a MAIN intent-filter for the corresponding activity " +
                    "or use the exported attribute for this activity.", e);
        }
    }

    /**
     * Receives notifications when applications are added/removed.
     */
    private class CloseSystemDialogsIntentReceiver extends BroadcastReceiver {
        @Override
        public void onReceive(Context context, Intent intent) {
            closeSystemDialogs();
            String reason = intent.getStringExtra("reason");
            if (!"homekey".equals(reason)) {
                boolean animate = true;
                if (mPaused || "lock".equals(reason)) {
                    animate = false;
                }
                //closeAllApps(animate);
                //closeAppsFavorites(animate);
            }
        }
    }
}
