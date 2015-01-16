package com.softwinner.TvdFileManager;

import java.io.File;
import java.net.Inet4Address;
import java.net.InetAddress;
import java.net.MalformedURLException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;

import org.apache.http.conn.util.InetAddressUtils;

import jcifs.netbios.NbtAddress;
import jcifs.smb.NtStatus;
import jcifs.smb.NtlmPasswordAuthentication;
import jcifs.smb.SmbException;
import jcifs.smb.SmbFile;
import jcifs.smb.SmbFileFilter;
import jcifs.smb.WinError;
import jcifs.util.LocalNetwork;
import android.app.Activity;
import android.app.Dialog;
import android.app.ProgressDialog;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.DialogInterface.OnCancelListener;
import android.content.Intent;
import android.content.IntentFilter;
import android.database.Cursor;
import android.net.ConnectivityManager;
import android.net.DhcpInfo;
import android.net.LinkAddress;
import android.net.LinkProperties;
import android.net.NetworkInfo;
import android.net.NetworkUtils;
import android.net.NetworkInfo.DetailedState;
import android.net.ethernet.EthernetDevInfo;
import android.net.ethernet.EthernetManager;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.Window;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.EditText;
import android.widget.Toast;

import com.softwinner.SystemMix;

public class SambaManager
{
	private Context mContext;
	private ArrayList<SmbFile> mWorkgroupList = null;
	private ArrayList<SmbFile> mServiceList = null;
	private ArrayList<SmbFile> mShareList = null;
	private ArrayList<String> mMountedPointList = null;
	private HashMap<String, String> mMap = null;
	
	private SmbFile[] mSmbList = null;
	private SmbLoginDB mLoginDB = null;
	private static File mountRoot = null;
	private ProgressDialog pr_dialog;
	
	private String TAG = "Samba";
	
	public SambaManager(Context context)
	{
		mContext = context;
		mWorkgroupList = new ArrayList<SmbFile>();
		mServiceList = new ArrayList<SmbFile>();
		mShareList = new ArrayList<SmbFile>();
		mMountedPointList = new ArrayList<String>();
		mMap = new HashMap<String, String>();
		mLoginDB = new SmbLoginDB(context);
		
		mountRoot = mContext.getDir("share", 0);
		
		initSambaProp();
	}
	
	private BroadcastReceiver mNetStatusReveicer;
	private void initSambaProp(){
		jcifs.Config.setProperty("jcifs.encoding", "GBK");
		jcifs.Config.setProperty("jcifs.util.loglevel", "0");
		LocalNetwork.setCurIpv4("172.16.10.66");
		//start to obser network state
		
		mNetStatusReveicer = new BroadcastReceiver() {
			
			@Override
			public void onReceive(Context context, Intent intent) {
				String action = intent.getAction();
				NetworkInfo info;
				LinkProperties link;
				Log.d("Samba", "Networks action:" + action);
				if(action.equals(WifiManager.NETWORK_STATE_CHANGED_ACTION)){
					info = (NetworkInfo)intent.getParcelableExtra(WifiManager.EXTRA_NETWORK_INFO);
					link = (LinkProperties)intent.getParcelableExtra(WifiManager.EXTRA_LINK_PROPERTIES);
					if(info.getDetailedState() == DetailedState.CONNECTED){
						for(LinkAddress l:link.getLinkAddresses()){
							LocalNetwork.setCurIpv4(l.getAddress().getHostAddress());
							return;
						}
					}
				}else if(action.equals(EthernetManager.NETWORK_STATE_CHANGED_ACTION)){
					EthernetManager ethMng = (EthernetManager) context.getSystemService(Context.ETHERNET_SERVICE);
					ConnectivityManager cm = (ConnectivityManager)context.getSystemService(Context.CONNECTIVITY_SERVICE);
					boolean ethConnected = cm.getNetworkInfo(ConnectivityManager.TYPE_ETHERNET).isConnected();
					info = (NetworkInfo)intent.getParcelableExtra(EthernetManager.EXTRA_NETWORK_INFO);
					link = (LinkProperties)intent.getParcelableExtra(EthernetManager.EXTRA_LINK_PROPERTIES);
					int event = intent.getIntExtra(EthernetManager.EXTRA_ETHERNET_STATE, EthernetManager.EVENT_CONFIGURATION_SUCCEEDED);
					if(event == EthernetManager.EVENT_CONFIGURATION_SUCCEEDED && ethConnected){
						for(LinkAddress l:link.getLinkAddresses()){
							LocalNetwork.setCurIpv4(l.getAddress().getHostAddress());
							return;
						}
						
					}
				}
			}
		};
		IntentFilter filter = new IntentFilter();
		filter.addAction(WifiManager.NETWORK_STATE_CHANGED_ACTION);
		filter.addAction(EthernetManager.NETWORK_STATE_CHANGED_ACTION);
		mContext.registerReceiver(mNetStatusReveicer, filter);
		
	}
	
	/* 搜索网络邻居,结果以回调方式返回 */
	public void startSearch(final String smbUrl, final OnSearchListenner ls)
	{
		Log.d(TAG, "search smb:" + smbUrl);
		
		/* 弹出搜索对话框 */
		pr_dialog = showProgressDialog(R.drawable.icon, mContext.getResources().getString(R.string.search), 
				null, ProgressDialog.STYLE_SPINNER, true);
		pr_dialog.setOnCancelListener(new OnCancelListener() {
			@Override
			public void onCancel(DialogInterface dialog) {
				ls.onFinish(true);
			}
		});
		Thread thread = new Thread(new Runnable() {
			@Override
			public void run() {
				SmbFile smbFile = null;
				try{
					if(smbUrl.equals("smb://"))
					{
						/* 搜索下面所有的workgroup*/
						smbFile = new SmbFile("smb://");
						
					}
					else if(isSambaWorkgroup(smbUrl))
					{
						/* 搜索下面所有的service */
						smbFile = getSambaWorkgroup(smbUrl);
					}
					else if(isSambaServices(smbUrl))
					{
						/* 搜索下面所有的shared文件夹 */
						smbFile = getSambaService(smbUrl);
					}
					else if(isSambaShare(smbUrl))
					{
						/* 挂载shared文件夹 */
						smbFile = getSambaShare(smbUrl);
					}else{
						smbFile = new SmbFile(smbUrl);
					}
					boolean ret = startLogin(smbFile, ls);
					
					final SmbFile f = smbFile;
					pr_dialog.cancel();
					if(!ret)
					{
						((Activity) mContext).runOnUiThread(new Runnable() {
							@Override
							public void run() {
								createLoginDialog(f, ls);
							}
						});
						
					}
				}catch(MalformedURLException e){
					mSmbList = null;
					pr_dialog.cancel();
					Log.d(TAG, e.getMessage());
					e.printStackTrace();
				}
			}
		});
		thread.start();
	}
	
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
	
	private int addFileList(SmbFile samba, final ArrayList<SmbFile> list, final OnSearchListenner ls)
	{
		Log.d(TAG, "start search " + samba.getPath());
		SmbFileFilter filter = new SmbFileFilter() {
			@Override
			public boolean accept(SmbFile file) throws SmbException {
				if(pr_dialog.isShowing())
				{
					if(!file.getPath().endsWith("$") && !file.getPath().endsWith("$/")){
						list.add(file);
						Log.d(TAG, "child:" + file.getPath());
						Log.d(TAG, file.getURL().getHost().toString());
						ls.onReceiver(file.getPath());
						return true;
					}
				}
				return false;
			}
		};
		int netStatus = NtStatus.NT_STATUS_UNSUCCESSFUL;
		try {
			SmbFile[] smbList = samba.listFiles(filter);
			if(smbList != null){
				netStatus = NtStatus.NT_STATUS_OK;
			}else{
				netStatus = NtStatus.NT_STATUS_UNSUCCESSFUL;
			}
		} catch (SmbException e) {
			netStatus = e.getNtStatus();
			e.printStackTrace();
		}
		Log.d(TAG, String.format("search end, return %x, means net status %s, win status %s", netStatus, 
				statusToStr(netStatus), SmbException.getMessageByWinerrCode(netStatus)));
		return netStatus;
	}
	
	private String statusToStr(int statusCode){
		for(int i = 0; i < NtStatus.NT_STATUS_CODES.length; i++){
			if(statusCode == NtStatus.NT_STATUS_CODES[i]){
				return NtStatus.NT_STATUS_MESSAGES[i];
			}
		}
		return null;
	}
	
	private boolean login(SmbFile samba,final ArrayList<SmbFile> list, final OnSearchListenner ls)
	{
		int ret = addFileList(samba, list, ls);
		switch(ret)
		{
		case NtStatus.NT_STATUS_OK:
			return true;
		case NtStatus.NT_STATUS_ACCESS_DENIED:
		case WinError.ERROR_ACCESS_DENIED:
			NtlmPasswordAuthentication ntlm = getLoginDataFromDB(samba);
			String path = samba.getPath();
			if(ntlm != null)
			{
				try {
					samba = new SmbFile(path, ntlm);
				} catch (MalformedURLException e) {
					// TODO Auto-generated catch block
					Log.e(TAG, e.getMessage());
					e.printStackTrace();
					return false;
				}
			}
			Log.d("Samba", String.format("ntlm domain=%s, usr=%s, psw=%s", ntlm.getDomain(), 
					ntlm.getUsername(), ntlm.getPassword()));
			int r = addFileList(samba, list, ls);
			if(r == NtStatus.NT_STATUS_OK)
				return true;
			else
			{
				mLoginDB.delete(path);
				return false;
			}
		default:
			Log.e(TAG, String.format("other error code: %x",ret));
			if(!ls.onFinish(false)){
				showMessage(R.string.access_fail);
			}
		}
		return true;
	}
	
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
	 * 
	 * @param samba
	 * @param ls
	 * @return 返回真表示登陆成功
	 */
	public boolean startLogin(SmbFile samba,final OnSearchListenner ls)
	{
		NtlmPasswordAuthentication ntlm = null;
		//fix me:when create samba with url 'smb://', the value of samba.getPath() is 'smb:////'
		if("smb:////".equals(samba.getPath())){
			mWorkgroupList.clear();
			return login(samba, mWorkgroupList, ls);
		}
		int type;
		try {
			type = samba.getType();
		} catch (SmbException e1) {
			// TODO Auto-generated catch block
			Log.e(TAG, e1.getMessage());
			e1.printStackTrace();
			return false;
		}
		Log.d(TAG, "getType " + type);
		try{
			switch(type)
			{
			case SmbFile.TYPE_WORKGROUP:
				mServiceList.clear();
				return login(samba, mServiceList, ls);
			case SmbFile.TYPE_SERVER:
				mShareList.clear();
				return login(samba, mShareList, ls);
			case SmbFile.TYPE_SHARE:
				/* 如果之前已经成功登陆过,则跳转到挂载点 */
				String mountPoint = getSambaMountedPoint(samba.getPath());
				if(mountPoint != null)
				{
					ls.onReceiver(mountPoint);
					return true;
				}
				/* 否则尝试无输入账号密码登陆,如果成功则返回下面所有的文件 */
				
				String serverName = samba.getServer();
				Log.d("chen", "server name of " + samba.getPath() + " is " + serverName);
				NbtAddress addr = NbtAddress.getByName(serverName);
				Log.d("chen", "nbt address is " + addr.getHostAddress());
				mountPoint = createNewMountedPoint(samba.getPath().substring(0, samba.getPath().length() - 1));
				umountSmb(mountPoint);		//确保该路径不属于挂载状态,防止由于程序以外中断,导致某个服务器没被卸载,下次尝试挂载时会失败

				int ret = mountSmb(samba.getPath(), mountPoint, "", "", addr.getHostAddress());
				if(ret == 0)
				{
					mMountedPointList.add(mountPoint);
					mMap.put(samba.getPath(), mountPoint);
					ls.onReceiver(mountPoint);
					return true;
				}
				else
				{
					String path = samba.getPath();
					ntlm = getLoginDataFromDB(samba);
					if(ntlm != null)
					{
						samba = new SmbFile(path, ntlm);
					}else{
						String pPath = samba.getParent();
						SmbFile parent;
						parent = new SmbFile(pPath, (NtlmPasswordAuthentication)samba.getPrincipal());
						ntlm = getLoginDataFromDB(parent);
						if(ntlm != null){
							//or use username and password of its's server
							samba = new SmbFile(path, ntlm);
						}else{
							return false;
						}
					}
					
					ret = mountSmb(samba.getPath(), mountPoint, ntlm.getUsername(), ntlm.getPassword(), addr.getHostAddress());
					if(ret == 0){
						mMountedPointList.add(mountPoint);
						mMap.put(samba.getPath(), mountPoint);
						ls.onReceiver(mountPoint);
						return true;
					}
					/* 失败则弹出登陆框进行登陆 */
					else {
						mLoginDB.delete(path);
						return false;
					}
				}
			}
		}catch(Exception e){
			e.printStackTrace();
			return false;
		}
		return false;
	}
	
	private NtlmPasswordAuthentication getLoginDataFromDB(SmbFile file)
	{
		if(file == null)
			return null;
		final int SMB_PATH_COLUME = 0;
		final int DOMAIN_COLUME = 1;
		final int USERNAME_COLUME = 2;
		final int PASSWORD_COLUME = 3;
		String[] columns = null;
		String selection = SmbLoginDB.SMB_PATH + "=?";
		String selectionArgs[] = {file.getPath()};
		String domain = null;
		String username = null;
		String password = null;
		NtlmPasswordAuthentication ntlm = null;
		Cursor cr = mLoginDB.query(columns, selection, selectionArgs, null);
		if(cr != null)
		{
			try
			{
				while (cr.moveToNext()) {
					domain = cr.getString(DOMAIN_COLUME);
					Log.d("Samba","------------------get ntlm ------------------");
					Log.d("Samba","fileDom  " + ((NtlmPasswordAuthentication)file.getPrincipal()).getDomain());
					Log.d("Samba","path     " + cr.getString(SMB_PATH_COLUME));
					Log.d("Samba","domain   " + domain);
					Log.d("Samba","username " + cr.getString(USERNAME_COLUME));
					Log.d("Samba","password " + cr.getString(PASSWORD_COLUME));
					Log.d("Samba","------------------end ntlm ------------------");
					if(domain != null && domain.equals(((NtlmPasswordAuthentication)file.getPrincipal()).getDomain()));
					{
						username = cr.getString(USERNAME_COLUME);
						password = cr.getString(PASSWORD_COLUME);
						ntlm = new NtlmPasswordAuthentication(domain, username, password);
						break;
					}
				}
			}finally
			{
				cr.close();
				cr = null;
			}
		}
		return ntlm;
	}
	
	private void createLoginDialog(final SmbFile samba, final OnSearchListenner ls)
	{
		final Dialog dg = new Dialog(mContext, R.style.menu_dialog);
		dg.setCancelable(true);
		LayoutInflater infrater = LayoutInflater.from(mContext);
		View v = infrater.inflate(R.layout.login_dialog, null);
		dg.setContentView(v);
		final EditText account = (EditText) v.findViewById(R.id.account);
		final EditText password = (EditText) v.findViewById(R.id.password);
		Button ok = (Button) v.findViewById(R.id.login_ok);
		ok.setOnClickListener(new OnClickListener() {
			@Override
			public void onClick(View arg0) {
				dg.dismiss();
				/* 开始登陆 */
				final ProgressDialog pdg = showProgressDialog(R.drawable.icon, mContext.getResources().getString(R.string.login), 
						null, ProgressDialog.STYLE_SPINNER, true);
				Thread thread = new Thread(new Runnable() {
					
					@Override
					public void run() {
						try{
							// TODO Auto-generated method stub
							String inputStr = account.getEditableText().toString();
							String domain = null;
							String user = null;
							int i = inputStr.indexOf("\\");
							if(i > 0){
								domain = inputStr.substring(0, i);
								user = inputStr.substring(i + 1, inputStr.length());
							}else{
								domain = ((NtlmPasswordAuthentication)samba.getPrincipal()).getDomain();
								user = inputStr;
							}
							NtlmPasswordAuthentication ntlm = new NtlmPasswordAuthentication(domain, 
									user, password.getEditableText().toString());
							SmbFile smbfile;
							Log.d("Samba", String.format("create dialog: ntlm domain=%s, usr=%s, psw=%s", ntlm.getDomain(), 
									ntlm.getUsername(), ntlm.getPassword()));
							smbfile = new SmbFile(samba.getPath(), ntlm);
							int type = samba.getType();
							switch (type) 
							{
							/* 如果samba类型为server,则列出所有的share */
							case SmbFile.TYPE_SERVER:
								SmbFile[] shareList = smbfile.listFiles();
								if(shareList == null)
								{
									showMessage(R.string.login_fail);
								}
								else
								{
									addLoginMessage(ntlm, smbfile.getPath());
									mShareList.clear();
									for(SmbFile file:shareList)
									{
										if(!file.getPath().endsWith("$")){
											mShareList.add(file);
											ls.onReceiver(file.getPath());
										}
									}
									ls.onFinish(true);
								}
								break;
							/* 如果samba类型为share,挂载该share,并列出所有的子文件 */
							case SmbFile.TYPE_SHARE:
								//for test
								String smbPath = samba.getPath();
								smbPath = smbPath.substring(0, smbPath.length() - 1);
								String mountedPoint = createNewMountedPoint(smbPath);
								String serverName = samba.getServer();
								Log.d("chen", "server name of " + samba.getPath() + " is " + serverName);
								NbtAddress addr = NbtAddress.getByName(serverName);
								Log.d("chen", "nbt address is " + addr.getHostAddress());
								int success = mountSmb(smbPath, mountedPoint, ntlm.getUsername(), ntlm.getPassword(), addr.getHostAddress());
								/* 挂载成功,列出子目录文件 */
								if(success == 0)
								{
									addLoginMessage(ntlm, smbPath);
									mMountedPointList.add(mountedPoint);
									mMap.put(samba.getPath(), mountedPoint);
									ls.onReceiver(mountedPoint);
									ls.onFinish(true);
								}
								else
								{
									showMessage(R.string.login_fail);
								}
								break;
							}
							pdg.dismiss();
						}catch (Exception e) {
							e.printStackTrace();
							pdg.dismiss();
						}
					}
					
				});
				thread.start();
			}
		});
		Button cancel = (Button) v.findViewById(R.id.login_cancel);
		cancel.setOnClickListener(new OnClickListener() {
			@Override
			public void onClick(View arg0) {
				dg.dismiss();	
			}
		});
		
		Window dialogWindow = dg.getWindow();
		WindowManager.LayoutParams lp = dialogWindow.getAttributes();
		lp.width = 300;
		dialogWindow.setAttributes(lp);
		dg.show();
	}
	
	/* 添加登录信息到数据库中 */
	public void addLoginMessage(NtlmPasswordAuthentication ntlm, String smbPath)
	{
		Log.d("chen", "-------------add login message------------");
		Log.d("chen", "path     " + smbPath);
		Log.d("chen", "domain   " + ntlm.getDomain());
		Log.d("chen", "username " + ntlm.getUsername());
		Log.d("chen", "password " + ntlm.getPassword());
		Log.d("chen", "-------------end adding-------------------");
		mLoginDB.insert(smbPath, ntlm.getDomain(), 
				ntlm.getUsername(), ntlm.getPassword());
	}
	
	/* 清除所有登陆点 */
	public void clear()
	{
		for(String mountPoint:mMountedPointList)
		{
			umountSmb(mountPoint);
		}
		mLoginDB.closeDB();
		mContext.unregisterReceiver(mNetStatusReveicer);
	}
	
	public boolean isSambaServices(String path)
	{
		if(LocalNetwork.getIpType(path) != LocalNetwork.IP_TYPE_UNKNOWN){
			path = "smb://" + path + "/";
		}
		for(int i = 0; i < mServiceList.size(); i++)
		{
			if(mServiceList.get(i).getPath().equals(path))
			{
				return true;
			}
		}
		return false;
	}
	
	public boolean isSambaShare(String path)
	{
		for(int i = 0; i < mShareList.size(); i++)
		{
			if(mShareList.get(i).getPath().equals(path))
			{
				return true;
			}
		}
		return false;
	}
	
	public boolean isSambaWorkgroup(String path)
	{
		for(int i = 0; i < mWorkgroupList.size(); i++)
		{
			if(mWorkgroupList.get(i).getPath().equals(path))
			{
				return true;
			}
		}
		return false;
	}
	
	public SmbFile getSambaWorkgroup(String sambaFile)
	{
		for(int i = 0; i < mWorkgroupList.size(); i++)
		{
			SmbFile file = mWorkgroupList.get(i);
			if(file.getPath().equals(sambaFile))
			{
				return file;
			}
		}
		return null;
	}
	
	public SmbFile getSambaService(String sambaFile)
	{
		if(LocalNetwork.getIpType(sambaFile) != LocalNetwork.IP_TYPE_UNKNOWN){
			sambaFile = "smb://" + sambaFile + "/";
		}
		for(int i = 0; i < mServiceList.size(); i++)
		{
			SmbFile file = mServiceList.get(i);
			if(file.getPath().equals(sambaFile))
			{
				return file;
			}
		}
		return null;
	}
	
	public SmbFile getSambaShare(String sambaFile)
	{
		for(int i = 0; i < mShareList.size(); i++)
		{
			SmbFile file = mShareList.get(i);
			if(file.getPath().equals(sambaFile))
			{
				return file;
			}
		}
		return null;
	}
	public ArrayList<SmbFile> getAllSmbWorkgroup()
	{
		return (ArrayList<SmbFile>) mWorkgroupList.clone();
	}
	
	public ArrayList<SmbFile> getAllSmbServices()
	{
		return (ArrayList<SmbFile>) mServiceList.clone();
	}
	
	public ArrayList<SmbFile> getAllSmbShared()
	{
		return (ArrayList<SmbFile>) mShareList.clone();
	}
	
	public void clearAllCache(){
		mWorkgroupList.clear();
		mServiceList.clear();
		mShareList.clear();
	}
	
	public void addInCache(int type, String path){
		SmbFile file = null;
		try{
			if(LocalNetwork.getIpType(path) != LocalNetwork.IP_TYPE_UNKNOWN){
				file = new SmbFile("smb://" + path + "/");
			}else{
				file = new SmbFile(path);
			}
		}catch(MalformedURLException e){
			e.printStackTrace();
			return;
		}
		
		switch (type) {
		case SmbFile.TYPE_WORKGROUP:
			mWorkgroupList.add(file);
			break;
		case SmbFile.TYPE_SERVER:
			mServiceList.add(file);
			break;
		case SmbFile.TYPE_SHARE:
			mShareList.add(file);
			break;
		}
	}
	
	public void removeFromCache(int type, String ip){
		ArrayList<SmbFile> list = null;
		if(LocalNetwork.getIpType(ip) != LocalNetwork.IP_TYPE_UNKNOWN){
			ip = "smb://" + ip + "/";
		}
		switch(type){
		case SmbFile.TYPE_WORKGROUP:
			list = mWorkgroupList;
			break;
		case SmbFile.TYPE_SERVER:
			list = mServiceList;
			break;
		case SmbFile.TYPE_SHARE:
			list = mShareList;
			break;
		}
		if(list == null){
			return;
		}
		for(SmbFile file:list){
			if(file.getPath().equals(ip)){
				list.remove(file);
				break;
			}
		}
	}
	
	public static String createNewMountedPoint(String path)
	{ 
		String mountedPoint = null;
		
		/* for test */
		path = path.replaceFirst("smb://", "smb_");
		path = path.replace("/", "_");
		mountedPoint = path;
		File file = new File(mountRoot,mountedPoint);
		Log.d("chen","mounted create:  " + file.getPath());
		if(!file.exists())
		{
			try {
				file.mkdir();
			} catch (Exception e) {
				Log.e("chen", "create " + mountedPoint + " fail");
				e.printStackTrace();
			}
		}
		return file.getAbsolutePath();
	}
	
	private String getSambaMountedPoint(String samba)
	{
		String mountedPoint = null;
		mountedPoint = mMap.get(samba);
		return mountedPoint;
	}
	
	public boolean isSambaMountedPoint(String mountedPoint)
	{
		Log.d("chen","mountedPoint   " + mountedPoint);
		if(mMountedPointList.size() == 0)
		{
			Log.d("chen","list is 0");
		}
		for(String item:mMountedPointList)
		{
			Log.d("chen","list.....  " + item);
			if(item.equals(mountedPoint))
			{
				return true;
			}
		}
		return false;
	}
	
	public interface OnSearchListenner
	{
		void onReceiver(String path);
		boolean onFinish(boolean success);
	}
	
	public interface OnLoginFinishListenner
	{
		void onLoginFinish(String mountedPoint);
	}
	
	private int mountSmb(String source, String target, String username, String password, String ip){
		if(source.endsWith("/")){
            source = source.substring(0, source.length() - 1);
		}
		int begin = source.lastIndexOf("smb:") + "smb:".length();
		String src = source.substring(begin, source.length());
		String mountPoint = target;
		String fs = "cifs";
		int flag = 64;
		String sharedName = src.substring(src.lastIndexOf("/") + 1, src.length());
		String server = src.substring(0, src.lastIndexOf("/"));
		String unc = String.format("%s\\%s", server, sharedName);
		String ver = "1";
		
		String options = String.format("unc=%s,ver=%s,user=%s,pass=%s,ip=%s,iocharset=%s", 
				unc, ver, username, password, ip, "utf8");
		int ret = SystemMix.Mount(src, mountPoint, fs, flag, options);
		Log.d("chen", "------------------------");
		Log.d("chen", "src            " + src);
		Log.d("chen", "mountPoint     " + mountPoint);
		Log.d("chen", "fs             " + fs);
		Log.d("chen", "flag           " + flag);
		Log.d("chen", "options        " + options);
		Log.d("chen", "ret            " + ret);
		Log.d("chen", "------------------------");
		return ret;
	}

	private int umountSmb(String target){
		return SystemMix.Umount(target);
	}
}
