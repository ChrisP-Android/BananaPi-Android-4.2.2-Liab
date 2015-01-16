/* //device/java/android/android/view/IWindowManager.aidl
**
** Copyright 2006, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License"); 
** you may not use this file except in compliance with the License. 
** You may obtain a copy of the License at 
**
**     http://www.apache.org/licenses/LICENSE-2.0 
**
** Unless required by applicable law or agreed to in writing, software 
** distributed under the License is distributed on an "AS IS" BASIS, 
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
** See the License for the specific language governing permissions and 
** limitations under the License.
*/

package android.view;

import com.android.internal.view.IInputContext;
import com.android.internal.view.IInputMethodClient;

import android.content.res.Configuration;


/**
 * System private interface to the window manager.
 *
 * {@hide}
 */
interface IDisplayManagerAw
{
	int getDisplayCount();
	boolean getDisplayOpenStatus(int mDisplay);
	int getDisplayHotPlugStatus(int mDisplay);
	int getDisplayOutputType(int mDisplay);
	int getDisplayOutputFormat(int mDisplay);
	int getDisplayWidth(int mDisplay);
	int getDisplayHeight(int mDisplay);
	int setDisplayParameter(int mDisplay,int param0,int param1);
	int getDisplayPixelFormat(int mDisplay);
	int setDisplayMode(int mode);
	int getDisplayMode();
	int setDisplayOutputType(int mDisplay,int type,int format);
	int openDisplay(int mDisplay);
	int closeDisplay(int mDisplay);
	int setDisplayMaster(int mDisplay);
	int getDisplayMaster();
	int getMaxWidthDisplay();
	int getMaxHdmiMode();
	int setDisplayBacklightMode(int mode);
	int getDisplayBacklightMode();
    int isSupportHdmiMode(int mode);
    int isSupport3DMode();
    int getHdmiHotPlugStatus();
    int getTvHotPlugStatus();
    int setDisplayAreaPercent(int displayno,int percent);
    int getDisplayAreaPercent(int displayno);
    int setDisplayBright(int displayno,int bright);
    int getDisplayBright(int displayno);
    int setDisplayContrast(int displayno,int contrast);
    int getDisplayContrast(int displayno);
    int setDisplaySaturation(int displayno,int saturation);
    int getDisplaySaturation(int displayno);
    int setDisplayHue(int displayno,int hue);
    int getDisplayHue(int displayno);
	int startWifiDisplaySend(int mDisplay,int mode);
	int endWifiDisplaySend(int mDisplay);
	int startWifiDisplayReceive(int mDisplay,int mode);
	int endWifiDisplayReceive(int mDisplay);
	int updateSendClient(int mode);
    int set3DLayerOffset(int displayno, int left, int right);
}

