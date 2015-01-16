package com.softwinner.TvdFileManager;

import java.io.BufferedWriter;
import java.io.File;
import java.io.FileFilter;
import java.io.FileWriter;
import java.io.IOException;
import java.net.Inet4Address;
import java.net.InetAddress;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Comparator;
import java.util.EmptyStackException;
import java.util.Stack;
import java.util.Timer;
import java.util.TimerTask;

import jcifs.smb.SmbFile;
import jcifs.util.LocalNetwork;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.ProgressDialog;
import android.content.ActivityNotFoundException;
import android.content.ComponentName;
import android.content.ContentResolver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.database.Cursor;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.net.Uri;
import android.os.AsyncTask;
import android.os.Handler;
import android.os.Message;
import android.os.StatFs;
import android.provider.MediaStore;
import android.provider.Settings;
import android.text.InputType;
import android.util.Log;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.SurfaceView;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.View.OnFocusChangeListener;
import android.view.ViewGroup;
import android.view.ViewGroup.LayoutParams;
import android.view.WindowManager;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.AdapterView.OnItemLongClickListener;
import android.widget.AdapterView.OnItemSelectedListener;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;
import com.softwinner.TvdFileManager.SambaManager.OnSearchListenner;
import com.softwinner.TvdFileManager.NfsManagerWrapper.OnNfsSearchListenner;
import com.softwinner.nfs.NFSFolder;
import com.softwinner.nfs.NFSServer;
public class EventHandler implements OnClickListener,OnItemSelectedListener,OnItemClickListener, OnItemLongClickListener
{
	private static final String TAG = "EventHandler";
	
	private boolean isListAdjusted = false;
	private boolean directNfsConnect = false;
	private int itemWidth = 0;
	private int itemHeight = 0;
	
	public int currentNavigation; 
	
	private MediaProvider mMedia;
	private DeviceManager mDevices;
	private NfsManagerWrapper mNfs;
	private SambaManager mSamba;
	private ISOManager mIso;
	private String picPath;
	
	private Context mContext;
	private TableRow mTable;
	private ListView list;
	
	public String rootPath  = "/";
	public String currentDir = rootPath;
	public int currentPosition = 0; 
	public ArrayList<String> mDataSource;
	private String browseDir = null;
	
	private SurfaceView videoThumb;
	private ImageView imageThumb;
	private TextView preview;
	private TextView path;
	private TextView index;
	private IEventHandlerCallbacks mCallbacks;
	
	/* 文件目录扫描进度 */
	private ProgressDialog scanDialog;
	private int num = 0;
	private static final int SCANFILES = 0x03;
	private static final String GO_INTO_DIR = "goIntoDir";
	private static final String BACK_TO_PRE = "backToPreDir";
	private static final String NONE        = "NONE";
	
	/* 定义过滤类型 */
	private Comparator sort = MediaProvider.alph;
	
	/* 用于切换不同按键焦点  */
	private View pre = null;
	private View now = null;
	
	/* 用于判断是否该播放视频（用户上下选择太快时不必播放） */
	private String videoPath   = null;
	private Timer  mTimer      = null;
	private TimerTask mTask	   = null;
	private Handler mHandler   = null;
	private static int VIDEO_DELAY   = 1500;
	/* 设置缩略图延缓显示的时间 */
	private static int PIC_DELAY = 500;
	
	/* 默认过滤类型：所有文件 */
	private FileFilter mFilter;
	
	/* 每次进入一个文件夹时，保存该文件夹的位置，方便返回时焦点能落在该文件夹上 */
	private Stack<Integer> mPathStack;
	
	private SharedPreferences pf;
	private SharedPreferences.Editor editor;
	
	private static final int SEARCH_TYPE = 0x00;
	private static final int COPY_TYPE   = 0x01;
	private static final int DELETE_TYPE = 0x02;
	private boolean delete_after_copy    = false;
	private String  fileToOperate        = null;
	
	private static final int PICTURE = 0;
	private static final int MUSIC   = 1;
	private static final int VIDEO   = 2;
	
	private static final int BROWSER_LOCAL = 0x00;
	private static final int BROWSER_SAMBA = 0x01;
	
	public EventHandler(Context context, final IEventHandlerCallbacks mCallbacks, MediaProvider mProvider, DeviceManager mDevices) 
	{
		this.mContext   = context;
		this.mCallbacks = mCallbacks;
		mDataSource = new ArrayList<String>();
		mMedia = mProvider;
		this.mDevices   = mDevices;
		mNfs = new NfsManagerWrapper(mContext);
		mSamba = new SambaManager(mContext);
		mIso = new ISOManager(mContext);
		
		mPathStack = new Stack<Integer>();
		mPathStack.push(Integer.valueOf(0));
		
		mTimer = new Timer();
		mHandler = new Handler()
		{
			@Override
			public void handleMessage(Message msg)
			{
				switch(msg.what)
				{
				case PICTURE:
					Bitmap bm = mMedia.getImageThumbFromMK(picPath, imageThumb.getWidth(), imageThumb.getHeight());
					if(picPath.equals(mDataSource.get(currentPosition)))
					{
						imageThumb.setImageBitmap(bm);
						getPicDetail(picPath);
					}
					break;
				case VIDEO:
					mCallbacks.playThumbVideo(videoPath);
					break;
				}
			}
		};
		
		/* 设置默认的过滤*/
		mFilter = mMedia.ALLTYPE_FILTER;
		
		/* 保存设备分区名字的映射 */
		pf = mContext.getSharedPreferences("partition", 0);
		editor = pf.edit();
	}
	
	public void setViewResource(SurfaceView video, ImageView image, TextView preview, TextView path, TextView index)
	{
		this.videoThumb = video;
		this.imageThumb = image;
		this.preview	= preview;
		this.path		= path;
		this.index		= index;
	}
	
	@Override
	public void onClick(View view)
	{
		int id = view.getId();
		currentNavigation = id;
		setButtonSelected(view);
		
		mCallbacks.releaseMediaPlayerAsync();
		imageThumb.setVisibility(View.VISIBLE);
		videoThumb.setVisibility(View.GONE);
		mTimer.cancel();
		if(currentDir.equals(MediaProvider.NETWORK_NEIGHBORHOOD))
		{
			return;
		}
		showNothing();
		
		
		switch (id)
		{
		case R.id.device_button:
			getDeviceList();
			break;
		case R.id.video_button:
			mFilter = mMedia.MOVIE_FILTER;
			if(currentDir.equals(rootPath))
			{
				getDeviceList();
			}
			else 
				scanDir(currentDir, NONE);
			break;
		case R.id.picture_button:
			mFilter = mMedia.PICTURE_FILTER;
			if(currentDir.equals(rootPath))
			{
				getDeviceList();
			}
			else
				scanDir(currentDir, NONE);
			break;
		case R.id.music_button:
			mFilter = mMedia.MUSIC_FILTER;
			if(currentDir.equals(rootPath))
			{
				getDeviceList();

			}
			else
				scanDir(currentDir, NONE);
			break;
		case R.id.file_button:
			mFilter = mMedia.ALLTYPE_FILTER;
			if(currentDir.equals(rootPath))
			{
				getDeviceList();
			}
			else
				scanDir(currentDir, NONE);
			break;
			
		
		}
	}
	
	public void saveBrowseDir()
	{
		browseDir = currentDir;
	}
	
	public void loadBrowseDir()
	{
		if(browseDir != null)
		{
			currentDir = browseDir;
		}
	}
	
	
  private String getData(int position) {
		
		if(position > mDataSource.size() - 1 || position < 0)
		{
			Log.d(TAG,"MediaProvider.RETURN="+MediaProvider.RETURN);
			return MediaProvider.RETURN;
		}
		return mDataSource.get(position);
	}
	
	public void getDeviceList() {
		mDataSource.clear();
		currentDir = rootPath;
		currentPosition = 0;
		mDataSource.addAll(mDevices.getMountedDevicesList());
		if(list.requestFocusFromTouch())
		{
		}
		if(mDataSource.size() > 0)
		{
			/* 判断哪些设备有多分区 */
			for(int i = 0; i < mDataSource.size(); i++)
			{
				if(mDevices.hasMultiplePartition(mDataSource.get(i)))
				{
					MapPartitionName(mDataSource.get(i));
				}
			}
			list.setSelection(0);
		}
		mDataSource.add(mContext.getResources().getString(R.string.samba));
		
		mDataSource.add(mContext.getResources().getString(R.string.nfs));
		
		ArrayList<String> netlist = mDevices.getNetDeviceList();
		if(netlist != null){
			mDataSource.addAll(netlist);
			mSamba.clearAllCache();
			mNfs.clearAllCache();
			for(String ip:netlist){
				mSamba.addInCache(SmbFile.TYPE_SERVER, ip);
			}
		}
		mTable.notifyDataSetChanged();
		
	}
	
	private void MapPartitionName(String devices)
	{
		/* 映射其分区名,要区分插拔时卸载失败导致的假分区 */
		File f = new File(devices);
		String partition;
		File[] list = f.listFiles();
		String[] map  = {mContext.getResources().getString(R.string.partitionA),
				mContext.getResources().getString(R.string.partitionB),
				mContext.getResources().getString(R.string.partitionC),
				mContext.getResources().getString(R.string.partitionD),
				mContext.getResources().getString(R.string.partitionE),
				mContext.getResources().getString(R.string.partitionF),
				mContext.getResources().getString(R.string.partitionG),
				mContext.getResources().getString(R.string.partitionH)};
		int j = 0;
		for(int i = 0; i < list.length; i++)
		{
//			partition = list[i].substring(list[i].lastIndexOf("/") + 1);
			partition = list[i].getAbsolutePath();
			
			try{
				Log.d("chen","partition:" + partition + "   start--------");
				StatFs statFs = new StatFs(partition);
				Log.d("chen","----------------------end");
				if(statFs.getBlockCount() == 0){
					continue;
				}
			}catch(Exception e){
				continue;
			}
			if(j < map.length)
			{
				editor.putString(partition, map[j]);
				j++;
			}
			else
			{
				editor.putString(partition, mContext.getResources().getString(R.string.partitionOther));
			}
		}
		editor.commit();
	}
	
	private String getMappedName(String name)
	{
		return pf.getString(name, name);
	}
	
	
	@Override
	public void onItemSelected(AdapterView<?> parent, View view, int position,
			long id) {
        
		videoThumb.setVisibility(View.GONE);
		imageThumb.setVisibility(View.VISIBLE);
		if(mTimer != null)
		{
			mTimer.cancel();
		}
		mCallbacks.releaseMediaPlayerAsync();
		currentPosition = position;
		showNothing();
		getDetailForPosition(position);
		/*
		if(!isListAdjusted)
		{
			adjustListSize(view);
		}
		*/
	}
	
	private void adjustListSize(View item)
	{
		int height = list.getHeight();
		int deviderHeight;
		int over = 0;
		if(height == 0)
		{
			return;
		}
		else
		{
			itemHeight = item.getHeight();
			if(itemHeight != 0)
			{
				LayoutParams lp = list.getLayoutParams();
				deviderHeight = list.getDividerHeight();
				over = (height % (itemHeight + deviderHeight)) - deviderHeight ;
				lp.height = height - over;
				list.setPadding(list.getPaddingLeft(), list.getPaddingTop(), list.getPaddingRight(), list.getPaddingBottom() + over);
				isListAdjusted = true;
			}
		}
	}
	
	@Override
	public void onNothingSelected(AdapterView<?> parent) {
		// TODO Auto-generated method stub
		mCallbacks.releaseMediaPlayerAsync();
		Log.d(TAG, "nothing-------");
	}
	
	@Override
	public void onItemClick(final AdapterView<?> parent, final View view, final int position, final long id) {
		mTimer.cancel();
		
		mCallbacks.releaseMediaPlayerAsync();
		
		String filePath = mDataSource.get(position);
		isListAdjusted = false;
		if(Config.isSupportMouse())
		{
			currentPosition = position;
		}
		
		File file = new File(filePath);
		if(filePath.endsWith(MediaProvider.RETURN))
		{
			backToPreDir();
		}
		/* 如果点中的是"网络邻居",或者点击的是samba服务器,或者点击的是samba的Share文件夹,则开始搜索 */
		else if(filePath.equals(mContext.getResources().getString(R.string.samba)))
		{			
			if(!checkNet(mContext)){
				Toast.makeText(mContext, mContext.getResources().getString(R.string.net_not_connected), 
						Toast.LENGTH_SHORT).show();
				return;
			}
			filePath = "smb://";
			mSamba.startSearch(filePath, new OnSearchListenner() {
				@Override
				public void onReceiver(final String path) {
					((Activity) mContext).runOnUiThread(new Runnable() {
						@Override
						public void run() {
							if(!isListAdjusted)
							{
								mDataSource.clear();
								mDataSource.add(MediaProvider.RETURN);
								isListAdjusted = true;
							}
							mDataSource.add(path);
							mTable.notifyDataSetChanged();
						}
					});	
				}
				
				@Override
				public boolean onFinish(boolean finish) {
					if(!finish){
						currentDir = rootPath;
					}
					return false;
				}
			});
			currentDir = MediaProvider.NETWORK_NEIGHBORHOOD;
		}
		else if(mSamba.isSambaWorkgroup(filePath))
		{
			mSamba.startSearch(filePath, new OnSearchListenner() {	
				@Override
				public void onReceiver(final String path) {
					((Activity) mContext).runOnUiThread(new Runnable() {
						@Override
						public void run() {
							if(!isListAdjusted)
							{
								mDataSource.clear();
								mDataSource.add(MediaProvider.RETURN);
								isListAdjusted = true;
							}
							mDataSource.add(path);
							mTable.notifyDataSetChanged();
						}
					});	
				}
				
				@Override
				public boolean onFinish(boolean finish) {
					return false;
				}
			});
			currentDir = filePath;
		}
		else if(mSamba.isSambaServices(filePath))
		{
			final String smbServer = filePath;
			mSamba.startSearch(filePath, new OnSearchListenner() {
				@Override
				public void onReceiver(final String path) {
					((Activity) mContext).runOnUiThread(new Runnable() {
						@Override
						public void run() {
							if(!isListAdjusted)
							{
								mDataSource.clear();
								mDataSource.add(MediaProvider.RETURN);
								isListAdjusted = true;
							}
							mDataSource.add(path);
							mTable.notifyDataSetChanged();
						}
					});	
				}
				
				@Override
				public boolean onFinish(boolean finish) {	
					if(!finish && LocalNetwork.getIpType(smbServer) != LocalNetwork.IP_TYPE_UNKNOWN){
						directNfsConnect = true;
						mSamba.removeFromCache(SmbFile.TYPE_SERVER, smbServer);
						mNfs.addInCache(NfsManagerWrapper.TYPE_SERVER, smbServer);
						((Activity)mContext).runOnUiThread(new Runnable() {
							@Override
							public void run() {
								onItemClick(parent, view, position, id);
							}
						});
						return true;
					}
					return false;
				}
			});
			currentDir = filePath;
		}
		else if(mSamba.isSambaShare(filePath))
		{
			mSamba.startSearch(filePath, new OnSearchListenner() {
				@Override
				public void onReceiver(final String path) {
					((Activity)mContext).runOnUiThread(new Runnable() {
						@Override
						public void run() {
							goIntoDir(path);
						}
					});
					
				}
				
				@Override
				public boolean onFinish(boolean finish) {
					return false;
				}
			});
		}
		/* 如果是文件夹，则显示其里面第一级子目录的文件 */
		else if(filePath.equals(mContext.getResources().getString(R.string.nfs)))
		{			
			if(!checkNet(mContext)){
				Toast.makeText(mContext, mContext.getResources().getString(R.string.net_not_connected), 
						Toast.LENGTH_SHORT).show();
				return;
			}
			mNfs.startSearch(filePath, new OnNfsSearchListenner() {
				@Override
				public void onReceiver(final String path) {
					((Activity) mContext).runOnUiThread(new Runnable() {
						@Override
						public void run() {
							if(!isListAdjusted)
							{
								mDataSource.clear();
								mDataSource.add(MediaProvider.RETURN);
								isListAdjusted = true;
							}
							mDataSource.add(path);
							mTable.notifyDataSetChanged();
						}
					});	
				}
				
				@Override
				public boolean onFinish(boolean finish) {
					if(!finish){
						currentDir = rootPath;
					}
					return false;
				}
			});
			//currentDir = MediaProvider.NETWORK_NEIGHBORHOOD;
			currentDir = MediaProvider.NFS_SHARE;
		}
		else if(mNfs.isNfsServer(filePath))
		{
			final String nfsServer = filePath;
			mNfs.startSearch(filePath, new OnNfsSearchListenner() {
				@Override
				public void onReceiver(final String path) {
					((Activity) mContext).runOnUiThread(new Runnable() {
						@Override
						public void run() {
							if(!isListAdjusted)
							{
								mDataSource.clear();
								mDataSource.add(MediaProvider.RETURN);
								isListAdjusted = true;
							}
							mDataSource.add(path);
							mTable.notifyDataSetChanged();
						}
					});	
				}
				
				@Override
				public boolean onFinish(boolean finish) {	
					if(directNfsConnect){
						mSamba.addInCache(SmbFile.TYPE_SERVER, nfsServer);
						mNfs.removeFromCache(NfsManagerWrapper.TYPE_SERVER, nfsServer);
						directNfsConnect = false;
						Log.d("chen","---onfinish():" + finish + "----");
						if(!finish){
							currentDir = rootPath;
						}
					}
					return false;
				}
			});
			currentDir = filePath;
		}
		else if(mNfs.isNfsShare(filePath))
		{
			mNfs.startSearch(filePath, new OnNfsSearchListenner() {
				@Override
				public void onReceiver(final String path) {
					((Activity)mContext).runOnUiThread(new Runnable() {
						@Override
						public void run() {
							goIntoDir(path);
						}
					});
					
				}
				
				@Override
				public boolean onFinish(boolean finish) {
					return false;
				}
			});
		}
		else if(file.isDirectory())
		{
			if(!playBlurayFolder(filePath)){
				goIntoDir(mDataSource.get(currentPosition));
			}
		}
		else 
		{
			playFile(filePath);
		}
	}
	
	private boolean checkNet(Context context){
		try {  
            ConnectivityManager connectivity = (ConnectivityManager) context.getSystemService(Context.CONNECTIVITY_SERVICE);  
            if (connectivity != null) {  
              
                NetworkInfo info = connectivity.getActiveNetworkInfo();  
                if (info != null && info.isConnected()) {  
                  
                    if (info.getState() == NetworkInfo.State.CONNECTED) {  
                        return true;  
                    }  
                }  
            }  
        } catch (Exception e) {  
        	return false;  
        }  
        return false; 
	}
	
	private String findBlurayVideo(String path){
		/* 是否在设置中打开了蓝光文件夹播放功能 */
		boolean enable = Settings.System.getInt(mContext.getContentResolver(), 
				Settings.System.BD_FOLDER_PLAY_MODE, 0) != 0?true:false;
		if(!enable){
			Log.d(TAG,"the BD_FOLDER_PLAY_MODE is unable in setting");
			return null;
		}
		/* 是否符合蓝光文件夹的目录结构 */
		String subDir = "BDMV/STREAM";
		String p = path + "/" + subDir;
		File f = new File(p);
		if(!f.exists() || !f.isDirectory()){
			return null;
		}
		String[] list = f.list();
		if(list == null){
			return null;
		}
		for(String st:list){
			File file = new File(f,st);
			if(TypeFilter.isMovieFile(st)) Log.d("chen","movie");
			if(file.isFile() && TypeFilter.isMovieFile(st)){
				String pre = st.substring(0, st.lastIndexOf("."));
				Log.d("chen","findBlurayVideo()  st=" + st + "  pre=" + pre);
				Integer itg = Integer.valueOf(pre);
				if(itg != null) Log.d("chen","integer  " + itg.intValue());
				if(itg == null || pre.length() != 5){
					continue;
				}
			
				return file.getAbsolutePath();
			}
		}
		return null;
	}
	private boolean playBlurayFolder(String path){
		String videoPath = findBlurayVideo(path);
		if(videoPath == null){
			Log.d(TAG, "it is not a bluray folder");
			return false;
		}
		else Log.d("chen","play " + videoPath);
		File file = new File(videoPath);
		Intent bdIntent = new Intent();
		bdIntent.putExtra(MediaStore.EXTRA_BD_FOLDER_PLAY_MODE , true);
		ComponentName cm = new ComponentName("com.softwinner.TvdVideo", "com.softwinner.TvdVideo.TvdVideoActivity");
    	bdIntent.setComponent(cm); 
    	bdIntent.setDataAndType(Uri.fromFile(file), "video/*");
    	try{
    		Log.d(TAG, "begin to play bluray folder");
    		mContext.startActivity(bdIntent);
    		return true;
    	}catch(ActivityNotFoundException e){
    		Log.e(TAG, "can not find app to play bluray folder");
			return false;
		}
    	
	}
	
	private boolean playBlurayFolder(String path,String realpath){
		
	  	Log.d("zheng","enter playBlurayFolder ");
	    String videoPath = findBlurayVideo(path);
			if(videoPath == null){
			 Log.d(TAG, "it is not a bluray folder");
	  		return false;
	  	}
	  	else Log.d("chen","play " + videoPath);
		  File file = new File(videoPath);
	    Intent bdIntent = new Intent();
		  bdIntent.putExtra("VideoPath000", realpath);
		  bdIntent.putExtra(MediaStore.EXTRA_BD_FOLDER_PLAY_MODE , true);
		  ComponentName cm = new ComponentName("com.softwinner.TvdVideo", "com.softwinner.TvdVideo.TvdVideoActivity");
    	bdIntent.setComponent(cm); 
    
      bdIntent.setDataAndType(Uri.fromFile(file), "video/*");
    
    	try{
    		Log.d(TAG, "begin to play bluray folder");
    		mContext.startActivity(bdIntent);
    		return true;
    	}catch(ActivityNotFoundException e){
    		Log.e(TAG, "can not find app to play bluray folder");
			return false;
		}
		
		
	}
  
	private void playMusic(String path)
	{
		File file = new File(path); 
		Intent picIntent = new Intent();
        picIntent.setAction(android.content.Intent.ACTION_VIEW);
        picIntent.setDataAndType(Uri.fromFile(file), "audio/*");
        try
        {
        	mContext.startActivity(picIntent);
        }catch(ActivityNotFoundException e)
        {
        	DisplayToast(mContext.getResources().getString(R.string.not_app_to_play_the_music));
        }
	}

	private void playVideo(String path)
	{
		
		File file = new File(path);
		Intent movieIntent = new Intent();
		movieIntent.putExtra(MediaStore.PLAYLIST_TYPE, MediaStore.PLAYLIST_TYPE_CUR_FOLDER);
		movieIntent.putExtra(MediaStore.EXTRA_FINISH_ON_COMPLETION, false);
		movieIntent.putExtra(MediaStore.EXTRA_BD_FOLDER_PLAY_MODE , false);
		movieIntent.setAction(android.content.Intent.ACTION_VIEW);
		movieIntent.setDataAndType(Uri.fromFile(file), "video/*");
		
		/* make sure that media must be reset */
		mCallbacks.releaseMediaPlayerSync();
		Log.d(TAG, "Start");
		try
		{
			mContext.startActivity(movieIntent);
		}catch(ActivityNotFoundException e)
		{
			DisplayToast(mContext.getResources().getString(R.string.not_app_to_play_the_video));
		}
	}
	
	private void playPicture(String path)
	{
		File file = new File(path);
		Intent picIntent = new Intent();
		picIntent.setAction(android.content.Intent.ACTION_VIEW);
		picIntent.setDataAndType(Uri.fromFile(file), "image/*");
		try
		{
			mContext.startActivity(picIntent);
		}catch(ActivityNotFoundException e)
		{
			DisplayToast(mContext.getResources().getString(R.string.not_app_to_oepn_the_pic));
		}
	}
	
	private void playPdf(String path)
	{
		File file = new File(path);
		Intent pdfIntent = new Intent();
		pdfIntent.setAction(android.content.Intent.ACTION_VIEW);
		pdfIntent.setDataAndType(Uri.fromFile(file), "application/pdf");
		try
		{
			mContext.startActivity(pdfIntent);
		}
		catch(ActivityNotFoundException e)
		{
			selectFileType_dialog(new File(path));
		}
		
	}
	
	private void playApk(String path)
	{
		File file = new File(path);
		Intent apkIntent = new Intent();
		apkIntent.setAction(android.content.Intent.ACTION_VIEW);
		apkIntent.setDataAndType(Uri.fromFile(file), "application/vnd.android.package-archive");
		try
		{
			mContext.startActivity(apkIntent);
		}
		catch(ActivityNotFoundException e)
		{
			DisplayToast(mContext.getResources().getString(R.string.not_app_to_open_the_file));
		}
	}
	
	private void playHtml(String path)
	{
		File file = new File(path);
		Intent htmlIntent = new Intent();
		htmlIntent.setAction(android.content.Intent.ACTION_VIEW);
		htmlIntent.setDataAndType(Uri.fromFile(file), "text/html");
		try
		{
			mContext.startActivity(htmlIntent);
		}
		catch(ActivityNotFoundException e)
		{
			selectFileType_dialog(new File(path));
		}
	}
	
	private void playTxt(String path)
	{
		File file = new File(path);
		Intent txtIntent = new Intent();
		txtIntent.setAction(android.content.Intent.ACTION_VIEW);
		txtIntent.setDataAndType(Uri.fromFile(file), "text/plain");
		try
		{
			mContext.startActivity(txtIntent);
		}
		catch(ActivityNotFoundException e)
		{
			DisplayToast(mContext.getResources().getString(R.string.not_app_to_open_the_file));
		}
	}
	
	private void playIso(String path){
		String realpath = path ;
		Log.d("zheng","realpath " + realpath);
		String cdromPath = mIso.getVirtualCDRomPath(path);
		Log.d("zheng","cdromPath " + cdromPath);
		if(cdromPath != null){
			Log.d(TAG,"playIso()  browser iso file");
			if(!playBlurayFolder(cdromPath,realpath)){
				goIntoDir(cdromPath);
			}
		}
	}
	
	private String openType = null;
	private void selectFileType_dialog(final File openFile) {
    	String mFile = mContext.getResources().getString(R.string.open_file);
		String mText = mContext.getResources().getString(R.string.text);
		String mAudio = mContext.getResources().getString(R.string.music);
		String mVideo = mContext.getResources().getString(R.string.video);
		String mImage = mContext.getResources().getString(R.string.picture);
		CharSequence[] FileType = {mText,mAudio,mVideo,mImage};
		AlertDialog.Builder builder;
    	AlertDialog dialog;
		builder = new AlertDialog.Builder(mContext);
		builder.setTitle(mFile);
		builder.setIcon(R.drawable.help);
		builder.setItems(FileType, new DialogInterface.OnClickListener() {
			
			@Override
			public void onClick(DialogInterface dialog, int which) {
				// TODO Auto-generated method stub
				Intent mIntent = new Intent();
				switch(which) {
				case 0:
					openType = "text/*";
					break;
				case 1:
					openType = "audio/*";
					break;
				case 2:
					openType = "video/*";
					break;
				case 3:
					openType = "image/*";
					break;
				}
				mIntent.setAction(android.content.Intent.ACTION_VIEW);
				mIntent.setDataAndType(Uri.fromFile(openFile), openType);
				try {
	    			mContext.startActivity(mIntent);
	    		} catch(ActivityNotFoundException e) {
	    			DisplayToast(mContext.getResources().getString(R.string.not_app_to_open_the_file));
	    		}
			}
		});	
		dialog = builder.create();
    	dialog.show();
    }
	
	private void playFile(String path)
	{
        if(mCallbacks.returnFile(new File(path))){
            return;
        }

		if(TypeFilter.isMovieFile(path))
		{
			playVideo(path);
		}
		else if(TypeFilter.isMusicFile(path))
		{
			playMusic(path);
		}
		else if(TypeFilter.isPictureFile(path))
		{
			playPicture(path);
		}
		else if(TypeFilter.isApkFile(path))
		{
			playApk(path);
		}
		else if(TypeFilter.isTxtFile(path))
		{
			playTxt(path);
		}
		else if(TypeFilter.isHtml32File(path))
		{
			playHtml(path);
		}
		else if(TypeFilter.isPdfFile(path))
		{
			playPdf(path);
		}
		else if(TypeFilter.isISOFile(path)){
			playIso(path);
		}
		else 
		{
			selectFileType_dialog(new File(path));
		}
	}
	
	
	private void showNothing() {
		//imageThumb.setImageDrawable(null);
		imageThumb.setImageResource(R.drawable.thumbnail_equipment);
		preview.setBackgroundDrawable(null);
		preview.setText("");
		index.setText("");
		path.setText("");
	}
	
	private void showDeviceMessage(String path)
	{
		String target	= null;
		imageThumb.setImageResource(R.drawable.thumbnail_equipment);
		target = getDeviceName(path);
		preview.setBackgroundResource(R.drawable.preview);
		try
		{
			long totalsize = 0;
			long availsize = 0;
			long usedsize  = 0;
			if(mDevices.hasMultiplePartition(path))
			{
				File f = new File(path);
				File[] list = f.listFiles();
				for(int i = 0; i < list.length; i++)
				{
					totalsize += getTotalSize(list[i].getAbsolutePath());
					availsize += getAvailableSize(list[i].getAbsolutePath());
				}
				usedsize = totalsize - availsize;
			}
			else 
			{
				totalsize = getTotalSize(path);
				availsize = getAvailableSize(path);
				usedsize  = totalsize - availsize;
			}
			String availSize = toSize(availsize);
			String usedSize  = toSize(usedsize);
		
			String Target = mContext.getResources().getString(R.string.target) + target + "\n";
			String Usedsize = mContext.getResources().getString(R.string.used_size) + usedSize + "\n";
			String Availsize = mContext.getResources().getString(R.string.avail_size) + availSize + "\n";
			String Display = Target + Usedsize + Availsize;
		
			preview.setText(Display);
		}
		catch(Exception e)
		{
			Log.e(TAG, "fail to catch the size of the devices");
		}
		
	}
	
	/* 获取全部空间,..GB */
	private long getTotalSize(String path)
	{
		StatFs statfs = new StatFs(path);
		long totalBlocks = statfs.getBlockCount();
		long blockSize 	 = statfs.getBlockSize();
		long totalsize		 = blockSize * totalBlocks;
		return totalsize;
	}
	
	
	/* 获取可用空间 */
	private long getAvailableSize(String path)
	{
		StatFs statfs = new StatFs(path);
		long blockSize 	 = statfs.getBlockSize();
		long availBlocks = statfs.getAvailableBlocks();
		long availsize	 = blockSize * availBlocks;
		return availsize;
	}
	
	private void showMusicMessage(String path)
	{
		imageThumb.setImageResource(R.drawable.thumbnail_music);
		
		final String EXTERNAL_VOLUME = "external";
		final int PATH_INDEX 	 = 0;
		final int ARTIST_INDEX	 = 1;
		final int DURATION_INDEX = 2;
		final int TITLE_INDEX	 = 3;
		
		String Singer 	= null;
		String Title  	= null;
		String Duration = null;
		String Size		= null;
		
		String[] PROJECTION = {MediaStore.Audio.Media.DATA, MediaStore.Audio.Media.ARTIST, 
				MediaStore.Audio.Media.DURATION, MediaStore.Audio.Media.TITLE};
		Uri uri = MediaStore.Audio.Media.getContentUri(EXTERNAL_VOLUME);
		ContentResolver cr = mContext.getContentResolver();
		String[] selectionArgs = {path};
		
		Cursor c = cr.query(uri, PROJECTION, MediaStore.Audio.Media.DATA + "=?", selectionArgs, null);
		if(c != null)
		{
			try
			{
				while(c.moveToNext())
				{
					String filePath = c.getString(PATH_INDEX);
					Singer = mContext.getResources().getString(R.string.singer) + c.getString(ARTIST_INDEX) + "\n";
					Title  = mContext.getResources().getString(R.string.title) + c.getString(TITLE_INDEX) + "\n";
					long time = Integer.valueOf(c.getString(DURATION_INDEX)).longValue();
					Duration  = mContext.getResources().getString(R.string.duration) + toDuration(time) + "\n";
					Size   = mContext.getResources().getString(R.string.size) + toSize(new File(path).length()) + "\n";
				}
			}
			finally
			{
				c.close();
				c = null;
			}
		}
		else
		{
			Log.d(TAG, "cursor is null");
		}
		preview.setBackgroundResource(R.drawable.preview);
		if(Singer == null || Title == null || Duration == null || Size == null)
		{
			/* 不能获取该信息,返回 */
			return;
		}
		String Display  = Singer + Title + Duration + Size;
		preview.setText(Display);
	}
	
	private void showVideoMessage(String path)
	{
		imageThumb.setImageResource(R.drawable.thumbnail_video);
		preview.setBackgroundResource(R.drawable.preview);
		videoPath = path;
		if(mTimer != null)
		{
			mTimer.cancel();
			mTimer = null;
		}
		mTimer = new Timer();
		mTask  = new TimerTask()
		{

			@Override
			public void run() {
				//延时播放
				Message msg = new Message(); 
				msg.what = VIDEO;
				mHandler.sendMessage(msg);
			}
		};
		mTimer.schedule(mTask, VIDEO_DELAY);	
	}
	
	private void showPictureMessage(String path)
	{
		/* 获取图片缩略图 */
		Bitmap thumb = mMedia.getImageThumbFromDB(path);
		if(thumb != null)
		{
			imageThumb.setImageBitmap(thumb);
			getPicDetail(path);
		}
		else
		{
			/* 先设置一个默认图片 */
			imageThumb.setImageResource(R.drawable.thumbnail_picture);
			/* 如果不能直接从数据库中获得其缩略图文件,则需要延缓时间再解码生成 */
			picPath = path;
			if(mTimer != null)
			{
				mTimer.cancel();
				mTimer = null;
			}
			mTimer = new Timer();
			mTask  = new TimerTask()
			{

				@Override
				public void run() {
					//延时播放
					Message msg = new Message();
					msg.what = PICTURE;
					mHandler.sendMessage(msg);
				}
			};
			mTimer.schedule(mTask, PIC_DELAY);	
		}
		
	}
	
	private void getPicDetail(String path)
	{
		//获取图片大小
		String s = toSize(new File(path).length());
		String Size	= mContext.getResources().getString(R.string.size)
				+ s + "\n";
		
		//获取图片分辨率
		BitmapFactory.Options options = new BitmapFactory.Options();
        options.inJustDecodeBounds = true;
        BitmapFactory.decodeFile(path, options); 
        String wPic	= String.valueOf(options.outWidth);
        String hPic = String.valueOf(options.outHeight);
        String Resolution = mContext.getResources().getString(R.string.resolution) + wPic + "*" + hPic +"\n";
        
        String Display = Size + Resolution;
        preview.setText(Display);
	}
	
	private void showFileIndex(String path, int position)
	{
		showPath(path);
		showIndex(position);
	}
	
	private void showPath(String path)
	{
		//显示文件路径
		String dir  = mContext.getResources().getString(R.string.directory);
		String type = getTypeName(path);
		String dev  = getDeviceName(path);
		String filepath;
		if(path.equals(MediaProvider.RETURN))
		{
			filepath = dir + type;
		}
		else if(currentDir.equals(rootPath))
		{
			filepath = dir + type + "/" + dev;
		}
		else
		{
			String file = null;
			try
			{
				file = path.substring(path.lastIndexOf("/") + 1);
			}
			catch(NullPointerException e)
			{
				e.printStackTrace();
				file = "";
			}
				filepath = dir + ":" + type + "/" + dev + "/" + file;
		}
		this.path.setText(filepath);
	}
	
	private void showIndex(int position)
	{
		//显示item在列表中的索引
		String index = String.valueOf(position + 1) + "/" + String.valueOf(mDataSource.size());
		this.index.setText(index);
	}

	private String getDeviceName(String path)
	{
		String target = "";
		ArrayList<String> list = mDevices.getLocalDevicesList();
		for(int i = 0; i < list.size(); i++)
		{
			if(path.startsWith(list.get(i)))
			{
				if(mDevices.isInterStoragePath(list.get(i)))
				{
					target = mContext.getResources().getString(R.string.flash);
				}
				else if(mDevices.isSdStoragePath(list.get(i)))
				{
					target = mContext.getResources().getString(R.string.sdcard);
				}
				else if(mDevices.isUsbStoragePath(list.get(i)))
				{
					target = mContext.getResources().getString(R.string.usb);
				}
				else if(mDevices.isSataStoragePath(list.get(i)))
				{
					target = mContext.getResources().getString(R.string.sata);
				}
				return target;
			}
		}

		return target;
	}
	
	private String getTypeName(String path)
	{
		String music   = mContext.getResources().getString(R.string.music);
		String picture = mContext.getResources().getString(R.string.picture);
		String video   = mContext.getResources().getString(R.string.video);
		String all 	   = mContext.getResources().getString(R.string.file);
		String device  = mContext.getResources().getString(R.string.equipment);
		switch (currentNavigation)
		{
		case R.id.music_button : 
			return music;
		case R.id.picture_button:
			return picture;
		case R.id.video_button:
			return video;
		case R.id.file_button:
			return all;
		case R.id.device_button:
			return device;
		}
		return "";
	}
	
	private int kb = 1024;
	private int mb = 1024 * 1024;
	private int gb = 1024 * 1024 * 1024;
	public String toSize(long mbyte)
	{
		if(mbyte >= gb)
			return String.format("%.2f Gb ", (double)mbyte / gb);
		else if(mbyte >= mb)
			return String.format("%.2f Mb ", (double)mbyte / mb);
		else if(mbyte >= kb)
			return String.format("%.2f Kb ", (double)mbyte / kb);
		else 
			return String.format("%d byte", mbyte);
	}
	
	public String toDuration(long ms)
	{
		
		long hours;
		long minutes;
		long seconds;
		long t1;
		long t2;
		hours 	= ms / (1000*60*60);
		t1		= ms - hours*1000*60*60;
		minutes = t1 / (1000*60);
		t2	 	= t1 - minutes*1000*60;
		seconds = t2/1000;
		String duration;
		duration= String.valueOf(hours) + ":" + String.valueOf(minutes) + ":" + String.valueOf(seconds);
		return duration;
	}
	
	public void setButtonSelected(View view)
	{
		now = view;
		if(!now.equals(pre))
		{
			if(pre != null)
			{
				((Button) pre).setSelected(false);
			}
			now.setSelected(true);
			pre = now;
		}
	}
	
	public void setListAdapter(TableRow mTable, ListView mlist) {
		// TODO Auto-generated method stub
		this.mTable = mTable;
		this.list	= mlist;
		this.list.setOnFocusChangeListener(new OnFocusChangeListener() {
			@Override
			public void onFocusChange(View v, boolean hasFocus) {
				if(hasFocus)
				{
					/* 获取焦点时，显示条目中第一个文件的信息  */
					currentPosition = 0;
					getDetailForPosition(0);
				}
				else
				{
					/* 文件列表栏丢失焦点时，清除预览信息 */
					mCallbacks.releaseMediaPlayerAsync();
					currentPosition = -1;
					showNothing();
				}
			}
		});
	}
	
	/**
	 * 返回上一层，如果成功返回真
	 */
	public boolean backToPreDir()
	{
		Log.d(TAG,"back to pre dir " + currentDir);
		videoThumb.setVisibility(View.GONE);
		imageThumb.setVisibility(View.VISIBLE);
		if(mTimer != null)
		{
			mTimer.cancel();
		}
		mCallbacks.releaseMediaPlayerAsync();
		showNothing();
		/* 如果所在目录是某个设备的根目录，则返回设备总列表 根目录*/
		int i;
		int j;
		ArrayList<String> devicesList;
		devicesList = mDevices.getLocalDevicesList();
		for(j = 0; j < devicesList.size(); j++)
		{
			if(currentDir.equals(devicesList.get(j)))
			{
				getDeviceList();
				mTable.notifyDataSetChanged();	
				i = mPathStack.pop().intValue();
				if(currentPosition == i)
				{
					getDetailForPosition(i);
				}
				else
				{
					list.setSelection(i);
				}
				saveBrowseDir();
				return true;
			}
		}
		/* 如果当前目录是samba的share文件夹的挂载点,则返回samba的share文件夹总列表 */
		if(mSamba.isSambaMountedPoint(currentDir))
		{
			try{
				mPathStack.pop();
			}catch (EmptyStackException e) {
				Log.d("TvdFileManager", e.getMessage());
			}
			mDataSource.clear();
			mDataSource.add(MediaProvider.RETURN);
			ArrayList<SmbFile> lst = mSamba.getAllSmbShared();
			for(SmbFile file:lst)
			{
				mDataSource.add(file.getPath());
			}
			mTable.notifyDataSetChanged();
			try{
				currentDir = mSamba.getAllSmbServices().get(0).getPath();
			}
			catch(Exception e)
			{
				currentDir = MediaProvider.NETWORK_NEIGHBORHOOD;
			}
			list.setSelection(0);
			return true;
		}
		else if(mSamba.isSambaServices(currentDir))
		{
			mDataSource.clear();
			if(mSamba.getAllSmbWorkgroup().size() == 0){
				getDeviceList();
				mTable.notifyDataSetChanged();
				getDetailForPosition(1);
				list.setSelection(1);
			}else{
				mDataSource.add(MediaProvider.RETURN);
				ArrayList<SmbFile> lst = mSamba.getAllSmbServices();
				for(SmbFile file:lst)
				{
					mDataSource.add(file.getPath());
				}
				mTable.notifyDataSetChanged();
				try{
					currentDir = mSamba.getAllSmbWorkgroup().get(0).getPath();
				}
				catch(Exception e)
				{
					currentDir = MediaProvider.NETWORK_NEIGHBORHOOD;
				}
				list.setSelection(0);
			}
			return true;
		}
		else if(mSamba.isSambaWorkgroup(currentDir))
		{
			mDataSource.clear();
			mDataSource.add(MediaProvider.RETURN);
			ArrayList<SmbFile> lst = mSamba.getAllSmbWorkgroup();
			for(SmbFile file:lst)
			{
				mDataSource.add(file.getPath());
			}
			mTable.notifyDataSetChanged();
			currentDir = MediaProvider.NETWORK_NEIGHBORHOOD;
			return true;
		}
		else if(currentDir.equals(MediaProvider.NETWORK_NEIGHBORHOOD))
		{
			getDeviceList();
			mTable.notifyDataSetChanged();
			getDetailForPosition(1);
			list.setSelection(1);
			return true;
		}
		else if(mNfs.isNfsMountedPoint(currentDir))
		{
			try{
				mPathStack.pop();
			}catch (EmptyStackException e) {
				Log.d("TvdFileManager", e.getMessage());
			}
			mDataSource.clear();
			mDataSource.add(MediaProvider.RETURN);
			ArrayList<NFSFolder> lst = mNfs.getAllNfsShared();
			for(NFSFolder folder:lst)
			{
				mDataSource.add(folder.getFolderPath().replace('/', '_'));
			}
			mTable.notifyDataSetChanged();
			try{
				currentDir = mNfs.getAllNfsServers().get(0).getServerIP();
			}
			catch(Exception e)
			{
				currentDir = MediaProvider.NFS_SHARE;
			}
			list.setSelection(0);
			return true;
		}
		else if(mNfs.isNfsServer(currentDir))
		{
			mDataSource.clear();
			mDataSource.add(MediaProvider.RETURN);
			ArrayList<NFSServer> lst = mNfs.getAllNfsServers();
			for(NFSServer server:lst)
			{
				mDataSource.add(server.getServerIP());
			}
			mTable.notifyDataSetChanged();
			try{
				currentDir = MediaProvider.NFS_SHARE;
			}
			catch(Exception e)
			{
				currentDir = MediaProvider.NFS_SHARE;
			}
			list.setSelection(0);
			return true;
		}
		else if(currentDir.equals(MediaProvider.NFS_SHARE))
		{
			getDeviceList();
			mTable.notifyDataSetChanged();
			getDetailForPosition(2);
			list.setSelection(2);
			return true;
		}
		else if(mIso.isVirtualCDRom(currentDir)){
			Log.d("chen","is Virtual CD Rom");
			mDataSource.clear();
			mDataSource.add(MediaProvider.RETURN);
			currentDir = mIso.getIsoFile(currentDir);
			backToPreDir();
			return true;
		}
		/* 如果当前所在目录不是某个设备的根目录，则返回上一层目录*/
		else if(!currentDir.equals(rootPath))
		{
			Log.d("chen","--------back to pre ");
			currentDir = currentDir.substring(0, currentDir.lastIndexOf("/"));
			saveBrowseDir();
			scanDir(currentDir, BACK_TO_PRE);
			return true;
		}
		return false;
	}
	
	/**
	 * 进入下一层。成功返回真 
	 */
	public boolean goIntoDir(String path)
	{
		File file = new File(path);
		if(file.isDirectory())
		{
			if(file.canRead())
			{
				currentDir  = path;
				saveBrowseDir();
				
				scanDir(path, GO_INTO_DIR);
				return true;
			}
			else
			{
				Toast.makeText(mContext, mContext.getResources().getString(R.string.can_not_open), 
						Toast.LENGTH_SHORT).show();
				return false;
			}
		}
		else 
		{
			return false;
		}
	}
	
	/**
	 * 获取position位置上的文件的信息,
	 */
	public void getDetailForPosition(int position)
	{
		try
		{
			String path = mDataSource.get(position);
			/* 显示索引条 */
			showFileIndex(path, position);
			/* 显示预览信息  */
			File file   = new File(path);
			int i;
			ArrayList<String> devicesList = mDevices.getLocalDevicesList();
			for(i = 0; i < devicesList.size(); i++)
			{
				if(path.equals(devicesList.get(i)))
				{
					showDeviceMessage(path);
					return;
				}
			}
			if(path.equals(MediaProvider.RETURN))
			{
				showDeviceMessage(currentDir);
			}
			else if(!file.exists())
			{
				/* 有可能显示的只是一个字符串,如"网络邻居",或者其他非文件,这时什么都不做 */
			}
			else
			{
				if(file.isDirectory())
				{
					showDeviceMessage(path);
				}
				else if(TypeFilter.isMovieFile(path))
				{
					showVideoMessage(path);
				}
				else if(TypeFilter.isMusicFile(path))
				{
					showMusicMessage(path);
				}
				else if(TypeFilter.isPictureFile(path))
				{
					showPictureMessage(path);
				}
				else
				{
					showDeviceMessage(path);
				}
			}
		}
		catch(Exception e)
		{
			Log.e(TAG,"exception");
		}
	}
	
	/**
	 * 扫描文件
	 */
	public void scanDir(String path, String extraFlag)
	{
		mDataSource.clear();
		currentDir = path;
		String[] st = {path, extraFlag};
		mTable.notifyDataSetChanged();
		new BackgroundWork(SCANFILES).execute(st);
	}
	
	/**
	 * 清除暂存区内容
	 */
	public void clear()
	{
		Log.d("umount", "EventHandler.clear");
		mNfs.clear();
		mCallbacks.releaseMediaPlayerAsync();
		mMedia.clearThumbnailData();
		mMedia.closeDB();
		mSamba.clear();
	}
	
	/** 
	 * 设置过滤类型
	 */
	public void setFilterType(FileFilter filter)
	{
		mFilter = filter;
	}
	
	public boolean isDeviceList() {
		if(currentDir.equals(rootPath))
		{
			return true;
		}
		return false;
	}
	
	public boolean isPartitionList()
	{
		try
		{
			File f = new File(currentDir);
			if(!f.exists() || !f.isDirectory())
				return false;
			if(mDevices.hasMultiplePartition(currentDir)){
				return true;
			}
			else return false;
		}catch(Exception e)
		{
			return false;
		}
	}
	
	public boolean isInSambaMode(){
		if(currentDir.equals(MediaProvider.NETWORK_NEIGHBORHOOD) || mSamba.isSambaWorkgroup(currentDir) || mSamba.isSambaServices(currentDir))
			return true;
		return false;
	}
	
	public boolean isReturnItemSelected()
	{
		try
		{
			if(currentPosition >= 0 && mDataSource.size() > currentPosition)
			{
				String name = mDataSource.get(currentPosition);
				if(name.equals(MediaProvider.RETURN))
				{
					return true;
				}
				else return false;
			}
			else 
			{
				return true;
			}
			
		}catch(Exception e)
		{
			return true;
		}
	}
	
	public boolean isNetDevSelected()
	{
		String name = null;
		if(currentPosition >= 0 && mDataSource.size() > currentPosition)
		{
			name = mDataSource.get(currentPosition);
			if(LocalNetwork.getIpType(name) == LocalNetwork.IP_TYPE_UNKNOWN){
				return false;
			}
			return true;
		}
		return false;
	}
	
	/* 判断是否可以操作文件:复制/粘贴... */
	public boolean hasFileOperate()
	{
		if(fileToOperate != null)
			return true;
		else return false;
	}
	
	/* 定义文件操作  */
	public void searchForFile(String name)
	{
		new BackgroundWork(SEARCH_TYPE).execute(name);
	}
	
	public void deleteFile(String name)
	{
		/* 符合以下规则之一时，操作无效，直接返回
		 * 1.删除项为“返回”
		 * 2.为某设备的根路径
		 * 3.为某设备分区路径
		 */
		if(name.equals(MediaProvider.RETURN) || mDevices.isLocalDevicesRootPath(name) ||
				mDevices.hasMultiplePartition(name.substring(0, name.lastIndexOf("/"))))
		{
			return;
		}
		/* 符合以下规则之一时，权限不足，操作无效
		 * 1.该文件不可写
		 */
		File f = new File(name);
		if(!f.canWrite())
		{
			DisplayToast(mContext.getResources().getString(R.string.operate_fail_dueto_permission));
			return;
		}
		new BackgroundWork(DELETE_TYPE).execute(name);
	}
	
	public void copyFile(String oldLocation, String newLocation)
	{
		String msg = "";
		File old = new File(oldLocation);
		String name = old.getName();
		File newDir = new File(newLocation);
		final String[] data = {oldLocation, newLocation};
		
		/* 符合以下规则之一时，操作无效，直接返回  
		 * 1.复制项为“返回”
		 * 2.复制项为某设备路径，或为某设备分区的路径
		 * 3.粘贴时处于设备列表状态，或属于设备分区列表状态
		 * 4.复制或剪切项的粘贴路径为自己所在路径
		 */
		if(oldLocation.equals(MediaProvider.RETURN) || mDevices.isLocalDevicesRootPath(oldLocation) ||
				 newLocation.equals(rootPath) || mDevices.hasMultiplePartition(newLocation) ||
				 mDevices.hasMultiplePartition(oldLocation.substring(0, oldLocation.lastIndexOf("/"))))
		{
			return;
		}
		
		/*
		 * 不能粘贴到自己或自己的子目录中
		 */
		if(newLocation.contains(oldLocation))
		{
			if(delete_after_copy){
				msg = mContext.getResources().getString(R.string.can_not_cut) + name;
			}else{
				msg = mContext.getResources().getString(R.string.can_not_copy) + name;
			}
			if(newLocation.equals(oldLocation)){
				msg = msg + mContext.getResources().getString(R.string.target_equals_src);
			}else{
				msg = msg + mContext.getResources().getString(R.string.target_is_child);
			}
			DisplayToast(msg);
			return;
		}
		
		/* 当复制/剪切的地方有一个同名字的文件时,提示是否覆盖 */
		File newFile = new File(newDir, name);
		if(newFile.exists() &&
				newFile.getPath().equals(oldLocation) &&
				((newFile.isDirectory() && old.isDirectory()) || (newFile.isFile() && old.isFile()))){
			AlertDialog.Builder builder = new AlertDialog.Builder(mContext);
			//builder.setTitle(mContext.getResources().getString(R.string.Warning));
			//builder.setIcon(R.drawable.warning);
			builder.setMessage(mContext.getResources().getString(R.string.sure_cover_file) + name);
			builder.setCancelable(false);
			
			builder.setNegativeButton(mContext.getResources().getString(R.string.cancel), new DialogInterface.OnClickListener() {
				public void onClick(DialogInterface dialog, int which) {
					dialog.dismiss();
				}
			});
			String convice = "";
			if(delete_after_copy){
				convice = mContext.getResources().getString(R.string.cut);
			}else{
				convice = mContext.getResources().getString(R.string.copy);
			}
			builder.setPositiveButton(convice, new DialogInterface.OnClickListener() {
				public void onClick(DialogInterface dialog, int which) {
					new BackgroundWork(COPY_TYPE).execute(data);
				}
			});
			AlertDialog alert_d = builder.create();
			alert_d.show();
			return;
		} 
		
		/* 符合以下规则之一时，权限不足，操作失败
		 * 1.复制项不可读，
		 * 2.如果为剪切，还需要可写权限
		 * 2.粘贴路径不可写
		 */
		if(!old.exists())
		{
			DisplayToast(mContext.getResources().getString(R.string.operate_no_exit_file));
		}
		
		if(!old.canRead() || (delete_after_copy && !old.canWrite()) || !newDir.canWrite())
		{
			DisplayToast(mContext.getResources().getString(R.string.operate_fail_dueto_permission));
			return;
		}
		
		new BackgroundWork(COPY_TYPE).execute(data);
	}
	
	private void DisplayToast(String str){
		Toast.makeText(mContext, str, Toast.LENGTH_SHORT).show();
	}
	
	private static class ViewHolder
	{
		ImageView icon;
		TextView text;
	}
	
	public class TableRow extends ArrayAdapter<String> 
	{
    	public TableRow() 
    	{
    		super(mContext, R.layout.tablerow, mDataSource);
    	}
    	
    	@Override 
    	public View getView(int position, View convertView, ViewGroup parent)
    	{
    		ViewHolder holder;
    		if(convertView == null)
    		{
    			LayoutInflater inflater = (LayoutInflater) mContext.
    						getSystemService(Context.LAYOUT_INFLATER_SERVICE);
    			convertView = inflater.inflate(R.layout.tablerow, parent, false);
    			holder = new ViewHolder();
    			holder.icon = (ImageView)convertView.findViewById(R.id.row_image);
    			holder.text = (TextView)convertView.findViewById(R.id.text_view);
    			convertView.setTag(holder);
    		}
    		else
    		{
    			holder = (ViewHolder) convertView.getTag();
    		}
    		
    		/*
    		 * 根据类型来选择icon
    		 */
    		String path = getData(position);
    		File file = new File(path);
    		String fileName = path.substring(path.lastIndexOf("/") + 1);
    		if(mDevices.isInterStoragePath(path))
    		{
   				holder.text.setText(R.string.local_disk);
    			holder.icon.setImageResource(R.drawable.litter_disk);
   			}
   			else if(mDevices.isSdStoragePath(path))
   			{
   				holder.text.setText(R.string.sdcard);
   				holder.icon.setImageResource(R.drawable.litter_sd);
   			}
   			else if(mDevices.isUsbStoragePath(path))
   			{
   				holder.text.setText(R.string.usb);
   				holder.icon.setImageResource(R.drawable.litter_usb);
    		}
   			else if(mDevices.isSataStoragePath(path))
   			{
   				holder.text.setText(R.string.sata);
   				holder.icon.setImageResource(R.drawable.litter_sata);
   			}
   			else if(mDevices.isNetStoragePath(path)){
   				holder.text.setText(path);
   				holder.icon.setImageResource(R.drawable.litter_disk);
   			}
   			else if(path.equals(MediaProvider.RETURN))
    		{
    			holder.text.setText("");
    			holder.icon.setImageResource(R.drawable.litter_back);
    		}
   			else if(path.equals(mContext.getResources().getString(R.string.samba)))
   			{
   				holder.text.setText(path);
   				holder.icon.setImageResource(R.drawable.litter_samba);
   			}
   			else if(path.startsWith("smb://"))
   			{
   				if(path.endsWith("/")){
   					path = path.substring(0, path.length() - 1);
   					fileName = path.substring(path.lastIndexOf("/") + 1);
   				}
   				holder.text.setText(fileName);
   				holder.icon.setImageResource(R.drawable.litter_samba);
   			}
			else if (path.equals(mContext.getResources().getString(R.string.nfs))) {
   				holder.text.setText(path);
   				holder.icon.setImageResource(R.drawable.litter_nfs);
   			}
   			else if (mNfs.isNfsServer(path)) { 
   				holder.text.setText(path);
   				holder.icon.setImageResource(R.drawable.litter_nfs);
   			}
   			else if (mNfs.isNfsShare(path)) {
   				holder.text.setText(path);
   				holder.icon.setImageResource(R.drawable.litter_nfs);
    		}
    		else
    		{
    			if(file.isDirectory())
    			{
    				if(mDevices.isLocalDevicesRootPath(file.getParent()))
    				{
    					String tmp = getMappedName(path);
    					if(tmp.equals(path))
    					{
    						holder.icon.setImageResource(R.drawable.litter_file);
    					}
    					else
    					{
    						fileName = tmp;
    						holder.icon.setImageResource(R.drawable.litter_partition);
    					}
    				}
    				else
    				{
    					holder.icon.setImageResource(R.drawable.litter_file);
    				}
    			}
    			else if(TypeFilter.isMovieFile(path))
    			{
    				holder.icon.setImageResource(R.drawable.litter_video);
    			}
    			else if(TypeFilter.isMusicFile(path))
    			{
    				holder.icon.setImageResource(R.drawable.litter_music);
    			}
    			else if(TypeFilter.isPictureFile(path))
    			{
    				holder.icon.setImageResource(R.drawable.litter_picture);
    			}
    			else if(TypeFilter.isApkFile(path))
    			{
    				holder.icon.setImageResource(R.drawable.litter_apk);
    			}
    			else if(TypeFilter.isExcelFile(path))
    			{
    				holder.icon.setImageResource(R.drawable.litter_xls);
    			}
    			else if(TypeFilter.isHtml32File(path))
    			{
    				holder.icon.setImageResource(R.drawable.litter_html);
    			}
    			else if(TypeFilter.isPdfFile(path))
    			{
    				holder.icon.setImageResource(R.drawable.litter_pdf);
    			}
    			else if(TypeFilter.isPptFile(path))
    			{
    				holder.icon.setImageResource(R.drawable.litter_ppt);
    			}
    			else if(TypeFilter.isTxtFile(path))
    			{
    				holder.icon.setImageResource(R.drawable.litter_txt);
    			}
    			else
    			{
    				holder.icon.setImageResource(R.drawable.litter_txt);
    			}
    			holder.text.setText(fileName);
    		}
    		return convertView;
    	}
	}

	public class MenuItemListener implements OnClickListener
	{
		private Dialog menu;
		public MenuItemListener()
		{
		}
		@Override
		public void onClick(View v) {
			// TODO Auto-generated method stub
			if( currentPosition == -1 )
			{
				/* 如果列表失去了焦点，无操作 */
				menu.dismiss();
				return;
			}
			switch(v.getId())
			{
				/* 以下响应的是menu操作栏的内容  */
			case R.id.sort_button:
				selectFileType_dialog();
				break;
			case R.id.copy_button:
				fileToOperate = mDataSource.get(currentPosition);
				
				saveCopyFilePath(fileToOperate);
				
				delete_after_copy = false;
				break;
			case R.id.paste_button:
				if((fileToOperate != null) && (currentDir != null))
				{
					copyFile(fileToOperate, currentDir);
					//fileToOperate = null;
				}
				break;
			case R.id.cut_button:
				fileToOperate = mDataSource.get(currentPosition);
				delete_after_copy = true;
				break;
			case R.id.delete_button:
				fileToOperate = mDataSource.get(currentPosition);
				deleteFile(fileToOperate);
				fileToOperate = null;
				break;
			case R.id.rename_button:
				fileToOperate = mDataSource.get(currentPosition);
				rename(fileToOperate);
				fileToOperate = null;
				break;
			case R.id.mkdir_button:
				fileToOperate = currentDir;
				mkdir(fileToOperate);
				fileToOperate = null;
				break;
			case R.id.add_new_dev_button:
				addNewDev_dialog();
				break;
			case R.id.del_dev_button:
				delDev();
				break;
			}
			menu.dismiss();
		}
		
		public void setListenedMenu(Dialog menu)
		{
			this.menu = menu;
		}
		
		private void saveCopyFilePath(String filePath)
		{
			/* 用于测试
			 * 把每次选中的文件路径添加到一个文件中
			 */
			File appDir = mContext.getFilesDir();
			File f = new File(appDir.getAbsolutePath() + "/copy.txt");
			if(!f.exists())
			{
				try {
					f.createNewFile();
				} catch (IOException e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				}
			}
			try {
				BufferedWriter o_stream = new BufferedWriter(
						new FileWriter(f,true));
				o_stream.append(filePath);
				o_stream.append('\n');
				o_stream.flush();
				o_stream.close();
				Log.d(TAG, "append \'" + filePath + "\' to file " + f.getAbsolutePath());
			} catch (Exception e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
		}
	}
	
	private void selectFileType_dialog() {
		String sortType     = mContext.getResources().getString(R.string.sort_type);
		String alpha        = mContext.getResources().getString(R.string.alpha);
		String modifiedTime = mContext.getResources().getString(R.string.modified_time);
		String size			= mContext.getResources().getString(R.string.by_size);
		CharSequence[] type = {alpha,modifiedTime,size};
		AlertDialog.Builder builder;
		AlertDialog dialog;
		builder = new AlertDialog.Builder(mContext);
		builder.setTitle(sortType);
		builder.setItems(type, new DialogInterface.OnClickListener() {
			
			@Override
			public void onClick(DialogInterface dialog, int which) {
				switch(which)
				{
				case 0:
					sort = MediaProvider.alph;
					break;
				case 1:
					sort = MediaProvider.lastModified;
					break;
				case 2:
					sort = MediaProvider.size;
					break;
				}
				ProgressDialog dg = ProgressDialog.show(mContext, null, mContext.getResources().getString(R.string.sorting));
				sorting();
				mTable.notifyDataSetChanged();
				if(currentPosition > 0)
					list.setSelection(currentPosition - 1);
				dg.dismiss();
			}
		});
		dialog = builder.create();
	
		dialog.show();
		WindowManager.LayoutParams params = dialog.getWindow().getAttributes();
		//params.gravity = Gravity.CENTER;
		params.width  = 400;
		dialog.getWindow().setAttributes(params);
		
    }
	
	private void addNewDev_dialog(){
		final Dialog dialog = new Dialog(mContext);
		dialog.setContentView(R.layout.input_layout);
		dialog.setTitle(mContext.getResources().getString(R.string.add_new_dev));
		ImageView add_dev_icon = (ImageView)dialog.findViewById(R.id.input_icon);
		add_dev_icon.setImageResource(R.drawable.mkdir_selected);
		final EditText add_input = (EditText)dialog.findViewById(R.id.input_inputText);
		Button dev_cancel = (Button)dialog.findViewById(R.id.input_cancel_b);
		Button dev_create = (Button)dialog.findViewById(R.id.input_create_b);
		dev_create.setText(mContext.getResources().getString(R.string.add_new_dev));
		dev_create.setOnClickListener(new OnClickListener()
		{
			public void onClick(View v)
			{
				if(add_input.getText().length() < 1)
				{
					dialog.dismiss();
				}
				String name = add_input.getText().toString();
				ArrayList<String> netlist = mDevices.getNetDeviceList();
				if(LocalNetwork.getIpType(name) != LocalNetwork.IP_TYPE_UNKNOWN && (netlist == null || !netlist.contains(name))){
					DisplayToast(name + " " + mContext.getResources().getString(R.string.add_new_dev) +
							mContext.getResources().getString(R.string.success));
					mDataSource.add(name);
					mTable.notifyDataSetChanged();
					//save the net dev
					mSamba.addInCache(SmbFile.TYPE_SERVER, name);
					mDevices.saveNetDevice(name);
				}
				else
					DisplayToast(mContext.getResources().getString(R.string.add_new_dev) + 
							mContext.getResources().getString(R.string.fail));
				dialog.dismiss();
			}
		});
		dev_cancel.setOnClickListener(new OnClickListener()
		{
			public void onClick(View v)
			{
				dialog.dismiss();
			}
		});
		dialog.show();
	}
	
	private void delDev(){
		String path = mDataSource.get(currentPosition);
		mDevices.delNetDevice(path);
		mDataSource.remove(path);
		mTable.notifyDataSetChanged();
	}
	
	private void rename(final String path)
	{
		/* 以下情况操作无效，直接返回
		 * 1.选中的为“返回”，
		 * 2.选中的为某设备路径
		 */
		if(path.equals(MediaProvider.RETURN) || mDevices.isLocalDevicesRootPath(path) )
		{
			return;
		}
		/* 文件权限不足时，也不能改名 */
		File f = new File(path);
		if(!f.canWrite())
		{
			DisplayToast(mContext.getResources().getString(R.string.operate_fail_dueto_permission));
			return;
		}
		
		final String name = path.substring(path.lastIndexOf("/") + 1);
		final Dialog dialog = new Dialog(mContext);
		dialog.setContentView(R.layout.input_layout);
		dialog.setTitle(mContext.getResources().getString(R.string.rename));
		ImageView rename_icon = (ImageView)dialog.findViewById(R.id.input_icon);
		rename_icon.setImageResource(R.drawable.rename_selected);
		final EditText rename_input = (EditText)dialog.findViewById(R.id.input_inputText);
		Button rename_cancel = (Button)dialog.findViewById(R.id.input_cancel_b);
		Button rename_create = (Button)dialog.findViewById(R.id.input_create_b);
		rename_create.setText(mContext.getResources().getString(R.string.rename));
		rename_create.setOnClickListener(new OnClickListener()
		{
			public void onClick(View v)
			{
				if(rename_input.getText().length() < 1)
				{
					dialog.dismiss();
				}
				FileOperate operate = new FileOperate(mContext);
				int ret = operate.renameTarget(path, rename_input.getText().toString());
				switch(ret){
				case -1:
					DisplayToast(mContext.getResources().getString(R.string.renamed_to_exist_file));
					break;
				case 0:
					DisplayToast(name + mContext.getResources().getString(R.string.rename_to) +
							rename_input.getText().toString());
					
					String ext = "";
					try
					{
						ext = path.substring(path.lastIndexOf("."));
					}
					catch (IndexOutOfBoundsException e)
					{
						ext = "";
					}
					String dirPath = path.substring(0, path.lastIndexOf("/"));
					String newPath = dirPath + "/" + rename_input.getText().toString() + ext;
					Log.d(TAG, newPath);
					mDataSource.set(currentPosition, newPath);
					//sorting();
					mTable.notifyDataSetChanged();
					break;
				case -2:
				default:
					DisplayToast(mContext.getResources().getString(R.string.rename_fail));
					break;
				}
					
				dialog.dismiss();
			}
		});
		rename_cancel.setOnClickListener(new OnClickListener()
		{
			public void onClick(View v)
			{
				dialog.dismiss();
			}
		});
		dialog.show();
	}
	
	private void mkdir(final String path){
		/* 该目录文件权限不足时无法新建文件夹 */
		File file = new File(path);
		if(!file.canWrite()){
			DisplayToast(mContext.getResources().getString(R.string.operate_fail_dueto_permission));
			return;
		}
		final Dialog dialog = new Dialog(mContext);
		dialog.setContentView(R.layout.input_layout);
		dialog.setTitle(mContext.getResources().getString(R.string.mkdir));
		ImageView mkdir_icon = (ImageView)dialog.findViewById(R.id.input_icon);
		mkdir_icon.setImageResource(R.drawable.mkdir_selected);
		final EditText mkdir_input = (EditText)dialog.findViewById(R.id.input_inputText);
		Button mkdir_cancel = (Button)dialog.findViewById(R.id.input_cancel_b);
		Button mkdir_create = (Button)dialog.findViewById(R.id.input_create_b);
		mkdir_create.setText(mContext.getResources().getString(R.string.mkdir));
		mkdir_create.setOnClickListener(new OnClickListener()
		{
			public void onClick(View v)
			{
				if(mkdir_input.getText().length() < 1)
				{
					dialog.dismiss();
				}
				FileOperate operate = new FileOperate(mContext);
				String name = mkdir_input.getText().toString();
				if(operate.mkdirTarget(path, name))
				{
					DisplayToast(name + " " + mContext.getResources().getString(R.string.create) +
							mContext.getResources().getString(R.string.success));
					String newPath = path + "/" + name;
					Log.d(TAG, newPath);
					try{
						mDataSource.add(currentPosition + 1, newPath);
					}catch (Exception e) {
						Log.e(TAG,e.getMessage());
						mDataSource.add(newPath);
					}
					
					//sorting();
					mTable.notifyDataSetChanged();
				}
				else
					DisplayToast(mContext.getResources().getString(R.string.create) + 
							mContext.getResources().getString(R.string.fail));
				dialog.dismiss();
			}
		});
		mkdir_cancel.setOnClickListener(new OnClickListener()
		{
			public void onClick(View v)
			{
				dialog.dismiss();
			}
		});
		dialog.show();
	}
	
	private void sorting()
	{
		Object[] t = mDataSource.toArray();
		Arrays.sort(t, sort);
		mDataSource.clear();
		for(Object a:t)
		{
			mDataSource.add((String)a);
		}
	}

	private class BackgroundWork extends AsyncTask<String, Integer, ArrayList<String>> {
    	private ProgressDialog pr_dialog;
    	private int type;
    	private int copy_rtn = 0;
    	private int delete_rtn;
    	private long totalSize = 0;
    	private long totalNum  = 0;
    	private int  persent   = 0;
    	private Timer time;
		private TimerTask task;
		private String extraFlag;
		private String copyFile;
		private String newFile;
		private String deletedFile;
		private FileOperate fp;
		
		private static final int DELETE_OLD_FILES = -1;
		/* 延时弹出扫描对话框 */
		private static final int DELAY_SCAN = -2;
		private boolean isScanning = true;
    	
    	private BackgroundWork(int type) {
    		this.type = type;
    		fp = new FileOperate(mContext);
    	}
    	
    	/**
    	 * This is done on the EDT thread. this is called before 
    	 * doInBackground is called
    	 */
    	@Override
    	protected void onPreExecute() {
    		
    		switch(type) {
    			case SEARCH_TYPE:
    				showProgressDialog(R.drawable.icon, mContext.getResources().getString(R.string.hello), null, ProgressDialog.STYLE_SPINNER, true);
    				break;
    			case COPY_TYPE:
    				String title;
    				title = mContext.getResources().getString(R.string.copy_to_newDir);
    				showProgressDialog(R.drawable.icon, title, null, ProgressDialog.STYLE_HORIZONTAL, true);
    				break;
    			case DELETE_TYPE:
    				showProgressDialog(R.drawable.icon, mContext.getResources().getString(R.string.delete_files), null, ProgressDialog.STYLE_HORIZONTAL, true);
    				break;
    			case SCANFILES:
    				//showProgressDialog(R.drawable.icon, mContext.getResources().getString(R.string.scan), null, ProgressDialog.STYLE_SPINNER, false);
    				break;
    		}
    	}
    	
    	private void showProgressDialog(int icon, String title, String message, int style, final boolean cancelable)
    	{
    		pr_dialog = new ProgressDialog(mContext);
    		pr_dialog.setProgressStyle(style);
    		pr_dialog.setIcon(icon);
    		pr_dialog.setTitle(title);
    		pr_dialog.setIndeterminate(false);
    		pr_dialog.setCancelable(cancelable);
    		pr_dialog.setOnKeyListener(new DialogInterface.OnKeyListener() {
				@Override
				public boolean onKey(DialogInterface dialog, int keyCode, KeyEvent event) {
					if(event.getAction() == KeyEvent.ACTION_DOWN)
						if(keyCode == KeyEvent.KEYCODE_BACK)
							fp.setCancel();
					return false;
				}
			});
    		if(message != null)
    		{
    			pr_dialog.setMessage(message);
    		}
    		pr_dialog.show();
    		pr_dialog.getWindow().setLayout(600, 300);
    	}

    	/**
    	 * background thread here
    	 */
    	@Override
		protected ArrayList<String> doInBackground(String... params) {
			switch(type) {
				case SEARCH_TYPE:
					return null;
				case COPY_TYPE:
					copyFile = params[0];
					newFile  = params[1] + copyFile.substring(copyFile.lastIndexOf("/"));
					

					/* 如果在同个盘上剪切,相当于重命名 */
					if(delete_after_copy)
					{
						File old = new File(copyFile);
						File f = new File(newFile);
						if(old.renameTo(f))
						{
							RefreshMedia mRefresh = new RefreshMedia(mContext);
							mRefresh.notifyMediaAdd(f.getAbsolutePath());
							mRefresh.notifyMediaDelete(old.getAbsolutePath());
							return null;
						}
					}
					
					fp.scanFiles(params[0]);
					totalSize = fp.getScanSize();
					totalNum = fp.getScanNum();
					
					/* 判断磁盘空间是否足够 */
					long available = getAvailableSize(params[1]);
					if(available < totalSize){
						copy_rtn = -2;
						return null;
					}
					
					time = new Timer();
					task = new TimerTask(){
						@Override
						public void run() {
							int t;
							if(totalSize > 0)
							{
								t = (int) (fp.getCopySize() * 100 / totalSize);
							}
							else
							{
								/* 当复制的都是文件夹时,总大小为0，这时要按复制的文件数量算 */
								t = (int) (fp.getCopyNum() * 100 / totalNum);
							}
							if(t != persent)
							{
								persent = t;
							}
							if(persent > 100)
							{
								persent = 100;
							}
							publishProgress(Integer.valueOf(persent));
						}
					};
					time.schedule(task, 0, 100);
					copy_rtn = fp.copyToDirectory(params[0], params[1]);	
					
					if(delete_after_copy)
					{
						publishProgress(Integer.valueOf(DELETE_OLD_FILES));
						fp.deleteTarget(params[0]); 
					}
					return null;
				case DELETE_TYPE:
					deletedFile = params[0];
					fp.scanFiles(deletedFile);
					totalNum = fp.getScanNum();
					time = new Timer();
					task = new TimerTask()
					{
						@Override
						public void run()
						{
							int t = (int) (fp.getDeletedNum() * 100 / totalNum);
							if(t != persent)
							{
								persent = t;
							}
							if(persent > 100)
							{
								persent = 100;
							}
							publishProgress(Integer.valueOf(persent));
						}
					};
					time.schedule(task, 0, 100);
					
					int size = params.length;
					for(int i = 0; i < size; i++)
					{
						delete_rtn = fp.deleteTarget(params[i]);
					}
					return null;
				case SCANFILES:
					extraFlag = params[1];
					time = new Timer();
					task = new TimerTask(){
						@Override
						public void run()
						{
							publishProgress(Integer.valueOf(DELAY_SCAN));
						}
					};
					time.schedule(task, 200);
					return mMedia.getList(params[0], mFilter);
			}
			return null;
		}
		
    	@Override
    	  protected void onProgressUpdate(Integer... values) {
    		int i = values[0].intValue();
    		if(i >= 0)
    		{
    			pr_dialog.setProgress(values[0].intValue());
    		}
    		else
    		{
    			switch(i)
    			{
    			case DELETE_OLD_FILES:
    				pr_dialog.setTitle(mContext.getResources().getString(R.string.delete_old_files));
    				break;
    			case DELAY_SCAN:
					if(isScanning)
					{
						showProgressDialog(R.drawable.icon, mContext.getResources().getString(R.string.scan), null, ProgressDialog.STYLE_SPINNER, false);
					}
    				break;
    			}
    		}
    	}
    	
    	/**
    	 * This is called when the background thread is finished. Like onPreExecute, anything
    	 * here will be done on the EDT thread. 
    	 */
    	@Override
		protected void onPostExecute(final ArrayList<String> file) 
    	{			
			switch(type) 
			{
				case SEARCH_TYPE:				
					pr_dialog.dismiss();
					break;
				case COPY_TYPE:
					
					if(copy_rtn == -1)
					{
						DisplayToast(mContext.getResources().getString(R.string.copy_fail));
						pr_dialog.dismiss();
						return;
					}
					
					if(copy_rtn == -2){
						DisplayToast(mContext.getResources().getString(R.string.copy_fail) + "," + mContext.getResources().getString(R.string.not_enough_space));
						pr_dialog.dismiss();
						return;
					}
					
					/* 如果只是覆盖当前目录下的文件，则不需要更新文件列表  */
					if(!mDataSource.contains(newFile))
					{
						mDataSource.add(newFile);
						sorting();
						mTable.notifyDataSetChanged();
					}
					if(time != null)
					{
						time.cancel();
					}
					pr_dialog.dismiss();
					break;
				case DELETE_TYPE:
					if(delete_rtn != 0)
					{
						DisplayToast(mContext.getResources().getString(R.string.delete_fail));
						pr_dialog.dismiss();
						return;
					}
					mDataSource.remove(currentPosition);
					mTable.notifyDataSetChanged();
					if(time != null)
					{
						time.cancel();
					}
					pr_dialog.dismiss();
					break;
					
				case SCANFILES:	
					
					isScanning = false;
					if(time != null)
					{
						time.cancel();
					}
					mDataSource.addAll(file);	
					sorting();
					if(pr_dialog != null)
					{
						pr_dialog.dismiss();
					}
					mTable.notifyDataSetChanged();
					if(extraFlag.equals(BACK_TO_PRE))
					{
						int i = mPathStack.pop().intValue();
						if(currentPosition == i)
						{
							getDetailForPosition(i);
						}
						else
						{
							list.setSelection(i);
						}
					}
					else if(extraFlag.equals(GO_INTO_DIR))
					{
						mPathStack.push(Integer.valueOf(currentPosition));
						if(currentPosition == 0)
						{
							/* 当前位置不是第一个，进入目录时焦点不变（第0个保持亮高），只需要更新预览信息的内容  */
							getDetailForPosition(currentPosition);
						}
						else
						{
							/* 这时焦点会改变，需要触发onItemSelected()，使第0个亮高并且更新预览信息内容  */
							list.setSelection(0);
						}
					}
					else
					{
						list.setSelection(0);
					}
					break;
					
			}
		}
    }	
	
	private static class Config
	{
		public static boolean isSupportMouse()
		{
			return true;
		}
	}

	@Override
	public boolean onItemLongClick(AdapterView<?> arg0, View arg1, int arg2,
			long arg3) {
		onItemSelected(arg0, arg1, arg2, arg3);
		((Activity)mContext).openOptionsMenu();
		return true;
	}
}
