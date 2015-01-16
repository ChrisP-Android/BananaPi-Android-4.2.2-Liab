/*
 * Copyright (C) 2006 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package android.view;

import android.content.pm.ActivityInfo;
import android.graphics.PixelFormat;
import android.os.IBinder;
import android.os.Parcel;
import android.os.Parcelable;
import android.text.TextUtils;
import android.util.Log;
import android.content.Context;
import android.os.Binder;
import android.os.RemoteException;
import android.os.IBinder;
import android.os.ServiceManager;
import android.view.IDisplayManagerAw;
import android.os.SystemProperties;

/**
 * The interface that apps use to talk to the display manager.
 * <p>
 * Use <code>Context.getSystemService(Context.WINDOW_SERVICE)</code> to get one of these.
 *
 * @see android.content.Context#getSystemService
 * @see android.content.Context#DISPLAY_SERVICE
 */
public class DisplayManagerAw 
{
	private static final String TAG = "DisplayManagerAw";
    /**
     * Use this method to get the default Display object.
     * 
     * @return default Display object
     */
    public static final String EXTRA_HDMISTATUS = "hdmistatus";
    
    /**
     * Extra for {@link android.content.Intent#ACTION_TVDACSTATUS_CHANGED}:
     * integer containing the current health constant.
     */
    public static final String EXTRA_TVSTATUS = "tvdacstatus";

    /**
     * Extra for {@link android.content.Intent#ACTION_DISPLAY_OUTPUT_CHANGED}:
     * integer containing the current health constant.
     */
    public static final String EXTRA_DISPLAY_TYPE = "display_type";

    /**
     * Extra for {@link android.content.Intent#ACTION_DISPLAY_OUTPUT_CHANGED}:
     * integer containing the current health constant.
     */
    public static final String EXTRA_DISPLAY_MODE = "display_mode";
    
    public static final int DISPLAY_DEVICE_ON					 = 0;
	public static final int DISPLAY_DEVICE_OFF					 = 1;
		
	public static final int DISPLAY_DEVICE_PLUGIN			 	 = 0;
	public static final int DISPLAY_DEVICE_PLUGOUT				 = 1;

	public static final int DISPLAY_OUTPUT_TYPE_NONE			 = 0;
    public static final int DISPLAY_OUTPUT_TYPE_LCD 			 = 1;
    public static final int DISPLAY_OUTPUT_TYPE_TV 				 = 2;
    public static final int DISPLAY_OUTPUT_TYPE_HDMI			 = 3;
    public static final int DISPLAY_OUTPUT_TYPE_VGA				 = 4;
                                                            	 
    public static final int DISPLAY_MODE_SINGLE					 = 0;
	public static final int DISPLAY_MODE_DUALLCD				 = 1;
	public static final int DISPLAY_MODE_DUALDIFF				 = 2;
	public static final int DISPLAY_MODE_DUALSAME				 = 3;
	public static final int DISPLAY_MODE_DUALSAME_TWO_VIDEO	     = 4;
    public static final int DISPLAY_MODE_SINGLE_VAR              = 5;
    public static final int DISPLAY_MODE_SINGLE_VAR_BE           = 6;
    public static final int DISPLAY_MODE_SINGLE_FB_VAR           = 7;
    public static final int DISPLAY_MODE_SINGLE_FB_GPU           = 8;

	public static final int DISPLAY_TVDAC_NONE					 = 0;
	public static final int DISPLAY_TVDAC_YPBPR					 = 1;
	public static final int DISPLAY_TVDAC_CVBS					 = 2;
	public static final int DISPLAY_TVDAC_SVIDEO				 = 3;
	public static final int DISPLAY_TVDAC_ALL				     = 4;
	
	public static final int	DISPLAY_TVFORMAT_480I                = 0;
    public static final int DISPLAY_TVFORMAT_576I                = 1;
    public static final int DISPLAY_TVFORMAT_480P                = 2;
    public static final int DISPLAY_TVFORMAT_576P                = 3;
    public static final int DISPLAY_TVFORMAT_720P_50HZ           = 4;
    public static final int DISPLAY_TVFORMAT_720P_60HZ           = 5;
    public static final int DISPLAY_TVFORMAT_1080I_50HZ          = 6;
    public static final int DISPLAY_TVFORMAT_1080I_60HZ          = 7;
    public static final int DISPLAY_TVFORMAT_1080P_24HZ          = 8;
    public static final int DISPLAY_TVFORMAT_1080P_50HZ          = 9;
    public static final int DISPLAY_TVFORMAT_1080P_60HZ          = 0xa;
    public static final int DISPLAY_TVFORMAT_PAL                 = 0xb;
    public static final int DISPLAY_TVFORMAT_PAL_SVIDEO          = 0xc;
    public static final int DISPLAY_TVFORMAT_PAL_CVBS_SVIDEO     = 0xd;
    public static final int DISPLAY_TVFORMAT_NTSC                = 0xe;
    public static final int DISPLAY_TVFORMAT_NTSC_SVIDEO         = 0xf;
    public static final int DISPLAY_TVFORMAT_NTSC_CVBS_SVIDEO    = 0x10;
    public static final int DISPLAY_TVFORMAT_PAL_M               = 0x11;
    public static final int DISPLAY_TVFORMAT_PAL_M_SVIDEO        = 0x12;
    public static final int DISPLAY_TVFORMAT_PAL_M_CVBS_SVIDEO   = 0x13;
    public static final int DISPLAY_TVFORMAT_PAL_NC              = 0x14;
    public static final int DISPLAY_TVFORMAT_PAL_NC_SVIDEO       = 0x15;
    public static final int DISPLAY_TVFORMAT_PAL_NC_CVBS_SVIDEO  = 0x16;
    
    
    public static final int DISPLAY_VGA_H1680_V1050    			 = 0x17;
    public static final int DISPLAY_VGA_H1440_V900     			 = 0x18;
    public static final int DISPLAY_VGA_H1360_V768     			 = 0x19;
    public static final int DISPLAY_VGA_H1280_V1024    			 = 0x1a;
    public static final int DISPLAY_VGA_H1024_V768     			 = 0x1b;
    public static final int DISPLAY_VGA_H800_V600      			 = 0x1c;
    public static final int DISPLAY_VGA_H640_V480      			 = 0x1d;
    public static final int DISPLAY_VGA_H1440_V900_RB  			 = 0x1e;
    public static final int DISPLAY_VGA_H1680_V1050_RB 			 = 0x1f;
    public static final int DISPLAY_VGA_H1920_V1080_RB 			 = 0x20;
    public static final int DISPLAY_VGA_H1920_V1080    			 = 0x21;
    public static final int DISPLAY_VGA_H1280_V720     			 = 0x22;

    public static final int DISPLAY_TVFORMAT_1080P_25HZ          = 0x23;
    public static final int DISPLAY_TVFORMAT_1080P_30HZ          = 0x24;
    public static final int DISPLAY_TVFORMAT_1080P_24HZ_3D_FP    = 0x25;
    public static final int DISPLAY_TVFORMAT_720P_50HZ_3D_FP     = 0x26;
    public static final int DISPLAY_TVFORMAT_720P_60HZ_3D_FP     = 0x27;
    
    public static final int MINIMUX_BRIGHTNESS = 20;
    public static final int MAXIMUX_BRIGHTNESS = 100;
    public static final int MINIMUX_SATURATION = 20;
    public static final int MAXIMUX_SATURATION = 100;
    public static final int MINIMUX_CONTRAST = 20;
    public static final int MAXIMUX_CONTRAST = 100;
    public static final int MINIMUX_DISP_AREA = 90;
    public static final int MAXIMUX_DISP_AREA = 100;
    public static final int DEF_BRIGHTNESS = 50;
    public static final int DEF_SATURATION = 50;
    public static final int DEF_CONTRAST = 50;
    public static final int DEF_DISP_AREA = 95;

    private IDisplayManagerAw mService;
    private IBinder mToken = new Binder();
	
    public DisplayManagerAw() 
    {
        mService = IDisplayManagerAw.Stub.asInterface(ServiceManager.getService(Context.DISPLAY_SERVICE_AW));
    }
    
	public int getDisplayCount()
	{
		try 
		{
			if(mService == null)
			{
				mService = IDisplayManagerAw.Stub.asInterface(ServiceManager.getService(Context.DISPLAY_SERVICE_AW));
			}
            return  mService.getDisplayCount();
        } 
        catch (RemoteException ex) 
        {
            return -1;
        }
	}
	
	public boolean getDisplayOpenStatus(int mDisplay)
	{
		try 
		{
			if(mService == null)
			{
				mService = IDisplayManagerAw.Stub.asInterface(ServiceManager.getService(Context.DISPLAY_SERVICE_AW));
			}
            return  mService.getDisplayOpenStatus(mDisplay);
        } 
        catch (RemoteException ex) 
        {
            return false;
        }
	}

	public int getDisplayHotPlugStatus(int mDisplay)
	{
		try 
		{
			if(mService == null)
			{
				mService = IDisplayManagerAw.Stub.asInterface(ServiceManager.getService(Context.DISPLAY_SERVICE_AW));
			}
            return  mService.getDisplayHotPlugStatus(mDisplay);
        } 
        catch (RemoteException ex) 
        {
            return -1;
        }
	}
	
	public int getDisplayOutputType(int mDisplay)
	{
		try 
		{
			if(mService == null)
			{
				mService = IDisplayManagerAw.Stub.asInterface(ServiceManager.getService(Context.DISPLAY_SERVICE_AW));
			}
            return  mService.getDisplayOutputType(mDisplay);
        } 
        catch (RemoteException ex) 
        {
            return -1;
        }
	}
	
	public int getDisplayOutputFormat(int mDisplay)
	{
		try 
		{
			if(mService == null)
			{
				mService = IDisplayManagerAw.Stub.asInterface(ServiceManager.getService(Context.DISPLAY_SERVICE_AW));
			}
            return  mService.getDisplayOutputFormat(mDisplay);
        } 
        catch (RemoteException ex) 
        {
            return -1;
        }
	}
	
	public int getDisplayWidth(int mDisplay)
	{
		try 
		{
            return  mService.getDisplayWidth(mDisplay);
        } 
        catch (RemoteException ex) 
        {
            return -1;
        }
	}
	
	public int getDisplayHeight(int mDisplay)
	{
		try 
		{
            return  mService.getDisplayHeight(mDisplay);
        } 
        catch (RemoteException ex) 
        {
            return -1;
        }
	}
	
	public int getDisplayPixelFormat(int mDisplay)
	{
		try 
		{
            return  mService.getDisplayPixelFormat(mDisplay);
        } 
        catch (RemoteException ex) 
        {
            return -1;
        }
	}

	public int setDisplayParameter(int mDisplay,int param0,int param1)
	{
		try 
		{
            return  mService.setDisplayParameter(mDisplay,param0,param1);
        } 
        catch (RemoteException ex) 
        {
            return -1;
        }
	}
	
	public int setDisplayMode(int mode)
	{
		try 
		{
			setDisplayBacklightMode(1);
            return  mService.setDisplayMode(mode);
        } 
        catch (RemoteException ex) 
        {
            return -1;
        }
	}

	public int getDisplayMode()
	{
		try 
		{
            return  mService.getDisplayMode();
        } 
        catch (RemoteException ex) 
        {
            return -1;
        }
	}
	
	public int setDisplayOutputType(int mDisplay,int type,int format)
	{
		try 
		{
            return  mService.setDisplayOutputType(mDisplay,type,format);
        } 
        catch (RemoteException ex) 
        {
            return -1;
        }
	}
	
	public int openDisplay(int mDisplay)
	{
		try 
		{
            return  mService.openDisplay(mDisplay);
        } 
        catch (RemoteException ex) 
        {
            return -1;
        }
	}
	
	public int closeDisplay(int mDisplay)
	{
		try 
		{
            return  mService.closeDisplay(mDisplay);
        } 
        catch (RemoteException ex) 
        {
            return -1;
        }
	}
    
	public int setDisplayBacklightMode(int mode)
	{
		try
		{
			return mService.setDisplayBacklightMode(mode);
		}
		catch (RemoteException ex)
		{
			return -1;
		}
	}
	
	public int setDisplayMaster(int mDisplay)
	{
		try 
		{
            return  mService.setDisplayMaster(mDisplay);
        } 
        catch (RemoteException ex) 
        {
            return -1;
        }
	}

	public int getDisplayMaster()
	{
		try 
		{
            return  mService.getDisplayMaster();
        } 
        catch (RemoteException ex) 
        {
            return -1;
        }
	}
	
	public int getMaxWidthDisplay()
	{
		try 
		{			
            return  mService.getMaxWidthDisplay();
        } 
        catch (RemoteException ex) 
        {
            return -1;
        }
	}
	
	public int getMaxHdmiMode()
	{
		try 
		{			
            return  mService.getMaxHdmiMode();
        } 
        catch (RemoteException ex) 
        {
            return -1;
        }
	}
	
	public int getDisplayBacklightMode()
	{
		try
		{
			return mService.getDisplayBacklightMode();
		}
		catch (RemoteException ex)
		{
			return -1;
		}
	}

    public int isSupportHdmiMode(int mode)
    {
        try 
        {
            return  mService.isSupportHdmiMode(mode);
        } 
        catch (RemoteException ex) 
        {
            return -1;
        }
    }
     
    public int isSupport3DMode()
    {
        try 
        {
            return  mService.isSupport3DMode();
        } 
        catch (RemoteException ex) 
        {
            return -1;
        }
    }
     
    public int getHdmiHotPlugStatus()
    {
        try 
        {
            return  mService.getHdmiHotPlugStatus();
        } 
        catch (RemoteException ex) 
        {
            return -1;
        }
    }
     
    public int getTvHotPlugStatus()
    {
        try 
        {
            return  mService.getTvHotPlugStatus();
        } 
        catch (RemoteException ex) 
        {
            return -1;
        }
    }
     
    public int setDisplayAreaPercent(int displayno,int percent)
    {
        try 
        {
            return  mService.setDisplayAreaPercent(displayno,percent);
        } 
        catch (RemoteException ex) 
        {
            return -1;
        }
    }
     
    public int getDisplayAreaPercent(int displayno)
    {
        try 
        {
            return  mService.getDisplayAreaPercent(displayno);
        } 
        catch (RemoteException ex) 
        {
            return -1;
        }
    }
     
    public int setDisplayBright(int displayno,int bright)
    {
        try 
        {
            return  mService.setDisplayBright(displayno,bright);
        } 
        catch (RemoteException ex) 
        {
            return -1;
        }
    }
     
     public int getDisplayBright(int displayno)
     {
        try 
        {
            return  mService.getDisplayBright(displayno);
        } 
        catch (RemoteException ex) 
        {
            return -1;
        }
     }
     
     public int setDisplayContrast(int displayno,int contrast)
     {
        try 
        {
            return  mService.setDisplayContrast(displayno,contrast);
        } 
        catch (RemoteException ex) 
        {
            return -1;
        }
     }
     
     public int getDisplayContrast(int displayno)
     {
        try 
        {
            return  mService.getDisplayContrast(displayno);
        } 
        catch (RemoteException ex) 
        {
            return -1;
        }
     }
     
     public int setDisplaySaturation(int displayno,int saturation)
     {
        try 
        {
            return  mService.setDisplaySaturation(displayno,saturation);
        } 
        catch (RemoteException ex) 
        {
            return -1;
        }
     }
     
     public int getDisplaySaturation(int displayno)
     {
        try 
        {
            return  mService.getDisplaySaturation(displayno);
        } 
        catch (RemoteException ex) 
        {
            return -1;
        }
     }

     
     public int setDisplayHue(int displayno,int hue)
     {
        try 
        {
            return  mService.setDisplayHue(displayno,hue);
        } 
        catch (RemoteException ex) 
        {
            return -1;
        }
     }
     
     public int getDisplayHue(int displayno)
     {
        try 
        {
            return  mService.getDisplayHue(displayno);
        } 
        catch (RemoteException ex) 
        {
            return -1;
        }
     }
     
     public int set3DLayerOffset(int displayno, int left, int right){
         try 
         {
             return  mService.set3DLayerOffset(displayno, left, right);
         } 
         catch (RemoteException ex) 
         {
             return -1;
         }
     }
}

