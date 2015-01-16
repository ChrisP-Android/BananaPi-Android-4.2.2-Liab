package com.softwinner;


import java.lang.String;
import java.lang.ref.WeakReference;
import java.lang.IndexOutOfBoundsException;
import android.util.Log;
import java.lang.System;

public class SecureFile {
    private final static String TAG = "SecureFile";

    private String mAbsolutePath;
    private static final String PRIVATE_DIRECTORY = getDirectory("PRIVATE_STORAGE", "/private");
    private static String getDirectory(String variableName, String defaultPath) {
        String path = System.getenv(variableName);
        return path == null ? defaultPath : path;
    }
    
    private int mSecureFileService; // accessed by native methods

    static {
        System.loadLibrary("securefile_jni");
        native_init();
    }

    private boolean isAbsolutePath(String path)
    {
    	if(path.startsWith("/")){
    		return true;
    	}
    	return false;
    }
    
    private String changePathToAbsolute(String path)
    {
    	String absolutePath = path;
    	String parent = PRIVATE_DIRECTORY;
    	if(path == null)
    		return null;
    	if(!isAbsolutePath(path)){
    		absolutePath = parent + "/" + path;
    	}
    	return absolutePath;
    }
    
    public SecureFile(String path){
    	mAbsolutePath = changePathToAbsolute(path);
        native_setup(new WeakReference<SecureFile>(this));
    }

	public boolean createFile(){
		if(!exists())
			return native_createFile(mAbsolutePath);
		return false;
	}

	public boolean delete(){
		return native_delete(mAbsolutePath);
	}

	public boolean exists(){
		return native_exists(mAbsolutePath);
	}

	public String getParent(){
		String parent = null;
        try{
			parent = mAbsolutePath.substring(0,mAbsolutePath.lastIndexOf("/"));
		}
		catch(IndexOutOfBoundsException e){
			Log.e(TAG, "get parent fail");
		}
		return parent;
	}
	
    public String getPath(){
        return mAbsolutePath;
    }

	public long getTotalSpace(){
		return native_getTotalSpace(mAbsolutePath);
	}

	public long getUsableSpace(){
		return native_getUsableSpace(mAbsolutePath);
	}
	
    public boolean  isDirectory(){
        return native_isDirectory(mAbsolutePath);
    }

	public boolean isFile(){
		return native_isFile(mAbsolutePath);
	}

	public long length(){
		return native_length(mAbsolutePath);
	}

	public String[] list(){
		return native_list(mAbsolutePath);
	}

    public boolean  mkdir(){
        return native_mkdir(mAbsolutePath);
    }

	public boolean mkdirs(){
		/* If the terminal directory already exists, answer false */
        if (exists()) {
            return false;
        }

        /* If the receiver can be created, answer true */
        if (mkdir()) {
            return true;
        }

		String parentDir = getParent();
        /* If there is no parent and we were not created, answer false */
        if (parentDir == null) {
            return false;
        }

        /* Otherwise, try to create a parent directory and then this directory */
        return (new SecureFile(parentDir).mkdirs() && mkdir());
	}

	public long lastModified(){
		return native_lastModified(mAbsolutePath);
	}

	public boolean renameTo(String newPath){
		String newFilePath = changePathToAbsolute(newPath);
		if(newFilePath.isEmpty()){
			return false;
		}
		return native_renameTo(mAbsolutePath,newFilePath);
	}

	public boolean setLastModified(long time){
		return native_setLastModified(mAbsolutePath,time);
	}

	public boolean write(String srcFilePath, boolean append){
		return native_writeToFile(changePathToAbsolute(srcFilePath), mAbsolutePath, append);
	}

	public boolean write(byte[] srcData, boolean append){
		int count = srcData.length;
		return native_writeInData(srcData, count, mAbsolutePath, append);
	}

	public boolean read(String destFilePath){
		return native_writeToFile(mAbsolutePath, changePathToAbsolute(destFilePath), false);
	}

	public boolean read(byte[] destData){
		int count = destData.length;
		return native_read(destData, count, mAbsolutePath);
	}
	
	
    @Override
    protected void finalize() { native_finalize(); }

    private static native final void native_init();
    private native final void native_finalize();
    private native final void native_setup(Object mediaplayer_this);
	private native boolean native_createFile(String path);
	private native boolean native_delete(String path);
	private native boolean native_exists(String path);
	private native long    native_getTotalSpace(String path);
	private native long	   native_getUsableSpace(String path);
    private native boolean native_isDirectory(String path);
	private native boolean native_isFile(String path);
	private native long native_length(String path);
	private native String[] native_list(String path);
    private native boolean native_mkdir(String path);
	private native long    native_lastModified(String path);
	private native boolean native_renameTo(String oldPath, String newPath);
	private native boolean native_setLastModified(String path, long time);
	private native boolean native_writeToFile(String srcFilePath, String desFilePath, boolean append);
	private native boolean native_writeInData(byte[] srcData, int count, String desFilePath, boolean append);
	private native boolean native_read(byte[] desData, int count, String srcFilePath);
}
