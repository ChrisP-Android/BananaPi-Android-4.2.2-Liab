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

#undef NDEBUG
#define LOD_TAG "ISystemMixService"
#include "utils/Log.h"

#include <memory.h>
#include <stdint.h>
#include <sys/types.h>

#include <binder/Parcel.h>
#include <binder/IMemory.h>
#include "ISystemMixService.h"

#include <utils/Errors.h>
#include <stdlib.h>
#include <properties.h>
#define DEBUG true

namespace android{

enum {
	GET_PROPERTY = IBinder::FIRST_CALL_TRANSACTION,
	GET_FILEDATA,
	MOUNT,
	UMOUNT,
	SET_PROPERTY = IBinder::LAST_CALL_TRANSACTION
};

class BpSystemMixService: public BpInterface<ISystemMixService>{

public:
	BpSystemMixService(const sp<IBinder>& impl)
		: BpInterface<ISystemMixService>(impl){
	}

	int setProperty(const char *key, const char *value){
		if(DEBUG){
			ALOGV("ISystemMixService::setproperty()  key=%s value=%s ", key, value);
		}
		Parcel data, reply;
		data.writeInterfaceToken(ISystemMixService::getInterfaceDescriptor());
		data.writeCString(key);
		data.writeCString(value);
		remote()->transact(SET_PROPERTY, data, &reply);
		return reply.readInt32();
	}

	int getProperty(const char *key, char *value){
		if(DEBUG){
			ALOGV("ISystemMixService::getproperty()  key = %s", key);
		}
		Parcel data, reply;
		data.writeInterfaceToken(ISystemMixService::getInterfaceDescriptor());
		data.writeCString(key);

		remote()->transact(GET_PROPERTY, data, &reply);
		int ret = reply.readInt32();
		ALOGD("get() ret=%d", ret);
		if(ret > 0){
			const char* rpy = reply.readCString();
			strcpy(value, rpy);
			ALOGD("get() value=%s  rpy=%s",value, rpy);
		}
		return ret;
	}
	
	int getFileData(int8_t *dat, int count, const char *filePath){
	    if(DEBUG){
	        ALOGV("ISystemMixService::getFileData()  filePath = %s, count = %d", filePath, count);
	    }    
	    Parcel data, reply;
	    data.writeInterfaceToken(ISystemMixService::getInterfaceDescriptor());
	    data.writeInt32(count);
	    data.writeCString(filePath);
	    
	    remote()->transact(GET_FILEDATA, data, &reply);
	    int ret = reply.readInt32();
	    ALOGD("getfiledata() ret=%d", ret);
	    reply.read(dat, count * sizeof(int8_t));
	    return ret;
	}
	
	int mountDev(const char *src, const char *mountPoint, const char *fs, 
		unsigned int flag, const char *options){
		if(DEBUG){
			ALOGV("ISystemMixService::mount() source=%s, mountPoint=%s, fs=%s, flag=%d, options=%s", src, mountPoint, fs, flag, options);
		}		
		Parcel data, reply;
		data.writeInterfaceToken(ISystemMixService::getInterfaceDescriptor());
		data.writeCString(src);
		data.writeCString(mountPoint);
		data.writeCString(fs);
		data.writeInt32(flag);
		data.writeCString(options);
		remote()->transact(MOUNT, data, &reply);
		int ret = reply.readInt32();
		return ret;
	}
	
	int umountDev(const char *mountPoint){
		if(DEBUG){
			ALOGD("ISystemMixService::umount() mountPoint=%s", mountPoint);
		}	
		Parcel data, reply;
		data.writeInterfaceToken(ISystemMixService::getInterfaceDescriptor());
		data.writeCString(mountPoint);
		remote()->transact(UMOUNT, data, &reply);
		int ret = reply.readInt32();
		return ret;
	}
};

IMPLEMENT_META_INTERFACE(SystemMixService, "com.softwinner.ISystemMixService");

status_t BnSystemMixService::onTransact(
	uint32_t code, const Parcel& data, Parcel* reply, uint32_t flags){

	switch(code){
	case SET_PROPERTY:{
		CHECK_INTERFACE(ISystemMixService, data, reply);
		const char *key = data.readCString();
		const char *value = data.readCString();
		reply->writeInt32(setProperty(key, value));
		return NO_ERROR;
	}break;
	case GET_PROPERTY:{
		CHECK_INTERFACE(ISystemMixService, data, reply);
		const char *key = data.readCString();
		char *value = new char[PROPERTY_VALUE_MAX];
		ALOGD("BnSystemMixService key=%s", key);
		int ret = getProperty(key, value);
		ALOGD("BnSystemMixService value=%s, ret=%d", value, ret);
		reply->writeInt32(ret);
		if(ret > 0){
			reply->writeCString(value);
		}
		return NO_ERROR;
	}break;
	case GET_FILEDATA:{
	    CHECK_INTERFACE(ISystemMixService, data, reply);
	    int count = data.readInt32();
	    const char *filePath = data.readCString();
	    int8_t *d = new int8_t[count];
	    memset(d, 0, count * sizeof(int8_t));
	    int ret = getFileData(d, count, filePath);
	    ALOGD("read data is %s", d);
	    reply->writeInt32(ret);
	    reply->write(d, count * sizeof(int8_t));
	    delete[] d;
	    return NO_ERROR;
	}break;
	case MOUNT:{
		CHECK_INTERFACE(ISystemMixService, data, reply);
		const char *src = data.readCString();
		const char *mountpoint = data.readCString();
		const char *fs = data.readCString();
		unsigned int flag = data.readInt32();
		const char *options = data.readCString();
		reply->writeInt32(mountDev(src, mountpoint, fs, flag, options));
		return NO_ERROR;
	}break;
	case UMOUNT:{
		CHECK_INTERFACE(ISystemMixService, data, reply);
		const char *umountpoint = data.readCString();
		reply->writeInt32(umountDev(umountpoint));
		return NO_ERROR;
	}break;
	default:
		return BBinder::onTransact(code, data, reply, flags);
	}
}

};
