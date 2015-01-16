package com.softwinner.TvdFileManager;

import java.io.File;
import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;

import com.softwinner.ISOMountManager;
import com.softwinner.ISOMountManager.MountInfo;

import android.content.Context;
import android.util.Log;

public class ISOManager{
	private Context mContext;
	private static File cdromRoot = null;
	private static final String TAG = ISOManager.class.getSimpleName();
	private ArrayList<MountInfo> cdromList = null;
	public ISOManager(Context context){
		mContext = context;
		cdromRoot = mContext.getDir("CDROM", 0);
		cdromList = new ArrayList<MountInfo>();
	}
	
	public String getVirtualCDRomPath(String isoFile){
		String cdromPath;
		for(MountInfo info:cdromList){
			if(info.mISOPath.equals(isoFile)){
				return info.mMountPath;
			}
		}
		cdromPath = createVirtualCDRomPathIfNeed(isoFile);
		int ret;
		//卸载该挂载点,为了确保能成功挂载
		ret = ISOMountManager.umount(cdromPath);
		//挂载该挂载点
		ret = ISOMountManager.mount(cdromPath, isoFile);
		//根据挂载返回值,不为null时保存该iso文件与虚拟光驱的映射关系
		switch(ret){
		case 0:
			MountInfo isoInfo = new MountInfo(cdromPath, isoFile);
			cdromList.add(isoInfo);
			return cdromPath;
		default:
			return null;
		}
	}
	
	private String createVirtualCDRomPathIfNeed(String isoFile){
		File file = new File(isoFile);
		String name = file.getName();
		File f = new File(cdromRoot, name);
		Log.d(TAG,"createVirtualCDRomPathIfNeed()  cdromPath is " + f.getAbsolutePath());
		if(!f.exists()){
			try{
				f.mkdir();
			}catch(Exception e){
				Log.e(TAG,"createVirtualCDRomPathIfNeed()  create path fail!");
				return null;
			}
		}
		return f.getAbsolutePath();
	}
	
	public boolean isVirtualCDRom(String filePath){
		for(MountInfo info:cdromList){
			if(info.mMountPath.equals(filePath))
				return true;
		}
		return false;
	}
	
	public void clear(){
		for(MountInfo info:cdromList){
			ISOMountManager.umount(info.mMountPath);
		}
		cdromList.clear();
	}
	
	public String getIsoFile(String cdromFile){
		for(MountInfo info:cdromList){
			if(info.mMountPath.equals(cdromFile))
				return info.mISOPath;
		}
		return null;
	}
	
	private class IsoMountInfo{
		public String isoFilePath;
		public String mountPointPath;
	}
}