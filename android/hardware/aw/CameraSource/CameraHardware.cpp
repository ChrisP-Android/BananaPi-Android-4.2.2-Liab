//#define LOG_NDEBUG 0
#define LOG_TAG "CameraHardware"
#include <utils/Log.h>
#include "AWOmxDebug.h"

#include <fcntl.h> 
#include <sys/mman.h> 
#include <utils/threads.h>
#include "CameraHardware.h"
#include "videodev2.h"

#define NB_BUFFER 4

#define V4L2_MODE_VIDEO				0x0002	/*  For video capture */
#define V4L2_MODE_IMAGE				0x0003	/*  For image capture */

#define CHECK_ERROR(err)											\
	if ((err) != OK)												\
	{																\
		ALOGE("ERROR: %s, line: %d", __FUNCTION__, __LINE__);		\
		return UNKNOWN_ERROR;										\
	}

namespace android {

CameraHardware::CameraHardware(int deviveid) :
	mFirstFrame(true),
	mFrameId(0),
	mVideoFormat(0),
	mV4l2Handle(0),
	mBufferCnt(NB_BUFFER),
	mThread(0),
	mCameraStarted(false),
	mDeviceID(deviveid),
	mVideoWidth(640),
	mVideoHeight(480),
	mTvdFlag(false),
	mTakingPicture(false) {

	ALOGV("mVideoWidth: %d, mVideoHeight: %d", mVideoWidth, mVideoHeight);
}

CameraHardware::~CameraHardware()
{
	ALOGV("CameraHardware, Destructor");
}

void CameraHardware::resetParam()
{
	memset(&mMapMem, 0, sizeof(v4l2_mem_map_t));	
	initDefaultParameters();
}

void CameraHardware::initDefaultParameters()
{
	mCameraStarted = false;
	mVideoFormat = V4L2_PIX_FMT_NV12;
	mFirstFrame = true;
}

int CameraHardware::setListener(const sp<VideoCaptureListener> &Listener)
{
	mListener = Listener;
	return OK;
}

int CameraHardware::connectCamera()
{	
	// open v4l2 camera device
	return openCameraDev();
}

void CameraHardware::disconnectCamera()
{
	// close v4l2 camera device
	closeCameraDev();
}

int CameraHardware::startCamera(int width, int height, bool isTakingPicture)
{
	if (mCameraStarted) {
		ALOGW("CameraHardware::startTVDecoder: state already started");
		return OK;
	}

	resetParam();
	mVideoWidth = width;
	mVideoHeight = height;
	mTakingPicture = isTakingPicture;
	mCameraStarted = true;
	mFrameId = 0;

	CHECK_ERROR(v4l2Init());

	if (mTakingPicture == false) {
		mThread = new videoCaptureThread(this);
		mThread->startThread();
	} else {
		videoCaptureGetFrame();
	}

	return OK;
}

int CameraHardware::stopCamera()
{	
	if (!mCameraStarted) {
		ALOGW("CameraHardware::stopTVDecoder: state already stopped");
		return OK;
	}

	if (mTakingPicture == false) {
		if (mThread != 0) {
			mThread->stopThread();		
			mThread.clear();
			mThread = 0;
		}
	}

	v4l2DeInit();

	mFirstFrame = true;
	mCameraStarted = false;
	return OK;
}

int CameraHardware::videoCaptureGetFrame()
{
	int ret = UNKNOWN_ERROR;
	ret = v4l2WaitCamerReady();
	if (ret != 0) {
		ALOGE("wait time out");
		return __LINE__;
	}

	// get one video frame
	struct v4l2_buffer buf;
	memset(&buf, 0, sizeof(v4l2_buffer));
	ret = getPreviewFrame(&buf);
	if (ret != OK)
	{
		usleep(10000);
		return ret;
	}

	mCurFrameTimestamp = (int64_t)((int64_t)buf.timestamp.tv_usec + (((int64_t)buf.timestamp.tv_sec) * 1000000));
	mListener->postDataTimestamp(mDeviceID, mCurFrameTimestamp, (void *)&buf, (void*)mMapMem.mem[buf.index]);

	ALOGV("CameraHardware, mCurFrameTimestamp: %lld", mCurFrameTimestamp);
	
	return OK;
}

void CameraHardware::videoCaptureReleaseFrame(int index)
{
	int ret = UNKNOWN_ERROR;
	struct v4l2_buffer buf;
	
	memset(&buf, 0, sizeof(v4l2_buffer));
	buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE; 
    buf.memory = V4L2_MEMORY_MMAP; 
	buf.index = index;
	
	ALOGV("videoCaptureReleaseFrame,buf.index: %d", buf.index);
    ret = ioctl(mV4l2Handle, VIDIOC_QBUF, &buf); 
    if (ret != 0) 
	{
		// comment for temp, to do
        ALOGE("releasePreviewFrame: VIDIOC_QBUF Failed: index = %d, ret = %d, %s", 
			buf.index, ret, strerror(errno)); 
    }
}

int CameraHardware::v4l2Init()
{
	struct v4l2_input inp;
	inp.index = 0;
	if (-1 == ioctl (mV4l2Handle, VIDIOC_S_INPUT, &inp))
	{
		ALOGE("VIDIOC_S_INPUT error!\n");
		return -1;
	}

	// check v4l2 device capabilities
	int ret = -1;
	struct v4l2_capability cap; 
	ret = ioctl (mV4l2Handle, VIDIOC_QUERYCAP, &cap); 
    if (ret < 0) 
	{ 
        ALOGE("Error opening device: unable to query device."); 
        return -1; 
    } 

    if ((cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) == 0) 
	{ 
        ALOGE("Error opening device: video capture not supported."); 
        return -1; 
    } 
  
    if ((cap.capabilities & V4L2_CAP_STREAMING) == 0) 
	{ 
        ALOGE("Capture device does not support streaming i/o"); 
        return -1; 
    } 

	
	if(strcmp((char*)cap.driver, "sun4i_csi") == 0) {
		   ALOGD("cap.driver: %s", cap.driver);
	} else {
		//mTvdFlag = true;
		mTvdFlag = false;
	}

	// set capture mode
	struct v4l2_streamparm params;
  	params.parm.capture.timeperframe.numerator = 1;
	params.parm.capture.timeperframe.denominator = 30;
	params.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (mTakingPicture)
	{
		params.parm.capture.capturemode = V4L2_MODE_IMAGE;
	}
	else
	{
		params.parm.capture.capturemode = V4L2_MODE_VIDEO;
	}
	v4l2setCaptureParams(&params);
		
	// set v4l2 device parameters
	CHECK_ERROR(v4l2SetVideoParams());
	
	// v4l2 request buffers
	CHECK_ERROR(v4l2ReqBufs());
	
	// v4l2 query buffers
	CHECK_ERROR(v4l2QueryBuf());
	
	// stream on the v4l2 device
	CHECK_ERROR(v4l2StartStreaming());

	ALOGD("start camera ok");
	return OK;
}

void CameraHardware::v4l2DeInit()
{
	// v4l2 device stop stream
	v4l2StopStreaming();

	// v4l2 device unmap buffers
    v4l2UnmapBuf();
}

int CameraHardware::openCameraDev()
{
    //struct v4l2_input inp;
	// open V4L2 device
	
	if(mDeviceID == 0)
	{
		mV4l2Handle = open("/dev/video0", O_RDWR | O_NONBLOCK, 0);
		//        inp.index = 0;  //mDeviceID;
    //    	if (-1 == ioctl (mV4l2Handle, VIDIOC_S_INPUT, &inp))
    //    	{
    //    		ALOGE("VIDIOC_S_INPUT error!");
        		//goto END_ERROR;
    //    	}

	}
	else
	{
		mV4l2Handle = open("/dev/video1", O_RDWR | O_NONBLOCK, 0);
	}
	
	if (mV4l2Handle == -1) 
	{ 
        ALOGE("ERROR opening V4L interface: %s", strerror(errno)); 
		return -1; 
	} 

	ALOGV("mDeviceID:%d, mV4l2Handle:%d", mDeviceID, mV4l2Handle);

	return OK;
}

void CameraHardware::closeCameraDev()
{
	if (mV4l2Handle != NULL)
	{
		close(mV4l2Handle);
		mV4l2Handle = NULL;
	}
}


int CameraHardware::v4l2SetVideoParams()
{
	int ret = UNKNOWN_ERROR;
	struct v4l2_format format;
	int system;

	ALOGI("%s, line: %d, w: %d, h: %d, pfmt: %d", 
		__FUNCTION__, __LINE__, mVideoWidth, mVideoHeight, mVideoFormat);


	if(mTvdFlag)
	{
		//tvin 提供两个ioctl来设置和查询
		//VIDIOC_S_FMT、VIDIOC_G_FMT
		//注意类型是V4L2_BUF_TYPE_PRIVATE
		//
		//对raw_data进行如下约定
		//raw_data[0]: interface --- 0=composite 1=ypbpr
		//raw_data[1]: system	 --- 0=ntsc/480i 1=pal/576i
		//raw_data[8]: row		 --- channel number in row
		//raw_data[9]: column	 --- channel number in column
		//raw_data[10]: channel_index[0]	--- channel0 index, 0=disable, non-0=index
		//raw_data[11]: channel_index[1]	--- channel1 index, 0=disable, non-0=index
		//raw_data[12]: channel_index[2]	--- channel2 index, 0=disable, non-0=index
		//raw_data[13]: channel_index[3]	--- channel3 index, 0=disable, non-0=index
		//raw_data[16 + i]: status	 -------(i=0~3) bit0=detect(0=no detect, 1=detect) bit4=system(0=nstc, 1=pal)
		//
		//interface表示接口，如复合视频和分量
		//system制式
		//row表示横向的通道数
		//column表示纵向的通道数
		//channel_index如果是0表示不适用该通路，是非零数字表示在屏幕的摆放位置，从左到右，从上到下，从1开始，最大为4
		//例如，四路复合视频、制式为ntsc、排成2*2，从左上角依次排放，则
		//interface=0, system=0, row=2, column=0, channel_index[0]=1, channel_index[1]=2, channel_index[2]=3, channel_index[3]=4
		//
		//上层根据配置的row、column、system来得到显示尺寸
		//比如row=1, column=1, system=0, 则width=720, height=480
		//row=2, column=2, system=1, 则width=1440, height=1152
		
		memset(&format, 0, sizeof(format));
		format.type                = V4L2_BUF_TYPE_PRIVATE;
		if (-1 == ioctl (mV4l2Handle, VIDIOC_G_FMT, &format)) 
		{
			ALOGE("VIDIOC_G_FMT error!  a\n");
			ret = -1;
			return ret; 
		}
		if((format.fmt.raw_data[16 ] & 1) == 0)
		{
			ALOGE("format.fmt.raw_data[16], error");
			ret = -1;
			return ret; 
		}
		system = (format.fmt.raw_data[16]  & (1 << 4) ) != 0 ? 1: 0;

		memset(&format, 0, sizeof(format));
		format.type                = V4L2_BUF_TYPE_PRIVATE;
		format.fmt.raw_data[0] =0;//interface
		format.fmt.raw_data[1] = system;//system

		format.fmt.raw_data[2] =0;//format 1=mb, for test only		
		format.fmt.raw_data[8] =1;//row
		format.fmt.raw_data[9] =1;//column
		
		format.fmt.raw_data[10] =1;//channel_index
		format.fmt.raw_data[11] =0;//channel_index
		format.fmt.raw_data[12] =0;//channel_index
		format.fmt.raw_data[13] =0;//channel_index

		if (-1 == ioctl (mV4l2Handle, VIDIOC_S_FMT, &format)) //设置自定义
		{
			ALOGE("VIDIOC_S_FMT error!  a\n");
			ret = -1;
			return ret; 
		}
	}

	memset(&format, 0, sizeof(format));
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE; 
    format.fmt.pix.width  = mVideoWidth; 
    format.fmt.pix.height = mVideoHeight; 
    format.fmt.pix.pixelformat = mVideoFormat; 
	format.fmt.pix.field = V4L2_FIELD_NONE;
	
	ret = ioctl(mV4l2Handle, VIDIOC_S_FMT, &format); 
	if (ret < 0) 
	{ 
		ALOGE("VIDIOC_S_FMT Failed: %s", strerror(errno)); 
		return ret; 
	} 

	mVideoWidth = format.fmt.pix.width;
	mVideoHeight= format.fmt.pix.height;
	ALOGV("camera params: w: %d, h: %d, pfmt: %d, pfield: %d", 
		mVideoWidth, mVideoHeight, mVideoFormat, V4L2_FIELD_NONE);

	return OK;
}

int CameraHardware::v4l2ReqBufs()
{
	int ret = UNKNOWN_ERROR;
	struct v4l2_requestbuffers rb; 

	if (mTakingPicture)
	{
		mBufferCnt = 1;
	}
	else
	{
		mBufferCnt = NB_BUFFER;
	}

	ALOGV("TO VIDIOC_REQBUFS count: %d", mBufferCnt);
	
	memset(&rb, 0, sizeof(rb));
    rb.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE; 
    rb.memory = V4L2_MEMORY_MMAP; 
    rb.count  = mBufferCnt; 
	
	ret = ioctl(mV4l2Handle, VIDIOC_REQBUFS, &rb); 
    if (ret < 0) 
	{ 
        ALOGE("Init: VIDIOC_REQBUFS failed: %s", strerror(errno)); 
		return ret;
    } 

	if (mBufferCnt != rb.count)
	{
		mBufferCnt = rb.count;
		ALOGV("VIDIOC_REQBUFS count: %d", mBufferCnt);
	}

	return OK;
}

int CameraHardware::v4l2QueryBuf()
{
	int ret = UNKNOWN_ERROR;
	struct v4l2_buffer buf;
	
	for (int i = 0; i < mBufferCnt; i++) 
	{  
        memset (&buf, 0, sizeof (struct v4l2_buffer)); 
		buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE; 
		buf.memory = V4L2_MEMORY_MMAP; 
		buf.index  = i; 
		
		ret = ioctl (mV4l2Handle, VIDIOC_QUERYBUF, &buf); 
        if (ret < 0) 
		{ 
            ALOGE("Unable to query buffer (%s)", strerror(errno)); 
            return ret; 
        } 
 
        mMapMem.mem[i] = mmap (0, buf.length, 
                            PROT_READ | PROT_WRITE, 
                            MAP_SHARED, 
                            mV4l2Handle, 
                            buf.m.offset); 
		mMapMem.length = buf.length;
		ALOGI("index: %d, mem: %x, len: %x, offset: %x", i, (int)mMapMem.mem[i], buf.length, buf.m.offset);
 
        if (mMapMem.mem[i] == MAP_FAILED) 
		{ 
			ALOGE("Unable to map buffer (%s)", strerror(errno)); 
            return -1; 
        } 

		// start with all buffers in queue
        ret = ioctl(mV4l2Handle, VIDIOC_QBUF, &buf); 
        if (ret < 0) 
		{ 
            ALOGE("VIDIOC_QBUF Failed"); 
            return ret; 
        } 
	} 

	return OK;
}

int CameraHardware::v4l2setCaptureParams(struct v4l2_streamparm * params)
{
	int ret = -1;

	ret = ioctl(mV4l2Handle, VIDIOC_S_PARM, params);
	if (ret < 0)
		ALOGE("v4l2setCaptureParams failed!");
	else 
		ALOGV("v4l2setCaptureParams ok");

	return ret;
}

int CameraHardware::v4l2StartStreaming()
{
	int ret = UNKNOWN_ERROR; 
	enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE; 
	
  	ret = ioctl (mV4l2Handle, VIDIOC_STREAMON, &type); 
	if (ret < 0) 
	{ 
		ALOGE("StartStreaming: Unable to start capture: %s", strerror(errno)); 
		return ret; 
	} 

	return OK;
}

int CameraHardware::v4l2StopStreaming()
{
	int ret = UNKNOWN_ERROR; 
	enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE; 
	
	ret = ioctl (mV4l2Handle, VIDIOC_STREAMOFF, &type); 
	if (ret < 0) 
	{ 
		ALOGE("StopStreaming: Unable to stop capture: %s", strerror(errno)); 
		return ret; 
	} 
	ALOGD("V4L2Camera::v4l2StopStreaming OK");
	
	return OK;
}

int CameraHardware::v4l2UnmapBuf()
{
	int ret = UNKNOWN_ERROR;
	
	for (int i = 0; i < mBufferCnt; i++) 
	{
		ret = munmap(mMapMem.mem[i], mMapMem.length);
        if (ret < 0) 
		{
            ALOGE("v4l2CloseBuf Unmap failed"); 
			return ret;
		}
	}
	
	return OK;
}

int CameraHardware::v4l2WaitCamerReady()
{
	fd_set fds;		
	struct timeval tv;
	int r;

	FD_ZERO(&fds);
	FD_SET(mV4l2Handle, &fds);		
	
	/* Timeout */
	tv.tv_sec  = 2;
	tv.tv_usec = 0;
	
	r = select(mV4l2Handle + 1, &fds, NULL, NULL, &tv);
	if (r == -1) 
	{
		ALOGE("select err");
		return -1;
	} 
	else if (r == 0) 
	{
		ALOGE("select timeout");
		return -1;
	}

	return 0;
}

int CameraHardware::getPreviewFrame(v4l2_buffer *buf)
{
	int ret = UNKNOWN_ERROR;
	
	buf->type   = V4L2_BUF_TYPE_VIDEO_CAPTURE; 
    buf->memory = V4L2_MEMORY_MMAP; 
 
    ret = ioctl(mV4l2Handle, VIDIOC_DQBUF, buf); 
    if (ret < 0) 
	{ 
        ALOGE("GetPreviewFrame: VIDIOC_DQBUF Failed"); 
        return __LINE__; 			// can not return false
    }
	
	return OK;
}

int CameraHardware::tryFmtSize(int * width, int * height)
{
	int ret = -1;
	struct v4l2_format fmt;

	ALOGV("V4L2Camera::TryFmtSize: w: %d, h: %d", *width, *height);

	memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE; 
    fmt.fmt.pix.width  = *width; 
    fmt.fmt.pix.height = *height; 
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_NV12; 
	fmt.fmt.pix.field = V4L2_FIELD_NONE;

	ret = ioctl(mV4l2Handle, VIDIOC_TRY_FMT, &fmt); 
	if (ret < 0) 
	{ 
		ALOGE("VIDIOC_TRY_FMT Failed: %s", strerror(errno)); 
		return ret; 
	} 

	// driver surpport this size
	*width = fmt.fmt.pix.width; 
    *height = fmt.fmt.pix.height; 

	return 0;
}

int CameraHardware::setWhiteBalance(int wb)
{
	struct v4l2_control ctrl;
	int ret = -1;

	ctrl.id = V4L2_CID_DO_WHITE_BALANCE;
	ctrl.value = wb;
	ret = ioctl(mV4l2Handle, VIDIOC_S_CTRL, &ctrl);
	if (ret < 0)
		ALOGV("setWhiteBalance failed!");
	else 
		ALOGV("setWhiteBalance ok");

	return ret;
}

int CameraHardware::setExposure(int exp)
{
	int ret = -1;
	struct v4l2_control ctrl;

	ctrl.id = V4L2_CID_EXPOSURE;
	ctrl.value = exp;
	ret = ioctl(mV4l2Handle, VIDIOC_S_CTRL, &ctrl);
	if (ret < 0)
		ALOGV("setExposure failed!");
	else 
		ALOGV("setExposure ok");

	return ret;
}

}; // namespace android


