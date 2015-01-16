package com.softwinner.explore;

import java.io.File;
import java.lang.ref.WeakReference;
import java.util.ArrayList;
import java.util.HashMap;

import android.graphics.Bitmap;

public class MessageCache 
{
	private static MessageCache mMessageCache = new MessageCache();
	private HashMap<String, Bitmap> mThumbnailMap = new HashMap<String, Bitmap>(200);
	private HashMap<String, String> mFileMessageMap = new HashMap<String, String>(200);
	private HashMap<String, String> mFileDescriptionMap = new HashMap<String, String>(200);
	private HashMap<String, String> mFileNameMap = new HashMap<String, String>(200);
	private HashMap<String, Long> mModifiedTimeMap = new HashMap<String, Long>(200);
	
	private MessageCache()
	{
		
	}
	public static MessageCache getInstance()
	{
		return mMessageCache;
	}
	
	public void saveThumbnailMessage(String filePath, Bitmap mp)
	{
		WeakReference<Bitmap> reference = new WeakReference<Bitmap>(mp);
		Bitmap bmp = reference.get();
		mThumbnailMap.put(filePath, bmp);
	}
	
	public Bitmap loadThumbnailMessage(String filePath)
	{
		return mThumbnailMap.get(filePath);
	}
	
	public void saveFileMessage(String filePath, String fileMessage)
	{
		mFileMessageMap.put(filePath, fileMessage);
	}
	
	public String loadFileMessage(String filePath)
	{
		return mFileMessageMap.get(filePath);
	}
	
	public void saveFileDescription(String filePath, String fileDiscription)
	{
		mFileDescriptionMap.put(filePath, fileDiscription);
	}
	
	public String loadFileDescription(String filePath)
	{
		return mFileDescriptionMap.get(filePath);
	}
	
	public void saveFileName(String filePath, String fileName)
	{
		mFileNameMap.put(filePath, fileName);
	}
	
	public String loadFileName(String filePath)
	{
		return mFileNameMap.get(filePath);
	}
	
	public void saveModifiedTime(String filePath, long modifiedTime){
		Long lg = Long.valueOf(modifiedTime);
		mModifiedTimeMap.put(filePath, lg);
	}
	
	public long loadModifiedTime(String filePath){
		Long lg = mModifiedTimeMap.get(filePath);
		if(lg != null){
			return lg.longValue();
		}else{
			return 0;
		}
	}
	
	public boolean hasFileMessage(String filePath)
	{
		return mFileMessageMap.containsKey(filePath);
	}
	
	public boolean hasThumbnail(String filePath)
	{
		return mThumbnailMap.containsKey(filePath);
	}
	
	public boolean hasFileDiscription(String filePath)
	{
		return mFileDescriptionMap.containsKey(filePath);
	}
	
	public boolean hasFileName(String filePath)
	{
		return mFileNameMap.containsKey(filePath);
	}
	
	public boolean hasModifiedTime(String filePath){
		return mModifiedTimeMap.containsKey(filePath);
	}
	
	public void clearThumbnailCache()
	{
		if(mThumbnailMap.size() >= 200)
		{
			mThumbnailMap.clear();
			mFileMessageMap.clear();
			mFileDescriptionMap.clear();
			mFileNameMap.clear();
			mModifiedTimeMap.clear();
		}
	}
}