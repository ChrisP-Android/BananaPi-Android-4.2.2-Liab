/*
 * Copyright (C) 2008 The Android Open Source Project
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

#ifndef ANDROID_ISURFACECLIENT_H
#define ANDROID_ISURFACECLIENT_H

#include <utils/RefBase.h>
#include <binder/IInterface.h>
#include <binder/Parcel.h>
#include <binder/IMemory.h>
#include <utils/Timers.h>

#define SURFACECLIENT_MSG_FBCHANGED     0x50
#define SURFACECLIENT_MSG_CONVERTFB		0x51

namespace android {

class ISurfaceClient: public IInterface
{
public:
    DECLARE_META_INTERFACE(SurfaceClient);

    virtual void            notifyCallback(int32_t msgType, int32_t ext1, int32_t ext2,int32_t ext3,int64_t ext4) = 0;
};

// ----------------------------------------------------------------------------

class BnSurfaceClient: public BnInterface<ISurfaceClient>
{
public:
    virtual status_t    onTransact( uint32_t code,
                                    const Parcel& data,
                                    Parcel* reply,
                                    uint32_t flags = 0);
};

}; // namespace android

#endif
