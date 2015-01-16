package com.softwinner.update;

import android.app.Application;

public class UpdateApp extends Application {

	private static UpdateApp appInstance;
	
	public UpdateApp(){
		super();
		appInstance = this;
	}
	
	public static final UpdateApp getAppInstance(){
		return appInstance;
	}
}
