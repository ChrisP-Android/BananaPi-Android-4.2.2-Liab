package com.softwinner.TvdFileManager;

import java.io.File;
import java.net.MalformedURLException;
import java.util.ArrayList;
import java.util.HashMap;

import jcifs.smb.SmbFile;

import android.app.Activity;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.DialogInterface.OnCancelListener;
import android.util.Log;
import android.widget.Toast;

import com.softwinner.nfs.NFSFolder;
import com.softwinner.nfs.NFSServer;
import com.softwinner.nfs.NfsManager;

public class NfsManagerWrapper {

	// Mount point root path exam: /data/data/com.softwinner.TvdFileManager/app_share
	private static File mountRoot = null;
	private static String TAG = "nfs";
	private Context mContext = null;
	private NfsManager nm;
	private boolean manualCancel = false;
	// When running in background, Show this widget.
	private ProgressDialog pr_dialog = null;
	
	// absolute path: exam /data/data/com.softwinner.TvdFileManager/app_share/nfs_home_wanran_share
	private ArrayList<String> mMountedPointList = null;
	// NFSServer object, contain server ip address, exam: 192.168.1.1
	private ArrayList<NFSServer> mServerList = new ArrayList<NFSServer>();
	// NFSFolder object, contain shared folder path, exam: /home/wanran/share
	private ArrayList<NFSFolder> mFolderList = new ArrayList<NFSFolder>(); // 
	// exam: key(/home/wanran/share) value(/data/data/com.softwinner.TvdFileManager/app_share/nfs_home_wanran_share)
	private HashMap<String, String> mMap = new HashMap<String, String>();      
	
	public static final int TYPE_SERVER = 0x01;
	public static final int TYPE_FOLDER = 0x02;
	
	public NfsManagerWrapper(Context context) {
		mContext = context;
		nm = NfsManager.getInstance(context);
		
		mServerList = new ArrayList<NFSServer>();
		mFolderList = new ArrayList<NFSFolder>();
		mMountedPointList = new ArrayList<String>();
		mMap = new HashMap<String, String>();
		
		mountRoot = mContext.getDir("share", 0);  // obtain the mount root
	}

	/**
	 * Clear all mounted point from local disk
	 */
	public void clear() {
		for(String mountPoint:mMountedPointList)
		{
			Log.d("unmount", mountPoint);
			nm.nfsUnmount(mountPoint);
		}
	}

	/**
	 * create mount point folder for SystemMix.Mount function.
	 * @param path name for creating directory. exam: /home/wanran/share
	 * @return the full path of the created folder. exam: /data/data/com.softwinner.TvdFileManager/app_share/nfs_home_wanran_share
	 */
	public static String createNewMountedPoint(String path)
	{ 
		String mountedPoint = null;
		
		path = path.replaceFirst("/", "nfs_"); // result: /home/wanran/share ------> nfs_home/wanran/share
		path = path.replace("/", "_"); // result: nfs_home/wanran/share -----> nfs_home_wanran_share 
		mountedPoint = path;
		Log.d(TAG, "MountRoot: " + mountRoot);
		File file = new File(mountRoot,mountedPoint);
		Log.d(TAG,"mounted create:  " + file.getPath());
		if(!file.exists())
		{
			try {
				file.mkdir();
			} catch (Exception e) {
				e.printStackTrace();
			}
		}
		return file.getAbsolutePath();
	}

	/**
	 * Obtain current all nfs server list
	 * @return
	 * ArrayList<NFSServer>: nfs server list
	 */
	public ArrayList<NFSServer> getAllNfsServers()
	{
		return (ArrayList<NFSServer>) mServerList.clone();
	}
	
	/**
	 * Obtain current server's all shared folder
	 * @return
	 * ArrayList<NFSFolder>: A list contain all shared folder
	 */
	public ArrayList<NFSFolder> getAllNfsShared()
	{
		return (ArrayList<NFSFolder>) mFolderList.clone();
	}
	
	/**
	 * judge the mount point whether a nfs mount point
	 * @param mountedPoint
	 * @return
	 * true: the mount point is belong to nfs
	 * false: the mount point is not belong to nfs
	 */
	public boolean isNfsMountedPoint(String mountedPoint)
	{
		if(mMountedPointList.size() == 0)
		{
			Log.d(TAG,"list is 0");
		}
		for(String item:mMountedPointList)
		{
			if(item.equals(mountedPoint))
			{
				return true;
			}
		}
		return false;
	}
	
	/**
	 * Judge selected item whether a nfs server
	 * @param filePath
	 * @return
	 * true: path is a nfs server ip
	 * false: path is not a nfs server ip
	 */
	public boolean isNfsServer(String filePath) {
		for (NFSServer server:mServerList) {
			if (server.getServerIP().equals(filePath))
				return true;
		}
		return false;
	}
	
	public boolean isNfsShare(String filePath) {
		for (NFSFolder folder : mFolderList) {
			if (folder.getFolderPath().replace('/', '_').equals(filePath))
				return true;
		}
		return false;
	}
	
	public void clearAllCache(){
		mServerList.clear();
		mFolderList.clear();
	}
	
	public void addInCache(int type, String path){
		switch (type) {
		case TYPE_FOLDER:
			NFSFolder folder = new NFSFolder();
			folder.setFolderPath(path);
			mFolderList.add(folder);
			break;
		case TYPE_SERVER:
			NFSServer server = new NFSServer();
			server.setServerIP(path);
			mServerList.add(server);
			break;
		}
	}
	
	public void removeFromCache(int type, String path){
		switch(type){
		case TYPE_SERVER:
			for(NFSServer server:mServerList){
				if(path.equals(server.getServerIP())){
					mServerList.remove(server);
					break;
				}
			}
			break;
		case TYPE_FOLDER:
			for(NFSFolder folder:mFolderList){
				if(path.equals(folder.getFolderPath())){
					mFolderList.remove(folder);
				}
			}
			break;
		}
	}
	
	/**
	 * When user select a item and click it, the EventHandler will call this function,
	 * the func is the core.
	 * @param nfsUrl: current directory name(From item's datasource)
	 * exam: 
	 * @param ls: Callback object
	 */
	public void startSearch(final String nfsUrl, final OnNfsSearchListenner ls) {
		Log.d(TAG, "search nfs:" + nfsUrl);
		manualCancel = false;
		
		pr_dialog = showProgressDialog(R.drawable.icon, mContext.getResources().getString(R.string.search), 
				null, ProgressDialog.STYLE_SPINNER, true);
		pr_dialog.setOnCancelListener(new OnCancelListener() {
			@Override
			public void onCancel(DialogInterface dialog) {
				ls.onFinish(true);
				
				manualCancel = true;
			}
		});
		
		Thread thd = new Thread(new Runnable() {

			@Override
			public void run() {
				if (nfsUrl.equals(mContext.getResources().getString(R.string.nfs))) {
					 
					mServerList = nm.getServers();
					if (manualCancel)
						return;
					
					if (mServerList.size() <= 0) {
						ls.onFinish(false);
					} else {
						for (NFSServer server : mServerList) {
							ls.onReceiver(server.getServerIP());
						}
						ls.onFinish(true);
					}
					
				} // end nfs
				else if (isNfsServer(nfsUrl)) {
					
					for (NFSServer server : mServerList) {
						if (server.getServerIP().equals(nfsUrl)) {
							mFolderList.clear();
							mFolderList = nm.getSharedFolders(server);
						}
					}
					if ((mFolderList != null) && (mFolderList.size() > 0)) {
						if (manualCancel)
							return;
						
						for (NFSFolder folder : mFolderList) {
							Log.d("folder", folder.getFolderPath());
							ls.onReceiver(folder.getFolderPath().replace('/', '_'));
						}
						ls.onFinish(true);
					} else {
						ls.onFinish(false);
					}
					pr_dialog.cancel();
					
				} // end nfs server
				else if (isNfsShare(nfsUrl)) {
					if (manualCancel)
						return;
					String nfsPath = nfsUrl.replace('_', '/');            // tranlate to /home/wanran/share
					
					// judge whether have mounted
					if (mMap != null && mMap.containsKey(nfsPath)) {
							ls.onReceiver(mMap.get(nfsPath));
							ls.onFinish(true);
					}
					// if not mounted before, mounted and return 
					else {
						for (NFSServer server : mServerList) {
							for (NFSFolder folder : mFolderList) {
								if (folder.getFolderPath().equals(nfsPath)) {
									String mountPoint = createNewMountedPoint(nfsPath); // pass the original path, exam: /home/wanran/share
									nm.nfsUnmount(mountPoint);   // first try to umount the point
									boolean success = nm.nfsMount(nfsPath, mountPoint, server.getServerIP());
									
									if (success) {
										mMountedPointList.add(mountPoint);
										mMap.put(nfsPath.replace('_', '/'), mountPoint);
										if (manualCancel)
											break;
										ls.onReceiver(mountPoint);
										ls.onFinish(true);
										break;
									} 
									else {
										showMessage(R.string.mount_fail);
										ls.onFinish(false);
										break;
									}
							} // end if
						} // end for
					} // end for
				} // end else
				//pdg.dismiss();
				} // end nfsshare
				pr_dialog.cancel();
			}
		});
		thd.start();
		
	}
	
	/**
	 * Show progress widget
	 * @param icon
	 * @param title
	 * @param message
	 * @param style
	 * @param cancelable
	 * @return
	 */
	private ProgressDialog showProgressDialog(int icon, String title, String message, int style, final boolean cancelable)
	{
		ProgressDialog pr_dialog = null;
		pr_dialog = new ProgressDialog(mContext);
		pr_dialog.setProgressStyle(style);
		pr_dialog.setIcon(icon);
		pr_dialog.setTitle(title);
		pr_dialog.setIndeterminate(false);
		pr_dialog.setCancelable(cancelable);
		if(message != null)
		{
			pr_dialog.setMessage(message);
		}
		pr_dialog.show();
		pr_dialog.getWindow().setLayout(600, 300);
		return pr_dialog;
	}
	
	/**
	 * Show message from R.String.id
	 * @param resId
	 */
	private void showMessage(final int resId)
	{
		((Activity)mContext).runOnUiThread(new Runnable() {
			@Override
			public void run() {
				try{
					Toast.makeText(mContext, resId, Toast.LENGTH_SHORT).show();
				}catch (Exception e) {
				}
			}
		});
	}
	
	/**
	 * Callback listener interface
	 * @author ethanshan
	 *
	 */
	public interface OnNfsSearchListenner
	{
		void onReceiver(String path);
		boolean onFinish(boolean finish);
	}
}
