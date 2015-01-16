#ifndef CDX_DEBUG_H
#define CDX_DEBUG_H
#include <sys/types.h>

#ifdef __cplusplus
extern "C"
{
#endif

void CdxCallStack(void);

#define CDX_LOGI(fmt, arg...) \
    ALOGI("<%s:%u>"fmt, \
        strrchr(__FILE__, '/') + 1, __LINE__, ##arg)

#define CDX_TRACE() \
    ALOGI("\033[40;33m<%s:%u> tid(%d)\033[0m", strrchr(__FILE__, '/') + 1, __LINE__, gettid())

#ifdef __cplusplus
}
#endif

#endif
