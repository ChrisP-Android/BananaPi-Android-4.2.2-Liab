package com.softwinner.launcher;

import android.content.Intent;
import android.content.IntentFilter;

public interface IStatusBar {
    public void registerReceiver();
    public void unregisterReceiver();
}
