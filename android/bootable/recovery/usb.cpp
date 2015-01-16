#include <errno.h>
#include <stdlib.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/time.h>

#include "usb.h"

#define MAX_DISK 4
#define MAX_PARTITION 8
#define TIME_OUT 6000000

static const char *USB_ROOT = "/usb/";
struct timeval tpstart,tpend;
float timeuse = 0;

void startTiming(){
    gettimeofday(&tpstart, NULL);
}

void endTimming(){
    gettimeofday(&tpend, NULL);
    timeuse = 1000000 * (tpend.tv_sec - tpstart.tv_sec) +
        (tpend.tv_usec - tpstart.tv_usec);
    LOGD("spend Time %f\n", timeuse);
}

int check_file_exists(const char * path){
    int ret = -1;
    if(path == NULL){
        return -1;
    }
    ret = access(path, F_OK);
    return ret;
}

int ensure_dev_mounted(const char * devPath,const char * mountedPoint){
    int ret;
    if(devPath == NULL || mountedPoint == NULL){
        return -1;
    }
    mkdir(mountedPoint, 0755);  //in case it doesn't already exist
    startTiming();
    ret = mount(devPath, mountedPoint, "vfat",
        MS_NOATIME | MS_NODEV | MS_NODIRATIME, "");
    endTimming();
    if(ret == 0){
        LOGD("mount %s with fs 'vfat' success\n", devPath);
        return 0;
    }else{
        startTiming();
        ret = mount(devPath, mountedPoint, "ntfs",
            MS_NOATIME | MS_NODEV | MS_NODIRATIME, "");
        endTimming();
        if(ret == 0){
            LOGD("mount %s with fs 'ntfs' success\n", devPath);
            return 0;
        }else{
            startTiming();
            ret = mount(devPath, mountedPoint, "ext4",
                MS_NOATIME | MS_NODEV | MS_NODIRATIME, "");
            endTimming();
            if(ret == 0){
                LOGD("mount %s with fs 'ext4' success\n", devPath);
                return 0;
            }
        }
        LOGD("failed to mount %s (%s)\n", devPath, strerror(errno));
        return -1;
    }
}

int search_file_in_dev(const char * file, char *absolutePath,
        const char *devPath, const char *devName){
    if(!check_file_exists(devPath)){
        LOGD("dev %s exists\n", devPath);
        char mountedPoint[32];
        sprintf(mountedPoint, "%s%s", USB_ROOT, devName);
        //if the dev exists, try to mount it
        if(!ensure_dev_mounted(devPath, mountedPoint)){
            LOGD("dev %s mounted in %s\n", devPath, mountedPoint);
            char desFile[32];
            sprintf(desFile, "%s/%s", mountedPoint, file);
            //if mount success.search des file in it
            if(!check_file_exists(desFile)){
                //if find the file,return its absolute path
                LOGD("file %s exist\n", desFile);
                sprintf(absolutePath, "%s", desFile);
                return 0;
            }else{
                ensure_dev_unmounted(mountedPoint);
            }
        }
    }
    return -1;
}

int search_file_in_usb(const char * file,char * absolutePath){
    timeval now;
    gettimeofday(&now, NULL);
    int i = 0;
    int j = 0;
    timeval workTime;
    long spends;
    mkdir(USB_ROOT, 0755);  //in case dir USB_ROOT doesn't already exist
    //do main work here
    do{
        LOGD("begin....\n");
        for(i = 0; i < MAX_DISK; i++){
            char devDisk[32];
            char devPartition[32];
            char devName[8];
            char parName[8];
            sprintf(devName, "sd%c", 'a' + i);
            sprintf(devDisk, "/dev/block/%s", devName);
            LOGD("check disk %s\n", devDisk);
            if(check_file_exists(devDisk)){
                LOGD("dev %s does not exists (%s),waiting ...\n", devDisk, strerror(errno));
                break;
            }
            for(j = 1; j <= MAX_PARTITION; j++){
                sprintf(parName, "%s%d", devName, j);
                sprintf(devPartition, "%s%d" ,devDisk, j);
                if(!search_file_in_dev(file, absolutePath, devPartition, parName)){
                    return 0;
                }
            }
            if(j > MAX_PARTITION){
                if(!search_file_in_dev(file, absolutePath, devDisk, devName)){
                    return 0;
                }
            }
        }
        usleep(500000);
        gettimeofday(&workTime, NULL);
        spends = (workTime.tv_sec - now.tv_sec)*1000000 + (workTime.tv_usec - now.tv_usec);
    }while(spends < TIME_OUT);
    LOGD("Time to search %s is %ld\n", file, spends);
    return -1;
}



int ensure_dev_unmounted(const char * mountedPoint){
    int ret = umount(mountedPoint);
    return ret;
}

int in_usb_device(const char * file){
    int len = strlen(USB_ROOT);
    if (strncmp(file, USB_ROOT, len) == 0){
        return 0;
    }
    return -1;
}
