package com.softwinner.launcher;

import android.content.Intent;
import android.content.res.Configuration;
import android.os.Bundle;

public interface ActivityRuntimeInterface {
    public void onResume();
    public void onPause();
    public void onDestroy();
    
    public void onNewIntent(Intent intent);
    public void onBackPressed();
    public void onSaveInstanceState(Bundle outState);
    public void onRestoreInstanceState(Bundle savedInstanceState);
    
    public void onConfigurationChanged(Configuration newConfig);
}
