#define bb_debug(fmt, args...) printf("burnboot:"fmt, ## args)

typedef struct {
    unsigned char* buffer;
    long len;
} BufferExtractCookie;

typedef int (*DeviceBurn)(BufferExtractCookie *cookie, char* path);


int getBufferExtractCookieOfFile(const char* path, BufferExtractCookie* cookie);

int getDeviceInfo(int boot_num, char* dev_node, char* boot_bin, DeviceBurn* burnFunc);

int checkBoot0Sum(BufferExtractCookie* cookie);

int checkUbootSum(BufferExtractCookie* cookie);