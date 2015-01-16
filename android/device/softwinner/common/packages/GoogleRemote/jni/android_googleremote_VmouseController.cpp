#define LOG_TAG "VmouseController-JNI"

#include "JNIHelp.h"
#include "jni.h"
#include <utils/Log.h>
#include <android_runtime/AndroidRuntime.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

namespace android
{

int fd;

struct mouse_event{
	int flag;
	int x;
	int y;
};

static void nativeDispatchMouseEvent(JNIEnv *env, jobject clazz, jint x, jint y, jint flag)
{
	struct mouse_event event;
	event.x = x;
	event.y = y;
	event.flag = flag;
	write(fd, &event, sizeof(event));
}

static void nativeOpenVmouse(JNIEnv *env, jobject clazz){
	if (fd == NULL){
		fd = open("dev/vmouse", O_RDWR);
		if (fd < 0)
		{	
			ALOGE(" could not open dev/vmouse ");
		}
	}
}

static void nativeCloseVmouse(JNIEnv *env, jobject clazz){
	if (fd != NULL){
		close(fd);
		fd = NULL;
	}
}

static JNINativeMethod gMethods[] = {
	{"nativeDispatchMouseEvent", "(III)V", (void *)nativeDispatchMouseEvent},
	{"nativeOpenVmouse", "()V", (void *)nativeOpenVmouse},
	{"nativeCloseVmouse", "()V", (void *)nativeCloseVmouse},
};

int register_android_googleremote_VmouseController(JNIEnv* env)
{	
	int res = jniRegisterNativeMethods(env, "com/softwinner/tvremoteserver/VmouseController",
            gMethods, NELEM(gMethods));
	return 0;
}

}
