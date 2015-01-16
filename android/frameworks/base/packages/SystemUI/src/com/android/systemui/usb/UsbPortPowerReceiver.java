/*
 * Copyright (C) 2010 The Android Open Source Project
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
 
 This File is creat by kinier
 
 */

package com.android.systemui.usb;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.app.Notification;
import android.app.NotificationManager;
import android.util.Log;
import com.android.systemui.R;

public class UsbPortPowerReceiver extends BroadcastReceiver{

	 private Notification mNotification;
	 private NotificationManager mNotificationManager;

	 private static final int USB_PORT_POWER_STATE_NOTIFICATION_ID = 0x22334411;

	@Override
	public void onReceive(Context context, Intent intent) {
		
			this.mNotificationManager = ((NotificationManager)context.getSystemService(Context.NOTIFICATION_SERVICE));
				
			//Log.v("kinier", intent.getStringExtra("USB_PORT_STATE"));	
				
			if("POWER OFF".equals(intent.getStringExtra("USB_PORT_STATE"))){
						
			      this.mNotification = new Notification();      
			      //this.mNotification.icon = R.drawable.hd_on;
			      this.mNotification.icon = com.android.internal.R.drawable.indicator_input_error;
			      this.mNotification.flags = Notification.FLAG_NO_CLEAR;;
			      this.mNotification.defaults = Notification.DEFAULT_SOUND;
			      this.mNotification.tickerText = context.getString(R.string.usb_port_title);
			      this.mNotification.setLatestEventInfo(context, context.getString(R.string.usb_port_message),
			      																	context.getString(R.string.usb_port_hint), null);
			      this.mNotificationManager.notify(USB_PORT_POWER_STATE_NOTIFICATION_ID, this.mNotification);
		
			}else{
			
					mNotificationManager.cancel(USB_PORT_POWER_STATE_NOTIFICATION_ID);
			
			}
	
	}
}  