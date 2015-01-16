/**
 * Copyright (c) 2010, The Android-x86 Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package android.net.ethernet;

import android.net.ethernet.EthernetDevInfo;

/**
 * Interface that allows controlling and querying Ethernet connectivity.
 *
 * {@hide}
 */
interface IEthernetManager
{
    List<EthernetDevInfo> getDeviceNameList();
    void setState(int state);
    int getState( );
    void updateDevInfo(in EthernetDevInfo info);
    boolean isConfigured();
    EthernetDevInfo getSavedConfig();
    int getTotalInterface();
    void setMode(int mode);
	boolean isOn();
	boolean isDhcp();
	boolean addInterfaceToService(String name);
	void removeInterfaceFormService(String name);
	int CheckLink(String ifname);
}

