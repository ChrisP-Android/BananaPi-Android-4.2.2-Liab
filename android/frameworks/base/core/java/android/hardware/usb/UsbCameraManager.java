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
 */


package android.hardware.usb;

public class UsbCameraManager {
    
    public static final String  USB_CAMERA_TOTAL_NUMBER     = "UsbCameraTotalNumber";
    public static final String  USB_CAMERA_STATE            = "UsbCameraState";
    public static final int     PLUG_IN                     = 1;
    public static final int     PLUG_OUT                    = 0;
    public static final String  USB_CAMERA_NAME             = "UsbCameraName";
    public static final String  EXTRA_MNG                   = "extral_mng";

    public static final String ACTION_USB_CAMERA_PLUG_IN_OUT =
            "android.hardware.usb.action.USB_CAMERA_PLUG_IN_OUT";

}