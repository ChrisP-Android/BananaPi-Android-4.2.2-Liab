/*
 * Copyright (C) 2009 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//#include <CDX_LogNDebug.h>
#define LOG_TAG "CedarXRecorder"
#include <utils/Log.h>

#include "CedarXRecorder.h"

#include <binder/IPCThreadState.h>
#include <media/stagefright/MediaDebug.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/MetaData.h>
#include <media/MediaProfiles.h>
#include <camera/ICamera.h>
#include <camera/Camera.h>
#include <camera/CameraParameters.h>
#include <surfaceflinger/ISurface.h>
#include <utils/Errors.h>
#include <sys/types.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/mman.h>

#include <type_camera.h>


extern "C" int CedarXRecorderCallbackWrapper(void *cookie, int event, void *info);

namespace android {

// WP or SP will cause unexpected exception, C++ pointer is ok!
// #define __WP__
// #define __SP__
#define __C__POINT__

//----------------------------------------------------------------------------------
// CDXCameraListener
//----------------------------------------------------------------------------------
struct CDXCameraListener : public CameraListener {
#ifdef __WP__
	CDXCameraListener(const wp<CedarXRecorder> &recorder);
#endif	// __WP__
#ifdef __SP__
    CDXCameraListener(const sp<CedarXRecorder> &recorder);
#endif	// __SP__
#ifdef __C__POINT__
	CDXCameraListener(CedarXRecorder * recorder);
#endif	// __C__POINT__

    virtual void notify(int32_t msgType, int32_t ext1, int32_t ext2);
    virtual void postData(int32_t msgType, const sp<IMemory> &dataPtr);

    virtual void postDataTimestamp(
            nsecs_t timestamp, int32_t msgType, const sp<IMemory>& dataPtr);

protected:
    virtual ~CDXCameraListener();

private:
#ifdef __WP__
	wp<CedarXRecorder> mRecorder;
#endif	// __WP__
#ifdef __SP__
	sp<CedarXRecorder> mRecorder;
#endif	// __SP__
#ifdef __C__POINT__
	CedarXRecorder * mRecorder;
#endif	// __C__POINT__

    CDXCameraListener(const CDXCameraListener &);
    CDXCameraListener &operator=(const CDXCameraListener &);
};

#ifdef __WP__
CDXCameraListener::CDXCameraListener(const wp<CedarXRecorder> &recorder)
    : mRecorder(recorder) 
#endif	// __WP__
#ifdef __SP__
CDXCameraListener::CDXCameraListener(const sp<CedarXRecorder> &recorder)
    : mRecorder(recorder) 
#endif	// __SP__
#ifdef __C__POINT__
CDXCameraListener::CDXCameraListener(CedarXRecorder * recorder)
    : mRecorder(recorder) 
#endif	// __C__POINT__
{
	LOGV("CDXCameraListener Construct\n");
}

CDXCameraListener::~CDXCameraListener() {
	LOGV("CDXCameraListener Destruct\n");
}

void CDXCameraListener::notify(int32_t msgType, int32_t ext1, int32_t ext2) 
{
    LOGV("notify(%d, %d, %d)", msgType, ext1, ext2);
}

void CDXCameraListener::postData(int32_t msgType, const sp<IMemory> &dataPtr) 
{
    LOGV("postData(%d, ptr:%p, size:%d)",
         msgType, dataPtr->pointer(), dataPtr->size());
}

void CDXCameraListener::postDataTimestamp(
        nsecs_t timestamp, int32_t msgType, const sp<IMemory>& dataPtr) 
{
#ifdef __WP__
    sp<CedarXRecorder> recorder = mRecorder.promote();
	LOGV("CDXCameraListener::postDataTimestamp\n");
    if (recorder.get() != NULL) 
	{
        recorder->dataCallbackTimestamp(timestamp, msgType, dataPtr);
    }
	else
	{
		LOGE("recorder.get() is NULL\n");
	}
#endif	// __WP__
#ifdef __SP__
	// LOGD("CDXCameraListener::postDataTimestamp\n");
	mRecorder->dataCallbackTimestamp(timestamp, msgType, dataPtr);
#endif	// __SP__
#ifdef __C__POINT__
	// LOGD("CDXCameraListener::postDataTimestamp\n");
	mRecorder->dataCallbackTimestamp(timestamp, msgType, dataPtr);
#endif	// __C__POINT__

}

//----------------------------------------------------------------------------------
// CedarXRecorder
//----------------------------------------------------------------------------------
CedarXRecorder::CedarXRecorder()
    : mOutputFd(-1) 
    , bRecorderRunning(false)
    , mRecModeFlag(0)
    , mRecord(NULL)
    , mLatencyStartUs(0)
{

    LOGV("Constructor");

	reset();

	mFrameHeap = new MemoryHeapBase(sizeof(int));
	if (mFrameHeap->getHeapID() < 0)
	{
		LOGE("ERR(%s): Record heap creation fail", __func__);
        mFrameHeap.clear();
	}
	mFrameBuffer = new MemoryBase(mFrameHeap, 0, sizeof(int));
}

CedarXRecorder::~CedarXRecorder() 
{
    LOGV("CedarXRecorder Destructor");

	if (mFrameHeap != NULL)
	{
		mFrameHeap.clear();
		mFrameHeap = NULL;
	}

	LOGV("CedarXRecorder Destructor OK");
}

status_t CedarXRecorder::init() 
{
    LOGV("init");
	
	CDXRecorder_Create((void**)&mCdxRecorder);
	
	LOGV("CedarXRecorder::init OK");
    return OK;
}

status_t CedarXRecorder::setAudioSource(audio_source as) 
{
    LOGV("setAudioSource: %d", as);

	mAudioSource = as;

    return OK;
}

status_t CedarXRecorder::setVideoSource(video_source vs) 
{
    LOGV("setVideoSource: %d", vs);

	mVideoSource = vs;
	
    return OK;
}

status_t CedarXRecorder::setOutputFormat(output_format of) 
{
    LOGV("setOutputFormat: %d", of);

	mOutputFormat = of;
	
    return OK;
}

status_t CedarXRecorder::setAudioEncoder(audio_encoder ae) 
{
    LOGV("setAudioEncoder: %d", ae);
   
	mRecModeFlag |= RECORDER_MODE_AUDIO;
	
    return OK;
}

status_t CedarXRecorder::setVideoEncoder(video_encoder ve) 
{
    LOGV("setVideoEncoder: %d", ve);
    
	mRecModeFlag |= RECORDER_MODE_VIDEO;
	
    return OK;
}

status_t CedarXRecorder::setVideoSize(int width, int height) 
{
    LOGV("setVideoSize: %dx%d", width, height);
    if (width <= 0 || height <= 0) {
        LOGE("Invalid video size: %dx%d", width, height);
        return BAD_VALUE;
    }

    // Additional check on the dimension will be performed later
    mVideoWidth = width;
    mVideoHeight = height;

    return OK;
}

status_t CedarXRecorder::setVideoFrameRate(int frames_per_second) 
{
    LOGV("setVideoFrameRate: %d", frames_per_second);
    if (frames_per_second <= 0 || frames_per_second > 30) {
        LOGE("Invalid video frame rate: %d", frames_per_second);
        return BAD_VALUE;
    }

    // Additional check on the frame rate will be performed later
    mFrameRate = frames_per_second;

    return OK;
}

status_t CedarXRecorder::setCamera(const sp<ICamera> &camera) 
{
    LOGV("setCamera");
	
    if (camera == 0) {
        LOGE("camera is NULL");
        return BAD_VALUE;
    }

    int64_t token = IPCThreadState::self()->clearCallingIdentity();
    mFlags &= ~FLAGS_HOT_CAMERA;
    mCamera = Camera::create(camera);
    if (mCamera == 0) {
        LOGE("Unable to connect to camera");
        IPCThreadState::self()->restoreCallingIdentity(token);
        return -EBUSY;
    }

    LOGV("Connected to camera");
    if (mCamera->previewEnabled()) {
        LOGE("camera is hot");
        mFlags |= FLAGS_HOT_CAMERA;
    }

	// star 
	if (mCamera->lock() != OK)
	{
		LOGE("Unable to lock camera");
		return -EBUSY;
	}

	// send command to hal for hw encoder
	mCamera->sendCommand(CAMERA_CMD_HW_ENCODER, 0, 0);

    IPCThreadState::self()->restoreCallingIdentity(token);

    return OK;
}

status_t CedarXRecorder::setPreviewSurface(const sp<ISurface> &surface) 
{
    LOGV("setPreviewSurface: %p", surface.get());
    mPreviewSurface = surface;

    return OK;
}

status_t CedarXRecorder::setOutputFile(const char *path) 
{
    LOGV("setOutputFile(const char*) must not be called");
    // We don't actually support this at all, as the media_server process
    // no longer has permissions to create files.

    return -EPERM;
}

status_t CedarXRecorder::setOutputFile(int fd, int64_t offset, int64_t length) 
{
    LOGV("setOutputFile: %d, %lld, %lld", fd, offset, length);
    // These don't make any sense, do they?
    CHECK_EQ(offset, 0);
    CHECK_EQ(length, 0);

    if (fd < 0) {
        LOGE("Invalid file descriptor: %d", fd);
        return -EBADF;
    }

    if (mOutputFd >= 0) {
        ::close(mOutputFd);
    }
    mOutputFd = dup(fd);

    return OK;
}

// Attempt to parse an int64 literal optionally surrounded by whitespace,
// returns true on success, false otherwise.
static bool safe_strtoi64(const char *s, int64_t *val) 
{
    char *end;
    *val = strtoll(s, &end, 10);

    if (end == s || errno == ERANGE) {
        return false;
    }

    // Skip trailing whitespace
    while (isspace(*end)) {
        ++end;
    }

    // For a successful return, the string must contain nothing but a valid
    // int64 literal optionally surrounded by whitespace.

    return *end == '\0';
}

// Return true if the value is in [0, 0x007FFFFFFF]
static bool safe_strtoi32(const char *s, int32_t *val) 
{
    int64_t temp;
    if (safe_strtoi64(s, &temp)) {
        if (temp >= 0 && temp <= 0x007FFFFFFF) {
            *val = static_cast<int32_t>(temp);
            return true;
        }
    }
    return false;
}

// Trim both leading and trailing whitespace from the given string.
static void TrimString(String8 *s) 
{
    size_t num_bytes = s->bytes();
    const char *data = s->string();

    size_t leading_space = 0;
    while (leading_space < num_bytes && isspace(data[leading_space])) {
        ++leading_space;
    }

    size_t i = num_bytes;
    while (i > leading_space && isspace(data[i - 1])) {
        --i;
    }

    s->setTo(String8(&data[leading_space], i - leading_space));
}

status_t CedarXRecorder::setParamAudioEncodingBitRate(int32_t bitRate) {
    LOGV("setParamAudioEncodingBitRate: %d", bitRate);

    // The target bit rate may not be exactly the same as the requested.
    // It depends on many factors, such as rate control, and the bit rate
    // range that a specific encoder supports. The mismatch between the
    // the target and requested bit rate will NOT be treated as an error.
    mAudioBitRate = bitRate;
    return OK;
}

status_t CedarXRecorder::setParamAudioSamplingRate(int32_t sampleRate) 
{
	LOGV("setParamAudioSamplingRate: %d", sampleRate);

	// Additional check on the sample rate will be performed later.
	mSampleRate = sampleRate;
    return OK;
}

status_t CedarXRecorder::setParamAudioNumberOfChannels(int32_t channels) 
{
    LOGV("setParamAudioNumberOfChannels: %d", channels);

    // Additional check on the number of channels will be performed later.
    mAudioChannels = channels;
    return OK;
}

status_t CedarXRecorder::setParamMaxFileDurationUs(int64_t timeUs) 
{
    LOGV("setParamMaxFileDurationUs: %lld us", timeUs);
    if (timeUs <= 0) {
        LOGW("Max file duration is not positive: %lld us. Disabling duration limit.", timeUs);
        timeUs = 0; // Disable the duration limit for zero or negative values.
    } else if (timeUs <= 100000LL) {  // XXX: 100 milli-seconds
        LOGE("Max file duration is too short: %lld us", timeUs);
        return BAD_VALUE;
    }

    mMaxFileDurationUs = timeUs;
    return OK;
}

status_t CedarXRecorder::setParamMaxFileSizeBytes(int64_t bytes) {
    LOGV("setParamMaxFileSizeBytes: %lld bytes", bytes);
    if (bytes <= 1024) {  // XXX: 1 kB
        LOGE("Max file size is too small: %lld bytes", bytes);
        return BAD_VALUE;
    }
	
	mMaxFileSizeBytes = bytes;
    return OK;
}

status_t CedarXRecorder::setParamVideoEncodingBitRate(int32_t bitRate) 
{
    LOGV("setParamVideoEncodingBitRate: %d", bitRate);
    if (bitRate <= 0) {
        LOGE("Invalid video encoding bit rate: %d", bitRate);
        return BAD_VALUE;
    }

    // The target bit rate may not be exactly the same as the requested.
    // It depends on many factors, such as rate control, and the bit rate
    // range that a specific encoder supports. The mismatch between the
    // the target and requested bit rate will NOT be treated as an error.
    mVideoBitRate = bitRate;
    return OK;
}

// Always rotate clockwise, and only support 0, 90, 180 and 270 for now.
status_t CedarXRecorder::setParamVideoRotation(int32_t degrees) {
    LOGV("setParamVideoRotation: %d", degrees);
    if (degrees < 0 || degrees % 90 != 0) {
        LOGE("Unsupported video rotation angle: %d", degrees);
        return BAD_VALUE;
    }
    mRotationDegrees = degrees % 360;

    return OK;
}

status_t CedarXRecorder::setParameter(
        const String8 &key, const String8 &value) 
{
    LOGV("setParameter: key (%s) => value (%s)", key.string(), value.string());
    
    if (key == "max-duration") {
        int64_t max_duration_ms;
        if (safe_strtoi64(value.string(), &max_duration_ms)) {
            return setParamMaxFileDurationUs(1000LL * max_duration_ms);
        }
    } else if (key == "video-param-encoding-bitrate") {
        int32_t video_bitrate;
        if (safe_strtoi32(value.string(), &video_bitrate)) {
            return setParamVideoEncodingBitRate(video_bitrate);
        }
    } else {
        LOGW("warning setParameter: failed to find key %s", key.string());
    }
    return OK;
}


status_t CedarXRecorder::setParameters(const String8 &params) 
{
    LOGV("setParameters: %s", params.string());
    const char *cparams = params.string();
    const char *key_start = cparams;

#if 0
	for (;;) {
        const char *equal_pos = strchr(key_start, '=');
        if (equal_pos == NULL) {
            LOGE("Parameters %s miss a value", cparams);
            return BAD_VALUE;
        }
        String8 key(key_start, equal_pos - key_start);
        TrimString(&key);
        if (key.length() == 0) {
            LOGE("Parameters %s contains an empty key", cparams);
            return BAD_VALUE;
        }
        const char *value_start = equal_pos + 1;
        const char *semicolon_pos = strchr(value_start, ';');
        String8 value;
        if (semicolon_pos == NULL) {
            value.setTo(value_start);
        } else {
            value.setTo(value_start, semicolon_pos - value_start);
        }
        if (setParameter(key, value) != OK) {
            return BAD_VALUE;
        }
        if (semicolon_pos == NULL) {
            break;  // Reaches the end
        }
        key_start = semicolon_pos + 1;
    }
#endif
    return OK;
}


status_t CedarXRecorder::setListener(const sp<IMediaRecorderClient> &listener) 
{
    mListener = listener;

    return OK;
}

status_t CedarXRecorder::CreateAudioRecorder()
{
	// need audio recorder
	if (mRecModeFlag & RECORDER_MODE_AUDIO)
	{
		uint32_t flags = AudioRecord::RECORD_AGC_ENABLE |
                     AudioRecord::RECORD_NS_ENABLE  |
                     AudioRecord::RECORD_IIR_ENABLE;
		
		mRecord = new AudioRecord(
                mAudioSource, mSampleRate, AudioSystem::PCM_16_BIT,
                mAudioChannels > 1? AudioSystem::CHANNEL_IN_STEREO: AudioSystem::CHANNEL_IN_MONO,
                4 * kMaxBufferSize / sizeof(int16_t), /* Enable ping-pong buffers */
                flags);

		if (mRecord == NULL)
		{
			LOGE("create AudioRecord failed");
			return UNKNOWN_ERROR;
		}

		status_t err = mRecord->initCheck();
		if (err != OK)
		{
			LOGE("AudioRecord is not initialized");
			return UNKNOWN_ERROR;
		}

		LOGV("~~~~~~~~~~~~~~framesize: %d ", mRecord->frameSize());
		LOGV("~~~~~~~~~~~~~~frameCount: %d ", mRecord->frameCount());		
		LOGV("~~~~~~~~~~~~~~channelCount: %d ", mRecord->channelCount());
		LOGV("~~~~~~~~~~~~~~getSampleRate: %d ", mRecord->getSampleRate());
	}

	return OK;
}

status_t CedarXRecorder::prepare() 
{
	LOGV("prepare");

	int ret = OK;
	VIDEOINFO_t vInfo;
	AUDIOINFO_t aInfo;

	// Create audio recorder
	if (mRecModeFlag & RECORDER_MODE_AUDIO)
	{
		ret = CreateAudioRecorder();
		if (ret != OK)
		{
			LOGE("CreateAudioRecorder failed");
			return ret;
		}
	}
	
	// set recorder mode to CDX_Recorder
	mCdxRecorder->control((void*)mCdxRecorder, CDX_CMD_SET_REC_MODE, mRecModeFlag, 0);
	
	// create CedarX component
	ret = mCdxRecorder->control((void*)mCdxRecorder, CDX_CMD_PREPARE, 0, 0);
	if( ret == -1)
	{
		printf("CEDARX REPARE ERROR!\n");
		return UNKNOWN_ERROR;
	}

	// register callback
	mCdxRecorder->control((void*)mCdxRecorder, CDX_CMD_REGISTER_CALLBACK, (unsigned int)&CedarXRecorderCallbackWrapper, (unsigned int)this);
	
	// set file handle to CDX_Recorder render component
	ret = mCdxRecorder->control((void*)mCdxRecorder, CDX_CMD_SET_SAVE_FILE, (unsigned int)mOutputFd, 0);
	if(ret != OK)
	{
		LOGE("CedarXRecorder::prepare, CDX_CMD_SET_SAVE_FILE failed\n");
		return ret;
	}

	int64_t token = IPCThreadState::self()->clearCallingIdentity();
	// Set the actual video recording frame size
    CameraParameters params(mCamera->getParameters());
    params.setPreviewSize(mVideoWidth, mVideoHeight);
    String8 s = params.flatten();
    if (OK != mCamera->setParameters(s)) {
        LOGE("Could not change settings."
             " Someone else is using camera %d?", mCameraId);
		IPCThreadState::self()->restoreCallingIdentity(token);
        return -EBUSY;
    }
    CameraParameters newCameraParams(mCamera->getParameters());

    // Check on video frame size
    int srcWidth = 0, srcHeight = 0;
    newCameraParams.getPreviewSize(&srcWidth, &srcHeight);
    if (srcWidth  == 0 || srcHeight == 0) {
        LOGE("Failed to set the video frame size to %dx%d",
                mVideoWidth, mVideoHeight);
		IPCThreadState::self()->restoreCallingIdentity(token);
        return UNKNOWN_ERROR;
    }
	IPCThreadState::self()->restoreCallingIdentity(token);

	LOGV("src: %dx%d, video: %dx%d", srcWidth, srcHeight, mVideoWidth, mVideoHeight);
	
	// set video size and FrameRate to CDX_Recorder
	memset((void *)&vInfo, 0, sizeof(VIDEOINFO_t));
#if 0
	vInfo.src_width		= 176;
	vInfo.src_height	= 144;
	vInfo.width			= 160;			// mVideoWidth;
	vInfo.height		= 120;			// mVideoHeight;
	vInfo.frameRate		= 30*1000;		// mFrameRate;
	vInfo.bitRate		= 200000;		// mVideoBitRate;
	vInfo.profileIdc	= 66;
	vInfo.levelIdc		= 31;
#else
	vInfo.src_width		= srcWidth;
	vInfo.src_height	= srcHeight;
	vInfo.width			= mVideoWidth;
	vInfo.height		= mVideoHeight;
	vInfo.frameRate		= mFrameRate*1000;
	vInfo.bitRate		= mVideoBitRate;
	vInfo.profileIdc	= 66;
	vInfo.levelIdc		= 31;
#endif
	if (mVideoWidth == 0
		|| mVideoHeight == 0
		|| mFrameRate == 0
		|| mVideoBitRate == 0)
	{
		LOGE("error video para");
		return -1;
	}
	
	ret = mCdxRecorder->control((void*)mCdxRecorder, CDX_CMD_SET_VIDEO_INFO, (unsigned int)&vInfo, 0);
	if(ret != OK)
	{
		LOGE("CedarXRecorder::prepare, CDX_CMD_SET_VIDEO_INFO failed\n");
		return ret;
	}

	if (mRecModeFlag & RECORDER_MODE_AUDIO)
	{
		// set audio parameters
		memset((void*)&aInfo, 0, sizeof(AUDIOINFO_t));
		aInfo.bitRate = mAudioBitRate;
		aInfo.channels = mAudioChannels;
		aInfo.sampleRate = mSampleRate;
		aInfo.bitsPerSample = 16;
		if (mAudioBitRate == 0
			|| mAudioChannels == 0
			|| mSampleRate == 0)
		{
			LOGE("error audio para");
			return -1;
		}
		
		ret = mCdxRecorder->control((void*)mCdxRecorder, CDX_CMD_SET_AUDIO_INFO, (unsigned int)&aInfo, 0);
		if(ret != OK)
		{
			LOGE("CedarXRecorder::prepare, CDX_CMD_SET_AUDIO_INFO failed\n");
			return ret;
		}	
	}

    return OK;
}

status_t CedarXRecorder::start() 
{
	LOGV("start");
	Mutex::Autolock autoLock(mLock);
	
	CHECK(mOutputFd >= 0);

	// audio start
	if (mRecModeFlag & RECORDER_MODE_AUDIO
		&& mRecord != NULL)
	{
		mRecord->start();
	}
	
	int64_t token = IPCThreadState::self()->clearCallingIdentity();
	mCamera->setListener(new CDXCameraListener(this));
	CHECK_EQ(OK, mCamera->startRecording());
	IPCThreadState::self()->restoreCallingIdentity(token);

	bRecorderRunning = true;
	mCdxRecorder->control((void*)mCdxRecorder, CDX_CMD_START, 0, 0);

	mLatencyStartUs = systemTime() / 1000;
	LOGV("mLatencyStartUs: %lldus", mLatencyStartUs);
	LOGV("VIDEO_LATENCY_TIME: %dus, AUDIO_LATENCY_TIME: %dus", VIDEO_LATENCY_TIME, AUDIO_LATENCY_TIME);
	LOGV("CedarXRecorder::start OK\n");
    return OK;
}

status_t CedarXRecorder::pause() 
{
    LOGV("pause");
	Mutex::Autolock autoLock(mLock);

	bRecorderRunning = false;

	if(mCdxRecorder->control((void*)mCdxRecorder, CDX_CMD_GETSTATE, 0, 0) == CDX_STATE_PAUSE)
	{
		mCdxRecorder->control((void*)mCdxRecorder, CDX_CMD_RESUME, 0, 0);
	}
	else
	{
		mCdxRecorder->control((void*)mCdxRecorder, CDX_CMD_PAUSE, 0, 0);
	}
	
    return OK;
}

status_t CedarXRecorder::stop() 
{
    LOGV("stop");
	
    status_t err = OK;
	
	{
		Mutex::Autolock autoLock(mLock);
		if (bRecorderRunning == true)
		{
			bRecorderRunning = false;
		}
		else
		{
			return err;
		}
	}	
#if 0
	/*
	enum media_recorder_info_type {
		MEDIA_RECORDER_INFO_UNKNOWN 				  = 1,
		MEDIA_RECORDER_INFO_MAX_DURATION_REACHED	  = 800,
		MEDIA_RECORDER_INFO_MAX_FILESIZE_REACHED	  = 801,
		MEDIA_RECORDER_INFO_COMPLETION_STATUS		  = 802,
		MEDIA_RECORDER_INFO_PROGRESS_FRAME_STATUS	  = 803,
		MEDIA_RECORDER_INFO_PROGRESS_TIME_STATUS	  = 804,
	};
	*/
	if (mListener != NULL)
	{
		mListener->notify(MEDIA_RECORDER_EVENT_INFO, MEDIA_RECORDER_INFO_COMPLETION_STATUS, 0);
	}
	else
	{
		LOGE("mListener is null\n");
	}
#endif

	int64_t token = IPCThreadState::self()->clearCallingIdentity();
    mCamera->setListener(NULL);		// it maybe cause exception when use WP or SP pointer
	mCamera->stopRecording();
    IPCThreadState::self()->restoreCallingIdentity(token);

	// usleep(50000);

	LOGV("to stop CDX\n");
	mCdxRecorder->control((void*)mCdxRecorder, CDX_CMD_STOP, 0, 0);
	LOGV("CDX stopped\n");
	
	CDXRecorder_Destroy((void*)mCdxRecorder);
    LOGV("CDX Exit OK\n");
	
#if 1
	token = IPCThreadState::self()->clearCallingIdentity();
	// mCamera->setListener(NULL); 	// it maybe cause exception when use WP or SP pointer
	if (mCamera != NULL)
	{	
		if ((mFlags & FLAGS_HOT_CAMERA) == 0) {
            LOGV("Camera was cold when we started, stopping preview");
            mCamera->stopPreview();
        }
		mCamera->unlock();
		mCamera.clear();
    	mCamera = NULL;
	}
    IPCThreadState::self()->restoreCallingIdentity(token);
	mFlags = 0;
#endif

	if (mOutputFd >= 0) {
        ::close(mOutputFd);
        mOutputFd = -1;
    }

	// audio stop
	if (mRecModeFlag & RECORDER_MODE_AUDIO
		&& mRecord != NULL)
	{
		mRecord->stop();
		delete mRecord;
		mRecord = NULL;
	}

	LOGV("stopped\n");

	return err;
}

status_t CedarXRecorder::close() 
{
    LOGV("close");

    return OK;
}

status_t CedarXRecorder::reset() 
{
    LOGV("reset");

	// No audio or video source by default
    mAudioSource = AUDIO_SOURCE_LIST_END;
    mVideoSource = VIDEO_SOURCE_LIST_END;

    // Default parameters
    mOutputFormat  = OUTPUT_FORMAT_THREE_GPP;
    mAudioEncoder  = AUDIO_ENCODER_AAC;
    mVideoEncoder  = VIDEO_ENCODER_H264;
    mVideoWidth    = 176;
    mVideoHeight   = 144;
    mFrameRate     = 20;
    mVideoBitRate  = 192000;
    mSampleRate    = 8000;
    mAudioChannels = 1;
    mAudioBitRate  = 12200;
    mCameraId        = 0;

    mMaxFileDurationUs = 0;
    mMaxFileSizeBytes = 0;

    mRotationDegrees = 0;
    // mEncoderProfiles = MediaProfiles::getInstance();

    mOutputFd = -1;
    mFlags = 0;

	bRecorderRunning = false;
	mRecModeFlag = 0;
	mRecord = NULL;
	mLatencyStartUs = 0;

    return OK;
}

status_t CedarXRecorder::getMaxAmplitude(int *max) 
{
    LOGV("getMaxAmplitude");

	// to do
	*max = 100;
	
    return OK;
}

void CedarXRecorder::releaseOneRecordingFrame(const sp<IMemory>& frame) 
{
	// LOGV("releaseOneRecordingFrame\n");
	
    int64_t token = IPCThreadState::self()->clearCallingIdentity();
    mCamera->releaseRecordingFrame(frame);
    IPCThreadState::self()->restoreCallingIdentity(token);
}

void CedarXRecorder::dataCallbackTimestamp(int64_t timestampUs,
        int32_t msgType, const sp<IMemory> &data) 
{
	Mutex::Autolock autoLock(mLock);
	unsigned int duration = 0;
	int64_t fileSizeBytes = 0;
	V4L2BUF_t buf;
	int ret = -1;
	int64_t readTimeUs = systemTime() / 1000;
	
	if (data == NULL)
	{
		LOGE("error IMemory data\n");
		return;
	}
	
	memcpy((void *)&buf, data->pointer(), sizeof(V4L2BUF_t));
	
	// if encoder is stopped or paused ,release this frame
	if (bRecorderRunning == false)
	{
		CedarXReleaseFrame(buf.index);
		return ;
	}

	if ((readTimeUs - mLatencyStartUs) < VIDEO_LATENCY_TIME)
	{
		CedarXReleaseFrame(buf.index);
		return ;
	}
	
	LOGV("CedarXRecorder::dataCallbackTimestamp: addrPhyY %x, timestamp %lld us", buf.addrPhyY, timestampUs);

	ret = mCdxRecorder->control((void*)mCdxRecorder, CDX_CMD_SEND_BUF, (unsigned int)&buf, 0); 
	if (ret != 0)
	{
		CedarXReleaseFrame(buf.index);
	}

	mCdxRecorder->control((void*)mCdxRecorder, CDX_CMD_GET_DURATION, (unsigned int)&duration, 0); 
	LOGV("duration : %d", duration);
	
	if (mMaxFileDurationUs != 0 
		&& duration >= mMaxFileDurationUs / 1000)
	{
		mListener->notify(MEDIA_RECORDER_EVENT_INFO, MEDIA_RECORDER_INFO_MAX_DURATION_REACHED, 0);
	}

	mCdxRecorder->control((void*)mCdxRecorder, CDX_CMD_GET_FILE_SIZE, (int64_t)&fileSizeBytes, 0); 
	LOGV("fileSizeBytes : %lld", fileSizeBytes);
	
	if (fileSizeBytes != 0 
		&& fileSizeBytes >= mMaxFileSizeBytes)
	{
		mListener->notify(MEDIA_RECORDER_EVENT_INFO, MEDIA_RECORDER_INFO_MAX_FILESIZE_REACHED, 0);
	}

#if 0
	/*
	enum media_recorder_info_type {
		MEDIA_RECORDER_INFO_UNKNOWN 				  = 1,
		MEDIA_RECORDER_INFO_MAX_DURATION_REACHED	  = 800,
		MEDIA_RECORDER_INFO_MAX_FILESIZE_REACHED	  = 801,
		MEDIA_RECORDER_INFO_COMPLETION_STATUS		  = 802,
		MEDIA_RECORDER_INFO_PROGRESS_FRAME_STATUS	  = 803,
		MEDIA_RECORDER_INFO_PROGRESS_TIME_STATUS	  = 804,
	};
	*/
	static int cnt = 0;
	if (cnt++ > 300)
	{
		LOGD("notify MEDIA_RECORDER_INFO_MAX_DURATION_REACHED");
		if (mListener != NULL)
		{
			mListener->notify(MEDIA_RECORDER_EVENT_INFO, MEDIA_RECORDER_INFO_MAX_DURATION_REACHED, 0);
		}
		else
		{
			LOGE("mListener is null\n");
		}
	}
#endif
}

status_t CedarXRecorder::dump(
        int fd, const Vector<String16>& args) const 
{
    LOGV("dump");
    
    return OK;
}

void CedarXRecorder::CedarXReleaseFrame(int index)
{
	int * p = (int *)(mFrameBuffer->pointer());
	
	*p = index;

	releaseOneRecordingFrame(mFrameBuffer);
}

status_t CedarXRecorder::CedarXReadAudioBuffer(void *pbuf, int *size, int64_t *timeStamp)
{
    int64_t readTimeUs = systemTime() / 1000;

	LOGV("CedarXRecorder::CedarXReadAudioBuffer, readTimeUs: %lld", readTimeUs);

	*timeStamp = readTimeUs;
	
	ssize_t n = mRecord->read(pbuf, kMaxBufferSize);
	if (n < 0)
	{
		LOGE("mRecord read audio buffer failed");
		return UNKNOWN_ERROR;
	}

	if ((readTimeUs - mLatencyStartUs) < AUDIO_LATENCY_TIME)
	{
		return INVALID_OPERATION;
	}
	
	static int len = 0;
	len += n;
	LOGV("mRecord->read: %d", len);
	*size = n;

	LOGV("timestamp: %lld, len: %d", readTimeUs, n);

	return OK;
}


#if 0

extern "C" int CedarXRecReadAudioBuffer(void *p, void *pbuf, int *size, int64_t *timeStamp)
{
	return ((android::CedarXRecorder*)p)->CedarXReadAudioBuffer(pbuf, size, timeStamp);
}

extern "C" void CedarXRecReleaseOneFrame(void *p, int index) 
{
	((android::CedarXRecorder*)p)->CedarXReleaseFrame(index);
}

#else

int CedarXRecorder::CedarXRecorderCallback(int event, void *info)
{
	int ret = 0;
	int *para = (int*)info;

	//LOGV("----------CedarXRecorderCallback event:%d info:%p\n", event, info);

	switch (event) {
	case CDX_EVENT_READ_AUDIO_BUFFER:
		CedarXReadAudioBuffer((void *)para[0], (int*)para[1], (int64_t*)para[2]);
		break;
	case CDX_EVENT_RELEASE_VIDEO_BUFFER:
		CedarXReleaseFrame(*para);
		break;
	default:
		break;
	}

	return ret;
}

extern "C" int CedarXRecorderCallbackWrapper(void *cookie, int event, void *info)
{
	return ((android::CedarXRecorder *)cookie)->CedarXRecorderCallback(event, info);
}

#endif

}  // namespace android

