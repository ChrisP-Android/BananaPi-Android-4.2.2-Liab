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

import android.app.Application;
import android.content.ContentResolver;
import android.content.Intent;
import android.content.IntentFilter;
import android.database.ContentObserver;
import android.os.Handler;
import dalvik.system.VMRuntime;

public class LauncherApplication extends Application {
    public static LauncherApplication mApp;
    public static Launcher mLauncher;
    public static LauncherModel mModel;    
    public static IconCache mIconCache;

    static public IconCache getIconCache() {return mIconCache;}
    static public LauncherModel getModel() {return mModel;}
    static public Launcher getLauncher() {return mLauncher;};
    @Override
    public void onCreate() {
        VMRuntime.getRuntime().setMinimumHeapSize(4 * 1024 * 1024);

        super.onCreate();
        
        init();

        // Register intent receivers
        IntentFilter filter = new IntentFilter(Intent.ACTION_PACKAGE_ADDED);
        filter.addAction(Intent.ACTION_PACKAGE_REMOVED);
        filter.addAction(Intent.ACTION_PACKAGE_CHANGED);
        filter.addDataScheme("package");
        registerReceiver(mModel, filter);
        filter = new IntentFilter();
        filter.addAction(Intent.ACTION_EXTERNAL_APPLICATIONS_AVAILABLE);
        filter.addAction(Intent.ACTION_EXTERNAL_APPLICATIONS_UNAVAILABLE);
        registerReceiver(mModel, filter);

        // Register for changes to the favorites
        ContentResolver resolver = getContentResolver();
        resolver.registerContentObserver(LauncherSettings.Favorites.CONTENT_URI, true,
                mFavoritesObserver);
        resolver.registerContentObserver(LauncherSettings.FavoritesApp.CONTENT_URI, true, 
                mFavoritesAppObserver);
    }
    static public void setLauncher(Launcher launcher){
        mLauncher = launcher;
    }

    /**
     * There's no guarantee that this function is ever called.
     */
    @Override
    public void onTerminate() {
        super.onTerminate();

        unregisterReceiver(mModel);
        mModel.stopLoader();

        ContentResolver resolver = getContentResolver();
        resolver.unregisterContentObserver(mFavoritesObserver);
        resolver.unregisterContentObserver(mFavoritesAppObserver);
    }

    /**
     * Receives notifications whenever the user favorites have changed.
     */
    private final ContentObserver mFavoritesObserver = new ContentObserver(new Handler()) {
        @Override
        public void onChange(boolean selfChange) {
            mModel.startLoader(LauncherApplication.this, false);
        }
    };
    private final ContentObserver mFavoritesAppObserver = new ContentObserver(new Handler()) {
        @Override
        public void onChange(boolean selfChange) {
            mModel.startLoader(LauncherApplication.this, false);
        }
    };
    
    private void init(){
        if( mApp == null){
            mApp = this;
        }
        if( mIconCache == null){
            mIconCache = new IconCache(mApp);
        }
        if( mModel == null){
            mModel = new LauncherModel(mApp,mIconCache);
        }
    }
    
}
