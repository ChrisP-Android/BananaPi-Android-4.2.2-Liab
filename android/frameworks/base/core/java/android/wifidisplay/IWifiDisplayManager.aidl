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

package android.wifidisplay;

import android.wifidisplay.IWifiDisplayThread;
import android.view.MotionEvent;
/**
 * System private interface to the window manager.
 *
 * {@hide}
 */
interface IWifiDisplayManager
{
	int 		addWifiDisplayClient(IWifiDisplayThread client);
	int 		removeWifiDisplayClient(IWifiDisplayThread client);
	int 		startWifiDisplaySend(IWifiDisplayThread client);
	int 		endWifiDisplaySend(IWifiDisplayThread client);
	int 		startWifiDisplayReceive(IWifiDisplayThread client);
	int 		endWifiDisplayReceive(IWifiDisplayThread client);
	void 		injectMotionEvent(in MotionEvent event);
	boolean 	exitWifiDisplayReceive();
	boolean 	exitWifiDisplaySend();
	int 		getWifiDisplayStatus(); 
}

