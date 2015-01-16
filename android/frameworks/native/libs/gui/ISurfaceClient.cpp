/*
**
** Copyright 2008, The Android Open Source Project
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

//#define LOG_NDEBUG 0
#define LOG_TAG "ISurfaceClient"
#include <utils/Log.h>
#include <stdint.h>
#include <sys/types.h>
#include <gui/ISurfaceClient.h>

namespace android {

enum {
    NOTIFY_CALLBACK = IBinder::FIRST_CALL_TRANSACTION,
    DATA_CALLBACK,
};

class BpSurfaceClient: public BpInterface<ISurfaceClient>
{
public:
    BpSurfaceClient(const sp<IBinder>& impl)
        : BpInterface<ISurfaceClient>(impl)
    {
    }

    // generic callback from camera service to app
    void notifyCallback(int32_t msgType, int32_t ext1, int32_t ext2,int32_t ext3,int64_t ext4)
    {
        ALOGV("notifyCallback");
        Parcel data, reply;
        data.writeInterfaceToken(ISurfaceClient::getInterfaceDescriptor());
        data.writeInt32(msgType);
        data.writeInt32(ext1);
        data.writeInt32(ext2);
        data.writeInt32(ext3);
        data.writeInt64(ext4);
        remote()->transact(NOTIFY_CALLBACK, data, &reply, IBinder::FLAG_ONEWAY);
    }
};

IMPLEMENT_META_INTERFACE(SurfaceClient, "android.surface.ISurfaceClient");

// ----------------------------------------------------------------------

status_t BnSurfaceClient::onTransact(
    uint32_t code, const Parcel& data, Parcel* reply, uint32_t flags)
{
    switch(code) {
        case NOTIFY_CALLBACK: {
            ALOGV("NOTIFY_CALLBACK");
            CHECK_INTERFACE(ISurfaceClient, data, reply);
            int32_t msgType = data.readInt32();
            int32_t ext1 = data.readInt32();
            int32_t ext2 = data.readInt32();
            int32_t ext3 = data.readInt32();
            int64_t ext4 = data.readInt64();
            notifyCallback(msgType, ext1, ext2,ext3,ext4);
            return NO_ERROR;
        } break;
        default:
            return BBinder::onTransact(code, data, reply, flags);
    }
}

// ----------------------------------------------------------------------------

}; // namespace android

