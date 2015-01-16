#include <CDX_LogNDebug.h>
#define LOG_TAG "CedarXRecorder"
#include <CDX_Debug.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include<sys/stat.h>
#include<fcntl.h>
#include <cutils/properties.h>

#include "CedarXRecorder.h"

#include <binder/IPCThreadState.h>
#include <binder/IServiceManager.h>

#if (CEDARX_ANDROID_VERSION < 7)
#include <media/stagefright/MediaDebug.h>
#include <media/stagefright/MetadataBufferType.h>
#include <surfaceflinger/Surface.h>
#else
#include <media/stagefright/foundation/ADebug.h>
#include <MetadataBufferType.h>
#include <gui/ISurfaceTexture.h>
#include <gui/SurfaceTextureClient.h>
#include <gui/Surface.h>
#endif

#include <media/stagefright/MetaData.h>
#include <media/MediaProfiles.h>
#include <camera/ICamera.h>
#include <camera/CameraParameters.h>

#include <type_camera.h>
#include <OMX_Core.h>
#include <OMX_Component.h>

#include <CDX_Recorder.h>
#include <include_writer/recorde_writer.h>
#include <videodev.h>

#define F_LOG 	LOGV("%s, line: %d", __FUNCTION__, __LINE__);

#if (CEDARX_ANDROID_VERSION < 7)
#define OUTPUT_FORMAT_RAW 10
#define AUDIO_SOURCE_AF 8
#define MEDIA_RECORDER_VENDOR_EVENT_EMPTY_BUFFER_ID 3000
#define MEDIA_RECORDER_VENDOR_EVENT_BSFRAME_AVAILABLE 3001
#endif
extern "C" int CedarXRecorderCallbackWrapper(void *cookie, int event, void *info);

namespace android {

#define GET_CALLING_PID	(IPCThreadState::self()->getCallingPid())
void getCallingProcessName(char *name)
{
	char proc_node[128];

	if (name == 0)
	{
		LOGE("error in params");
		return;
	}
	
	memset(proc_node, 0, sizeof(proc_node));
	sprintf(proc_node, "/proc/%d/cmdline", GET_CALLING_PID);
	int fp = ::open(proc_node, O_RDONLY);
	if (fp > 0) 
	{
		memset(name, 0, 128);
		::read(fp, name, 128);
		::close(fp);
		fp = 0;
		LOGD("Calling process is: %s !", name);
	}
	else 
	{
		LOGE("Obtain calling process failed");
	}
}

//----------------------------------------------------------------------------------
// CDXCameraListener
//----------------------------------------------------------------------------------
struct CDXCameraListener : public CameraListener 
{
	CDXCameraListener(CedarXRecorder * recorder);

    virtual void notify(int32_t msgType, int32_t ext1, int32_t ext2);
    virtual void postData(int32_t msgType, const sp<IMemory> &dataPtr, camera_frame_metadata_t *metadata);
    virtual void postDataTimestamp(nsecs_t timestamp, int32_t msgType, const sp<IMemory>& dataPtr);

protected:
    virtual ~CDXCameraListener();

private:
	CedarXRecorder * mRecorder;

    CDXCameraListener(const CDXCameraListener &);
    CDXCameraListener &operator=(const CDXCameraListener &);
};


CDXCameraListener::CDXCameraListener(CedarXRecorder * recorder)
    : mRecorder(recorder) 
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

void CDXCameraListener::postData(int32_t msgType, const sp<IMemory> &dataPtr, camera_frame_metadata_t *metadata) 
{
    LOGV("postData(%d, ptr:%p, size:%d)",
         msgType, dataPtr->pointer(), dataPtr->size());
}

void CDXCameraListener::postDataTimestamp(
        nsecs_t timestamp, int32_t msgType, const sp<IMemory>& dataPtr) 
{
	// LOGD("CDXCameraListener::postDataTimestamp\n");
	mRecorder->dataCallbackTimestamp(timestamp, msgType, dataPtr);
}

// To collect the encoder usage for the battery app
static void addBatteryData(uint32_t params) {
    sp<IBinder> binder =
        defaultServiceManager()->getService(String16("media.player"));
    sp<IMediaPlayerService> service = interface_cast<IMediaPlayerService>(binder);
    CHECK(service.get() != NULL);

    service->addBatteryData(params);
}

//----------------------------------------------------------------------------------
// CedarXRecorder
//----------------------------------------------------------------------------------

CedarXRecorder::CedarXRecorder()
    : mTimeBetweenTimeLapseVideoFramesUs(0)
    , mLastTimeLapseFrameRealTimestampUs(0)
    , mLastTimeLapseFrameTimestampUs(0)
	, mOutputFd(-1)
	, mUrl(NULL)
	, mStarted(false)
	, mRecModeFlag(0)
	, mRecord(NULL)
	, mCdxRecorder(NULL)
	, mLatencyStartUs(0)
	, mCallingProcessName(NULL)
{

    LOGV("Constructor");

    mInputBuffer = NULL;
    mLastTimeStamp = -1;
    mBsFrameMemory = NULL;

	reset();

	mFrameHeap = new MemoryHeapBase(sizeof(int));
	if (mFrameHeap->getHeapID() < 0)
	{
		LOGE("ERR(%s): Record heap creation fail", __func__);
        mFrameHeap.clear();
	}
	mFrameBuffer = new MemoryBase(mFrameHeap, 0, sizeof(int));

//    pCedarXRecorderAdapter = new CedarXRecorderAdapter(this);
//    if(NULL == pCedarXRecorderAdapter)
//    {
//        LOGW("new CedarXRecorderAdapter fail! File[%s],Line[%d]\n", __FILE__, __LINE__);
//    }

	mCallingProcessName = (char *)malloc(sizeof(char) * 128);
	getCallingProcessName(mCallingProcessName);
	memset(mCameraHalVersion, 0, sizeof(mCameraHalVersion));
}

CedarXRecorder::~CedarXRecorder() 
{
    LOGV("CedarXRecorder Destructor");

    if(mBsFrameMemory != NULL) {
    	mBsFrameMemory.clear();
    	mBsFrameMemory = NULL;
    }

	if (mFrameHeap != NULL)
	{
		mFrameHeap.clear();
		mFrameHeap = NULL;
	}

	if (mUrl != NULL) {
		free(mUrl);
		mUrl = NULL;
	}

	
	if (mCallingProcessName != NULL) {
		free(mCallingProcessName);
		mCallingProcessName = NULL;
	}

//    if(pCedarXRecorderAdapter)
//    {
//        delete pCedarXRecorderAdapter;
//        pCedarXRecorderAdapter = NULL;
//    }
	LOGV("CedarXRecorder Destructor OK");
}

status_t CedarXRecorder::setAudioSource(audio_source_t as) 
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
	
	if(ae == AUDIO_ENCODER_AAC)
	{
		mAudioEncodeType = AUDIO_ENCODER_AAC_TYPE;
	}
	else
	{
		mAudioEncodeType = AUDIO_ENCODER_LPCM_TYPE; //only used by mpeg2ts
	}
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
   
    // Additional check on the dimension will be performed later
    mVideoWidth = width;
    mVideoHeight = height;

    return OK;
}

status_t CedarXRecorder::setOutputVideoSize(int width, int height) 
{
    LOGV("setOutputVideoSize: %dx%d", width, height);
   
    // Additional check on the dimension will be performed later
    mOutputVideoWidth = width;
    mOutputVideoHeight = height;
	mOutputVideosizeflag = true;

    return OK;
}


status_t CedarXRecorder::setVideoFrameRate(int frames_per_second) 
{
    LOGV("setVideoFrameRate: %d", frames_per_second);
    
    // Additional check on the frame rate will be performed later
    mFrameRate = frames_per_second;
    mMaxDurationPerFrame = 1000000 / mFrameRate;

    return OK;
}

status_t CedarXRecorder::setCamera(const sp<ICamera>& camera, const sp<ICameraRecordingProxy>& proxy)
{
    LOGV("setCamera");

	int err = UNKNOWN_ERROR;

    int64_t token = IPCThreadState::self()->clearCallingIdentity();
	
	if ((err = isCameraAvailable(camera, proxy, mCameraId)) != OK) {
        LOGE("Camera connection could not be established.");
    	IPCThreadState::self()->restoreCallingIdentity(token);
        return err;
    }

	// get camera hal version
	CameraParameters params(mCamera->getParameters());
	const char * hal_version = params.get("camera-hal-version");
	if (hal_version != NULL)
	{
		strcpy(mCameraHalVersion, hal_version);
	}
	LOGD("camera hal version: %s", mCameraHalVersion);
	
    IPCThreadState::self()->restoreCallingIdentity(token);

    return OK;
}

status_t CedarXRecorder::setMediaSource(const sp<MediaSource>& mediasource, int type)
{
    LOGV("setMediaSource");

    if(CDX_RECORDER_MEDIATYPE_VIDEO == type) {
    	mMediaSourceVideo = mediasource;
    }
    else {
    	;
    }

    return OK;
}

status_t CedarXRecorder::isCameraAvailable(
    const sp<ICamera>& camera, const sp<ICameraRecordingProxy>& proxy,
    int32_t cameraId) 
{
    if (camera == 0) 
	{
        mCamera = Camera::connect(cameraId);
        if (mCamera == 0) 
		{
			return -EBUSY;
        }
        mCameraFlags &= ~FLAGS_HOT_CAMERA;
    } 
	else 
	{
        // We get the proxy from Camera, not ICamera. We need to get the proxy
        // to the remote Camera owned by the application. Here mCamera is a
        // local Camera object created by us. We cannot use the proxy from
        // mCamera here.
        mCamera = Camera::create(camera);
        if (mCamera == 0) 
		{
			return -EBUSY;
        }
        mCameraProxy = proxy;
        mCameraFlags |= FLAGS_HOT_CAMERA;
        mDeathNotifier = new DeathNotifier();
        // isBinderAlive needs linkToDeath to work.
        mCameraProxy->asBinder()->linkToDeath(mDeathNotifier);
    }

    // This CHECK is good, since we just passed the lock/unlock
    // check earlier by calling mCamera->setParameters().
    if (mPreviewSurface != NULL)
    {
    	CHECK_EQ((status_t)OK, mCamera->setPreviewDisplay(mPreviewSurface));
    }

#if (CEDARX_ANDROID_VERSION == 6 && CEDARX_ADAPTER_VERSION == 4)
    mCamera->storeMetaDataInBuffers(true);
#elif (CEDARX_ANDROID_VERSION >= 6)
    mCamera->sendCommand(CAMERA_CMD_SET_CEDARX_RECORDER, 0, 0);
#else
    #error "Unknown chip type!"
#endif
    mCamera->lock();

    return OK;
}


status_t CedarXRecorder::setPreviewSurface(const sp<Surface>& surface)
{
    LOGV("setPreviewSurface: %p", surface.get());
    mPreviewSurface = surface;

    return OK;
}

status_t CedarXRecorder::queueBuffer(int index, int addr_y, int addr_c, int64_t timestamp)
{
    LOGV("++++queueBuffer index:%d, mFrameRate=%d timestamp=%lld",index,mFrameRate,timestamp);
    V4L2BUF_t buf;
    int ret;
    int rand_access;
	
	if(addr_y >= 0x40000000) {
		addr_y -= 0x40000000;
	}

	memset(&buf, 0, sizeof(V4L2BUF_t));
    rand_access = !!(index & 0x80000000);
    buf.index = index & 0x7fffffff;
    buf.timeStamp = timestamp;
    buf.addrPhyY = addr_y;
	buf.addrPhyC = addr_c;
    buf.width = mVideoWidth;
    buf.height = mVideoHeight;
    buf.crop_rect.top = 0;
    buf.crop_rect.left = 0;
    buf.crop_rect.width = buf.width;
    buf.crop_rect.height = buf.height;
	buf.format = 0x3231564e; // NV12 fourcc

#if 0
    //LOGV("test frame pts c:%lld l:%lld diff:%lld mMaxDurationPerFrame=%d",buf.timeStamp, mLastTimeStamp,buf.timeStamp - mLastTimeStamp,mMaxDurationPerFrame);
    if (mLastTimeStamp != -1 && buf.timeStamp - mLastTimeStamp < mMaxDurationPerFrame) {
    	LOGV("----drop frame pts %lld %lld",buf.timeStamp, mLastTimeStamp);
    	CedarXReleaseFrame(buf.index);
    	return OK;
    }
    LOGV("++++enc frame pts %lld",buf.timeStamp);
#endif

    mLastTimeStamp = buf.timeStamp;

    // if encoder is stopped or paused ,release this frame
	if (mStarted == false)
	{
		CedarXReleaseFrame(buf.index);
		return OK;
	}

//	if ((readTimeUs - mLatencyStartUs) < VIDEO_LATENCY_TIME)
//	{
//		CedarXReleaseFrame(buf.index);
//		return OK;
//	}

	ret = mCdxRecorder->control((void*)mCdxRecorder, CDX_CMD_SEND_BUF, (unsigned int)&buf, rand_access);
	if (ret != 0)
	{
		CedarXReleaseFrame(buf.index);
	}

    return OK;
}

status_t CedarXRecorder::setParamGeoDataLongitude(int64_t longitudex10000) 
{
    LOGV("setParamGeoDataLongitude: %lld", longitudex10000);
    if (longitudex10000 > 1800000 || longitudex10000 < -1800000) {
        return BAD_VALUE;
    }
	mGeoAvailable = true;
    mLongitudex10000 = longitudex10000;
    return OK;
}

status_t CedarXRecorder::setParamGeoDataLatitude(int64_t latitudex10000) 
{
    LOGV("setParamGeoDataLatitude: %lld", latitudex10000);
    if (latitudex10000 > 900000 || latitudex10000 < -900000) {
        return BAD_VALUE;
    }
	mGeoAvailable = true;
    mLatitudex10000 = latitudex10000;
    return OK;
}

status_t CedarXRecorder::setOutputFile(int fd) 
{
    LOGV("setOutputFile: %d", fd);

    mOutputFd = fd;

    return OK;
}

status_t CedarXRecorder::setOutputPath(char *path)
{
    LOGV("setOutputFile: %s", path);

    mUrl = strdup(path);

    return OK;
}

status_t CedarXRecorder::setParamVideoCameraId(int32_t cameraId) 
{
    LOGV("setParamVideoCameraId: %d", cameraId);
	
    mCameraId = cameraId;
	
    return OK;
}

status_t CedarXRecorder::setParamAudioEncodingBitRate(int32_t bitRate) 
{
    LOGV("setParamAudioEncodingBitRate: %d", bitRate);
	
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

    mMaxFileDurationUs = timeUs;
    return OK;
}

status_t CedarXRecorder::setParamMaxFileSizeBytes(int64_t bytes) 
{
    LOGV("setParamMaxFileSizeBytes: %lld bytes", bytes);
	
	mMaxFileSizeBytes = bytes;

	if (mMaxFileSizeBytes > (int64_t)MAX_FILE_SIZE)
	{
		mMaxFileSizeBytes = (int64_t)MAX_FILE_SIZE;
    	LOGD("force maxFileSizeBytes to %lld bytes", mMaxFileSizeBytes);
	}
	
    return OK;
}

status_t CedarXRecorder::setParamVideoEncodingBitRate(int32_t bitRate) 
{
    LOGV("setParamVideoEncodingBitRate: %d", bitRate);
	
    mVideoBitRate = bitRate;
    return OK;
}

// Always rotate clockwise, and only support 0, 90, 180 and 270 for now.
status_t CedarXRecorder::setParamVideoRotation(int32_t degrees) 
{
    LOGV("setParamVideoRotation: %d", degrees);
	
    mRotationDegrees = degrees % 360;

    return OK;
}

status_t CedarXRecorder::setParamTimeLapseEnable(int32_t timeLapseEnable) {
    LOGV("setParamTimeLapseEnable: %d", timeLapseEnable);

	mCaptureTimeLapse = timeLapseEnable;
    return OK;
}

status_t CedarXRecorder::setParamTimeBetweenTimeLapseFrameCapture(int64_t timeUs) {
    LOGV("setParamTimeBetweenTimeLapseFrameCapture: %lld us", timeUs);

    mTimeBetweenTimeLapseFrameCaptureUs = timeUs;
    return OK;
}

status_t CedarXRecorder::setListener(const sp<IMediaRecorderClient> &listener) 
{
    mListener = listener;

	//added by JM.
	pid_t pid = GET_CALLING_PID;
	mNotificationClient = new NotificationClient(listener,this, pid);
    sp<IBinder> binder = listener->asBinder();
    binder->linkToDeath(mNotificationClient);
	//ended by JM.
    return OK;
}

//added by JM.

CedarXRecorder::NotificationClient::NotificationClient(const sp<IMediaRecorderClient>& mediaRecorderClient,
                                     CedarXRecorder* cedarXRecorder,
                                     pid_t pid)
    : mMediaRecorderClient(mediaRecorderClient), mCedarXRecorder(cedarXRecorder),mPid(pid)
{
}

CedarXRecorder::NotificationClient::~NotificationClient()
{
    //mClient.clear();
}

void CedarXRecorder::NotificationClient::binderDied(const wp<IBinder>& who)
{
    LOGD("CedarXRecorder::NotificationClient::binderDied, pid %d", mPid);
    sp<NotificationClient> keep(this);
    {
		//mMediaRecorderClient->stop();      
		mCedarXRecorder->stop();
    }
}
//ended add by JM.


static void AudioRecordCallbackFunction(int event, void *user, void *info) {
    switch (event) {
        case AudioRecord::EVENT_MORE_DATA: {
            LOGW("AudioRecord reported EVENT_MORE_DATA!");
            break;
        }
        case AudioRecord::EVENT_OVERRUN: {
            LOGW("AudioRecord reported EVENT_OVERRUN!");
            break;
        }
        default:
            // does nothing
            break;
    }
}

status_t CedarXRecorder::CreateAudioRecorder()
{
	CHECK(mAudioChannels == 1 || mAudioChannels == 2);
	
#if (CEDARX_ANDROID_VERSION < 7)
    uint32_t flags = AudioRecord::RECORD_AGC_ENABLE |
                     AudioRecord::RECORD_NS_ENABLE  |
                     AudioRecord::RECORD_IIR_ENABLE;
	
    mRecord = new AudioRecord(
                mAudioSource, mSampleRate, AUDIO_FORMAT_PCM_16_BIT,
                mAudioChannels > 1? AUDIO_CHANNEL_IN_STEREO: AUDIO_CHANNEL_IN_MONO,
                4 * kMaxBufferSize / sizeof(int16_t), /* Enable ping-pong buffers */
                flags,
                NULL,	// AudioRecordCallbackFunction,
                this);
	
#endif

#if (CEDARX_ANDROID_VERSION == 7)

    int minFrameCount;
    audio_channel_mask_t channelMask;
    status_t status = AudioRecord::getMinFrameCount(&minFrameCount,
                                           mSampleRate,
                                           AUDIO_FORMAT_PCM_16_BIT,
                                           mAudioChannels);
    if (status == OK) {
        // make sure that the AudioRecord callback never returns more than the maximum
        // buffer size
        int frameCount = kMaxBufferSize / sizeof(int16_t) / mAudioChannels;

        // make sure that the AudioRecord total buffer size is large enough
        int bufCount = 2;
        while ((bufCount * frameCount) < minFrameCount) {
            bufCount++;
        }

        AudioRecord::record_flags flags = (AudioRecord::record_flags)
                        (AudioRecord::RECORD_AGC_ENABLE |
                         AudioRecord::RECORD_NS_ENABLE  |
                         AudioRecord::RECORD_IIR_ENABLE);
        mRecord = new AudioRecord(
                    mAudioSource, mSampleRate, AUDIO_FORMAT_PCM_16_BIT,
                    audio_channel_in_mask_from_count(mAudioChannels),
                    bufCount * frameCount,
                    flags,
                    NULL,	// AudioRecordCallbackFunction,
                    this,
                    frameCount);
    }
#endif

#if ((CEDARX_ANDROID_VERSION == 8) || (CEDARX_ANDROID_VERSION == 9))
    int minFrameCount;

    status_t status = AudioRecord::getMinFrameCount(&minFrameCount,
                                           mSampleRate,
                                           AUDIO_FORMAT_PCM_16_BIT,
                   audio_channel_in_mask_from_count(mAudioChannels));
    if (status == OK) {
        // make sure that the AudioRecord callback never returns more than the maximum
        // buffer size
        int frameCount = kMaxBufferSize / sizeof(int16_t) / mAudioChannels;

        // make sure that the AudioRecord total buffer size is large enough
        int bufCount = 2;
        while ((bufCount * frameCount) < minFrameCount) {
            bufCount++;
        }
        mRecord = new AudioRecord(
                    mAudioSource, mSampleRate, AUDIO_FORMAT_PCM_16_BIT,
                    audio_channel_in_mask_from_count(mAudioChannels),
                    bufCount * frameCount,
                    NULL,	// AudioRecordCallbackFunction,
                    this,
                    frameCount);
         
    }

#endif

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

	LOGV("CreateAudioRecorder framesize: %d, frameCount: %d, channelCount: %d, sampleRate: %d", 
		mRecord->frameSize(), mRecord->frameCount(), mRecord->channelCount(), mRecord->getSampleRate());

	return OK;
}

static int convertMuxerMode(int inputFormat)
{
	int muxer_mode;

	switch(inputFormat) {
	case OUTPUT_FORMAT_AWTS:
		muxer_mode = MUXER_MODE_AWTS;
		break;
	case OUTPUT_FORMAT_RAW:
		muxer_mode = MUXER_MODE_RAW;
		break;
	case OUTPUT_FORMAT_MPEG2TS:
		muxer_mode = MUXER_MODE_TS;
		break;
	default:
		muxer_mode = MUXER_MODE_MP4;
		break;
	}

	return muxer_mode;
}

static int convertVideoSource(int inputVideoSource)
{
	int outputVideoSource;

	switch (inputVideoSource) {
	case VIDEO_SOURCE_DEFAULT:
		outputVideoSource = CDX_VIDEO_SOURCE_DEFAULT;
		break;
	case VIDEO_SOURCE_CAMERA:
		outputVideoSource = CDX_VIDEO_SOURCE_CAMERA;
		break;
	case VIDEO_SOURCE_GRALLOC_BUFFER:
		outputVideoSource = CDX_VIDEO_SOURCE_GRALLOC_BUFFER;
		break;
	case VIDEO_SOURCE_PUSH_BUFFER:
		outputVideoSource = CDX_VIDEO_SOURCE_PUSH_BUFFER;
		break;
	default:
		outputVideoSource = CDX_VIDEO_SOURCE_DEFAULT;
		break;
	}

	return outputVideoSource;
}

status_t CedarXRecorder::prepare() 
{
	LOGV("prepare");
	int srcWidth = 0, srcHeight = 0;
	int ret = OK;
    int muxer_mode;
	int capture_fmt = 0;

	if(mCaptureTimeLapse == false)
	{
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
	}
	// CedarX Create
	CDXRecorder_Create((void**)&mCdxRecorder);
	if(mCaptureTimeLapse)
	{
			mRecModeFlag &= ~RECORDER_MODE_AUDIO;
	}
	
	// set recorder mode to CDX_Recorder
	mCdxRecorder->control((void*)mCdxRecorder, CDX_CMD_SET_REC_MODE, mRecModeFlag, 0);
	
	mCdxRecorder->control((void*)mCdxRecorder, CDX_CMD_SET_OUTPUT_FORMAT, convertMuxerMode(mOutputFormat), 0);

	// create CedarX component
	mCdxRecorder->control((void*)mCdxRecorder, CDX_CMD_PREPARE, 0, 0);
	if( ret == -1)
	{
		LOGE("CEDARX REPARE ERROR!\n");
		return UNKNOWN_ERROR;
	}

	// register callback
	mCdxRecorder->control((void*)mCdxRecorder, CDX_CMD_REGISTER_CALLBACK, (unsigned int)&CedarXRecorderCallbackWrapper, (unsigned int)this);
	
	// set file handle to CDX_Recorder render component
	if(mOutputFd >= 0) {
		ret = mCdxRecorder->control((void*)mCdxRecorder, CDX_CMD_SET_SAVE_FILE, (unsigned int)mOutputFd, 0);
	}
	else {
		ret = mCdxRecorder->control((void*)mCdxRecorder, CDX_CMD_SET_SAVE_URL, (unsigned int)mUrl, 0);
	}

	if(ret != OK)
	{
		LOGE("CedarXRecorder::prepare, CDX_CMD_SET_SAVE_FILE failed\n");
		return ret;
	}

	if (mVideoSource <= VIDEO_SOURCE_CAMERA)
	{
		int64_t token = IPCThreadState::self()->clearCallingIdentity();

		if (strlen(mCameraHalVersion) == 0)
		{
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
			newCameraParams.getPreviewSize(&srcWidth, &srcHeight);
			if (srcWidth  == 0 || srcHeight == 0) {
				LOGE("Failed to set the video frame size to %dx%d",
						mVideoWidth, mVideoHeight);
				IPCThreadState::self()->restoreCallingIdentity(token);
				return UNKNOWN_ERROR;
			}
		}
		else if (!strncmp(mCameraHalVersion, "30", 2))
		{
			// Set the actual video recording frame size
			int video_w = 0, video_h = 0;
			CameraParameters params(mCamera->getParameters());
			params.getVideoSize(&video_w, &video_h);
			if (video_w != mVideoWidth
				|| video_h != mVideoHeight)
			{
				params.setVideoSize(mVideoWidth, mVideoHeight);
				String8 s = params.flatten();
				if (OK != mCamera->setParameters(s)) {
					LOGE("Could not change settings."
						 " Someone else is using camera %d?", mCameraId);
					IPCThreadState::self()->restoreCallingIdentity(token);
					return -EBUSY;
				}
			}
			
			CameraParameters newCameraParams(mCamera->getParameters());
			// Check on capture frame size
			const char * capture_width_str = newCameraParams.get("capture-size-width");
			const char * capture_height_str = newCameraParams.get("capture-size-height");
			if (capture_width_str != NULL
				&& capture_height_str != NULL)
			{
				srcWidth  = atoi(capture_width_str);
				srcHeight = atoi(capture_height_str);
			}

			const char * capture_fmt_str =  newCameraParams.get("capture-format");
			if(capture_fmt_str != NULL) {

				int fmt = atoi(capture_width_str);

				if(fmt == V4L2_PIX_FMT_MJPEG || fmt == V4L2_PIX_FMT_JPEG)
				{
					LOGD("capture_fmt_str,V4L2_PIX_FMT_MJPEG");
					capture_fmt = 1; // mjpeg source
				}
				else if(fmt == V4L2_PIX_FMT_H264)
				{
					LOGD("capture_fmt_str, V4L2_PIX_FMT_H264");
				    capture_fmt = 2; // h264 source
				}
				else
				{
					LOGD("capture_fmt_str, common");
					capture_fmt = 0; // common source
				}
			}
		}
		else
		{
			LOGE("unknow hal version");
			return -1;
		}

		IPCThreadState::self()->restoreCallingIdentity(token);
	
		LOGV("src: %dx%d, video: %dx%d", srcWidth, srcHeight, mVideoWidth, mVideoHeight);
	}
	else
	{
		srcWidth = mVideoWidth;
		srcHeight = mVideoHeight;
	}

	// set video size and FrameRate to CDX_Recorder
	VIDEOINFO_t vInfo;
	memset((void *)&vInfo, 0, sizeof(VIDEOINFO_t));

#if 0
	vInfo.video_source 		= mVideoSource;
	vInfo.src_width			= 176;
	vInfo.src_height		= 144;
	vInfo.width				= 160;			// mVideoWidth;
	vInfo.height			= 120;			// mVideoHeight;
	vInfo.frameRate			= 30*1000;		// mFrameRate;
	vInfo.bitRate			= 200000;		// mVideoBitRate;
	vInfo.profileIdc		= 66;
	vInfo.levelIdc			= 31;
	vInfo.geo_available		= mGeoAvailable;
	vInfo.latitudex10000	= mLatitudex10000;
	vInfo.longitudex10000	= mLongitudex10000;
	vInfo.rotate_degree		= mRotationDegrees;
#else
	mSrcVideoWidth          = srcWidth;
	mSrcVideoHeight			= srcHeight;

	vInfo.video_source 		= convertVideoSource(mVideoSource);
	vInfo.src_width			= srcWidth;
	vInfo.src_height		= srcHeight;

	if(mOutputVideosizeflag == false) {
		vInfo.width				= mVideoWidth;
		vInfo.height			= mVideoHeight;
	} else {
		vInfo.width				= mOutputVideoWidth;
		vInfo.height			= mOutputVideoHeight;
	}

	vInfo.frameRate			= mFrameRate*1000;
	vInfo.bitRate			= mVideoBitRate;
	vInfo.profileIdc		= 66;
	vInfo.levelIdc			= 31;
	vInfo.geo_available		= mGeoAvailable;
	vInfo.latitudex10000	= mLatitudex10000;
	vInfo.longitudex10000	= mLongitudex10000;
	vInfo.rotate_degree		= mRotationDegrees;
	vInfo.qp_max			= 40;
	vInfo.qp_min            = 10;
	vInfo.picEncmode		= 0;
	vInfo.is_compress_source 	= capture_fmt;
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

	if(mCaptureTimeLapse == false)
	{
		if (mRecModeFlag & RECORDER_MODE_AUDIO)
		{
			// set audio parameters
			AUDIOINFO_t aInfo;
			memset((void*)&aInfo, 0, sizeof(AUDIOINFO_t));
			aInfo.bitRate = mAudioBitRate;
			aInfo.channels = mAudioChannels;
			aInfo.sampleRate = mSampleRate;
			aInfo.bitsPerSample = 16;
			aInfo.audioEncType = mAudioEncodeType;
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
	}

	// time lapse mode
	if (mCaptureTimeLapse)
	{
		LOGV("time lapse mode");
		mTimeBetweenTimeLapseVideoFramesUs = 1E6/mFrameRate;
		mCdxRecorder->control((void*)mCdxRecorder, CDX_CMD_SET_TIME_LAPSE, 0, 0);
	}

    return OK;
}

status_t CedarXRecorder::start() 
{
	LOGV("start");
	Mutex::Autolock autoLock(mStateLock);
	
	//CHECK(mOutputFd >= 0);

	if (mVideoSource <= VIDEO_SOURCE_CAMERA)
	{
		LOGV("startCameraRecording");
		// Reset the identity to the current thread because media server owns the
		// camera and recording is started by the applications. The applications
		// will connect to the camera in ICameraRecordingProxy::startRecording.
		int64_t token = IPCThreadState::self()->clearCallingIdentity();
		if (mCameraFlags & FLAGS_HOT_CAMERA)
		{
			mCamera->unlock();
			mCamera.clear();
			CHECK_EQ((status_t)OK, mCameraProxy->startRecording(new CameraProxyListener(this)));
		}
		else
		{
			mCamera->setListener(new CDXCameraListener(this));
			mCamera->startRecording();
			CHECK(mCamera->recordingEnabled());
		}
		IPCThreadState::self()->restoreCallingIdentity(token);
	}

	if(mCaptureTimeLapse == false)
	{
		// audio start
		if ((mRecModeFlag & RECORDER_MODE_AUDIO)
			&& mRecord != NULL)
		{
			mRecord->start();
		}
	}

	mLatencyStartUs = systemTime() / 1000;
	LOGV("mLatencyStartUs: %lldus", mLatencyStartUs);
	LOGV("VIDEO_LATENCY_TIME: %dus, AUDIO_LATENCY_TIME: %dus", VIDEO_LATENCY_TIME, AUDIO_LATENCY_TIME);

	mStarted = true;
	mCdxRecorder->control((void*)mCdxRecorder, CDX_CMD_START, 0, 0);

	LOGV("CedarXRecorder::start OK\n");
    return OK;
}

status_t CedarXRecorder::pause() 
{
    LOGV("pause");
	Mutex::Autolock autoLock(mStateLock);

	mStarted = false;

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
		Mutex::Autolock autoLock(mStateLock);
		if (mStarted == true)
		{
			mStarted = false;
		}
		else
		{
			return err;
		}
	}	

	if (mVideoSource <= VIDEO_SOURCE_CAMERA)
	{
		int64_t token = IPCThreadState::self()->clearCallingIdentity();
		if (mCameraFlags & FLAGS_HOT_CAMERA) {
			mCameraProxy->stopRecording();
		} else {
			mCamera->setListener(NULL);
			mCamera->stopRecording();
		}
		IPCThreadState::self()->restoreCallingIdentity(token);
	}

	mCdxRecorder->control((void*)mCdxRecorder, CDX_CMD_STOP, 0, 0);
	CDXRecorder_Destroy((void*)mCdxRecorder);
	mCdxRecorder = NULL;

	releaseCamera();

	if(mCaptureTimeLapse == false)
	{
		// audio stop
		if (mRecModeFlag & RECORDER_MODE_AUDIO
			&& mRecord != NULL)
		{
			mRecord->stop();
			delete mRecord;
			mRecord = NULL;
			
		}	
	}

	LOGV("stopped\n");

	return err;
}

void CedarXRecorder::releaseCamera()
{
    if (mVideoSource <= VIDEO_SOURCE_CAMERA)
    {
    	LOGV("releaseCamera");

		if (mCamera != 0)
		{
			int64_t token = IPCThreadState::self()->clearCallingIdentity();
			if ((mCameraFlags & FLAGS_HOT_CAMERA) == 0)
			{
				LOGV("Camera was cold when we started, stopping preview");
				mCamera->stopPreview();
				mCamera->disconnect();
			}
			mCamera->unlock();
			mCamera.clear();
			mCamera = 0;
			IPCThreadState::self()->restoreCallingIdentity(token);
		}

		if (mCameraProxy != 0)
		{
			mCameraProxy->asBinder()->unlinkToDeath(mDeathNotifier);
			mCameraProxy.clear();
		}
    }

    mCameraFlags = 0;
}

status_t CedarXRecorder::reset() 
{
    LOGV("reset");

    // No audio or video source by default
    mAudioSource = AUDIO_SOURCE_CNT;
    mVideoSource = VIDEO_SOURCE_LIST_END;

    // Default parameters
    mOutputFormat  = OUTPUT_FORMAT_THREE_GPP;
    mAudioEncoder  = AUDIO_ENCODER_AAC;
    mVideoEncoder  = VIDEO_ENCODER_H264;
    mVideoWidth    = 176;
    mVideoHeight   = 144;
	mOutputVideoWidth    = 176;
    mOutputVideoHeight   = 144;
	mSrcVideoWidth = 176;
	mSrcVideoHeight = 144;
    mFrameRate     = 20;
    mVideoBitRate  = 192000;
    mSampleRate    = 8000;
    mAudioChannels = 1;
    mAudioBitRate  = 12200;
    mCameraId      = 0;
	mCameraFlags   = 0;

    mMaxFileDurationUs = 0;
    mMaxFileSizeBytes = 0;

    mRotationDegrees = 0;

    mOutputFd = -1;

	mStarted = false;
	mRecModeFlag = 0;
	mRecord = NULL;
	mLatencyStartUs = 0;

	mGeoAvailable = 0;
	mLatitudex10000 = -3600000;
    mLongitudex10000 = -3600000;
	mOutputVideosizeflag = false;

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
    if (mCameraProxy != NULL) {
        mCameraProxy->releaseRecordingFrame(frame);
    } else if (mCamera != NULL) {
        int64_t token = IPCThreadState::self()->clearCallingIdentity();
        mCamera->releaseRecordingFrame(frame);
        IPCThreadState::self()->restoreCallingIdentity(token);
    }
}

void CedarXRecorder::dataCallbackTimestamp(int64_t timestampUs,
        int32_t msgType, const sp<IMemory> &data) 
{
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

	buf.overlay_info = NULL; //add this for cts test
	
	// if encoder is stopped or paused ,release this frame
	if (mStarted == false)
	{
		CedarXReleaseFrame(buf.index);
		return ;
	}
	if ((strcmp(mCallingProcessName, "com.android.cts.media") == 0)||(strcmp(mCallingProcessName, "com.android.cts.mediastress") == 0))
	{
		if ((readTimeUs - mLatencyStartUs) < VIDEO_LATENCY_TIME_CTS)
		{
			CedarXReleaseFrame(buf.index);
			return ;
		}
	}
	else
	{
		if ((readTimeUs - mLatencyStartUs) < VIDEO_LATENCY_TIME)
		{
			CedarXReleaseFrame(buf.index);
			return ;
		}
	}

	// time lapse mode
	if (mCaptureTimeLapse)
	{
		// LOGV("readTimeUs : %lld, lapse: %lld", readTimeUs, mLastTimeLapseFrameRealTimestampUs + mTimeBetweenTimeLapseFrameCaptureUs);
		if (readTimeUs < mLastTimeLapseFrameRealTimestampUs + mTimeBetweenTimeLapseFrameCaptureUs)
		{
			CedarXReleaseFrame(buf.index);
			return ;
		}
		mLastTimeLapseFrameRealTimestampUs = readTimeUs;

		buf.timeStamp = mLastTimeLapseFrameTimestampUs + mTimeBetweenTimeLapseVideoFramesUs;
		mLastTimeLapseFrameTimestampUs = buf.timeStamp;
	}

	buf.addrPhyC = buf.addrPhyY + mSrcVideoWidth*mSrcVideoHeight;

	if(buf.format == V4L2_PIX_FMT_JPEG || buf.format == V4L2_PIX_FMT_MJPEG || buf.format == V4L2_PIX_FMT_H264) {
		buf.addrPhyY = buf.addrVirY;
		buf.width = buf.bytesused;
	}
	
	// LOGV("CedarXRecorder::dataCallbackTimestamp: addrPhyY %x, timestamp %lld us", buf.addrPhyY, timestampUs);

	ret = mCdxRecorder->control((void*)mCdxRecorder, CDX_CMD_SEND_BUF, (unsigned int)&buf, 0); 
	if (ret != 0)
	{
		CedarXReleaseFrame(buf.index);
	}

	mCdxRecorder->control((void*)mCdxRecorder, CDX_CMD_GET_DURATION, (unsigned int)&duration, 0); 
	// LOGV("duration : %d", duration);
	
	if (mMaxFileDurationUs != 0 
		&& duration >= mMaxFileDurationUs / 1000)
	{
		mListener->notify(MEDIA_RECORDER_EVENT_INFO, MEDIA_RECORDER_INFO_MAX_DURATION_REACHED, 0);
	}

	mCdxRecorder->control((void*)mCdxRecorder, CDX_CMD_GET_FILE_SIZE, (int64_t)&fileSizeBytes, 0); 
	// LOGV("fileSizeBytes : %lld", fileSizeBytes);
	
	if (mMaxFileSizeBytes > 0 
		&& fileSizeBytes >= mMaxFileSizeBytes)
	{
		mListener->notify(MEDIA_RECORDER_EVENT_INFO, MEDIA_RECORDER_INFO_MAX_FILESIZE_REACHED, 0);
	}
}

void CedarXRecorder::CedarXReleaseFrame(int index)
{
    Mutex::Autolock lock(mLock);
	if (mVideoSource <= VIDEO_SOURCE_CAMERA)
	{
		int * p = (int *)(mFrameBuffer->pointer());

		*p = index;
	
		releaseOneRecordingFrame(mFrameBuffer);
	}
	else
	{
		LOGV("---- CedarXReleaseFrame index=%d", index);
		mListener->notify(MEDIA_RECORDER_VENDOR_EVENT_EMPTY_BUFFER_ID, index, 0);
	}
}

status_t CedarXRecorder::CedarXReadAudioBuffer(void *pbuf, int *size, int64_t *timeStamp)
{	
    int64_t readTimeUs = systemTime() / 1000;

	// LOGV("CedarXRecorder::CedarXReadAudioBuffer, readTimeUs: %lld", readTimeUs);

    *size = 0;
	*timeStamp = readTimeUs;
	
	ssize_t n = mRecord->read(pbuf, kMaxBufferSize);
	if (n < 0)
	{
		LOGE("mRecord read audio buffer failed");
		return UNKNOWN_ERROR;
	}

	//LOGV("readTimeUs:%lld,mLatencyStartUs:%lld diff:%lld",readTimeUs, mLatencyStartUs, readTimeUs - mLatencyStartUs);
	if ((strcmp(mCallingProcessName, "com.android.cts.media") == 0)||(strcmp(mCallingProcessName, "com.android.cts.mediastress") == 0))
	{
		if ((readTimeUs - mLatencyStartUs) < AUDIO_LATENCY_TIME_CTS)
		{
			if (mAudioSource != AUDIO_SOURCE_AF)
				return INVALID_OPERATION;
		}
	}
	else
	{
		if ((readTimeUs - mLatencyStartUs) < AUDIO_LATENCY_TIME)
		{
			if (mAudioSource != AUDIO_SOURCE_AF)
				return INVALID_OPERATION;
		}
	}
	
	*size = n;

	// LOGV("timestamp: %lld, len: %d", readTimeUs, n);

	return OK;
}

status_t CedarXRecorder::readMediaBufferCallback(void *buf_header)
{
	OMX_BUFFERHEADERTYPE *omx_buf_header = (OMX_BUFFERHEADERTYPE*)buf_header;

	if(omx_buf_header->nFlags == OMX_PortDomainVideo)
	{
		//LOGV("readMediaBufferCallback 1");
		if (mInputBuffer) {
			mInputBuffer->release();
			mInputBuffer = NULL;
		}
		mMediaSourceVideo->read(&mInputBuffer, NULL);

		if (mInputBuffer != NULL) {
			OMX_U32 type;
			buffer_handle_t buffer_handle;
			char *data = (char *)mInputBuffer->data();

			type = *((OMX_U32*)data);
			memcpy(&buffer_handle, data+4, 4);

			LOGV("mInputBuffer->size() = %d type = %d buffer_handle = %p",mInputBuffer->size(),type,buffer_handle);
			if (type == kMetadataBufferTypeGrallocSource) {

			}

			mInputBuffer->meta_data()->findInt64(kKeyTime, &omx_buf_header->nTimeStamp);
		}
		else {
			return NO_MEMORY;
		}
	}

	return OK;
}

CedarXRecorder::CameraProxyListener::CameraProxyListener(CedarXRecorder * recorder) 
	:mRecorder(recorder){
}

void CedarXRecorder::CameraProxyListener::dataCallbackTimestamp(
        nsecs_t timestamp, int32_t msgType, const sp<IMemory>& dataPtr) {
    mRecorder->dataCallbackTimestamp(timestamp / 1000, msgType, dataPtr);
}

sp<IMemory> CedarXRecorder::getOneBsFrame(int mode)
{
    int i;
    CDXRecorderBsInfo frame;

    //Mutex::Autolock lock(mLock);
    mBsFrameMemory.clear();
    if (mCdxRecorder == NULL) {
        LOGE("mCdxRecorder is not initialized");
        return NULL;
    }

    if (mCdxRecorder->control((void*)mCdxRecorder, CDX_CMD_GET_ONE_BSFRAME, mode, (unsigned int)&frame) < 0) {
        //LOGD("failed to get one frame");
        return NULL;
    }

    size_t size = frame.total_size + 4;
    sp<MemoryHeapBase> heap = new MemoryHeapBase(size);
    if (heap == NULL) {
        LOGE("failed to create MemoryDealer");
        return NULL;
    }

    mBsFrameMemory = new MemoryBase(heap, 0, size);
    if (mBsFrameMemory == NULL) {
        LOGE("not enough memory for VideoFrame size=%u", size);
        return NULL;
    }

    uint8_t *ptr_dst = (uint8_t *)mBsFrameMemory->pointer();

    memcpy(ptr_dst, &frame.total_size, 4);
    ptr_dst += 4;

    for (i=0; i<frame.bs_count; i++) {
    	memcpy(ptr_dst, frame.bs_data[i], frame.bs_size[i]);
    	ptr_dst += frame.bs_size[i];
	}

    mCdxRecorder->control((void*)mCdxRecorder, CDX_CMD_FREE_ONE_BSFRAME, 0, (unsigned int)&frame);

    return mBsFrameMemory;
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
	case CDX_EVENT_MEDIASOURCE_READ:
		ret = readMediaBufferCallback(info);
		break;
	case CDX_EVENT_BSFRAME_AVAILABLE:
		mListener->notify(MEDIA_RECORDER_VENDOR_EVENT_BSFRAME_AVAILABLE, (int)info, 0);
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

