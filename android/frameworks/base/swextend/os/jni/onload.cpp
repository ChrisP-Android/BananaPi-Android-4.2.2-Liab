#define LOG_TAG "SWOS"

#include "JNIHelp.h"
#include "jni.h"
#include "utils/Log.h"
#include "utils/misc.h"

namespace softwinner {
    int register_softwinner_os_RecoverySystemEx(JNIEnv *env);
}

using namespace android;
using namespace softwinner;

extern "C" jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
    JNIEnv* env = NULL;
    jint result = -1;

    if (vm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK) {
        ALOGE("GetEnv failed!");
        return result;
    }
    ALOG_ASSERT(env, "Could not retrieve the env!");

    register_softwinner_os_RecoverySystemEx(env);

    return JNI_VERSION_1_4;
}
