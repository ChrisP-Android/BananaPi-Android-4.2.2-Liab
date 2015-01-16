package com.android.systemui.statusbar.policy;

import java.util.ArrayList;

import com.android.systemui.R;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.media.AudioManager;
import android.os.Bundle;
import android.os.SystemService;
import android.util.Log;
import com.android.server.AudioDeviceManagerObserver;
import android.widget.Toast;
import android.os.Message;
import android.os.Handler;
import android.media.AudioSystem;
import android.view.DisplayManagerAw;

/**
 * 管理音频设备的热插拔,当音频输出设备插入时以通知方式提示用户是否需要设置从该通道输出(可选择多通道输出),当输入
 * 设备插入时,直接自动切换至该通道输入(只能单通道输入).
 * @author Chenjd
 * @since 2012-07-16
 * @email chenjd@allwinnertech.com
 */
public class SoundController extends BroadcastReceiver{

    public Context mContext;
    private static final String TAG = SoundController.class.getSimpleName();
    private static final int notificationIdForIn = 20120716;
    private static final int notificationIdForOut = 20120904;
    private AudioManager mAudioManager;
    private DisplayManagerAw mDisplayManager;
    private ArrayList<String> mAudioOutputChannels = null;
    private ArrayList<String> mAudioInputChannels = null;
    private ArrayList<String> mAudioOutputActivedChannels = null;
    private boolean justOne = true;
    public SoundController(Context context){
        mContext = context;
        mAudioManager = (AudioManager)mContext.getSystemService(Context.AUDIO_SERVICE);
        mDisplayManager = (DisplayManagerAw) mContext.getSystemService(Context.DISPLAY_SERVICE_AW);
        IntentFilter filter = new IntentFilter();
        filter.addAction(Intent.ACTION_AUDIO_PLUG_IN_OUT);
        mContext.registerReceiver(this, filter);

        mAudioOutputChannels = mAudioManager.getAudioDevices(AudioManager.AUDIO_OUTPUT_TYPE);
        mAudioInputChannels = mAudioManager.getAudioDevices(AudioManager.AUDIO_INPUT_TYPE);

        //check headphone's available
        if(AudioDeviceManagerObserver.getInstance(context).getHeadphoneAvailableState() == AudioSystem.DEVICE_STATE_AVAILABLE){
            headPhoneConnected = true;
            log("headphone is available");
            mAudioManager.setParameter("routing",String.valueOf(AudioSystem.ROUTE_HEADSET));
        }

        //init audio input channels
        ArrayList<String> ls = mAudioManager.getAudioDevices(AudioManager.AUDIO_INPUT_TYPE);
        int i = 0;
        if(ls != null){
            for(i = 0; i < ls.size(); i++){
                String st = ls.get(i);
                if(st.contains("USB")){
                    ArrayList<String> list = new ArrayList<String>();
                    list.add(st);
                    mAudioManager.setAudioDeviceActive(list, AudioManager.AUDIO_INPUT_ACTIVE);
                    break;
                }
            }
        }
        if(i == ls.size()){
            //use codec for default
            ArrayList<String> lst = new ArrayList<String>();
            lst.add(AudioManager.AUDIO_NAME_CODEC);
            mAudioManager.setAudioDeviceActive(lst, AudioManager.AUDIO_INPUT_ACTIVE);
        }





    }

    private Handler mHandler = null;
    private final int plug_in = 1;
    private final int plug_out = 1;

    //audio output devices state
    private boolean hdmiConnected = false;
    private boolean tvConnected = false;
    private boolean usbConnected = false;
    private boolean headPhoneConnected = false;
    private boolean spdifConnected = false;

    @Override
    public void onReceive(Context context, Intent intent) {

        Bundle bundle = intent.getExtras();
        if(bundle == null){
            log("bundle is null");
            return;
        }
        final int state = bundle.getInt(AudioDeviceManagerObserver.AUDIO_STATE);
        final String name = bundle.getString(AudioDeviceManagerObserver.AUDIO_NAME);
        final int type = bundle.getInt(AudioDeviceManagerObserver.AUDIO_TYPE);
        final String extra = bundle.getString(AudioDeviceManagerObserver.EXTRA_MNG);
        log("On Audio device plug in/out receive,name=" + name + " type=" + type + " state=" + state + " extra=" + extra);

        if(name == null){
            log("audio name is null");
            return;
        }

        if(name.equals(AudioManager.AUDIO_NAME_HDMI)
            || (name.equals(AudioManager.AUDIO_NAME_CODEC) && ((extra == null) || (extra != null) && !extra.equals(AudioDeviceManagerObserver.H2W_DEV)))
            || (name.equals(AudioManager.AUDIO_NAME_SPDIF))){
              //handleInternalDevice(name, type, state, extra);
        }else{
            handleExternalDevice(name, type, state, extra);
        }
    }

    private void log(String mng) {
        Log.d(TAG,mng);
    }

    private void toastPlugInNotification(String title,String mng,boolean isAudioOut) {
        Notification notification = new Notification(com.android.internal.R.drawable.stat_sys_data_usb,
                title,System.currentTimeMillis());
        String contentTitle = title;
        String contextText = mng;
        ComponentName cm = new ComponentName("com.android.settings", "com.android.settings.Settings$SoundSettingsActivity");
        Intent notificationIntent = new Intent();
        notificationIntent.setComponent(cm);
        notificationIntent.setAction("com.android.settings.SOUND_SETTINGS");

        PendingIntent contentIntent = PendingIntent.getActivity(mContext, 0, notificationIntent, 0);
        notification.setLatestEventInfo(mContext, contentTitle, contextText, contentIntent);

        notification.defaults &= ~Notification.DEFAULT_SOUND;
        notification.flags = Notification.FLAG_AUTO_CANCEL;

        NotificationManager notificationManager = (NotificationManager) mContext
            .getSystemService(Context.NOTIFICATION_SERVICE);

        notificationManager.notify(getId(isAudioOut), notification);

        handleToastMessage(mng);
    }

    private int getId(boolean isAudioOut){
        if(isAudioOut){
            return notificationIdForOut;
        }else{
            return notificationIdForIn;
        }
    }

    private void toastPlugOutNotification(String title,String mng, boolean isAudioOut){
        /*
        Notification notification = new Notification(com.android.internal.R.drawable.stat_sys_data_usb,
                title,System.currentTimeMillis());
        String contentTitle = title;
        String contextText = mng;
        notification.setLatestEventInfo(mContext, contentTitle, contextText, null);
        notification.defaults &= ~Notification.DEFAULT_SOUND;
        NotificationManager notificationManager = (NotificationManager) mContext
            .getSystemService(Context.NOTIFICATION_SERVICE);
        notificationManager.notify(getId(isAudioOut), notification);*/

        NotificationManager notificationManager = (NotificationManager) mContext
            .getSystemService(Context.NOTIFICATION_SERVICE);
        notificationManager.cancel(getId(isAudioOut));
        handleToastMessage(mng);
    }

    private void toastPlugInNotification(String title, boolean isAudioOut){
        Notification notification = new Notification(com.android.internal.R.drawable.stat_sys_data_usb,
                title,System.currentTimeMillis());
        String contentTitle = title;
        String contextText = title;
        notification.setLatestEventInfo(mContext, contentTitle, contextText, null);

        notification.defaults &= ~Notification.DEFAULT_SOUND;
        notification.flags = Notification.FLAG_AUTO_CANCEL;

        NotificationManager notificationManager = (NotificationManager) mContext
            .getSystemService(Context.NOTIFICATION_SERVICE);
        notificationManager.notify(getId(isAudioOut), notification);

        handleToastMessage(title);
    }

    private void handleToastMessage(String message){
        if(mHandler == null) return;
        Message mng = mHandler.obtainMessage();
        mng.obj = message;
        mHandler.sendMessage(mng);
    }

    private void toastMessage(String message){
        Toast.makeText(mContext, message, Toast.LENGTH_SHORT).show();
    }

    private void handleInternalDevice(String name, int type, int state, String extra){
        //handle device:hdmi, tv codec or spdif
        //if the external device is actived,do nothing
        /*
        try{
                Thread.currentThread().sleep(500); //wait display and audio mode change
            }catch(Exception e) {};
            mAudioOutputActivedChannels = mAudioManager.getActiveAudio
            Devices(AudioManager.AUDIO_OUTPUT_ACTIVE);
            log("save default channels is: " + mAudioOutputActivedChannels);
            //init audio output channels
            if(justOne){
                //check connection of usb audio
                ArrayList<String> list = mAudioManager.getAudioDevices(AudioManager.AUDIO_OUTPUT_TYPE);
                if(list != null){
                    for(String st:list){
                        if(st.contains("USB")){
                            log("the USB audio-out is cool boot");
                            list.clear();
                            list.add(st);
                            mAudioManager.setAudioDeviceActive(list,AudioManager.AUDIO_OUTPUT_ACTIVE);
                            justOne = false;
                            break;
                        }
                    }
                }
            }
            */
        mAudioOutputChannels = mAudioManager.getActiveAudioDevices(AudioManager.AUDIO_OUTPUT_ACTIVE);
        mAudioInputChannels = mAudioManager.getAudioDevices(AudioManager.AUDIO_INPUT_TYPE);
        switch(state){
        case AudioDeviceManagerObserver.PLUG_IN:
            switch(type){
            case AudioDeviceManagerObserver.AUDIO_INPUT_TYPE:
                break;
            case AudioDeviceManagerObserver.AUDIO_OUTPUT_TYPE:
                if(name.equals(AudioManager.AUDIO_NAME_CODEC)){
                    tvConnected = true;
                }else if(name.equals(AudioManager.AUDIO_NAME_HDMI)){
                    hdmiConnected = true;
                }else if(name.equals(AudioManager.AUDIO_NAME_SPDIF)){
                    spdifConnected = true;
                }
                if(headPhoneConnected || usbConnected){
                    return;
                }
                //mAudioOutputChannels.clear();
                if(!mAudioOutputChannels.contains(name)){
                    mAudioOutputChannels.add(name);
                }
                mAudioManager.setAudioDeviceActive(mAudioOutputChannels,AudioManager.AUDIO_OUTPUT_ACTIVE);
                break;
            }
            break;
        case AudioDeviceManagerObserver.PLUG_OUT:
            switch(type){
            case AudioDeviceManagerObserver.AUDIO_INPUT_TYPE:
                break;
            case AudioDeviceManagerObserver.AUDIO_OUTPUT_TYPE:
                if(name.equals(AudioManager.AUDIO_NAME_CODEC)){
                    tvConnected = false;
                }else if(name.equals(AudioManager.AUDIO_NAME_HDMI)){
                    hdmiConnected = false;
                }else if(name.equals(AudioManager.AUDIO_NAME_SPDIF)){
                    spdifConnected = false;
                }
                if(headPhoneConnected || usbConnected){
                    return;
                }
                //privileges:hdmi > spdif > codec
                mAudioOutputChannels.clear();
                if(hdmiConnected){
                    mAudioOutputChannels.add(AudioManager.AUDIO_NAME_HDMI);
                }else if(spdifConnected){
                    mAudioOutputChannels.add(AudioManager.AUDIO_NAME_SPDIF);
                }
                else if(tvConnected){
                    mAudioOutputChannels.add(AudioManager.AUDIO_NAME_CODEC);
                }else{
                    //by default
                    mAudioOutputChannels.add(AudioManager.AUDIO_NAME_HDMI);
                }
                mAudioManager.setAudioDeviceActive(mAudioOutputChannels,AudioManager.AUDIO_OUTPUT_ACTIVE);
                break;
            }
            break;
        }
    }

    private void handleExternalDevice(final String name, final int type, final int state, final String extra){
        //handle device:headphone, or usb, they have higner privileges then internal device
        if(mHandler == null){
            mHandler = new Handler(){
                @Override
                public void handleMessage(Message msg){
                    String message = (String)msg.obj;
                    toastMessage(message);
                }
            };
        }

        Thread thread = new Thread(new Runnable() {

            @Override
            public void run() {
                mAudioOutputChannels = mAudioManager.getAudioDevices(AudioManager.AUDIO_OUTPUT_TYPE);
                mAudioInputChannels = mAudioManager.getAudioDevices(AudioManager.AUDIO_INPUT_TYPE);

                String mng = "name: " + name + "  state: " + state + "  type: " + type;
                String title = null;
                String message = null;
                log(mng);
                switch(state){
                case AudioDeviceManagerObserver.PLUG_IN:
                    switch(type){
                    case AudioDeviceManagerObserver.AUDIO_INPUT_TYPE:
                        //auto change to this audio-in channel
                        log("audio input plug in");
                        ArrayList<String> audio_in = new ArrayList<String>();
                        audio_in.add(name);
                        mAudioManager.setAudioDeviceActive(audio_in, AudioManager.AUDIO_INPUT_ACTIVE);

                        title = mContext.getResources().getString(R.string.usb_audio_in_plug_in_title);
                        message = mContext.getResources().getString(R.string.usb_audio_plug_in_message);
                        toastPlugInNotification(title, message, false);
                        break;
                    case AudioDeviceManagerObserver.AUDIO_OUTPUT_TYPE:
                        log("audio output plug in");
                        //update devices state
                        if(!usbConnected && name.contains("USB")){
                            usbConnected = true;
                        }else if(extra != null && extra.equals(AudioDeviceManagerObserver.H2W_DEV)){
                            headPhoneConnected = true;
                        }

                        //switch audio output
                        ArrayList<String> audio_out = new ArrayList<String>();
                        if(extra != null && extra.equals(AudioDeviceManagerObserver.H2W_DEV)){
                            audio_out.add(name);
                            mAudioManager.setAudioDeviceActive(audio_out, AudioManager.AUDIO_OUTPUT_ACTIVE);
                            //mAudioManager.setParameter("routing",String.valueOf(AudioSystem.ROUTE_HEADSET));
                            title = mContext.getResources().getString(R.string.headphone_plug_in_title);
                            message = mContext.getResources().getString(R.string.headphone_plug_in_message);
                        }else if(name.contains("USB")){
                            audio_out.add(name);
                            mAudioManager.setAudioDeviceActive(audio_out,AudioManager.AUDIO_OUTPUT_ACTIVE);
                            title = mContext.getResources().getString(R.string.usb_audio_out_plug_in_title);
                            message = mContext.getResources().getString(R.string.usb_audio_plug_in_message);
                        }
                        toastPlugInNotification(title, message, true);
                        break;
                    }
                    break;
                case AudioDeviceManagerObserver.PLUG_OUT:
                    switch(type){
                    case AudioDeviceManagerObserver.AUDIO_INPUT_TYPE:
                        log("audio input plug out");
                        title = mContext.getResources().getString(R.string.usb_audio_in_plug_out_title);
                        message = mContext.getResources().getString(R.string.usb_audio_plug_out_message);
                        ArrayList<String> actived = mAudioManager.getActiveAudioDevices(AudioManager.AUDIO_INPUT_ACTIVE);
                        if(actived == null || actived.size() == 0 || actived.contains(name)){
                            ArrayList<String> ilist = new ArrayList<String>();
                            for(String dev:mAudioInputChannels){
                                if(dev.contains("USB")){
                                    usbConnected = true;
                                    ilist.add(dev);
                                    break;
                                }
                            }
                            if(ilist.size() == 0){
                                usbConnected = false;
                                ilist.add(AudioManager.AUDIO_NAME_CODEC);
                            }
                            mAudioManager.setAudioDeviceActive(ilist, AudioManager.AUDIO_INPUT_ACTIVE);
                            toastPlugOutNotification(title, message, false);
                        }
                        break;
                    case AudioDeviceManagerObserver.AUDIO_OUTPUT_TYPE:
                        ArrayList<String> olist = new ArrayList<String>();
                        log("audio output plug out");
                        if(extra != null && extra.equals(AudioDeviceManagerObserver.H2W_DEV)){
                            headPhoneConnected = false;
                            for(String dev:mAudioOutputChannels){
                                if(dev.contains("USB")){
                                    usbConnected = true;
                                    olist.add(dev);
                                    break;
                                }
                            }
                            if(olist.size() == 0){
                                usbConnected = false;
                                if(mDisplayManager.getDisplayOutputType(0) == DisplayManagerAw.DISPLAY_OUTPUT_TYPE_HDMI){
                                    olist.add(AudioManager.AUDIO_NAME_HDMI);
                                }else if(mDisplayManager.getDisplayOutputType(0) == DisplayManagerAw.DISPLAY_OUTPUT_TYPE_TV ||
                                    mDisplayManager.getDisplayOutputType(0) == DisplayManagerAw.DISPLAY_OUTPUT_TYPE_VGA ){
                                    olist.add(AudioManager.AUDIO_NAME_CODEC);
                                }else{
                                    olist.add(AudioManager.AUDIO_NAME_HDMI);
                                }
                            }
                            mAudioManager.setAudioDeviceActive(olist,AudioManager.AUDIO_OUTPUT_ACTIVE);
                            title = mContext.getResources().getString(R.string.headphone_plug_out_title);
                            message = mContext.getResources().getString(R.string.headphone_plug_out_message);
                        }else{
                            if(headPhoneConnected){
                                olist.add(AudioManager.AUDIO_NAME_CODEC);
                            }else{
                                for(String dev:mAudioOutputChannels){
                                    if(dev.contains("USB")){
                                        usbConnected = true;
                                        olist.add(dev);
                                        break;
                                    }
                                }
                                if(olist.size() == 0){
                                    usbConnected = false;
                                    if(mDisplayManager.getDisplayOutputType(0) == DisplayManagerAw.DISPLAY_OUTPUT_TYPE_HDMI){
                                        olist.add(AudioManager.AUDIO_NAME_HDMI);
                                    }else if(mDisplayManager.getDisplayOutputType(0) == DisplayManagerAw.DISPLAY_OUTPUT_TYPE_TV ||
                                        mDisplayManager.getDisplayOutputType(0) == DisplayManagerAw.DISPLAY_OUTPUT_TYPE_VGA ){
                                        olist.add(AudioManager.AUDIO_NAME_CODEC);
                                    }else{
                                        olist.add(AudioManager.AUDIO_NAME_HDMI);
                                    }
                                }
                            }

                            mAudioManager.setAudioDeviceActive(olist,AudioManager.AUDIO_OUTPUT_ACTIVE);
                            title = mContext.getResources().getString(R.string.usb_audio_out_plug_out_title);
                            message = mContext.getResources().getString(R.string.usb_audio_plug_out_message);
                        }
                        toastPlugOutNotification(title, message, true);
                        break;
                    }
                    break;
                }
            }
        });
        thread.start();
    }
}