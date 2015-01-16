#ifndef USB_H_
#define USB_H_

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

//return 0 if exists
int check_file_exists(const char *path);

//return 0 if the usb mounted success
int ensure_dev_mounted(const char *devPath, const char *mountedPoint);

//return 0 if the usb unmounted success
int ensure_dev_unmounted(const char *mountedPoint);

//search file in usbs,return 0 if success, then save its absolute path arg2.
int search_file_in_usb(const char *file, char *absolutePath);

int in_usb_device(const char *file);

#ifdef __cplusplus
}
#endif

#endif  // USB_H_

