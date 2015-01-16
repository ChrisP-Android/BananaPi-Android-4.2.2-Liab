package com.softwinner.launcher;

import android.content.Intent;
import android.os.Bundle;

public interface IActivityRuntimeAffairs {
    public void onCreate(Bundle savedInstanceState);
    public void onResume();
    public void onPause();
    public void onDestroy();
    
    public void onNewIntent(Intent intent);
    public void onActivityResult(int requestCode, int resultCode, Intent data);
    
    public void onSaveInstanceState(Bundle outState);
    public void onRestoreInstanceState(Bundle savedInstanceState);
}
