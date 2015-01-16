
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <errno.h>

#include "BootHead.h"
#include "Utils.h"

#define NAND_BLKBURNBOOT0 		_IO('v',127)
#define NAND_BLKBURNUBOOT 		_IO('v',128)

#define SECTOR_SIZE	512
#define SD_BOOT0_SECTOR_START	16
#define SD_BOOT0_SIZE_KBYTES	24
#define SD_UBOOT_SECTOR_START	38192
#define SD_UBOOT_SIZE_KBYTES	640

static int writeSdBoot(int fd, void *buf, off_t offset, size_t bootsize){
	if (lseek(fd, 0, SEEK_SET) == -1) {
		bb_debug("reset the cursor failed! the error num is %d:%s\n",errno,strerror(errno));
		return -1;
	}
	if (lseek(fd, offset, SEEK_CUR) == -1) {
		bb_debug("lseek failed! the error num is %d:%s\n",errno,strerror(errno));
		return -1;
	}
	int result = write(fd, buf, bootsize);
	close(fd);
	return result;
}

static void *readSdBoot(int fd ,off_t offset, size_t bootsize){
	void* buffer = malloc(bootsize);
	memset(buffer, 0, bootsize);

	if (lseek(fd, 0, SEEK_SET) == -1) {
		bb_debug("reset the cursor failed! the error num is %d:%s\n",errno,strerror(errno));
		return NULL;
	}

	if (lseek(fd, offset, SEEK_CUR) == -1) {
		bb_debug("lseek failed! the error num is %d:%s\n",errno,strerror(errno));
		return NULL;
	}

	read(fd,buffer,bootsize);
	close(fd);
	return buffer;
}

static int openDevNode(const char* path){
	int fd = open(path, O_RDWR);

	if (fd == -1){
		bb_debug("open device node failed ! errno is %d : %s\n", errno, strerror(errno));
	}

	return fd;
}


void* readSdBoot0(char* path){
	int fd = openDevNode(path);

	if (fd == -1){
		return NULL;
	}

	void* ret = readSdBoot(fd, SD_BOOT0_SECTOR_START * SECTOR_SIZE, SD_BOOT0_SIZE_KBYTES * 1024);
	close(fd);
	return ret;
}

void* readSdUboot(char* path){
	int fd = openDevNode(path);

	if (fd == -1){
		return NULL;
	}
	void* ret = readSdBoot(fd, SD_UBOOT_SECTOR_START * SECTOR_SIZE, SD_UBOOT_SIZE_KBYTES * 1024);
	close(fd);
	return ret;
}

int burnSdBoot0(BufferExtractCookie *cookie, char* path){
	if (checkBoot0Sum(cookie)){
		bb_debug("wrong wrong binary file!\n");
		return -1;
	}

	int fd = openDevNode(path);

	if (fd == -1){
		return -1;
	}


	int ret = writeSdBoot(fd, cookie->buffer, SD_BOOT0_SECTOR_START * SECTOR_SIZE, SD_BOOT0_SIZE_KBYTES * 1024);
	close(fd);
	if (ret){
		bb_debug("burnSdBoot0 succeed!\n");
	}
	return ret;
}

int burnSdUboot(BufferExtractCookie *cookie, char* path){
	
	if (checkUbootSum(cookie)){
		bb_debug("wrong uboot binary file!\n");
		return -1;
	}

	int fd = openDevNode(path);

	if (fd == -1){
		return -1;
	}

	int ret = writeSdBoot(fd, cookie->buffer, SD_UBOOT_SECTOR_START * SECTOR_SIZE, SD_UBOOT_SIZE_KBYTES * 1024);
	close(fd);

	if (ret){
		bb_debug("burnSdUboot succeed!\n");
	}
	return ret;
}

int burnNandBoot0(BufferExtractCookie *cookie, char* path){

	if (checkBoot0Sum(cookie)){
		bb_debug("wrong boot0 binary file!\n");
		return -1;
	}

	int fd = openDevNode(path);

	if (fd == -1){
		return -1;
	}

	int ret = ioctl(fd,NAND_BLKBURNBOOT0,(unsigned long)cookie);

	if (ret) {
		bb_debug("burnNandBoot0 failed ! errno is %d : %s\n", errno, strerror(errno));
	}else{
		bb_debug("burnNandBoot0 succeed!");
	}

	close(fd);
	return ret;
}

int burnNandUboot(BufferExtractCookie *cookie, char* path){
	
	if (checkUbootSum(cookie)){
		bb_debug("wrong uboot binary file!\n");
		return -1;
	}

	int fd = openDevNode(path);

	if (fd == -1){
		return -1;
	}

	int ret = ioctl(fd,NAND_BLKBURNUBOOT,(unsigned long)cookie);
	
	if (ret) {
		bb_debug("burnNandUboot failed ! errno is %d : %s\n", errno, strerror(errno));
	}else{
		bb_debug("burnNandUboot succeed!!");
	}

	close(fd);
	return ret;
}