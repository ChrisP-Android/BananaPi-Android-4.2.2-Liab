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

#ifndef CEDARX_RECORDER_H_

#define CEDARX_RECORDER_H_

#include <media/MediaRecorderBase.h>
#include <utils/String8.h>
#include <binder/MemoryBase.h>
#include <binder/MemoryHeapBase.h>
#include <media/AudioRecord.h>
#include <CDX_PlayerAPI.h>

namespace android {

class Camera;
class AudioRecord;

class CedarXRecorder : public MediaRecorderBase	, public RefBase{
public:
    CedarXRecorder();
    virtual ~CedarXRecorder();

    virtual status_t init();
    virtual status_t setAudioSource(audio_source as);
    virtual status_t setVideoSource(video_source vs);
    virtual status_t setOutputFormat(output_format of);
    virtual status_t setAudioEncoder(audio_encoder ae);
    virtual status_t setVideoEncoder(video_encoder ve);
    virtual status_t setVideoSize(int width, int height);
    virtual status_t setVideoFrameRate(int frames_per_second);
    virtual status_t setCamera(const sp<ICamera>& camera);
    virtual status_t setPreviewSurface(const sp<ISurface>& surface);
    virtual status_t setOutputFile(const char *path);
    virtual status_t setOutputFile(int fd, int64_t offset, int64_t length);
    virtual status_t setParameters(const String8& params);
    virtual status_t setListener(const sp<IMediaRecorderClient>& listener);
    virtual status_t prepare();
    virtual status_t start();
    virtual status_t pause();
    virtual status_t stop();
    virtual status_t close();
    virtual status_t reset();
    virtual status_t getMaxAmplitude(int *max);
    virtual status_t dump(int fd, const Vector<String16>& args) const;

	void releaseOneRecordingFrame(const sp<IMemory>& frame);
	void dataCallbackTimestamp(
				int64_t timestampUs, int32_t msgType, const sp<IMemory> &data);

	void CedarXReleaseFrame(int index);
	status_t CedarXReadAudioBuffer(void *pbuf, int *size, int64_t *timeStamp);
	
	// Encoding parameter handling utilities
	status_t setParameter(const String8 &key, const String8 &value);
	status_t setParamVideoEncodingBitRate(int32_t bitRate);
	status_t setParamAudioEncodingBitRate(int32_t bitRate);
    status_t setParamAudioNumberOfChannels(int32_t channles);
    status_t setParamAudioSamplingRate(int32_t sampleRate);
    status_t setParamAudioTimeScale(int32_t timeScale);
	
	status_t setParamMaxFileDurationUs(int64_t timeUs);
	status_t setParamMaxFileSizeBytes(int64_t bytes);
	status_t setParamVideoRotation(int32_t degrees);
	
	int CedarXRecorderCallback(int event, void *info);
	
private:
	enum CameraFlags {
        FLAGS_SET_CAMERA = 1L << 0,
        FLAGS_HOT_CAMERA = 1L << 1,
    };

	enum {
        kMaxBufferSize = 2048,

        // After the initial mute, we raise the volume linearly
        // over kAutoRampDurationUs.
        kAutoRampDurationUs = 300000,

        // This is the initial mute duration to suppress
        // the video recording signal tone
        kAutoRampStartUs = 700000,
    };

    sp<Camera> mCamera;
    sp<ISurface> mPreviewSurface;
    sp<IMediaRecorderClient> mListener;

	audio_source mAudioSource;
    video_source mVideoSource;
    output_format mOutputFormat;
    audio_encoder mAudioEncoder;
    video_encoder mVideoEncoder;
	
    int32_t mCameraId;
    int32_t mVideoWidth, mVideoHeight;
    int32_t mFrameRate;
    int32_t mVideoBitRate;
	int64_t mMaxFileDurationUs;
	int64_t mMaxFileSizeBytes;
	int32_t mRotationDegrees;  // Clockwise

    int32_t mAudioBitRate;
    int32_t mAudioChannels;
	int32_t mSampleRate;

    int mOutputFd;
	int32_t mFlags;

	Mutex mLock;
	
	sp<MemoryHeapBase>  mFrameHeap;
	sp<MemoryBase>      mFrameBuffer;

	bool				bRecorderRunning;
	uint 				mRecModeFlag;
	AudioRecord 		* mRecord;
	CDXRecorder 		*mCdxRecorder;

	int64_t				mLatencyStartUs;
#define AUDIO_LATENCY_TIME	900000		// US
#define VIDEO_LATENCY_TIME	900000		// US

	status_t CreateAudioRecorder();
		
    CedarXRecorder(const CedarXRecorder &);
    CedarXRecorder &operator=(const CedarXRecorder &);
};

}  // namespace android

#endif  // CEDARX_RECORDER_H_

