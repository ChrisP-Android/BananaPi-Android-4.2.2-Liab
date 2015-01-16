package com.android.gallery3d.app;

import java.io.File;
import java.lang.ref.WeakReference;
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;

import javax.xml.parsers.SAXParser;
import javax.xml.parsers.SAXParserFactory;

import org.apache.http.HttpEntity;
import org.apache.http.HttpResponse;
import org.apache.http.HttpStatus;
import org.apache.http.NameValuePair;
import org.apache.http.client.HttpClient;
import org.apache.http.client.entity.UrlEncodedFormEntity;
import org.apache.http.client.methods.HttpPost;
import org.apache.http.impl.client.DefaultHttpClient;
import org.apache.http.message.BasicNameValuePair;
import org.apache.http.params.BasicHttpParams;
import org.apache.http.params.HttpConnectionParams;
import org.apache.http.params.HttpParams;
import org.apache.http.protocol.HTTP;
import org.xml.sax.Attributes;
import org.xml.sax.SAXException;
import org.xml.sax.helpers.DefaultHandler;

import android.annotation.TargetApi;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.AlertDialog.Builder;
import android.app.Dialog;
import android.app.DownloadManager;
import android.app.DownloadManager.Request;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.database.Cursor;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.net.Uri;
import android.os.Build;
import android.os.Environment;
import android.os.Handler;
import android.os.Message;
import android.os.StatFs;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Toast;

import com.android.gallery3d.R;
import com.android.gallery3d.app.AppConfig;


public class Update extends Thread {

    private Context mContext;
    private Handler mHandler;
    private String mApkUrl;
    private String mDiscription;
    private String mRetVersion;
    private static final String TAG = "Update";
    private static String postUrl0 = "http://apk.softwinners.com/apkupdate/";
    private static String postUrl1 = "http://www.china-ota.com/ota/service/request";
    //private static String postUrl1 = "http://42.121.236.55/ota/service/request";
    
    private static final int MSG_NULL = -1;
    private static final int APK_START_DOWNLOAD = 0;
    private static final int APK_DOWNLOAD_FINISH = 2;
    private static final int UPDATE_NETWORK_ERROR = 3;
    private static final int APK_LATEST_VERSION = 4;
    private static final int NO_INTERNET = 5;
    private static final int SHOW_UPDATE_CHECKING_DIALOG = 6;
    
    private static final String KEY_GUARD = "guid";
    private static final String KEY_SERVICE = "service_type";
    private static final String KEY_PARA = "para_serial";
    
    private static final int CHECK_TIMEOUT = 5 * 1000;
    public static boolean CheckIng = false;
    private static long mCurrTime = 0;
    private static boolean isSoftwinnerUrl = true;
    private static boolean mManual = false;
    private static boolean isLatestVersion = false;
    private DownloadManager downManager;
    private AlertDialog mCheckingDialog = null;

    public Update(Context context, long currTime, boolean manual) {
        mContext = context;
        mHandler = new UpdateHandle(this);
        mCurrTime = currTime;
        mManual = manual;
    }
    
    private static class UpdateHandle extends Handler {
        WeakReference<Update> mUpdateObjRef;  
        
        UpdateHandle(Update updateObj) {
            mUpdateObjRef = new WeakReference<Update>(updateObj);
        }
        
        @Override
        public void handleMessage(Message msg) {
            super.handleMessage(msg);
            
            Update updateObj = mUpdateObjRef.get();
            if (updateObj == null) {
                return;
            }
            
            if(msg.what != SHOW_UPDATE_CHECKING_DIALOG) {
            	updateObj.showCheckingDialog(false);
            }
            
            switch (msg.what) {
            	case SHOW_UPDATE_CHECKING_DIALOG:
            		updateObj.showCheckingDialog(true);
            		break;
	            case NO_INTERNET:
	            	updateObj.showCustomMessageDialog(R.string.str_no_internet);
	            	break;
                case APK_START_DOWNLOAD:
                    if (updateObj.mApkUrl == null) {
                        return;
                    }
                    updateObj.showDialog(updateObj.mContext.getString(R.string.update_title), updateObj.mDiscription, false);
                    break;
                case APK_LATEST_VERSION:
                	updateObj.showCustomMessageDialog(R.string.str_latest_version);
                	break;
                case APK_DOWNLOAD_FINISH:
                    updateObj.installApk();
                    updateObj.noticeInstallApk(R.string.update_download_finish);
                    break;
                case UPDATE_NETWORK_ERROR:
                    Toast.makeText(updateObj.mContext, R.string.update_check_error_message, Toast.LENGTH_SHORT).show();
                    break;
            }
        }
    }
    
    @Override
    public void run() {
    	
        if (!checkInternet()) {
        	Log.v(TAG, "no internet");
        	if(mManual) {
        		mHandler.sendEmptyMessage(NO_INTERNET);
        	}
            return;
        }

        /* already updating */
        if (CheckIng == true) {
            return;
        }

        CheckIng = true;
        NotificationManager NotificationMg = (NotificationManager) mContext.getSystemService(Context.NOTIFICATION_SERVICE);
        NotificationMg.cancel(R.string.app_name);

        downManager = (DownloadManager) mContext.getSystemService(Context.DOWNLOAD_SERVICE);
        isSoftwinnerUrl = true;
        
        if(mManual) {
        	mHandler.sendEmptyMessage(SHOW_UPDATE_CHECKING_DIALOG);
        }
        
        if (sendPost(postUrl0) == false) {
        	isSoftwinnerUrl = false;
            sendPost(postUrl1);
        }
        
        //Log.v(TAG, "check update at " + mCurrTime);
        AppConfig.getInstance(mContext.getApplicationContext()).setLong(AppConfig.LAST_UPDATE_TIME, mCurrTime);
        mHandler.sendEmptyMessage(MSG_NULL); //dismiss dialog if possiable
        
        CheckIng = false;
    }

    private void installApk() {
        String apkName = mApkUrl.substring(mApkUrl.lastIndexOf('/') + 1);
        String strApkFile = Environment.getExternalStorageDirectory().getAbsolutePath() + "/Download/" + apkName;

        Intent intent = new Intent(Intent.ACTION_VIEW);
        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        intent.setDataAndType(Uri.fromFile(new File(strApkFile)), "application/vnd.android.package-archive");
        mContext.startActivity(intent);
    }
    
    @TargetApi(11)
    private void noticeInstallApk(int id) {
        
        String apkName = mApkUrl.substring(mApkUrl.lastIndexOf('/') + 1);
        String strApkFile = Environment.getExternalStorageDirectory().getAbsolutePath() + "/Download/" + apkName;
        NotificationManager NotificationMg = (NotificationManager) mContext
                .getSystemService(Context.NOTIFICATION_SERVICE);
        
        Intent intent;
        if (id == R.string.update_download_finish) {           
            intent = new Intent(Intent.ACTION_VIEW);
            intent.setDataAndType(Uri.fromFile(new File(strApkFile)),
                    "application/vnd.android.package-archive");

        } else {
            intent = new Intent();
        }
        PendingIntent pi = PendingIntent.getActivity(mContext, 0, intent, 0);
        
        Notification.Builder builder = new Notification.Builder(mContext)
            .setContentTitle(apkName)
            .setContentText(mContext.getString(id))
            .setContentIntent(pi)
            .setSmallIcon(android.R.drawable.stat_sys_download);

        Notification notification = builder.getNotification();
        notification.flags |= Notification.FLAG_AUTO_CANCEL;       
        NotificationMg.notify(R.string.app_name, notification);
    }

    public boolean checkInternet() {
        ConnectivityManager cm = (ConnectivityManager) mContext
                .getSystemService(Context.CONNECTIVITY_SERVICE);
        NetworkInfo info = cm.getActiveNetworkInfo();
        if (info != null && info.isConnected()) {
            return true;
        } 
        
        return false;
    }

    private void download_apk(String urlApk) {
        Log.i(TAG, "download_apk urlAPK =" + urlApk);
        String apkName = urlApk.substring(urlApk.lastIndexOf('/') + 1);
        File directory = new File(Environment.getExternalStorageDirectory().getAbsolutePath() + "/Download/");
        if (!directory.exists()) {
            directory.mkdir();
        }
        
        String strApkFile = Environment.getExternalStorageDirectory().getAbsolutePath() + "/Download/" + apkName;
        File apkfile = new File(strApkFile);
        apkfile.delete();
        
        Uri uri = Uri.parse(urlApk);
        Request request = new DownloadManager.Request(uri);
        request.setDestinationInExternalPublicDir(Environment.DIRECTORY_DOWNLOADS, apkName);
        request.setTitle(apkName); 
        request.setDescription(String.format(mContext.getString(R.string.update_download_ui), apkName)); 
        
        final long downloadId = downManager.enqueue(request);
        
        Thread checkThread = new Thread() {

            @Override
            public void run() {                
                int i = 0, j = 0;
                int status = 0, oldStatus = 0;
                while (true) {
                    Cursor cursor = downManager.query(new DownloadManager.Query().setFilterById(downloadId));
                    if (cursor != null && cursor.getCount() != 0) {
                        cursor.moveToFirst();
                        status = cursor.getInt(cursor.getColumnIndex(DownloadManager.COLUMN_STATUS));
                        if (oldStatus == status) {
                            j++;
                        } else {
                            j = 0;
                        }
                        
                        oldStatus = status;
                        if (status == DownloadManager.STATUS_SUCCESSFUL) {
                            String finalStrPath = cursor.getString(cursor.getColumnIndex(DownloadManager.COLUMN_LOCAL_FILENAME));
                            Log.i(TAG, "finalStrPath = " + finalStrPath);
                            if (cursor != null) {
                                cursor.close();
                            }
                            Message msg = mHandler.obtainMessage();
                            msg.what = APK_DOWNLOAD_FINISH;
                            mHandler.dispatchMessage(msg);

                            break;
                        } else if (status == DownloadManager.STATUS_FAILED || j >= 120) {
                            Log.e(TAG, "fail to download apk");
                            
                            if (cursor != null) {
                                cursor.close();
                            }
                            
                            mHandler.post(new Runnable() {                               
                                @Override
                                public void run() {
                                    Message msg = mHandler.obtainMessage();
                                    msg.what = UPDATE_NETWORK_ERROR;
                                    mHandler.dispatchMessage(msg);                                           
                                }
                            });
                            break;
                        }
                        cursor.close();
                    }

                    if (i++ >= 7200) {
                        downManager.remove(downloadId);
                        break;
                    }
                    
                    try {   
                        Thread.sleep(500); 
                    }catch (InterruptedException e) {                              
                        e.printStackTrace();
                    }      

                }
            }
            
        };
        
        checkThread.start();
    }

    private boolean sendPost(String postUrl) {
        HttpPost post = new HttpPost(postUrl);

        Information info = new Information(mContext);
        String apkname = "apkname=" + info.mPackageName;
        String version = "version=" + info.mVersionCode;
        String hardware = "hardware=" + info.mDevice;
        String os = "os=" + info.mAndroidOS;
        String other = "other=null";
        String firmware = "a20rc1";
        String chipid = "2f16e3a54ad65be";
              

        String oldversion = AppConfig.getInstance(mContext.getApplicationContext()).getString(AppConfig.OLD_UPDATE_VERSION, "no_value");
        
        String para = apkname + ";" + version + ";" + hardware + ";" + os + ";" + "firmware=" + firmware + ";"  + "oldversion=" + oldversion + ";"
        				+ "chipid=" + chipid + ";"+ other;
        //Log.i(TAG, "" + para);
        
        List<NameValuePair> params = new ArrayList<NameValuePair>();
        params.add(new BasicNameValuePair(KEY_GUARD, "96F1ABDC1C9A4e20AB74FEC1E4EC500F"));
        params.add(new BasicNameValuePair(KEY_SERVICE, "APK_UPDATE"));
        if (isSoftwinnerUrl) {
        	params.add(new BasicNameValuePair("apkname", info.mPackageName));
        	params.add(new BasicNameValuePair("version", info.mVersionCode));
        	params.add(new BasicNameValuePair("oldversion", oldversion));
        	params.add(new BasicNameValuePair("hardware", info.mDevice));
        	params.add(new BasicNameValuePair("os", info.mAndroidOS));
        	params.add(new BasicNameValuePair("firmware", firmware));
        	params.add(new BasicNameValuePair("chipid", chipid));
        	params.add(new BasicNameValuePair("chipver", "A20Gallery201303"));
        }
        else {
        	params.add(new BasicNameValuePair(KEY_PARA, para));
        }

        HttpParams httpParameters = new BasicHttpParams();
        HttpConnectionParams.setSoTimeout(httpParameters, CHECK_TIMEOUT);
        HttpClient httpClient = new DefaultHttpClient(httpParameters);
        try {
            post.setEntity(new UrlEncodedFormEntity(params, HTTP.UTF_8));
            HttpResponse response = httpClient.execute(post);

          Log.i(TAG, "response status:  " + response.getStatusLine().getStatusCode());
            if (response.getStatusLine().getStatusCode() == HttpStatus.SC_OK) {
                HttpEntity entity = response.getEntity();

                SAXParserFactory factory = SAXParserFactory.newInstance();
                SAXParser parser = factory.newSAXParser();
                XMLContentHandler XMLhandler = new XMLContentHandler();
                parser.parse(entity.getContent(), XMLhandler);
                return true;
            } else {
                Log.i(TAG, "Can't discover new version");
                return false;
            }
        } catch (Exception e) {
            Log.i(TAG, "post http request error"); 
//            e.printStackTrace();
            return false;
        } 
    }
    
    class XMLContentHandler extends DefaultHandler {
        private boolean success = false;
        private boolean isStatusTAG = false;
        private boolean isDisTAG = false;
        private boolean isUrlTAG = false;
        private boolean isVersionTAG = false;
        private String mResult = "";
        
        private static final String TAG_STATUS = "status";
        private static final String TAG_URL = "url";
        private static final String TAG_DIS = "description";
        private static final String TAG_NEWVERSION = "newversion";

        @Override
        public void startDocument() throws SAXException {

        }

        @Override
        public void endDocument() throws SAXException {
            if (success) {
                mHandler.post(new Runnable() {
                    @Override
                    public void run() {
                        Message msg = mHandler.obtainMessage();
                        msg.what = isLatestVersion ? APK_LATEST_VERSION : APK_START_DOWNLOAD;
                        mHandler.dispatchMessage(msg);
                    }
                });  
            }
        }

        @Override
        public void characters(char[] ch, int start, int length) throws SAXException {
            if (isStatusTAG) {
                mResult = new String(ch, start, length);
                
                success = false;
                Log.v(TAG, "mResult = " + mResult);
                if (mResult.equals("success")) {
                    success = true;
                } 
                else if(mManual /*&& mResult.equals("latest_version") */ ) //manual check version
                {
                	success = true;
                	isLatestVersion = true;
                }
            }
            
            if (success == true) {
                if (isUrlTAG) {
                    mApkUrl = new String(ch, start, length);
                    Log.i(TAG, "apkUrl = " + mApkUrl);
                } else if (isDisTAG) {
                    mDiscription = new String(ch, start, length);
                    Log.i(TAG, "discription = " + mDiscription);
                } else if(isVersionTAG){
                	mRetVersion = new String(ch, start, length);
                	AppConfig.getInstance(mContext.getApplicationContext()).setString(AppConfig.OLD_UPDATE_VERSION, mRetVersion);
                }
            }
        }

        @Override
        public void startElement(String uri, String localName, String name, Attributes attrs)
                throws SAXException {
            String tagName = localName;
            Log.i(TAG, "tagName = " + tagName);
            if (tagName.equals(TAG_STATUS)) {
                isStatusTAG = true;
            } else if (tagName.equals(TAG_URL)) {
                isUrlTAG = true;
            } else if (tagName.equals(TAG_DIS)) {
                if (attrs.getValue("country").equalsIgnoreCase(Locale.getDefault().getCountry())&&
                        attrs.getValue("language").equalsIgnoreCase(Locale.getDefault().getLanguage())) {
                    isDisTAG = true;
                }
            } else if(tagName.equals(TAG_NEWVERSION)) {
            	isVersionTAG = true;
            }
        }

        @Override
        public void endElement(String uri, String localName, String name) throws SAXException {
            if (localName.equals(TAG_STATUS)) {
                isStatusTAG = false;
            } else if (localName.equals(TAG_URL)) {
                isUrlTAG = false;
            } else if (localName.equals(TAG_DIS)) {
                isDisTAG = false;
            }
            else if (localName.equals(TAG_NEWVERSION)) {
            	isVersionTAG = false;
            }
        }
    }

	private void showCheckingDialog(boolean visible) {
		if (visible && mCheckingDialog == null) {
			LayoutInflater inflater = (LayoutInflater)mContext.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
			View layout = inflater.inflate(R.layout.update_checking, null);
			mCheckingDialog = new AlertDialog.Builder(mContext).setView(layout)
					.setCancelable(true)
					.show();
		}
		else {
			if (mCheckingDialog != null) {
				mCheckingDialog.dismiss();
				mCheckingDialog = null;
			}
		}
	}
    
	private void showCustomMessageDialog(int resId) {
		new AlertDialog.Builder(mContext)
				.setMessage(mContext.getResources().getString(resId))
				.setCancelable(false)
				.setTitle(mContext.getResources().getString(R.string.app_name))
				.setPositiveButton(mContext.getResources().getString(R.string.close),
						new DialogInterface.OnClickListener() {
							public void onClick(DialogInterface dialog, int whichButton) {
								dialog.dismiss();
							}
						}).show();
	}
    
    private void showDialog(String title, String description, boolean force) {
        final Builder builder = new AlertDialog.Builder(mContext);
        builder.setTitle(title);
        builder.setMessage(description.replace("##", "\n"));
        
        if (!force) {
            builder.setPositiveButton(R.string.update_sure, new DialogInterface.OnClickListener() {
                @Override
                public void onClick(DialogInterface dialog, int which) {
                    download_apk(mApkUrl);
                }
            });

            builder.setNegativeButton(R.string.update_cancel,new DialogInterface.OnClickListener() {

                @Override
                public void onClick(DialogInterface dialog, int which) {
                    return;
                }
            });
        } else {
            builder.setPositiveButton(R.string.update_sure, new DialogInterface.OnClickListener() {
                @Override
                public void onClick(DialogInterface dialog, int which) {
                    return;
                }
            });
        }
        
        builder.setOnCancelListener(new DialogInterface.OnCancelListener() {
            @Override
            public void onCancel(DialogInterface dialog) {
            }
        });
        
        Dialog dialog = builder.create();
        dialog.setCanceledOnTouchOutside(false);
        dialog.show();
    }
}

class Information {
    private Context mContext;
    public String mVersionCode;
    public String mBrand;
    public String mDevice;
    public String mBoard;
    public String mFirmware;
    public String mAndroidOS;
    public String mCountry;
    public String mPackageName;
//    public double mLatitude = 0.0;  
//    public double mLongitude = 0.0;
    public int mScreenWidth, mScreenHeight;
    public long mSDCardTotalSize, mSDCardAvailableSize;

    private static final boolean DEBUG = false;
    private static final String TAG = "Information";
    private static final String UNKNOWN = "unknown";
    

    
    public Information(Context context) {
        mContext = context;

        getVersionCode();
        mBrand = getString("ro.product.brand");
        mDevice = getString("ro.product.device");
        mBoard = getString("ro.product.board");
        mFirmware = getString("ro.product.firmware");
        mAndroidOS = getString("ro.build.version.release");
        mCountry = mContext.getResources().getConfiguration().locale.getCountry();

        if (DEBUG) {
            Log.i(TAG, "apk version = " + mVersionCode);
            Log.i(TAG, "brand version = " + mBrand);
            Log.i(TAG, "device = " + mDevice);
            Log.i(TAG, "board = " + mBoard);
            Log.i(TAG, "firmware = " + mFirmware);
            Log.i(TAG, "android os = " + mAndroidOS);
            Log.i(TAG, "country= " + mCountry);
            Log.i(TAG, "mPackageName= " + mPackageName);
            
//            Log.i(TAG, "Locale.getDefault().getCountry() = " + Locale.getDefault().getCountry());
//            Log.i(TAG, "Locale.getDefault().getCountry() = " + Locale.getDefault().getLanguage());
//            Log.i(TAG, "Locale.getDefault().getDisplayCountry() = " + Locale.getDefault().getDisplayCountry());
//            Log.i(TAG, "Locale.getDefault().getDisplayLanguage() = " + Locale.getDefault().getDisplayLanguage());
            
            //Log.i(TAG, "latitude=" + mLatitude + " longitude="+mLongitude);
            Log.i(TAG, "mScreenWidth=" + mScreenWidth + " mScreenHeight="+mScreenHeight);
            Log.i(TAG, "mSDCardTotalSize=" + mSDCardTotalSize + " mSDCardAvailableSize="+mSDCardAvailableSize);
        }
    }

    private String getString(String property) {                
        return android.os.SystemProperties.get(property, UNKNOWN);
    }

    private void getVersionCode() {
        mPackageName = mContext.getPackageName();

        try {
            PackageInfo packageInfo = mContext.getPackageManager().getPackageInfo(mPackageName, 0);
            mVersionCode = String.valueOf(packageInfo.versionCode);
        } catch (Exception e) {

        }
    }

    public boolean isPackageExist(String packagename) {
        PackageManager pm = mContext.getPackageManager();
        try {
            pm.getPackageInfo(packagename, 0);
        } catch (NameNotFoundException e) {
            Log.e(TAG, packagename + " is not exist");
            return false;
        }
        return true;
    }

}

