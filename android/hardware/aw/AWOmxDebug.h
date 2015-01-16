#ifndef _AWOMXDEBUG_H_
#define _AWOMXDEBUG_H_

#if (AW_OMX_PLATFORM_VERSION < 7)
#define ALOGV LOGV
#define ALOGD LOGD
#define ALOGI LOGI
#define ALOGW LOGW
#define ALOGE LOGE
#endif

//#define DBG_WARNING ALOGD
//#define DBG_WARNING(...)

#endif  /* _AWOMXDEBUG_H_ */

