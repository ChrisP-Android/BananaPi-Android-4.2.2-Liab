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


#define NDEBUG 0
#define LOG_TAG "SystemMixService"
#include "SystemMixService.h"
#include <binder/IServiceManager.h>
#include <utils/misc.h>
#include <utils/Log.h>
#include <stdio.h>
#include <malloc.h>
#include <dirent.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/vfs.h>
#include <sys/types.h>
#include <time.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/mount.h>
#include <errno.h>

#ifdef HAVE_ANDROID_OS      // just want PAGE_SIZE define
#include <asm/page.h>
#else
#include <sys/user.h>
#endif

#include <cutils/properties.h>

#define DEBUG true

namespace android{

void SystemMixService::instantiate(){
	defaultServiceManager()->addService(
    	String16("softwinner.systemmix"), new SystemMixService());
}

SystemMixService::SystemMixService(){
	ALOGD("SystemMixService created");
}

SystemMixService::~SystemMixService(){
	ALOGD("SystemMixService destroyed");
}

int SystemMixService::setProperty(const char *key, const char *value){
	if(DEBUG){
		ALOGD("SystemMixService::setproperty()  key = %s  value = %s", key, value);
	}
	int ret = property_set(key, value);
	return ret;
}

int SystemMixService::getProperty(const char *key, char *value){
	if(DEBUG){
		ALOGD("SystemMixService::getproperty()  key = %s", key);
	}

	int ret = property_get(key, value, 0);
	ALOGD("SystemMixService::get()  value = %s", value);
	return ret;

}

int SystemMixService::getFileData(int8_t *data, int count, const char *filePath){
    if(DEBUG){
        ALOGD("SystemMixService::getFileData()  filepath=%s count=%d", filePath, count);    
    }    
    if(filePath == NULL){
        return 0;    
    }
    if(access(filePath, F_OK | R_OK) != 0){
        ALOGE("file access error");
        return 0;    
    }
    int MAX_NUM = 2048;
    FILE *srcFp;
    if((srcFp = fopen(filePath, "r")) == NULL){
        ALOGE("cannot open file %s to read", filePath);
        return 0;    
    }
    
    int remainder = count;
    int total = 0;
    int num = 1;
    int8_t *p = data;
    if(count <= 0){
        fclose(srcFp);
        return 0;    
    }
    while(remainder > 0 && num > 0){
        if(remainder >= MAX_NUM){
            num = fread(p, sizeof(int8_t), MAX_NUM, srcFp);
            remainder = remainder - num;
            p = p + num;
            total += num;   
        }else{
            num = fread(p, sizeof(int8_t), remainder, srcFp);
            remainder = remainder - num;
            p = p + num;
            total += num;
        }
    }
    fclose(srcFp);
    return total;
}

int SystemMixService::mountDev(const char *src, const char *mountPoint, const char *fs, 
		unsigned int flag, const char *options){
	int ret = mount(src, mountPoint, fs, flag, options);
	ALOGD("============Mount============");
	ALOGD("src 			%s", src);
	ALOGD("mountPoint	%s", mountPoint);
	ALOGD("fs			%s", fs);
	ALOGD("flag			%d", flag);
	ALOGD("options		%s", options);
	ALOGD("ret			%d", ret);
	if(ret != 0){
		ALOGD("errno			%s", strerror(errno));
	}
	ALOGD("=============================");
	return (ret == 0) ? ret : errno;
}

int SystemMixService::umountDev(const char *mountPoint){
	int ret = umount(mountPoint);
	ALOGD("============Mount============");
	ALOGD("umountPoint	%s", mountPoint);
	ALOGD("ret			%d", ret);
	if(ret != 0){
		ALOGD("errno			%s", strerror(errno));
	}
	ALOGD("=============================");
	return (ret == 0) ? ret : errno;	
}
}
