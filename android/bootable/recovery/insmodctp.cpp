#include <hardware/sensors.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <math.h>
#include <poll.h>
#include <pthread.h>
#include <linux/input.h>
#include <cutils/atomic.h>
#include <cutils/log.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <cutils/misc.h>
#include <cutils/properties.h>

extern  "C" int init_module(void *, unsigned long, const char *);
extern  "C" int delete_module(const char *, unsigned int);

//#define DEBUG 1

static int insmod(const char *filename, const char *args)
{
        void *module;
        unsigned int size;
        int ret;
        
        module = load_file(filename, &size);
        if (!module)
                return -1;
        
        ret = init_module(module, size, args);
        
        free(module);
        
        return ret;
}

static int rmmod(const char *modname)
{
        int ret = -1;
        int maxtry = 10;
        
        while (maxtry-- > 0) {
                ret = delete_module(modname, O_NONBLOCK | O_EXCL);
                if (ret < 0 && errno == EAGAIN)
                        usleep(500000);
                else
                        break;
        }
        
        if (ret != 0)
        ALOGD("Unable to unload driver module \"%s\": %s\n",
                modname, strerror(errno));
        return ret;
}

static int insmod_modules(void)
{
        FILE *fp = NULL;
        char insmod_name[64] = {0};
        char module_name[128] = {0};
        char ko[] = ".ko";
        int name_len = 0;

        fp = fopen("/sys/devices/sw_device/ctp", "r");
        if (NULL == fp )
        {
            ALOGD("fopen /sys/devices/sw_device/ctp failed, waitting for 2ms time");
            usleep(2000);
            fp = fopen("/sys/devices/sw_device/ctp", "r");
        }
        
        if (NULL != fp)
        {
            name_len = fread(insmod_name, 1, 64, fp); 
            fclose(fp);
            fp = NULL;
        }
        else
        {
            ALOGD(" fopen /sys/devices/sw_device/back_node failed");
        }

        ALOGD("insmod_name:%s\n",insmod_name);

        if(name_len > 0) {
                sprintf(module_name,"%s%s",insmod_name,ko);
                 
                ALOGD("start to insmod %s\n",module_name);
            
                if (insmod(module_name, "") < 0) {
                        ALOGD(" %s insmod failed!\n",insmod_name);
                        rmmod(module_name);//it may be load driver already,try remove it and insmod again.
                        if (insmod(module_name, "") < 0){
                                ALOGD("%s,Once again fail to load!",module_name);
                                return 0;
                        }
                } 
        }
        return 1;
}

 int insmodctp()
{ 
        if(!insmod_modules()){
                ALOGD("%s:insmod fail!\n",__FILE__);
        }
               
        return 0;
}
