#ifndef CAMERA_HARDWARE_H
#define CAMERA_HARDWARE_H

#include <linux/videodev.h> 
#include <utils/threads.h>
#include "videodev2.h"

namespace android {

// ref-counted object for callbacks
class VideoCaptureListener: virtual public RefBase
{
public:
    virtual void postDataTimestamp(int devideId, int64_t timestamp, void *dataPtr, void *virAddr) = 0;
};

class CameraHardware {
public:	
	CameraHardware(int deviveid);
	~CameraHardware();
	
	int 			connectCamera();
	void 			disconnectCamera();
	int				tryFmtSize(int * width, int * height);
	int				setWhiteBalance(int wb);
	int				setExposure(int exp);
	int				startCamera(int width, int height, bool isTakingPicture);
	int				stopCamera();
	void			videoCaptureReleaseFrame(int index);
	int				setListener(const sp<VideoCaptureListener> &Listener);


	class videoCaptureThread : public Thread {
    public:
        videoCaptureThread(CameraHardware* hw) :
		              Thread(false),
		              mHardware(hw) {
        }
			  
		void startThread() {
			run("CameraHardwareThread", PRIORITY_URGENT_DISPLAY);
		}
		
		void stopThread() {
			requestExitAndWait();
        }
        virtual bool threadLoop() 
		{
            mHardware->videoCaptureGetFrame();
            // loop until we need to quit
            return true;
        }
		
	private:
        CameraHardware* mHardware;
		
    };


private:
	bool checkInit()
	{
		return mCheckInitOK;
	}
	
	void					resetParam();
	void 					initDefaultParameters();
	
	int						dispInit();
	void					dispDeInit();
	int						v4l2Init();
	void					v4l2DeInit();

	int						openCameraDev();
	void					closeCameraDev();
	int 					v4l2SetVideoParams();
	int 					v4l2ReqBufs();
	int 					v4l2QueryBuf();
	int  					v4l2setCaptureParams(struct v4l2_streamparm * params);
	int 					v4l2StartStreaming(); 
	int 					v4l2StopStreaming(); 
	int 					v4l2UnmapBuf();
	int						getFrameRate();								

	int						v4l2WaitCamerReady();
	int						getPreviewFrame(v4l2_buffer *buf);
	int						videoCaptureGetFrame();

    int						mDeviceID;
	bool					mCheckInitOK;
	bool					mFirstFrame;
	int						mFrameId;
	
	int						mScreenWidth;
	int						mScreenHeight;

	int						mVideoWidth;
	int						mVideoHeight;
	int						mVideoFormat;

	int						mV4l2Handle;
	
	// actually buffer counts
	int						mBufferCnt;
	sp<VideoCaptureListener>  mListener;
	
	typedef struct v4l2_mem_map_t{
		void *	mem[5]; 
		int 	length;
	}v4l2_mem_map_t;
	v4l2_mem_map_t			mMapMem;
	
	sp<videoCaptureThread>	mThread;
	bool					mCameraStarted;
	int                     overlayStarted;
	int                     videoLayer;

	bool					mOverlayFirstFrame;
	int64_t 				mCurFrameTimestamp;
	bool 					mTakingPicture;
	bool					mTvdFlag;
};

};  // namespace android

#endif // CAMERA_HARDWARE_H