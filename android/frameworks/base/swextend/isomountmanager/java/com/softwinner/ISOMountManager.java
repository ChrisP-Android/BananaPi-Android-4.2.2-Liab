package com.softwinner;

import android.util.Log;

public class ISOMountManager {
    private static final String TAG = "ISOMountManager";
			
    public static class MountInfo{
    	public String mMountPath;
        public String mISOPath;
        
        public MountInfo(String mountPath, String isoPath){
        	mMountPath = mountPath;
        	mISOPath = isoPath;
        }
    }
		
    static {
        System.loadLibrary("isomountmanager_jni");
        init();
    }
    
    private static native final void 		init();
    
    public static native int 				mount(String mountPath, String isoPath);
	public static native int 				umount(String mountPath);
	public static native void 				umountAll();
	public static native MountInfo[]		getMountInfoList();
	public static native String         	getIsoPath(String mountPath);
    public static native String[]       	getMountPoints(String isoPath);
}
