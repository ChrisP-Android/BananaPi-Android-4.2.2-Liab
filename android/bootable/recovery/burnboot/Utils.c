#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>

#include "BurnBoot.h"
#include "BootHead.h"

#define DEVNODE_PATH_NAND   "/dev/block/bootloader"
#define DEVNODE_PATH_SD "/dev/block/mmcblk0"

#define CMDLINE_FILE_PATH "/proc/cmdline"


static int spliteKeyAndValue(char* str, char** key, char** value){
	int elocation = strcspn(str,"=");
	if (elocation < 0){
		return -1;
	}
	str[elocation] = '\0';
	*key = str;
	*value = str + elocation + 1;
	return 0;
}

static int getInfoFromCmdline(char* key, char* value){
	FILE* fp;
    char cmdline[1024];
	//read partition info from /proc/cmdline
    if ((fp = fopen(CMDLINE_FILE_PATH,"r")) == NULL) {
        bb_debug("can't open /proc/cmdline \n");
        // goto error;
    }
    fgets(cmdline,1024,fp);
    fclose(fp);
    // bb_debug("%s\n", cmdline); 
    //splite the cmdline by space
    char* p = NULL;
    char* lkey = NULL;
    char* lvalue = NULL;
    p = strtok(cmdline, " ");
    if (!spliteKeyAndValue(p, &lkey, &lvalue)){
    	if (!strcmp(lkey,key)){
    		goto done;
    	}
    }
    // bb_debug("the first k-v is %s\n", p);

    while ((p = strtok(NULL, " "))){
    	// bb_debug("the other k-v is %s\n", p);
    	if (!spliteKeyAndValue(p, &lkey, &lvalue)){
	    	if (!strcmp(lkey,key)){
	    		goto done;
	    	}
	    }
    }

    bb_debug("no key named %s in cmdline.\n", key);
    strcpy(value, "-1");
    return -1;

    done:
    strcpy(value, lvalue);
    return 0;
}

static int getFlashType(){
	char ctype[8];
	getInfoFromCmdline("boot_type", ctype);
	printf("flash type = %s\n", ctype);

	int flash_type = atoi(ctype);
	//atoi出错时会返回0,当ctype字符串为0时也会返回0，所以这里要判断是否出错.
	if (flash_type == 0 && ctype[0] != '0'){
		return FLASH_TYPE_UNKNOW;
	}

	return flash_type;
}

int getBufferExtractCookieOfFile(const char* path, BufferExtractCookie* cookie){

	if (cookie == NULL){
		// printf("get file stat failed!\n");
		return -1;
	}

	struct stat statbuff;
	if(stat(path, &statbuff) < 0){
		bb_debug("get file stat failed!!\n");
		return -1;
	}
	cookie->len = statbuff.st_size;
	// bb_debug("file size is %d\n",(int)cookie->len);
	
	unsigned char* buffer = malloc(cookie->len);

	FILE* fp = fopen(path,"r");
	if (fp == NULL){
		bb_debug("open file failed!\n");
		return -1;
	}
	
	if (!fread(buffer, cookie->len, 1, fp)){
		bb_debug("read file failed!\n");
		return -1;
	}

	cookie->buffer = buffer;

	return 0;

}

//获取flash类型，根据flash类型选择烧写哪个boot0和确定设备文件地址
int getDeviceInfo(int boot_num, char* dev_node, char* boot_bin, DeviceBurn* burnFunc){
	
	int flash_type = getFlashType();

	switch(flash_type){
		case FLASH_TYPE_NAND:
			strcpy(dev_node, DEVNODE_PATH_NAND);
			bb_debug("use nand flash!!\n");
			break;
		case FLASH_TYPE_SD1:
        case FLASH_TYPE_SD2:
        	strcpy(dev_node, DEVNODE_PATH_SD);
        	bb_debug("use sd flash\n");
        	break;
        default:
        	goto error;
	}

    switch(boot_num){
        case BOOT0:
            switch(flash_type){
                case FLASH_TYPE_NAND:
                    strcpy(boot_bin, "boot0_nand.fex");
                    *burnFunc = burnNandBoot0;
                    break;
                case FLASH_TYPE_SD1:
                case FLASH_TYPE_SD2:
                    strcpy(boot_bin, "boot0_sdcard.fex");
                    *burnFunc = burnSdBoot0;
                    break;
            }
            break;
        case UBOOT:
            strcpy(boot_bin, "uboot.fex");
            switch(flash_type){
                case FLASH_TYPE_NAND:
                    *burnFunc = burnNandUboot;
                    break;
                case FLASH_TYPE_SD1:
                case FLASH_TYPE_SD2:
                    *burnFunc = burnSdUboot;
                    break;
            }
            break;
                   
            
    }

    return flash_type;
    error:
    return FLASH_TYPE_UNKNOW;
}

int checkBoot0Sum(BufferExtractCookie* cookie){

	standard_boot_file_head_t  *head_p;
    unsigned int length;
    unsigned int *buf;
    unsigned int loop;
    unsigned int i;
    unsigned int sum;
    unsigned int csum;

    head_p = (standard_boot_file_head_t *)cookie->buffer;

    length = head_p->length;
    if( ( length & 0x3 ) != 0 )                   // must 4-byte-aligned
            return -1;
    
    buf = (unsigned int *)cookie->buffer;
    csum = head_p->check_sum;
    head_p->check_sum = STAMP_VALUE;              // fill stamp
    loop = length >> 2;
    
    for( i = 0, sum = 0;  i < loop;  i++ )
        sum += buf[i];

    head_p->check_sum = csum;
    bb_debug("Boot0:File length is %u,old sum is %u,new sum is %u\n", length, head_p->check_sum, sum);
    return !(csum == sum);
}


int checkUbootSum(BufferExtractCookie* cookie){
    boot_file_head_t  *head_p;
    unsigned int length;
    unsigned int *buf;
    unsigned int loop;
    unsigned int i;
    unsigned int sum;
    unsigned int csum;

    head_p = (boot_file_head_t *)cookie->buffer;
    length = head_p->length;
    if( ( length & 0x3 ) != 0 )                   // must 4-byte-aligned
            return -1;

    buf = (unsigned int *)cookie->buffer;
    csum = head_p->check_sum;

    head_p->check_sum = STAMP_VALUE;              // fill stamp
    loop = length >> 2;
    
    for( i = 0, sum = 0;  i < loop;  i++ )
        sum += buf[i];

    head_p->check_sum = csum;
    bb_debug("Uboot:File length is %u,old sum is %u,new sum is %u\n", length, head_p->check_sum, sum);
    return !(csum == sum);
}
