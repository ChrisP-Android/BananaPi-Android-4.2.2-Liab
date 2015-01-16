#include "Utils.h"

#define FLASH_TYPE_NAND  0
#define FLASH_TYPE_SD1  1
#define FLASH_TYPE_SD2  2
#define FLASH_TYPE_UNKNOW	-1

#define BOOT0 0
#define UBOOT 1
#define TMP_FEX_FILE_PATH   "/tmp/tmp_boot.fex"

void* readSdBoot0(char* path);

void* readSdUboot(char* path);

int burnSdBoot0(BufferExtractCookie *cookie, char* path);

int burnSdUboot(BufferExtractCookie *cookie, char* path);

int burnNandBoot0(BufferExtractCookie *cookie, char* path);

int burnNandUboot(BufferExtractCookie *cookie, char* path);