//#define LOG_NDEBUG 0
#define LOG_TAG "AWcodecTest"
#include <utils/Log.h>
#include "AWOmxDebug.h"

#include "CameraHardware.h"
#include "H264encLibApi.h"
#include "CDX_Resource_Manager.h"
#include "libcedarv.h"
#include <pthread.h>
#include <stdio.h>

//#define DECODE_TEST


#define SAVE_ENCODE_DATA
#define SAVE_BITSTREAM_LENGTH

static unsigned int mWidth = 640;   //1280;
static unsigned int mHeight = 480;  //720;

static unsigned int recorder_time = 20; //s
static unsigned int mAvg_bit_rate = 3*1024*1024;
static unsigned int mframeRate = 30;

ve_mutex_t m_cedarv_req_enc_ctx = NULL;
//CEDARV_REQUEST_CONTEXT  m2_cedarv_req_enc_ctx;
ve_mutex_t m_cedarv_req_dec_ctx = NULL;
//CEDARV_REQUEST_CONTEXT  m2_cedarv_req_dec_ctx;
unsigned char * sps_pps_data = NULL;
unsigned int sps_pps_data_length = 0;


#ifdef SAVE_ENCODE_DATA
FILE *fp_enc = NULL;
#define ENC_FILE_NAME "/data/camera/test.h264"
#endif


#ifdef DECODE_TEST
FILE *fp_dec = NULL;
#define INPUT_DECODE_FILE_NAME "/data/camera/in.h264"
#endif

#define ENC_FIFO_LEVEL 4
typedef struct camera_buf_info
{
	VEnc_FrmBuf_Info		buf_info;
	int 					id;
}camera_buf_info;

typedef struct bufMrgQ_t
{
	camera_buf_info			omx_bufhead[ENC_FIFO_LEVEL];	
	int						write_id;
	int						read_id;
	int						buf_unused;
}bufMrgQ_t;


bool mStarted = false;
pthread_mutex_t mtx_cam_buf;

pthread_t thread_enc_id = 0;
pthread_t thread_dec_id = 0;


bufMrgQ_t	gBufMrgQ;
VENC_DEVICE * m_encoder = NULL;
cedarv_decoder_t* m_decoder = NULL;


using namespace android;

CameraHardware *cameraSource = NULL;

extern "C" void dataCallbackTimestamp(int32_t devideId, int64_t timestampUs, void *data, void *virAddr);
static VENC_DEVICE * CedarvEncInit(unsigned int width, unsigned int height, unsigned int avg_bit_rate);
static void CedarvEncExit(VENC_DEVICE *pCedarV);


// *1. camera module

//----------------------------------------------------------------------------------
// AWCameraListener
//----------------------------------------------------------------------------------
class AWCameraListener : public VideoCaptureListener {

public:
	AWCameraListener();
    virtual void postDataTimestamp(int32_t devideId, int64_t timestamp, void *dataPtr, void *virAddr);

protected:
    virtual ~AWCameraListener();

private:
    AWCameraListener(const AWCameraListener &);
    AWCameraListener &operator=(const AWCameraListener &);
};

AWCameraListener::AWCameraListener()
{
	ALOGV("AWCameraListener Construct\n");
}

AWCameraListener::~AWCameraListener() {
	ALOGV("AWCameraListener DeConstruct\n");
}

void AWCameraListener::postDataTimestamp(int32_t devideId, int64_t timestamp, void *dataPtr, void *virAddr) {
	dataCallbackTimestamp(devideId, timestamp, dataPtr, virAddr);
}


// use this callback to get camera frame buffer
extern "C" void dataCallbackTimestamp(int32_t devideId, int64_t timestampUs, void *data, void *virAddr)
{
	int ret;
	struct v4l2_buffer *pBuf;
	int write_id;
	int read_id;
	int buf_unused;

	pBuf = (struct v4l2_buffer *)data;

	//ALOGD("dataCallbackTimestamp, timestampUs", timestampUs);

	//* if encoder is stopped or paused ,release this camera frame buffer
	if(mStarted == false) {
		cameraSource->videoCaptureReleaseFrame(pBuf->index);
		return ;
	}

	pthread_mutex_lock(&mtx_cam_buf);
	write_id 	= gBufMrgQ.write_id;
	read_id 	= gBufMrgQ.read_id;
	buf_unused	= gBufMrgQ.buf_unused;
	if(buf_unused != 0)
	{
		gBufMrgQ.omx_bufhead[write_id].buf_info.pts = timestampUs;
		gBufMrgQ.omx_bufhead[write_id].id = pBuf->index;
		gBufMrgQ.omx_bufhead[write_id].buf_info.addrY = (unsigned char *)pBuf->m.offset;
		gBufMrgQ.omx_bufhead[write_id].buf_info.addrCb = (unsigned char *)pBuf->m.offset + mWidth* mHeight;		
		gBufMrgQ.buf_unused--;
		gBufMrgQ.write_id++;
		gBufMrgQ.write_id %= ENC_FIFO_LEVEL;
	}
	else
	{
		ALOGE("IN OMX_ErrorUnderflow\n");
	}
	
	pthread_mutex_unlock(&mtx_cam_buf);
}


//* 2.encoder module

static VENC_DEVICE * CedarvEncInit(unsigned int width, unsigned int height, unsigned int avg_bit_rate, unsigned int framerate)
{
	int ret = -1;

	VENC_DEVICE *pCedarV = NULL;
	
	pCedarV = H264EncInit(&ret);
	if (ret < 0)
	{
		ALOGE("H264EncInit failed\n");
	}

	__video_encode_format_t enc_fmt;
	enc_fmt.src_width = width;
	enc_fmt.src_height = height;
	enc_fmt.width = width;
	enc_fmt.height = height;
	enc_fmt.frame_rate = framerate * 1000; //frame rate set
	enc_fmt.color_format = PIXEL_YUV420;
	enc_fmt.color_space = BT601;
	enc_fmt.qp_max = 40;
	enc_fmt.qp_min = 20;
	enc_fmt.avg_bit_rate = avg_bit_rate;
	enc_fmt.maxKeyInterval = 25;

    //enc_fmt.profileIdc = 77; /* main profile */
	enc_fmt.profileIdc = 66; /* baseline profile */
	enc_fmt.levelIdc = 31;

	pCedarV->IoCtrl(pCedarV, VENC_SET_ENC_INFO_CMD, (unsigned int)(&enc_fmt));

	ret = pCedarV->open(pCedarV);

	ALOGD("pCedarV->open: %d\n", ret);

	if (ret < 0)
	{
		ALOGE("open H264Enc failed\n");
	}

	return pCedarV;
}

static void CedarvEncExit(VENC_DEVICE *pCedarV)
{
	if (pCedarV)
	{
		pCedarV->close(pCedarV);
		H264EncExit(pCedarV);
		pCedarV = NULL;
	}
}


void *thread_enc(void* data)
{
	int ret;
	int write_id;
	int read_id;
	int buf_unused;
	__vbv_data_ctrl_info_t data_info;
	int id;

	while(mStarted) {

		VEnc_FrmBuf_Info encBuf;

		memset((void*)&encBuf, 0, sizeof(VEnc_FrmBuf_Info));

		pthread_mutex_lock(&mtx_cam_buf);

		buf_unused	= gBufMrgQ.buf_unused;
		if(buf_unused == ENC_FIFO_LEVEL) {
			pthread_mutex_unlock(&mtx_cam_buf);

			ALOGV("camera buffer is empty\n");
			usleep(10*1000);
			continue;
		}
		pthread_mutex_unlock(&mtx_cam_buf);

		if (ve_mutex_lock(&m_cedarv_req_enc_ctx) < 0) {
        //if (RequestCedarVFrameLevel(&m2_cedarv_req_enc_ctx) < 0) {

			usleep(10*1000);
			continue;
		}

		pthread_mutex_lock(&mtx_cam_buf);
	    write_id 	= gBufMrgQ.write_id;
		read_id 	= gBufMrgQ.read_id;
		buf_unused	= gBufMrgQ.buf_unused;
		encBuf.addrY = gBufMrgQ.omx_bufhead[read_id].buf_info.addrY;
		encBuf.addrCb = gBufMrgQ.omx_bufhead[read_id].buf_info.addrCb;
		encBuf.pts_valid = 1;
		encBuf.pts = (s64)gBufMrgQ.omx_bufhead[read_id].buf_info.pts;
		encBuf.color_fmt = PIXEL_YUV420;
		encBuf.color_space = BT601;
		encBuf.pover_overlay = NULL;
		id = gBufMrgQ.omx_bufhead[read_id].id;
		gBufMrgQ.read_id++;
		gBufMrgQ.read_id %= ENC_FIFO_LEVEL;
		pthread_mutex_unlock(&mtx_cam_buf);

		ret = m_encoder->encode(m_encoder, &encBuf);
		
		ve_mutex_unlock(&m_cedarv_req_enc_ctx);
        //ReleaseCedarVFrameLevel(&m2_cedarv_req_enc_ctx);

		ALOGD("encode,ret: %d", ret);
		pthread_mutex_unlock(&mtx_cam_buf);
		gBufMrgQ.buf_unused++;
		pthread_mutex_unlock(&mtx_cam_buf);

		//* release camera buffer

		// ALOGD("release camera id: %d\n", id);
		cameraSource->videoCaptureReleaseFrame(id);

		if (ret != 0) {
			ALOGE("ret: %d", ret);
			usleep(10*1000);
			continue;
		}

		memset(&data_info, 0 , sizeof(__vbv_data_ctrl_info_t));
		ret = m_encoder->GetBitStreamInfo(m_encoder, &data_info);
		
		if(ret == 0) {

#ifdef SAVE_ENCODE_DATA


#ifdef SAVE_BITSTREAM_LENGTH
			int bit_stream_size = 0;
	
			bit_stream_size = data_info.uSize0 + data_info.uSize1;
	
			if(data_info.keyFrameFlag) {
	
				bit_stream_size += sps_pps_data_length;
			}
	
			fwrite(&bit_stream_size, 1, 4, fp_enc);
#endif


			if(data_info.keyFrameFlag) {
				fwrite(sps_pps_data, 1, sps_pps_data_length, fp_enc);
			}

			if(data_info.uSize0) {
				fwrite(data_info.pData0, 1, data_info.uSize0, fp_enc);
				if(data_info.uSize1) {
					fwrite(data_info.pData1, 1, data_info.uSize1, fp_enc);
				}
			}
#endif
			//* encode release bitstream
			m_encoder->ReleaseBitStreamInfo(m_encoder, data_info.idx);
		}
	}

	return (void*)0;
}



//* 3.decoder module

#ifdef DECODE_TEST

void *thread_dec(void *)
{
	int ret;
    unsigned char* pBuf0;
	unsigned char* pBuf1;
	unsigned int size0;
	unsigned int size1;
	unsigned int require_size;
	unsigned char* pData;
	cedarv_stream_data_info_t dataInfo;
	cedarv_picture_t picture;
	
	while(1)
	{
		ret = fread(&require_size, 1, 4, fp_dec);
		
		if(ret != 4) {
			ALOGD("Read file end");
			break;
		}
		
        if(m_decoder->request_write(m_decoder, require_size, &pBuf0, &size0, &pBuf1, &size1) != 0)
        {
            ALOGE("req vbs fail!");
			//* report a fatal error.
			break;
        }

        dataInfo.lengh = require_size;
        dataInfo.flags = CEDARV_FLAG_FIRST_PART | CEDARV_FLAG_LAST_PART;
		dataInfo.pts = -1; //the file do not have valid pts, fix it later
		//dataInfo.flags |= CEDARV_FLAG_PTS_VALID;

        if(require_size <= size0)
        {
        	ret = fread(pBuf0, 1, require_size, fp_dec);
        }
        else
        {
        	ret = fread(pBuf0, 1, size0, fp_dec);
        	ret = fread(pBuf1, 1, require_size - size0, fp_dec);
        }
        m_decoder->update_data(m_decoder, &dataInfo);	

 QEREST_VE_AGAIN:
		if (ve_mutex_lock(&m_cedarv_req_dec_ctx) < 0) {
        //if (RequestCedarVFrameLevel(&m2_cedarv_req_dec_ctx) < 0) {
		
			usleep(20*1000);
			ALOGD("decode can't get ve resource");
			goto QEREST_VE_AGAIN;
		}
		
		ret = m_decoder->decode(m_decoder);

		ALOGD("decode result: %d", ret);

		ve_mutex_unlock(&m_cedarv_req_dec_ctx);
        //ReleaseCedarVFrameLevel(&m2_cedarv_req_dec_ctx);

		if (CEDARV_RESULT_NO_FRAME_BUFFER == ret ||
			CEDARV_RESULT_NO_BITSTREAM == ret) {
			usleep(10*1000);
			continue;
		}


		/* decoder output one frame for display */
		ret = m_decoder->display_request(m_decoder, &picture);

		ALOGV("display result: %d", ret);
		if(ret == CEDARV_RESULT_OK)
		{
			/* decoder release the frame */
			m_decoder->display_release(m_decoder, picture.id);
		}
	}

	return (void *)0;
}

#endif


int main()
{	
	Data_Container sps_pps_container;
	cedarv_stream_info_t 	stream_info;
	int err;
	int result;

    ALOGD("(f:%s, l:%d) AWCodecTest Start!", __FUNCTION__, __LINE__);
	/***************************************************************/
    /******************** init encoder *****************************/
    /***************************************************************/

	// * use ve mutex to lock ve hardware resource for encoder
	if (ve_mutex_init(&m_cedarv_req_enc_ctx, CEDARV_ENCODE) < 0) {
		ALOGE("cedarv_req_ctx init fail!!");
		
        goto EXIT;
	}
//    m2_cedarv_req_enc_ctx.usage = CEDARV_USAGE_VIDEO;
//	if (RequestCedarVResource(&m2_cedarv_req_enc_ctx) < 0) {
//		ALOGE("cedarv_req_ctx init fail!!");
//        goto EXIT;
//	}

    //* when CedarvEncInit need to lock ve
	if (ve_mutex_lock(&m_cedarv_req_enc_ctx) < 0) {
    //if (RequestCedarVFrameLevel(&m2_cedarv_req_enc_ctx) < 0) {

		ALOGE("CedarvEncInit lock ve resource error");
		goto EXIT;
	}

	//* aw hardware encoder device
	m_encoder = CedarvEncInit(mWidth, mHeight,mAvg_bit_rate, mframeRate);
	ve_mutex_unlock(&m_cedarv_req_enc_ctx);
    //ReleaseCedarVFrameLevel(&m2_cedarv_req_enc_ctx);


	//* get sps pps data
	if(sps_pps_data == NULL) {
		sps_pps_data = (unsigned char *)malloc(64);
	}
	m_encoder->IoCtrl(m_encoder, VENC_GET_SPS_PPS_DATA, (unsigned int)(&sps_pps_container));
	memcpy(sps_pps_data, sps_pps_container.data, sps_pps_container.length);
	sps_pps_data_length = sps_pps_container.length;


	/***************************************************************/
    /******************** init decoder *****************************/
    /***************************************************************/


#ifdef DECODE_TEST

	// * use ve mutex to lock ve hardware resource for decoder
	if (ve_mutex_init(&m_cedarv_req_dec_ctx, CEDARV_DECODE) < 0) {
		ALOGE("cedarv_req_ctx init fail!!");
		
        goto EXIT;
	}
//    m2_cedarv_req_dec_ctx.usage = CEDARV_USAGE_VIDEO;
//	if (RequestCedarVResource(&m2_cedarv_req_dec_ctx) < 0) {
//		ALOGE("cedarv_req_ctx init fail!!");
//        goto EXIT;
//	}

	if (ve_mutex_lock(&m_cedarv_req_dec_ctx) < 0) {
    //if (RequestCedarVFrameLevel(&m2_cedarv_req_dec_ctx) < 0) {

		ALOGE("CedarvEncInit lock ve resource error");
		goto EXIT;
	}

	//* aw hardware decoder device
    m_decoder = libcedarv_init((s32*)&err);
	//ve_mutex_lock(&m_cedarv_req_dec_ctx);
    //RequestCedarVFrameLevel(&m2_cedarv_req_dec_ctx);
    ve_mutex_unlock(&m_cedarv_req_dec_ctx);
    //ReleaseCedarVFrameLevel(&m2_cedarv_req_dec_ctx);
	
    if(m_decoder == NULL)
    {
    	ALOGE("xxxxxxxxxxxx can not create video decoder.");
    	goto EXIT;
    }


	//* set video stream info.
	memset(&stream_info, 0, sizeof(stream_info));
	stream_info.video_width = mWidth;
	stream_info.video_height = mHeight;
	stream_info.format = CEDARV_STREAM_FORMAT_H264; 
	stream_info.container_format = CEDARV_CONTAINER_FORMAT_UNKNOW;
	stream_info.init_data = NULL;
	stream_info.init_data_len = 0;
	
	if(m_decoder->set_vstream_info(m_decoder, &stream_info) != 0)
	{
		ALOGE("set_vstream_info() return fail.\n");
	}
	

	//* open and start decoder.
	if (ve_mutex_lock(&m_cedarv_req_dec_ctx) < 0) {
    //if (RequestCedarVFrameLevel(&m2_cedarv_req_dec_ctx) < 0) {
		ALOGE("reqest ve resource failed\n");
	}
	
	result = m_decoder->open(m_decoder);
	if(result != 0)
	{
		ve_mutex_unlock(&m_cedarv_req_dec_ctx);
        //ReleaseCedarVFrameLevel(&m2_cedarv_req_dec_ctx);
		ALOGW("Idle transition failed, open decoder return fail, result: %d\n", result);
	}
	
	m_decoder->ioctrl(m_decoder, CEDARV_COMMAND_PLAY, 0);
	
	ve_mutex_unlock(&m_cedarv_req_dec_ctx);
    //ReleaseCedarVFrameLevel(&m2_cedarv_req_dec_ctx);
#endif


	pthread_mutex_init(&mtx_cam_buf, NULL);



	/***************************************************************/
    /******************** init camera *****************************/
    /***************************************************************/

	//* init camera FIFO buffer
	memset((void*)&gBufMrgQ, 0, sizeof(bufMrgQ_t));
	gBufMrgQ.buf_unused = ENC_FIFO_LEVEL;
	
	//* Create camera source
	cameraSource = new CameraHardware(0);

	//* conect camera
	cameraSource->connectCamera();
	
	//* set camera callback
	cameraSource->setListener(new AWCameraListener());

	//* start camera
	cameraSource->startCamera(mWidth, mHeight, 0);

#ifdef SAVE_ENCODE_DATA
	if(fp_enc == NULL) {
		fp_enc = fopen(ENC_FILE_NAME, "wb");
		if(fp_enc == NULL){
			ALOGE("open file %s failed", ENC_FILE_NAME);
			goto EXIT;
		}
	}
#endif


#ifdef DECODE_TEST

	if(fp_dec == NULL) {
		fp_dec = fopen(INPUT_DECODE_FILE_NAME, "rb");
		if(fp_dec == NULL){
			ALOGE("open input dec file %s failed", INPUT_DECODE_FILE_NAME);
			goto EXIT;
		}
	} 
#endif

	ALOGD("init ok");

	mStarted = true;
	/* create encode thread*/
	if(pthread_create(&thread_enc_id, NULL, thread_enc, NULL) != 0)
	{
		ALOGE("Create thread_enc fail !\n");
	}

#ifdef DECODE_TEST

	/* create decode thread*/
	if(pthread_create(&thread_dec_id, NULL, thread_dec, NULL) != 0)
	{
		ALOGE("Create thread_dec fail !\n");
	}
#endif


	sleep(recorder_time); 
    ALOGD("(f:%s, l:%d) AWCodecTest record [%d]s, will stop!", __FUNCTION__, __LINE__, recorder_time);
	mStarted = false;


	//* stop camera
    cameraSource->stopCamera();



 EXIT:
	//* wait encoder thread exit
	if(thread_enc_id !=0) {                 
        pthread_join(thread_enc_id,NULL);
    }

#ifdef DECODE_TEST

	//* wait decoder thread exit
	if(thread_dec_id !=0) {                 
        pthread_join(thread_dec_id,NULL);
    }
#endif

	if(cameraSource != NULL) {
		
		//* disconect camera
		cameraSource->disconnectCamera();

		delete cameraSource;
		cameraSource = NULL;
	}

	if(m_encoder) {
		CedarvEncExit(m_encoder);
		m_encoder = NULL;
	}

	if(sps_pps_data) {
		free(sps_pps_data);
		sps_pps_data = NULL;
	}


#ifdef DECODE_TEST

	if(m_decoder != NULL)
    {
	    if (ve_mutex_lock(&m_cedarv_req_dec_ctx) < 0) 
        //if (RequestCedarVFrameLevel(&m2_cedarv_req_dec_ctx) < 0) 
		{
			 ALOGW("Request VE failed\n");
		}
		
    	libcedarv_exit(m_decoder);
    	m_decoder = NULL;

		ve_mutex_unlock(&m_cedarv_req_dec_ctx);
        //ReleaseCedarVFrameLevel(&m2_cedarv_req_dec_ctx);
    }
	ve_mutex_destroy(&m_cedarv_req_dec_ctx);
    //ReleaseCedarVResource(&m2_cedarv_req_dec_ctx);
#endif

	ve_mutex_destroy(&m_cedarv_req_enc_ctx);
    //ReleaseCedarVResource(&m2_cedarv_req_enc_ctx);

	pthread_mutex_destroy(&mtx_cam_buf);

#ifdef SAVE_ENCODE_DATA
	if(fp_enc) {
		fclose(fp_enc);
		fp_enc = NULL;
	}
#endif


#ifdef DECODE_TEST
	if(fp_dec) {
		fclose(fp_dec);
		fp_dec = NULL;
	} 
#endif
    ALOGD("(f:%s, l:%d) AWCodecTest quit!", __FUNCTION__, __LINE__);
	return 0;
}

