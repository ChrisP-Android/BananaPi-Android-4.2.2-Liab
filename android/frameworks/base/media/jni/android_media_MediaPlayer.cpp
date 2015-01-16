/*
**
** Copyright 2007, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

//#define LOG_NDEBUG 0
#define LOG_TAG "MediaPlayer-JNI"
#include "utils/Log.h"

#include <media/mediaplayer.h>
#include <media/MediaPlayerInterface.h>
#include <stdio.h>
#include <assert.h>
#include <limits.h>
#include <unistd.h>
#include <fcntl.h>
#include <utils/threads.h>
#include "jni.h"
#include "JNIHelp.h"
#include "android_runtime/AndroidRuntime.h"
#include "android_runtime/android_view_Surface.h"
#include "utils/Errors.h"  // for status_t
#include "utils/KeyedVector.h"
#include "utils/String8.h"
#include "android_media_Utils.h"

#include "android_os_Parcel.h"
#include "android_util_Binder.h"
#include <binder/Parcel.h>
#include <gui/ISurfaceTexture.h>
#include <gui/Surface.h>
#include <binder/IPCThreadState.h>
#include <binder/IServiceManager.h>

// ----------------------------------------------------------------------------

using namespace android;

// ----------------------------------------------------------------------------

struct fields_t {
    jfieldID    context;
    jfieldID    surface_texture;

    jmethodID   post_event;
};
static fields_t fields;

static Mutex sLock;

// ----------------------------------------------------------------------------
// ref-counted object for callbacks
class JNIMediaPlayerListener: public MediaPlayerListener
{
public:
    JNIMediaPlayerListener(JNIEnv* env, jobject thiz, jobject weak_thiz);
    ~JNIMediaPlayerListener();
    virtual void notify(int msg, int ext1, int ext2, const Parcel *obj = NULL);
private:
    JNIMediaPlayerListener();
    jclass      mClass;     // Reference to MediaPlayer class
    jobject     mObject;    // Weak ref to MediaPlayer Java object to call on
};

JNIMediaPlayerListener::JNIMediaPlayerListener(JNIEnv* env, jobject thiz, jobject weak_thiz)
{

    // Hold onto the MediaPlayer class for use in calling the static method
    // that posts events to the application thread.
    jclass clazz = env->GetObjectClass(thiz);
    if (clazz == NULL) {
        ALOGE("Can't find android/media/MediaPlayer");
        jniThrowException(env, "java/lang/Exception", NULL);
        return;
    }
    mClass = (jclass)env->NewGlobalRef(clazz);

    // We use a weak reference so the MediaPlayer object can be garbage collected.
    // The reference is only used as a proxy for callbacks.
    mObject  = env->NewGlobalRef(weak_thiz);
}

JNIMediaPlayerListener::~JNIMediaPlayerListener()
{
    // remove global references
    JNIEnv *env = AndroidRuntime::getJNIEnv();
    env->DeleteGlobalRef(mObject);
    env->DeleteGlobalRef(mClass);
}

void JNIMediaPlayerListener::notify(int msg, int ext1, int ext2, const Parcel *obj)
{
    JNIEnv *env = AndroidRuntime::getJNIEnv();
    if (obj && obj->dataSize() > 0) {
        jobject jParcel = createJavaParcelObject(env);
        if (jParcel != NULL) {
            Parcel* nativeParcel = parcelForJavaObject(env, jParcel);
            nativeParcel->setData(obj->data(), obj->dataSize());
            env->CallStaticVoidMethod(mClass, fields.post_event, mObject,
                    msg, ext1, ext2, jParcel);
        }
    } else {
        env->CallStaticVoidMethod(mClass, fields.post_event, mObject,
                msg, ext1, ext2, NULL);
    }
    if (env->ExceptionCheck()) {
        ALOGW("An exception occurred while notifying an event.");
        LOGW_EX(env);
        env->ExceptionClear();
    }
}

// ----------------------------------------------------------------------------

static sp<MediaPlayer> getMediaPlayer(JNIEnv* env, jobject thiz)
{
    Mutex::Autolock l(sLock);
    MediaPlayer* const p = (MediaPlayer*)env->GetIntField(thiz, fields.context);
    return sp<MediaPlayer>(p);
}

static sp<MediaPlayer> setMediaPlayer(JNIEnv* env, jobject thiz, const sp<MediaPlayer>& player)
{
    Mutex::Autolock l(sLock);
    sp<MediaPlayer> old = (MediaPlayer*)env->GetIntField(thiz, fields.context);
    if (player.get()) {
        player->incStrong(thiz);
    }
    if (old != 0) {
        old->decStrong(thiz);
    }
    env->SetIntField(thiz, fields.context, (int)player.get());
    return old;
}

// If exception is NULL and opStatus is not OK, this method sends an error
// event to the client application; otherwise, if exception is not NULL and
// opStatus is not OK, this method throws the given exception to the client
// application.
static void process_media_player_call(JNIEnv *env, jobject thiz, status_t opStatus, const char* exception, const char *message)
{
    if (exception == NULL) {  // Don't throw exception. Instead, send an event.
        if (opStatus != (status_t) OK) {
            sp<MediaPlayer> mp = getMediaPlayer(env, thiz);
            if (mp != 0) mp->notify(MEDIA_ERROR, opStatus, 0);
        }
    } else {  // Throw exception!
        if ( opStatus == (status_t) INVALID_OPERATION ) {
            jniThrowException(env, "java/lang/IllegalStateException", NULL);
        } else if ( opStatus == (status_t) PERMISSION_DENIED ) {
            jniThrowException(env, "java/lang/SecurityException", NULL);
        } else if ( opStatus != (status_t) OK ) {
            if (strlen(message) > 230) {
               // if the message is too long, don't bother displaying the status code
               jniThrowException( env, exception, message);
            } else {
               char msg[256];
                // append the status code to the message
               sprintf(msg, "%s: status=0x%X", message, opStatus);
               jniThrowException( env, exception, msg);
            }
        }
    }
}

static void
android_media_MediaPlayer_setDataSourceAndHeaders(
        JNIEnv *env, jobject thiz, jstring path,
        jobjectArray keys, jobjectArray values) {

    sp<MediaPlayer> mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return;
    }

    if (path == NULL) {
        jniThrowException(env, "java/lang/IllegalArgumentException", NULL);
        return;
    }

    const char *tmp = env->GetStringUTFChars(path, NULL);
    if (tmp == NULL) {  // Out of memory
        return;
    }
    ALOGV("setDataSource: path %s", tmp);

    String8 pathStr(tmp);
    env->ReleaseStringUTFChars(path, tmp);
    tmp = NULL;

    // We build a KeyedVector out of the key and val arrays
    KeyedVector<String8, String8> headersVector;
    if (!ConvertKeyValueArraysToKeyedVector(
            env, keys, values, &headersVector)) {
        return;
    }

    status_t opStatus =
        mp->setDataSource(
                pathStr,
                headersVector.size() > 0? &headersVector : NULL);

    process_media_player_call(
            env, thiz, opStatus, "java/io/IOException",
            "setDataSource failed." );
}

static void
android_media_MediaPlayer_setDataSourceFD(JNIEnv *env, jobject thiz, jobject fileDescriptor, jlong offset, jlong length)
{
    sp<MediaPlayer> mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return;
    }

    if (fileDescriptor == NULL) {
        jniThrowException(env, "java/lang/IllegalArgumentException", NULL);
        return;
    }
    int fd = jniGetFDFromFileDescriptor(env, fileDescriptor);
    ALOGV("setDataSourceFD: fd %d", fd);
    process_media_player_call( env, thiz, mp->setDataSource(fd, offset, length), "java/io/IOException", "setDataSourceFD failed." );
}

static sp<ISurfaceTexture>
getVideoSurfaceTexture(JNIEnv* env, jobject thiz) {
    ISurfaceTexture * const p = (ISurfaceTexture*)env->GetIntField(thiz, fields.surface_texture);
    return sp<ISurfaceTexture>(p);
}

static void
decVideoSurfaceRef(JNIEnv *env, jobject thiz)
{
    sp<MediaPlayer> mp = getMediaPlayer(env, thiz);
    if (mp == NULL) {
        return;
    }

    sp<ISurfaceTexture> old_st = getVideoSurfaceTexture(env, thiz);
    if (old_st != NULL) {
        old_st->decStrong(thiz);
    }
}

static void
setVideoSurface(JNIEnv *env, jobject thiz, jobject jsurface, jboolean mediaPlayerMustBeAlive)
{
    sp<MediaPlayer> mp = getMediaPlayer(env, thiz);
    if (mp == NULL) {
        if (mediaPlayerMustBeAlive) {
            jniThrowException(env, "java/lang/IllegalStateException", NULL);
        }
        return;
    }

    decVideoSurfaceRef(env, thiz);

    sp<ISurfaceTexture> new_st;
    if (jsurface) {
        sp<Surface> surface(android_view_Surface_getSurface(env, jsurface));
        if (surface != NULL) {
            new_st = surface->getSurfaceTexture();
            if (new_st == NULL) {
                jniThrowException(env, "java/lang/IllegalArgumentException",
                    "The surface does not have a binding SurfaceTexture!");
                return;
            }
            new_st->incStrong(thiz);
        } else {
            jniThrowException(env, "java/lang/IllegalArgumentException",
                    "The surface has been released");
            return;
        }
    }

    env->SetIntField(thiz, fields.surface_texture, (int)new_st.get());

    // This will fail if the media player has not been initialized yet. This
    // can be the case if setDisplay() on MediaPlayer.java has been called
    // before setDataSource(). The redundant call to setVideoSurfaceTexture()
    // in prepare/prepareAsync covers for this case.
    mp->setVideoSurfaceTexture(new_st);
}

static void
android_media_MediaPlayer_setVideoSurface(JNIEnv *env, jobject thiz, jobject jsurface)
{
    setVideoSurface(env, thiz, jsurface, true /* mediaPlayerMustBeAlive */);
}

static void
android_media_MediaPlayer_prepare(JNIEnv *env, jobject thiz)
{
    sp<MediaPlayer> mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return;
    }

    // Handle the case where the display surface was set before the mp was
    // initialized. We try again to make it stick.
    sp<ISurfaceTexture> st = getVideoSurfaceTexture(env, thiz);
    mp->setVideoSurfaceTexture(st);

    process_media_player_call( env, thiz, mp->prepare(), "java/io/IOException", "Prepare failed." );
}

static void
android_media_MediaPlayer_prepareAsync(JNIEnv *env, jobject thiz)
{
    sp<MediaPlayer> mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return;
    }

    // Handle the case where the display surface was set before the mp was
    // initialized. We try again to make it stick.
    sp<ISurfaceTexture> st = getVideoSurfaceTexture(env, thiz);
    mp->setVideoSurfaceTexture(st);

    process_media_player_call( env, thiz, mp->prepareAsync(), "java/io/IOException", "Prepare Async failed." );
}

static void
android_media_MediaPlayer_start(JNIEnv *env, jobject thiz)
{
    ALOGV("start");
    sp<MediaPlayer> mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return;
    }
    process_media_player_call( env, thiz, mp->start(), NULL, NULL );
}

static void
android_media_MediaPlayer_stop(JNIEnv *env, jobject thiz)
{
    ALOGV("stop");
    sp<MediaPlayer> mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return;
    }
    process_media_player_call( env, thiz, mp->stop(), NULL, NULL );
}

static void
android_media_MediaPlayer_pause(JNIEnv *env, jobject thiz)
{
    ALOGV("pause");
    sp<MediaPlayer> mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return;
    }
    process_media_player_call( env, thiz, mp->pause(), NULL, NULL );
}

static jboolean
android_media_MediaPlayer_isPlaying(JNIEnv *env, jobject thiz)
{
    sp<MediaPlayer> mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return false;
    }
    const jboolean is_playing = mp->isPlaying();

    ALOGV("isPlaying: %d", is_playing);
    return is_playing;
}

static void
android_media_MediaPlayer_seekTo(JNIEnv *env, jobject thiz, int msec)
{
    sp<MediaPlayer> mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return;
    }
    ALOGV("seekTo: %d(msec)", msec);
    process_media_player_call( env, thiz, mp->seekTo(msec), NULL, NULL );
}

static int
android_media_MediaPlayer_getVideoWidth(JNIEnv *env, jobject thiz)
{
    sp<MediaPlayer> mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return 0;
    }
    int w;
    if (0 != mp->getVideoWidth(&w)) {
        ALOGE("getVideoWidth failed");
        w = 0;
    }
    ALOGV("getVideoWidth: %d", w);
    return w;
}

static int
android_media_MediaPlayer_getVideoHeight(JNIEnv *env, jobject thiz)
{
    sp<MediaPlayer> mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return 0;
    }
    int h;
    if (0 != mp->getVideoHeight(&h)) {
        ALOGE("getVideoHeight failed");
        h = 0;
    }
    ALOGV("getVideoHeight: %d", h);
    return h;
}


static int
android_media_MediaPlayer_getCurrentPosition(JNIEnv *env, jobject thiz)
{
    sp<MediaPlayer> mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return 0;
    }
    int msec;
    process_media_player_call( env, thiz, mp->getCurrentPosition(&msec), NULL, NULL );
    ALOGV("getCurrentPosition: %d (msec)", msec);
    return msec;
}

static int
android_media_MediaPlayer_getDuration(JNIEnv *env, jobject thiz)
{
    sp<MediaPlayer> mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return 0;
    }
    int msec;
    process_media_player_call( env, thiz, mp->getDuration(&msec), NULL, NULL );
    ALOGV("getDuration: %d (msec)", msec);
    return msec;
}

static void
android_media_MediaPlayer_reset(JNIEnv *env, jobject thiz)
{
    ALOGV("reset");
    sp<MediaPlayer> mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return;
    }
    process_media_player_call( env, thiz, mp->reset(), NULL, NULL );
}

static void
android_media_MediaPlayer_setAudioStreamType(JNIEnv *env, jobject thiz, int streamtype)
{
    ALOGV("setAudioStreamType: %d", streamtype);
    sp<MediaPlayer> mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return;
    }
    process_media_player_call( env, thiz, mp->setAudioStreamType((audio_stream_type_t) streamtype) , NULL, NULL );
}

static void
android_media_MediaPlayer_setLooping(JNIEnv *env, jobject thiz, jboolean looping)
{
    ALOGV("setLooping: %d", looping);
    sp<MediaPlayer> mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return;
    }
    process_media_player_call( env, thiz, mp->setLooping(looping), NULL, NULL );
}

static jboolean
android_media_MediaPlayer_isLooping(JNIEnv *env, jobject thiz)
{
    ALOGV("isLooping");
    sp<MediaPlayer> mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return false;
    }
    return mp->isLooping();
}

static void
android_media_MediaPlayer_setVolume(JNIEnv *env, jobject thiz, float leftVolume, float rightVolume)
{
    ALOGV("setVolume: left %f  right %f", leftVolume, rightVolume);
    sp<MediaPlayer> mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return;
    }
    process_media_player_call( env, thiz, mp->setVolume(leftVolume, rightVolume), NULL, NULL );
}

// FIXME: deprecated
static jobject
android_media_MediaPlayer_getFrameAt(JNIEnv *env, jobject thiz, jint msec)
{
    return NULL;
}


// Sends the request and reply parcels to the media player via the
// binder interface.
static jint
android_media_MediaPlayer_invoke(JNIEnv *env, jobject thiz,
                                 jobject java_request, jobject java_reply)
{
    sp<MediaPlayer> media_player = getMediaPlayer(env, thiz);
    if (media_player == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return UNKNOWN_ERROR;
    }

    Parcel *request = parcelForJavaObject(env, java_request);
    Parcel *reply = parcelForJavaObject(env, java_reply);

    // Don't use process_media_player_call which use the async loop to
    // report errors, instead returns the status.
    return media_player->invoke(*request, reply);
}

// Sends the new filter to the client.
static jint
android_media_MediaPlayer_setMetadataFilter(JNIEnv *env, jobject thiz, jobject request)
{
    sp<MediaPlayer> media_player = getMediaPlayer(env, thiz);
    if (media_player == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return UNKNOWN_ERROR;
    }

    Parcel *filter = parcelForJavaObject(env, request);

    if (filter == NULL ) {
        jniThrowException(env, "java/lang/RuntimeException", "Filter is null");
        return UNKNOWN_ERROR;
    }

    return media_player->setMetadataFilter(*filter);
}

static jboolean
android_media_MediaPlayer_getMetadata(JNIEnv *env, jobject thiz, jboolean update_only,
                                      jboolean apply_filter, jobject reply)
{
    sp<MediaPlayer> media_player = getMediaPlayer(env, thiz);
    if (media_player == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return false;
    }

    Parcel *metadata = parcelForJavaObject(env, reply);

    if (metadata == NULL ) {
        jniThrowException(env, "java/lang/RuntimeException", "Reply parcel is null");
        return false;
    }

    metadata->freeData();
    // On return metadata is positioned at the beginning of the
    // metadata. Note however that the parcel actually starts with the
    // return code so you should not rewind the parcel using
    // setDataPosition(0).
    return media_player->getMetadata(update_only, apply_filter, metadata) == OK;
}

/* add by Gary. start {{----------------------------------- */
static void 
android_media_MediaPlayer_setScreen(JNIEnv *env, jobject thiz, jint screen)
{
    ALOGV("setScreen");
    MediaPlayer::setScreen(screen);
}

static jint 
android_media_MediaPlayer_getScreen(JNIEnv *env, jobject thiz)
{
    ALOGV("getScreen");

    jint screen;
    if( OK != MediaPlayer::getScreen(&screen) ){
        ALOGV("Fail in getting screen");
        screen = MASTER_SCREEN;
    }
    
    return screen;
}

static jboolean 
android_media_MediaPlayer_isPlayingVideo(JNIEnv *env, jobject thiz)
{
    ALOGV("isPlayingVideo");

    bool playing;
    if( OK != MediaPlayer::isPlayingVideo(&playing) ){
        ALOGV("Fail in isPlayingVideo()");
        playing = false;
    }
    
    return (jboolean)playing;
}
/* add by Gary. end   -----------------------------------}} */

/* add by Gary. start {{----------------------------------- */
/* 2011-9-13 10:25:47 */
/* expend interfaces about subtitle, track and so on */
static jobject _composeObjSubInfo(JNIEnv *env, jclass classSubInfo, jmethodID methodSubInfo, MediaPlayer_SubInfo *info)
{
    jstring charset = env->NewStringUTF((const char*)info->charset);
    jbyteArray name = env->NewByteArray(info->len);
    env->SetByteArrayRegion(name, 0, info->len, (jbyte*)info->name);
    jobject objSubInfo = env->NewObject(classSubInfo, methodSubInfo, name, charset, info->type);
    if(objSubInfo == NULL )
        ALOGE("Fail in creating SubInfo object.");
        
    env->DeleteLocalRef(name);
    env->DeleteLocalRef(charset);
    return objSubInfo;
}

static jobjectArray
android_media_MediaPlayer_getSubList(JNIEnv *env, jobject thiz)
{
    ALOGE("enter android_media_MediaPlayer_getSubList");

    jobjectArray jsubList = NULL;
    MediaPlayer_SubInfo *csubList = NULL;
    status_t ret;
    jclass classSubInfo;    
    jmethodID methodSubInfo;
    
    sp<MediaPlayer> mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return NULL;
    }
    
    classSubInfo = env->FindClass("android/media/MediaPlayer$SubInfo");
    if(classSubInfo == NULL ){
        ALOGE("Fail in finding class android/media/MediaPlayer$SubInfo");
        return NULL;
    }
    
    int count = mp->getSubCount();
    if(count <= 0)
        return NULL;
        
    jsubList = env->NewObjectArray(count, classSubInfo, NULL );
    if(jsubList == NULL){
        ALOGE("Fail in creating subInfo array.");
        goto error;
    }
    csubList = new MediaPlayer_SubInfo[count];
    if(csubList == NULL ){
        ALOGE("Fail in allocating memory.");
        goto error;
    }
    count = mp->getSubList(csubList, count);
    if(count <= 0){
        ALOGE("Fail in getting sublist.");
        goto error;
    }
    methodSubInfo = env->GetMethodID(classSubInfo, "<init>", 
                                                  "([BLjava/lang/String;I)V");
    if(methodSubInfo == NULL){
        ALOGE("Fail in getting method \"SubInfo\".");
        goto error;
    }
    for(int i = 0; i < count; i++){
        jobject objSubInfo = _composeObjSubInfo(env, classSubInfo, methodSubInfo, csubList+i);
        env->SetObjectArrayElement(jsubList, i, objSubInfo);
        env->DeleteLocalRef(objSubInfo);
    }
    
    delete[] csubList;
    return jsubList;    
    
error:
    if(jsubList != NULL)
        env->DeleteLocalRef(jsubList);
    if(csubList != NULL)
        delete[] csubList;
    return NULL;
}

static jint
android_media_MediaPlayer_getCurSub(JNIEnv *env, jobject thiz)
{
    sp<MediaPlayer> mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return -1;
    }
    
    return mp->getCurSub();
}

static jint
android_media_MediaPlayer_switchSub(JNIEnv *env, jobject thiz, jint index)
{
    sp<MediaPlayer> mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return -1;
    }
    
    return mp->switchSub(index);
}

static jint
android_media_MediaPlayer_setSubGate(JNIEnv *env, jobject thiz, jboolean showSub)
{
    sp<MediaPlayer> mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return -1;
    }
    
    return mp->setSubGate((bool)showSub);
}

static jboolean
android_media_MediaPlayer_getSubGate(JNIEnv *env, jobject thiz)
{
    sp<MediaPlayer> mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return -1;
    }
    
    return (jboolean)mp->getSubGate();
}

static jint
android_media_MediaPlayer_setSubColor(JNIEnv *env, jobject thiz, jint color)
{
    sp<MediaPlayer> mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return -1;
    }
    
    return mp->setSubColor(color);
}

static jint
android_media_MediaPlayer_getSubColor(JNIEnv *env, jobject thiz)
{
    sp<MediaPlayer> mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return -1;
    }
    
    return mp->getSubColor();
}

static jint
android_media_MediaPlayer_setSubFrameColor(JNIEnv *env, jobject thiz, jint color)
{
    sp<MediaPlayer> mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return -1;
    }
    
    return mp->setSubFrameColor(color);
}

static jint
android_media_MediaPlayer_getSubFrameColor(JNIEnv *env, jobject thiz)
{
    sp<MediaPlayer> mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return 0xFFFFFFFF;
    }
    
    return mp->getSubFrameColor();
}

static jint
android_media_MediaPlayer_setSubFontSize(JNIEnv *env, jobject thiz, jint size)
{
    sp<MediaPlayer> mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return -1;
    }
    
    return mp->setSubFontSize(size);
}

static jint
android_media_MediaPlayer_getSubFontSize(JNIEnv *env, jobject thiz)
{
    sp<MediaPlayer> mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return -1;
    }
    
    return mp->getSubFontSize();
}

static jint
android_media_MediaPlayer_setSubFontPath(JNIEnv *env, jobject thiz, jstring charset)
{
	sp<MediaPlayer> mp = getMediaPlayer(env, thiz);
	if (mp == NULL ) {
		jniThrowException(env, "java/lang/IllegalStateException", NULL);
		return -1;
	}

	const char *ccharset = env->GetStringUTFChars(charset, NULL);
	if(ccharset == NULL){
		ALOGE("Fail in converting jstring to cstring.");
		return -1;
	}

	status_t ret = mp->setSubFontPath(ccharset);
	env->ReleaseStringUTFChars(charset, ccharset);
	return ret;
}


	static jint
android_media_MediaPlayer_setSubCharset(JNIEnv *env, jobject thiz, jstring charset)
{
    sp<MediaPlayer> mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return -1;
    }
    
    const char *ccharset = env->GetStringUTFChars(charset, NULL);
    if(ccharset == NULL){
        ALOGE("Fail in converting jstring to cstring.");
        return -1;
    }
    
    status_t ret = mp->setSubCharset(ccharset);
    env->ReleaseStringUTFChars(charset, ccharset);
    return ret;
}

static jstring
android_media_MediaPlayer_getSubCharset(JNIEnv *env, jobject thiz)
{
    sp<MediaPlayer> mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return NULL;
    }
    char *ccharset = new char[MEDIAPLAYER_NAME_LEN_MAX];
    if(ccharset == NULL){
        ALOGE("Fail in allocating memory.");
        return NULL;
    }
    
    status_t ret = mp->getSubCharset(ccharset);
    if(ret == OK){
        jstring charset = env->NewStringUTF(ccharset);
        if(charset == NULL){
            ALOGE("Fail in creating java string with %s.", ccharset);
        }
        delete[] ccharset;
        return charset;
    }else {
        delete[] ccharset;
        return NULL;
    }
}

static jint
android_media_MediaPlayer_setSubPosition(JNIEnv *env, jobject thiz, jint pencent)
{
    sp<MediaPlayer> mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return -1;
    }
    
    return mp->setSubPosition(pencent);
}

static jint
android_media_MediaPlayer_getSubPosition(JNIEnv *env, jobject thiz)
{
    sp<MediaPlayer> mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return -1;
    }
    
    return mp->getSubPosition();
}

static jint
android_media_MediaPlayer_setSubDelay(JNIEnv *env, jobject thiz, jint delay)
{
    sp<MediaPlayer> mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return -1;
    }
    
    return mp->setSubDelay(delay);
}

static jint
android_media_MediaPlayer_getSubDelay(JNIEnv *env, jobject thiz)
{
    sp<MediaPlayer> mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return -1;
    }
    
    return mp->getSubDelay();
}

static jobject _composeObjTrackInfo(JNIEnv *env, jclass classTrackInfo, jmethodID methodTrackInfo, MediaPlayer_TrackInfo *info)
{
    jstring charset = env->NewStringUTF((const char*)info->charset);
    jbyteArray name = env->NewByteArray(info->len);
    env->SetByteArrayRegion(name, 0, info->len, (jbyte*)info->name);
    jobject objTrackInfo = env->NewObject(classTrackInfo, methodTrackInfo, name, charset);
    if(objTrackInfo == NULL )
        ALOGE("Fail in creating TrackInfo object.");
        
    env->DeleteLocalRef(name);
    env->DeleteLocalRef(charset);
    return objTrackInfo;
}

static jobjectArray
android_media_MediaPlayer_getTrackList(JNIEnv *env, jobject thiz)
{
    ALOGE("enter android_media_MediaPlayer_getTrackList");

    jobjectArray jtrackList = NULL;
    MediaPlayer_TrackInfo *ctrackList = NULL;
    status_t ret;
    jclass classTrackInfoVendor;
    jmethodID methodTrackInfo;
    
    sp<MediaPlayer> mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return NULL;
    }
    
    classTrackInfoVendor = env->FindClass("android/media/MediaPlayer$TrackInfoVendor");
    if(classTrackInfoVendor == NULL ){
        ALOGE("Fail in finding class android/media/MediaPlayer$TrackInfo");
        return NULL;
    }
    
    int count = mp->getTrackCount();
    if(count <= 0)
        return NULL;
        
    jtrackList = env->NewObjectArray(count, classTrackInfoVendor, NULL );
    if(jtrackList == NULL){
        ALOGE("Fail in creating trackInfo array.");
        goto error;
    }
    ctrackList = new MediaPlayer_TrackInfo[count];
    if(ctrackList == NULL ){
        ALOGE("Fail in allocating memory.");
        goto error;
    }
    count = mp->getTrackList(ctrackList, count);
    if(count < 0){
        ALOGE("Fail in getting tracklist.");
        goto error;
    }
    methodTrackInfo = env->GetMethodID(classTrackInfoVendor, "<init>",
                                                  "([BLjava/lang/String;)V");
    if(methodTrackInfo == NULL){
        ALOGE("Fail in getting method \"TrackInfo\".");
        goto error;
    }
    for(int i = 0; i < count; i++){
        jobject objTrackInfo = _composeObjTrackInfo(env, classTrackInfoVendor, methodTrackInfo, ctrackList+i);
        env->SetObjectArrayElement(jtrackList, i, objTrackInfo);
        env->DeleteLocalRef(objTrackInfo);
    }
    
    delete[] ctrackList;
    return jtrackList;    
    
error:
    if(jtrackList != NULL)
        env->DeleteLocalRef(jtrackList);
    if(ctrackList != NULL)
        delete[] ctrackList;
    return NULL;
}


static jint
android_media_MediaPlayer_getCurTrack(JNIEnv *env, jobject thiz)
{
    sp<MediaPlayer> mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return -1;
    }
    
    return mp->getCurTrack();
}

static jint
android_media_MediaPlayer_switchTrack(JNIEnv *env, jobject thiz, jint index)
{
    sp<MediaPlayer> mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return -1;
    }
    
    return mp->switchTrack(index);
}

static jint
android_media_MediaPlayer_setInputDimensionType(JNIEnv *env, jobject thiz, jint type)
{
    sp<MediaPlayer> mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return -1;
    }
    
    return mp->setInputDimensionType(type);
}

static jint
android_media_MediaPlayer_getInputDimensionType(JNIEnv *env, jobject thiz)
{
    sp<MediaPlayer> mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return -1;
    }
    
    return mp->getInputDimensionType();
}

static jint
android_media_MediaPlayer_setOutputDimensionType(JNIEnv *env, jobject thiz, jint type)
{
    sp<MediaPlayer> mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return -1;
    }
    
    return mp->setOutputDimensionType(type);
}

static jint
android_media_MediaPlayer_getOutputDimensionType(JNIEnv *env, jobject thiz)
{
    sp<MediaPlayer> mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return -1;
    }
    
    return mp->getOutputDimensionType();
}

static jint
android_media_MediaPlayer_setAnaglaghType(JNIEnv *env, jobject thiz, jint type)
{
    sp<MediaPlayer> mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return -1;
    }
    
    return mp->setAnaglaghType(type);
}

static jint
android_media_MediaPlayer_getAnaglaghType(JNIEnv *env, jobject thiz)
{
    sp<MediaPlayer> mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return -1;
    }
    
    return mp->getAnaglaghType();
}

static jstring
android_media_MediaPlayer_getVideoEncode(JNIEnv *env, jobject thiz)
{
    sp<MediaPlayer> mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return NULL;
    }
    
    char *encode = new char[MEDIAPLAYER_NAME_LEN_MAX];
    if(encode == NULL){
        ALOGE("Fail in allocating memory.");
        return NULL;
    }
    status_t ret = mp->getVideoEncode(encode);
    if(ret != OK){
        delete[] encode;
        return NULL;
    }
    jstring jencode = env->NewStringUTF(encode);
    if(jencode == NULL){
        delete[] encode;
        return NULL;
    }
    return jencode;
}

static jint
android_media_MediaPlayer_getVideoFrameRate(JNIEnv *env, jobject thiz)
{
    sp<MediaPlayer> mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return -1;
    }
    
    return mp->getVideoFrameRate();
}

static jstring
android_media_MediaPlayer_getAudioEncode(JNIEnv *env, jobject thiz)
{
    sp<MediaPlayer> mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return NULL;
    }
    
    char *encode = new char[MEDIAPLAYER_NAME_LEN_MAX];
    if(encode == NULL){
        ALOGE("Fail in allocating memory.");
        return NULL;
    }
    status_t ret = mp->getAudioEncode(encode);
    if(ret != OK){
        delete[] encode;
        return NULL;
    }
    jstring jencode = env->NewStringUTF(encode);
    if(jencode == NULL){
        delete[] encode;
        return NULL;
    }
    return jencode;
}

static jint
android_media_MediaPlayer_getAudioBitRate(JNIEnv *env, jobject thiz)
{
    sp<MediaPlayer> mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return -1;
    }
    
    return mp->getAudioBitRate();
}

static jint
android_media_MediaPlayer_getAudioSampleRate(JNIEnv *env, jobject thiz)
{
    sp<MediaPlayer> mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return -1;
    }
    
    return mp->getAudioSampleRate();
}

/* add by Gary. end   -----------------------------------}} */

/* add by Gary. start {{----------------------------------- */
/* 2011-11-14 */
/* support scale mode */
static void
android_media_MediaPlayer_enableScaleMode(JNIEnv *env, jobject thiz, jboolean enable, jint width, jint height)
{
    sp<MediaPlayer> mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return;
    }
    
    mp->enableScaleMode((bool)enable, width, height);
}
/* add by Gary. end   -----------------------------------}} */

/* add by Gary. start {{----------------------------------- */
/* 2011-11-14 */
/* support adjusting colors while playing video */
static void 
android_media_MediaPlayer_setVppGate(JNIEnv *env, jobject thiz, jboolean enableVpp)
{
    MediaPlayer::setVppGate((bool)enableVpp);
}

static jboolean 
android_media_MediaPlayer_getVppGate(JNIEnv *env, jobject thiz)
{
    return (jboolean)MediaPlayer::getVppGate();
}

static jint 
android_media_MediaPlayer_setLumaSharp(JNIEnv *env, jobject thiz, jint value)
{
    if(OK == MediaPlayer::setLumaSharp(value))
        return 0;
    else
        return -1;
}

static jint 
android_media_MediaPlayer_getLumaSharp(JNIEnv *env, jobject thiz)
{
    return MediaPlayer::getLumaSharp();
}

static jint 
android_media_MediaPlayer_setChromaSharp(JNIEnv *env, jobject thiz, jint value)
{
    if(OK == MediaPlayer::setChromaSharp(value))
        return 0;
    else
        return -1;
}

static jint 
android_media_MediaPlayer_getChromaSharp(JNIEnv *env, jobject thiz)
{
    return MediaPlayer::getChromaSharp();
}

static jint 
android_media_MediaPlayer_setWhiteExtend(JNIEnv *env, jobject thiz, jint value)
{
    if(OK == MediaPlayer::setWhiteExtend(value))
        return 0;
    else
        return -1;
}

static jint 
android_media_MediaPlayer_getWhiteExtend(JNIEnv *env, jobject thiz)
{
    return MediaPlayer::getWhiteExtend();
}

static jint 
android_media_MediaPlayer_setBlackExtend(JNIEnv *env, jobject thiz, jint value)
{
    if(OK == MediaPlayer::setBlackExtend(value))
        return 0;
    else
        return -1;
}

static jint 
android_media_MediaPlayer_getBlackExtend(JNIEnv *env, jobject thiz)
{
    return MediaPlayer::getBlackExtend();
}

/* add by Gary. end   -----------------------------------}} */

/* add by Gary. start {{----------------------------------- */
/* 2012-03-07 */
/* set audio channel mute */
static jint
android_media_MediaPlayer_setChannelMuteMode(JNIEnv *env, jobject thiz, jint muteMode)
{
    sp<MediaPlayer> mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return -1;
    }
    
    return mp->setChannelMuteMode(muteMode);
}

static jint
android_media_MediaPlayer_getChannelMuteMode(JNIEnv *env, jobject thiz)
{
    sp<MediaPlayer> mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return -1;
    }
    
    return mp->getChannelMuteMode();
}
/* add by Gary. end   -----------------------------------}} */

/* add by Gary. start {{----------------------------------- */
/* 2012-03-12 */
/* add the global interfaces to control the subtitle gate  */
static jint 
android_media_MediaPlayer_setGlobalSubGate(JNIEnv *env, jobject thiz, jboolean showSub)
{
    if(OK == MediaPlayer::setGlobalSubGate(showSub))
        return 0;
    else
        return -1;
}

static jboolean 
android_media_MediaPlayer_getGlobalSubGate(JNIEnv *env, jobject thiz)
{
    return MediaPlayer::getGlobalSubGate();
}
/* add by Gary. end   -----------------------------------}} */

/* add by Gary. start {{----------------------------------- */
/* 2012-4-24 */
/* add two general interfaces for expansibility */
static jint
android_media_MediaPlayer_setBdFolderPlayMode(JNIEnv *env, jobject thiz, jboolean enable)
{
    sp<MediaPlayer> mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return -1;
    }
    int input = enable? 1 : 0;
    return mp->generalInterface(MEDIAPLAYER_CMD_SET_BD_FOLDER_PLAY_MODE, input, 0, 0, NULL);
}

static jboolean
android_media_MediaPlayer_getBdFolderPlayMode(JNIEnv *env, jobject thiz)
{
    sp<MediaPlayer> mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return false;
    }
    
    int enable = 0;
    mp->generalInterface(MEDIAPLAYER_CMD_GET_BD_FOLDER_PLAY_MODE, 0, 0, 0, &enable);
    return enable == 1;
}
/* add by Gary. end   -----------------------------------}} */

/*Start by Bevis. Rotate the video.*/
static jboolean
android_media_MediaPlayer_isRotatable(JNIEnv *env, jobject thiz)
{
    int enable = 0;
    MediaPlayer::generalGlobalInterface(MEDIAPLAYER_CMD_IS_ROTATABLE, 0, 0, 0, &enable);
    return enable == 1;
}

static jint
android_media_MediaPlayer_setRotation(JNIEnv *env, jobject thiz, jint rotation)
{
    return MediaPlayer::generalGlobalInterface(MEDIAPLAYER_CMD_SET_ROTATION, rotation, 0, 0, NULL);
}
/*End by Bevis. Rotate the video.*/

/* add by Gary. start {{----------------------------------- */
/* 2013-3-16 */
/* control the raw data mode */
static jint
android_media_MediaPlayer_setRawDataMode(JNIEnv *env, jobject thiz, jint audioDataMode)
{
    return MediaPlayer::generalGlobalInterface(MEDIAPLAYER_GLOBAL_CMD_SET_RAWDATA_MODE, audioDataMode, 0, 0, NULL);
}
/* add by Gary. end   -----------------------------------}} */

/*add by eric_wang. Notify hdmi status.*/
static jint
android_media_MediaPlayer_setHdmiState(JNIEnv *env, jobject thiz, jboolean bHdmiPlugged)
{
    return MediaPlayer::generalGlobalInterface(MEDIAPLAYER_CMD_SET_HDMISTATE, bHdmiPlugged, 0, 0, NULL);
}
/*End by eric_wang. Notify hdmi status.*/

// This function gets some field IDs, which in turn causes class initialization.
// It is called from a static block in MediaPlayer, which won't run until the
// first time an instance of this class is used.
static void


android_media_MediaPlayer_native_init(JNIEnv *env)
{
    jclass clazz;

    clazz = env->FindClass("android/media/MediaPlayer");
    if (clazz == NULL) {
        return;
    }

    fields.context = env->GetFieldID(clazz, "mNativeContext", "I");
    if (fields.context == NULL) {
        return;
    }

    fields.post_event = env->GetStaticMethodID(clazz, "postEventFromNative",
                                               "(Ljava/lang/Object;IIILjava/lang/Object;)V");
    if (fields.post_event == NULL) {
        return;
    }

    fields.surface_texture = env->GetFieldID(clazz, "mNativeSurfaceTexture", "I");
    if (fields.surface_texture == NULL) {
        return;
    }
}

static void
android_media_MediaPlayer_native_setup(JNIEnv *env, jobject thiz, jobject weak_this)
{
    ALOGV("native_setup");
    sp<MediaPlayer> mp = new MediaPlayer();
    if (mp == NULL) {
        jniThrowException(env, "java/lang/RuntimeException", "Out of memory");
        return;
    }

    // create new listener and give it to MediaPlayer
    sp<JNIMediaPlayerListener> listener = new JNIMediaPlayerListener(env, thiz, weak_this);
    mp->setListener(listener);

    // Stow our new C++ MediaPlayer in an opaque field in the Java object.
    setMediaPlayer(env, thiz, mp);
}

static void
android_media_MediaPlayer_release(JNIEnv *env, jobject thiz)
{
    ALOGV("release");
    decVideoSurfaceRef(env, thiz);
    sp<MediaPlayer> mp = setMediaPlayer(env, thiz, 0);
    if (mp != NULL) {
        // this prevents native callbacks after the object is released
        mp->setListener(0);
        mp->disconnect();
    }
}

static void
android_media_MediaPlayer_native_finalize(JNIEnv *env, jobject thiz)
{
    ALOGV("native_finalize");
    sp<MediaPlayer> mp = getMediaPlayer(env, thiz);
    if (mp != NULL) {
        ALOGW("MediaPlayer finalized without being released");
    }
    android_media_MediaPlayer_release(env, thiz);
}

static void android_media_MediaPlayer_set_audio_session_id(JNIEnv *env,  jobject thiz, jint sessionId) {
    ALOGV("set_session_id(): %d", sessionId);
    sp<MediaPlayer> mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return;
    }
    process_media_player_call( env, thiz, mp->setAudioSessionId(sessionId), NULL, NULL );
}

static jint android_media_MediaPlayer_get_audio_session_id(JNIEnv *env,  jobject thiz) {
    ALOGV("get_session_id()");
    sp<MediaPlayer> mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return 0;
    }

    return mp->getAudioSessionId();
}

static void
android_media_MediaPlayer_setAuxEffectSendLevel(JNIEnv *env, jobject thiz, jfloat level)
{
    ALOGV("setAuxEffectSendLevel: level %f", level);
    sp<MediaPlayer> mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return;
    }
    process_media_player_call( env, thiz, mp->setAuxEffectSendLevel(level), NULL, NULL );
}

static void android_media_MediaPlayer_attachAuxEffect(JNIEnv *env,  jobject thiz, jint effectId) {
    ALOGV("attachAuxEffect(): %d", effectId);
    sp<MediaPlayer> mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return;
    }
    process_media_player_call( env, thiz, mp->attachAuxEffect(effectId), NULL, NULL );
}

static jint
android_media_MediaPlayer_pullBatteryData(JNIEnv *env, jobject thiz, jobject java_reply)
{
    sp<IBinder> binder = defaultServiceManager()->getService(String16("media.player"));
    sp<IMediaPlayerService> service = interface_cast<IMediaPlayerService>(binder);
    if (service.get() == NULL) {
        jniThrowException(env, "java/lang/RuntimeException", "cannot get MediaPlayerService");
        return UNKNOWN_ERROR;
    }

    Parcel *reply = parcelForJavaObject(env, java_reply);

    return service->pullBatteryData(reply);
}

static jint
android_media_MediaPlayer_setRetransmitEndpoint(JNIEnv *env, jobject thiz,
                                                jstring addrString, jint port) {
    sp<MediaPlayer> mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return INVALID_OPERATION;
    }

    const char *cAddrString = NULL;

    if (NULL != addrString) {
        cAddrString = env->GetStringUTFChars(addrString, NULL);
        if (cAddrString == NULL) {  // Out of memory
            return NO_MEMORY;
        }
    }
    ALOGV("setRetransmitEndpoint: %s:%d",
            cAddrString ? cAddrString : "(null)", port);

    status_t ret;
    if (cAddrString && (port > 0xFFFF)) {
        ret = BAD_VALUE;
    } else {
        ret = mp->setRetransmitEndpoint(cAddrString,
                static_cast<uint16_t>(port));
    }

    if (NULL != addrString) {
        env->ReleaseStringUTFChars(addrString, cAddrString);
    }

    if (ret == INVALID_OPERATION ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
    }

    return ret;
}

static jboolean
android_media_MediaPlayer_setParameter(JNIEnv *env, jobject thiz, jint key, jobject java_request)
{
    ALOGV("setParameter: key %d", key);
    sp<MediaPlayer> mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return false;
    }

    Parcel *request = parcelForJavaObject(env, java_request);
    status_t err = mp->setParameter(key, *request);
    if (err == OK) {
        return true;
    } else {
        return false;
    }
}

static void
android_media_MediaPlayer_getParameter(JNIEnv *env, jobject thiz, jint key, jobject java_reply)
{
    ALOGV("getParameter: key %d", key);
    sp<MediaPlayer> mp = getMediaPlayer(env, thiz);
    if (mp == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return;
    }

    Parcel *reply = parcelForJavaObject(env, java_reply);
    process_media_player_call(env, thiz, mp->getParameter(key, reply), NULL, NULL );
}

static void
android_media_MediaPlayer_setNextMediaPlayer(JNIEnv *env, jobject thiz, jobject java_player)
{
    ALOGV("setNextMediaPlayer");
    sp<MediaPlayer> thisplayer = getMediaPlayer(env, thiz);
    if (thisplayer == NULL) {
        jniThrowException(env, "java/lang/IllegalStateException", "This player not initialized");
        return;
    }
    sp<MediaPlayer> nextplayer = (java_player == NULL) ? NULL : getMediaPlayer(env, java_player);
    if (nextplayer == NULL && java_player != NULL) {
        jniThrowException(env, "java/lang/IllegalStateException", "That player not initialized");
        return;
    }

    if (nextplayer == thisplayer) {
        jniThrowException(env, "java/lang/IllegalArgumentException", "Next player can't be self");
        return;
    }
    // tie the two players together
    process_media_player_call(
            env, thiz, thisplayer->setNextMediaPlayer(nextplayer),
            "java/lang/IllegalArgumentException",
            "setNextMediaPlayer failed." );
    ;
}

// ----------------------------------------------------------------------------

static JNINativeMethod gMethods[] = {
    {
        "_setDataSource",
        "(Ljava/lang/String;[Ljava/lang/String;[Ljava/lang/String;)V",
        (void *)android_media_MediaPlayer_setDataSourceAndHeaders
    },

    {"setDataSource",       "(Ljava/io/FileDescriptor;JJ)V",    (void *)android_media_MediaPlayer_setDataSourceFD},
    {"_setVideoSurface",    "(Landroid/view/Surface;)V",        (void *)android_media_MediaPlayer_setVideoSurface},
    {"prepare",             "()V",                              (void *)android_media_MediaPlayer_prepare},
    {"prepareAsync",        "()V",                              (void *)android_media_MediaPlayer_prepareAsync},
    {"_start",              "()V",                              (void *)android_media_MediaPlayer_start},
    {"_stop",               "()V",                              (void *)android_media_MediaPlayer_stop},
    {"getVideoWidth",       "()I",                              (void *)android_media_MediaPlayer_getVideoWidth},
    {"getVideoHeight",      "()I",                              (void *)android_media_MediaPlayer_getVideoHeight},
    {"seekTo",              "(I)V",                             (void *)android_media_MediaPlayer_seekTo},
    {"_pause",              "()V",                              (void *)android_media_MediaPlayer_pause},
    {"isPlaying",           "()Z",                              (void *)android_media_MediaPlayer_isPlaying},
    {"getCurrentPosition",  "()I",                              (void *)android_media_MediaPlayer_getCurrentPosition},
    {"getDuration",         "()I",                              (void *)android_media_MediaPlayer_getDuration},
    {"_release",            "()V",                              (void *)android_media_MediaPlayer_release},
    {"_reset",              "()V",                              (void *)android_media_MediaPlayer_reset},
    {"setAudioStreamType",  "(I)V",                             (void *)android_media_MediaPlayer_setAudioStreamType},
    {"setLooping",          "(Z)V",                             (void *)android_media_MediaPlayer_setLooping},
    {"isLooping",           "()Z",                              (void *)android_media_MediaPlayer_isLooping},
    {"setVolume",           "(FF)V",                            (void *)android_media_MediaPlayer_setVolume},
    {"getFrameAt",          "(I)Landroid/graphics/Bitmap;",     (void *)android_media_MediaPlayer_getFrameAt},
    {"native_invoke",       "(Landroid/os/Parcel;Landroid/os/Parcel;)I",(void *)android_media_MediaPlayer_invoke},
    {"native_setMetadataFilter", "(Landroid/os/Parcel;)I",      (void *)android_media_MediaPlayer_setMetadataFilter},
    {"native_getMetadata", "(ZZLandroid/os/Parcel;)Z",          (void *)android_media_MediaPlayer_getMetadata},
    {"native_init",         "()V",                              (void *)android_media_MediaPlayer_native_init},
    {"native_setup",        "(Ljava/lang/Object;)V",            (void *)android_media_MediaPlayer_native_setup},
    {"native_finalize",     "()V",                              (void *)android_media_MediaPlayer_native_finalize},
    {"getAudioSessionId",   "()I",                              (void *)android_media_MediaPlayer_get_audio_session_id},
    {"setAudioSessionId",   "(I)V",                             (void *)android_media_MediaPlayer_set_audio_session_id},
    {"setAuxEffectSendLevel", "(F)V",                           (void *)android_media_MediaPlayer_setAuxEffectSendLevel},
    {"attachAuxEffect",     "(I)V",                             (void *)android_media_MediaPlayer_attachAuxEffect},
    {"native_pullBatteryData", "(Landroid/os/Parcel;)I",        (void *)android_media_MediaPlayer_pullBatteryData},
    {"setParameter",        "(ILandroid/os/Parcel;)Z",          (void *)android_media_MediaPlayer_setParameter},
    {"getParameter",        "(ILandroid/os/Parcel;)V",          (void *)android_media_MediaPlayer_getParameter},
    {"native_setRetransmitEndpoint", "(Ljava/lang/String;I)I",  (void *)android_media_MediaPlayer_setRetransmitEndpoint},
    {"setNextMediaPlayer",  "(Landroid/media/MediaPlayer;)V",   (void *)android_media_MediaPlayer_setNextMediaPlayer},
    /* add by Gary. start {{----------------------------------- */
    {"getScreen",           "()I",                              (void *)android_media_MediaPlayer_getScreen},
    {"setScreen",           "(I)V",                             (void *)android_media_MediaPlayer_setScreen},
    {"isPlayingVideo",      "()Z",                              (void *)android_media_MediaPlayer_isPlayingVideo},
    /* add by Gary. end   -----------------------------------}} */
    /* add by Gary. start {{----------------------------------- */
    /* 2011-9-13 10:25:47 */
    /* expend interfaces about subtitle, track and so on */
    {"getSubList",             "()[Landroid/media/MediaPlayer$SubInfo;", (void *)android_media_MediaPlayer_getSubList},
    {"getCurSub",              "()I",                           (void *)android_media_MediaPlayer_getCurSub},
    {"switchSub",              "(I)I",                          (void *)android_media_MediaPlayer_switchSub},
//    {"setSubGate",             "(Z)I",                          (void *)android_media_MediaPlayer_setSubGate},
//    {"getSubGate",             "()Z",                           (void *)android_media_MediaPlayer_getSubGate},
    {"setSubColor",            "(I)I",                          (void *)android_media_MediaPlayer_setSubColor},
    {"getSubColor",            "()I",                           (void *)android_media_MediaPlayer_getSubColor},
    {"setSubFrameColor",       "(I)I",                          (void *)android_media_MediaPlayer_setSubFrameColor},
    {"getSubFrameColor",       "()I",                           (void *)android_media_MediaPlayer_getSubFrameColor},
    {"setSubFontSize",         "(I)I",                          (void *)android_media_MediaPlayer_setSubFontSize},
    {"getSubFontSize",         "()I",                           (void *)android_media_MediaPlayer_getSubFontSize},
    {"setSubCharset",          "(Ljava/lang/String;)I",         (void *)android_media_MediaPlayer_setSubCharset},
	{"getSubCharset",          "()Ljava/lang/String;",          (void *)android_media_MediaPlayer_getSubCharset},
	{"setSubFontPath",         "(Ljava/lang/String;)I",         (void *)android_media_MediaPlayer_setSubFontPath},
	{"setSubPosition",         "(I)I",                          (void *)android_media_MediaPlayer_setSubPosition},
    {"getSubPosition",         "()I",                           (void *)android_media_MediaPlayer_getSubPosition},
    {"setSubDelay",            "(I)I",                          (void *)android_media_MediaPlayer_setSubDelay},
    {"getSubDelay",            "()I",                           (void *)android_media_MediaPlayer_getSubDelay},
    {"getTrackList",           "()[Landroid/media/MediaPlayer$TrackInfoVendor;", (void *)android_media_MediaPlayer_getTrackList},
    {"getCurTrack",            "()I",                           (void *)android_media_MediaPlayer_getCurTrack},
    {"switchTrack",            "(I)I",                          (void *)android_media_MediaPlayer_switchTrack},
    {"setInputDimensionType",  "(I)I",                          (void *)android_media_MediaPlayer_setInputDimensionType},
    {"getInputDimensionType",  "()I",                           (void *)android_media_MediaPlayer_getInputDimensionType},
    {"setOutputDimensionType", "(I)I",                          (void *)android_media_MediaPlayer_setOutputDimensionType},
    {"getOutputDimensionType", "()I",                           (void *)android_media_MediaPlayer_getOutputDimensionType},
    {"setAnaglaghType",        "(I)I",                          (void *)android_media_MediaPlayer_setAnaglaghType},
    {"getAnaglaghType",        "()I",                           (void *)android_media_MediaPlayer_getAnaglaghType},
    {"getVideoEncode",         "()Ljava/lang/String;",          (void *)android_media_MediaPlayer_getVideoEncode},
    {"getVideoFrameRate",      "()I",                           (void *)android_media_MediaPlayer_getVideoFrameRate},
    {"getAudioEncode",         "()Ljava/lang/String;",          (void *)android_media_MediaPlayer_getAudioEncode},
    {"getAudioBitRate",        "()I",                           (void *)android_media_MediaPlayer_getAudioBitRate},
    {"getAudioSampleRate",     "()I",                           (void *)android_media_MediaPlayer_getAudioSampleRate},
    /* add by Gary. end   -----------------------------------}} */
    /* add by Gary. start {{----------------------------------- */
    /* 2011-11-14 */
    /* support scale mode */
    {"enableScaleMode",        "(ZII)V",                        (void *)android_media_MediaPlayer_enableScaleMode},
    /* add by Gary. end   -----------------------------------}} */
    /* add by Gary. start {{----------------------------------- */
    /* 2011-11-14 */
    /* support adjusting colors while playing video */
    {"setVppGate",             "(Z)I",                          (void *)android_media_MediaPlayer_setVppGate},
    {"getVppGate",             "()Z",                           (void *)android_media_MediaPlayer_getVppGate},
    {"setLumaSharp",           "(I)I",                          (void *)android_media_MediaPlayer_setLumaSharp},
    {"getLumaSharp",           "()I",                           (void *)android_media_MediaPlayer_getLumaSharp},
    {"setChromaSharp",         "(I)I",                          (void *)android_media_MediaPlayer_setChromaSharp},
    {"getChromaSharp",         "()I",                           (void *)android_media_MediaPlayer_getChromaSharp},
    {"setWhiteExtend",         "(I)I",                          (void *)android_media_MediaPlayer_setWhiteExtend},
    {"getWhiteExtend",         "()I",                           (void *)android_media_MediaPlayer_getWhiteExtend},
    {"setBlackExtend",         "(I)I",                          (void *)android_media_MediaPlayer_setBlackExtend},
    {"getBlackExtend",         "()I",                           (void *)android_media_MediaPlayer_getBlackExtend},
    /* add by Gary. end   -----------------------------------}} */
    /* add by Gary. start {{----------------------------------- */
    /* 2012-03-07 */
    /* set audio channel mute */
    {"setChannelMuteMode",     "(I)I",                          (void *)android_media_MediaPlayer_setChannelMuteMode},
    {"getChannelMuteMode",     "()I",                           (void *)android_media_MediaPlayer_getChannelMuteMode},
    /* add by Gary. end   -----------------------------------}} */
    /* add by Gary. start {{----------------------------------- */
    /* 2012-03-12 */
    /* add the global interfaces to control the subtitle gate  */
    {"getGlobalSubGate",        "()Z",                          (void *)android_media_MediaPlayer_getGlobalSubGate},
    {"setGlobalSubGate",        "(Z)I",                         (void *)android_media_MediaPlayer_setGlobalSubGate},
    /* add by Gary. end   -----------------------------------}} */
    {"getBdFolderPlayMode",     "()Z",                          (void *)android_media_MediaPlayer_getBdFolderPlayMode},
    {"setBdFolderPlayMode",     "(Z)I",                         (void *)android_media_MediaPlayer_setBdFolderPlayMode},
    /*Start by Bevis. Rotate the video*/
	{"isRotatable",     "()Z",                         		(void *)android_media_MediaPlayer_isRotatable},	
    {"setRotation",     "(I)I",                         		(void *)android_media_MediaPlayer_setRotation},
    /*End by Bevis. Rotate the video*/
    /* add by Gary. start {{----------------------------------- */
    /* 2013-3-16 */
    /* control the raw data mode */
    {"_setRawDataMode",         "(I)I",                         (void *)android_media_MediaPlayer_setRawDataMode},
    /* add by Gary. end   -----------------------------------}} */
    /*add by eric_wang. Notify hdmi status.*/
    {"setHdmiState",     "(Z)I",                         		(void *)android_media_MediaPlayer_setHdmiState},
    /*End by eric_wang. Notify hdmi status.*/
};

static const char* const kClassPathName = "android/media/MediaPlayer";

// This function only registers the native methods
static int register_android_media_MediaPlayer(JNIEnv *env)
{
    return AndroidRuntime::registerNativeMethods(env,
                "android/media/MediaPlayer", gMethods, NELEM(gMethods));
}

extern int register_android_media_Crypto(JNIEnv *env);
extern int register_android_media_MediaCodec(JNIEnv *env);
extern int register_android_media_MediaExtractor(JNIEnv *env);
extern int register_android_media_MediaCodecList(JNIEnv *env);
extern int register_android_media_MediaMetadataRetriever(JNIEnv *env);
extern int register_android_media_MediaRecorder(JNIEnv *env);
extern int register_android_media_MediaScanner(JNIEnv *env);
extern int register_android_media_ResampleInputStream(JNIEnv *env);
extern int register_android_media_MediaProfiles(JNIEnv *env);
extern int register_android_media_AmrInputStream(JNIEnv *env);
extern int register_android_mtp_MtpDatabase(JNIEnv *env);
extern int register_android_mtp_MtpDevice(JNIEnv *env);
extern int register_android_mtp_MtpServer(JNIEnv *env);

jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
    JNIEnv* env = NULL;
    jint result = -1;

    if (vm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK) {
        ALOGE("ERROR: GetEnv failed\n");
        goto bail;
    }
    assert(env != NULL);

    if (register_android_media_MediaPlayer(env) < 0) {
        ALOGE("ERROR: MediaPlayer native registration failed\n");
        goto bail;
    }

    if (register_android_media_MediaRecorder(env) < 0) {
        ALOGE("ERROR: MediaRecorder native registration failed\n");
        goto bail;
    }

    if (register_android_media_MediaScanner(env) < 0) {
        ALOGE("ERROR: MediaScanner native registration failed\n");
        goto bail;
    }

    if (register_android_media_MediaMetadataRetriever(env) < 0) {
        ALOGE("ERROR: MediaMetadataRetriever native registration failed\n");
        goto bail;
    }

    if (register_android_media_AmrInputStream(env) < 0) {
        ALOGE("ERROR: AmrInputStream native registration failed\n");
        goto bail;
    }

    if (register_android_media_ResampleInputStream(env) < 0) {
        ALOGE("ERROR: ResampleInputStream native registration failed\n");
        goto bail;
    }

    if (register_android_media_MediaProfiles(env) < 0) {
        ALOGE("ERROR: MediaProfiles native registration failed");
        goto bail;
    }

    if (register_android_mtp_MtpDatabase(env) < 0) {
        ALOGE("ERROR: MtpDatabase native registration failed");
        goto bail;
    }

    if (register_android_mtp_MtpDevice(env) < 0) {
        ALOGE("ERROR: MtpDevice native registration failed");
        goto bail;
    }

    if (register_android_mtp_MtpServer(env) < 0) {
        ALOGE("ERROR: MtpServer native registration failed");
        goto bail;
    }

    if (register_android_media_MediaCodec(env) < 0) {
        ALOGE("ERROR: MediaCodec native registration failed");
        goto bail;
    }

    if (register_android_media_MediaExtractor(env) < 0) {
        ALOGE("ERROR: MediaCodec native registration failed");
        goto bail;
    }

    if (register_android_media_MediaCodecList(env) < 0) {
        ALOGE("ERROR: MediaCodec native registration failed");
        goto bail;
    }

    if (register_android_media_Crypto(env) < 0) {
        ALOGE("ERROR: MediaCodec native registration failed");
        goto bail;
    }

    /* success -- return valid version number */
    result = JNI_VERSION_1_4;

bail:
    return result;
}

// KTHXBYE
