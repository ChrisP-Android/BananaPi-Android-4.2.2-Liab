

/*============================================================================
                            O p e n M A X   w r a p p e r s
                             O p e n  M A X   C o r e

*//** @file omx_vdec.cpp
  This module contains the implementation of the OpenMAX core & component.

*//*========================================================================*/

//////////////////////////////////////////////////////////////////////////////
//                             Include Files
//////////////////////////////////////////////////////////////////////////////

//#define LOG_NDEBUG 0
#define LOG_TAG "omx_vdec"
#include <utils/Log.h>
#include "AWOmxDebug.h"

#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>
#include "omx_vdec.h"
#include <fcntl.h>
#include <media/stagefright/foundation/ADebug.h>
#include "AWOMX_VideoIndexExtension.h"

#include "transform_color_format.h"

#include <ui/GraphicBufferMapper.h>
#include <ui/Rect.h>

#define OPEN_STATISTICS (0)

using namespace android;

/* H.263 Supported Levels & profiles */
VIDEO_PROFILE_LEVEL_TYPE SupportedH263ProfileLevels[] = {
  {OMX_VIDEO_H263ProfileBaseline, OMX_VIDEO_H263Level10},
  {OMX_VIDEO_H263ProfileBaseline, OMX_VIDEO_H263Level20},
  {OMX_VIDEO_H263ProfileBaseline, OMX_VIDEO_H263Level30},
  {OMX_VIDEO_H263ProfileBaseline, OMX_VIDEO_H263Level40},
  {OMX_VIDEO_H263ProfileBaseline, OMX_VIDEO_H263Level45},
  {OMX_VIDEO_H263ProfileBaseline, OMX_VIDEO_H263Level50},
  {OMX_VIDEO_H263ProfileBaseline, OMX_VIDEO_H263Level60},
  {OMX_VIDEO_H263ProfileBaseline, OMX_VIDEO_H263Level70},
  {-1, -1}};

/* MPEG4 Supported Levels & profiles */
VIDEO_PROFILE_LEVEL_TYPE SupportedMPEG4ProfileLevels[] ={
  {OMX_VIDEO_MPEG4ProfileSimple, OMX_VIDEO_MPEG4Level0},
  {OMX_VIDEO_MPEG4ProfileSimple, OMX_VIDEO_MPEG4Level0b},
  {OMX_VIDEO_MPEG4ProfileSimple, OMX_VIDEO_MPEG4Level1},
  {OMX_VIDEO_MPEG4ProfileSimple, OMX_VIDEO_MPEG4Level2},
  {OMX_VIDEO_MPEG4ProfileSimple, OMX_VIDEO_MPEG4Level3},
  {OMX_VIDEO_MPEG4ProfileSimple, OMX_VIDEO_MPEG4Level4},
  {OMX_VIDEO_MPEG4ProfileSimple, OMX_VIDEO_MPEG4Level4a},
  {OMX_VIDEO_MPEG4ProfileSimple, OMX_VIDEO_MPEG4Level5},
  {OMX_VIDEO_MPEG4ProfileAdvancedSimple, OMX_VIDEO_MPEG4Level0},
  {OMX_VIDEO_MPEG4ProfileAdvancedSimple, OMX_VIDEO_MPEG4Level0b},
  {OMX_VIDEO_MPEG4ProfileAdvancedSimple, OMX_VIDEO_MPEG4Level1},
  {OMX_VIDEO_MPEG4ProfileAdvancedSimple, OMX_VIDEO_MPEG4Level2},
  {OMX_VIDEO_MPEG4ProfileAdvancedSimple, OMX_VIDEO_MPEG4Level3},
  {OMX_VIDEO_MPEG4ProfileAdvancedSimple, OMX_VIDEO_MPEG4Level4},
  {OMX_VIDEO_MPEG4ProfileAdvancedSimple, OMX_VIDEO_MPEG4Level5},
  {-1,-1}};

/* AVC Supported Levels & profiles */
VIDEO_PROFILE_LEVEL_TYPE SupportedAVCProfileLevels[] ={
  {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel1},
  {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel1b},
  {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel11},
  {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel12},
  {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel13},
  {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel2},
  {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel21},
  {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel22},
  {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel3},
  {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel31},
  {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel32},
  {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel4},
  {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel41},
  {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel42},
  {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel5},
  {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel51},
  {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevelMax},

  {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel1},
  {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel1b},
  {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel11},
  {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel12},
  {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel13},
  {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel2 },
  {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel21},
  {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel22},
  {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel3 },
  {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel31},
  {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel32},
  {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel4},
  {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel41},
  {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel42},
  {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel5},
  {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel51},
  {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevelMax},

  {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel1 },
  {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel1b},
  {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel11},
  {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel12},
  {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel13},
  {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel2 },
  {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel21},
  {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel22},
  {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel3 },
  {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel31},
  {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel32},
  {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel4},
  {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel41},
  {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel42},
  {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel5},
  {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel51},
  {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevelMax},
  {-1,-1}};
/*
 *     M A C R O S
 */

/*
 * Initializes a data structure using a pointer to the structure.
 * The initialization of OMX structures always sets up the nSize and nVersion fields
 *   of the structure.
 */
#define OMX_CONF_INIT_STRUCT_PTR(_s_, _name_)	\
    memset((_s_), 0x0, sizeof(_name_));	\
    (_s_)->nSize = sizeof(_name_);		\
    (_s_)->nVersion.s.nVersionMajor = 0x1;	\
    (_s_)->nVersion.s.nVersionMinor = 0x1;	\
    (_s_)->nVersion.s.nRevision = 0x0;		\
    (_s_)->nVersion.s.nStep = 0x0



static VIDDEC_CUSTOM_PARAM sVideoDecCustomParams[] =
{
	{VIDDEC_CUSTOMPARAM_ENABLE_ANDROID_NATIVE_BUFFER, (OMX_INDEXTYPE)AWOMX_IndexParamVideoEnableAndroidNativeBuffers},
	{VIDDEC_CUSTOMPARAM_GET_ANDROID_NATIVE_BUFFER_USAGE, (OMX_INDEXTYPE)AWOMX_IndexParamVideoGetAndroidNativeBufferUsage},
	{VIDDEC_CUSTOMPARAM_USE_ANDROID_NATIVE_BUFFER2, (OMX_INDEXTYPE)AWOMX_IndexParamVideoUseAndroidNativeBuffer2}
};


static void* ComponentThread(void* pThreadData);
static void* ComponentVdrvThread(void* pThreadData);


//* factory function executed by the core to create instances
void *get_omx_component_factory_fn(void)
{
	return (new omx_vdec);
}

#if (OPEN_STATISTICS)
static int64_t OMX_GetNowUs() {
    struct timeval tv;
    gettimeofday(&tv, NULL);

    return (int64_t)tv.tv_sec * 1000000ll + tv.tv_usec;
}
#endif

typedef enum OMX_VDRV_COMMANDTYPE
{
    OMX_VdrvCommand_PrepareVdecLib,
    OMX_VdrvCommand_CloseVdecLib,
    OMX_VdrvCommand_FlushVdecLib,
    OMX_VdrvCommand_Stop,
    OMX_VdrvCommand_EndOfStream,
    OMX_VdrvCommand_Max = 0X7FFFFFFF,
} OMX_VDRV_COMMANDTYPE;

void post_message_to_vdrv(omx_vdec *omx, OMX_S32 id)
{
      int ret_value;
      ALOGV("omx_vdec: post_message %d pipe out%d to vdrv\n", id,omx->m_vdrv_cmdpipe[1]);
      ret_value = write(omx->m_vdrv_cmdpipe[1], &id, sizeof(OMX_S32));
      ALOGV("post_message to pipe done %d\n",ret_value);
}

omx_vdec::omx_vdec()
{
    ALOGD("(f:%s, l:%d) ", __FUNCTION__, __LINE__);
	m_state              = OMX_StateLoaded;
	m_cRole[0]           = 0;
	m_cName[0]           = 0;
	m_eCompressionFormat = OMX_VIDEO_CodingUnused;
	m_pAppData           = NULL;
	m_thread_id          = 0;
    m_vdrv_thread_id     = 0;
	m_cmdpipe[0]         = 0;
	m_cmdpipe[1]         = 0;
	m_cmddatapipe[0]     = 0;
	m_cmddatapipe[1]     = 0;
    m_vdrv_cmdpipe[0]   = 0;
    m_vdrv_cmdpipe[1]   = 0;

	m_decoder            = NULL;

	memset(&m_Callbacks, 0, sizeof(m_Callbacks));
	memset(&m_sInPortDef, 0, sizeof(m_sInPortDef));
	memset(&m_sOutPortDef, 0, sizeof(m_sOutPortDef));
	memset(&m_sInPortFormat, 0, sizeof(m_sInPortFormat));
	memset(&m_sOutPortFormat, 0, sizeof(m_sOutPortFormat));
	memset(&m_sPriorityMgmt, 0, sizeof(m_sPriorityMgmt));
	memset(&m_sInBufSupplier, 0, sizeof(m_sInBufSupplier));
	memset(&m_sOutBufSupplier, 0, sizeof(m_sOutBufSupplier));
	memset(&m_sInBufList, 0, sizeof(m_sInBufList));
	memset(&m_sOutBufList, 0, sizeof(m_sOutBufList));

	memset(&m_streamInfo, 0, sizeof(m_streamInfo));

    m_useAndroidBuffer = OMX_FALSE;
    m_nInportPrevTimeStamp = 0;
    m_JumpFlag = OMX_FALSE;
	pthread_mutex_init(&m_inBufMutex, NULL);
	pthread_mutex_init(&m_outBufMutex, NULL);

    pthread_mutex_init(&m_pipeMutex, NULL);
    sem_init(&m_vdrv_cmd_lock,0,0);
    sem_init(&m_sem_vbs_input,0,0);
    sem_init(&m_sem_frame_output,0,0);
}


omx_vdec::~omx_vdec()
{
	OMX_S32 nIndex;
    // In case the client crashes, check for nAllocSize parameter.
    // If this is greater than zero, there are elements in the list that are not free'd.
    // In that case, free the elements.
    ALOGV("(f:%s, l:%d) ", __FUNCTION__, __LINE__);
	pthread_mutex_lock(&m_inBufMutex);

    for(nIndex=0; nIndex<m_sInBufList.nBufArrSize; nIndex++)
    {
        if(m_sInBufList.pBufArr==NULL)
        {
            break;
        }

        if(m_sInBufList.pBufArr[nIndex].pBuffer != NULL)
        {
            if(m_sInBufList.nAllocBySelfFlags & (1<<nIndex))
            {
                free(m_sInBufList.pBufArr[nIndex].pBuffer);
                m_sInBufList.pBufArr[nIndex].pBuffer = NULL;
            }
        }
    }

    if (m_sInBufList.pBufArr != NULL)
    	free(m_sInBufList.pBufArr);

    if (m_sInBufList.pBufHdrList != NULL)
    	free(m_sInBufList.pBufHdrList);

	memset(&m_sInBufList, 0, sizeof(struct _BufferList));
	m_sInBufList.nBufArrSize = m_sInPortDef.nBufferCountActual;

	pthread_mutex_unlock(&m_inBufMutex);


	pthread_mutex_lock(&m_outBufMutex);

	for(nIndex=0; nIndex<m_sOutBufList.nBufArrSize; nIndex++)
	{
		if(m_sOutBufList.pBufArr[nIndex].pBuffer != NULL)
		{
			if(m_sOutBufList.nAllocBySelfFlags & (1<<nIndex))
			{
				free(m_sOutBufList.pBufArr[nIndex].pBuffer);
				m_sOutBufList.pBufArr[nIndex].pBuffer = NULL;
			}
		}
	}

    if (m_sOutBufList.pBufArr != NULL)
    	free(m_sOutBufList.pBufArr);

    if (m_sOutBufList.pBufHdrList != NULL)
    	free(m_sOutBufList.pBufHdrList);

	memset(&m_sOutBufList, 0, sizeof(struct _BufferList));
	m_sOutBufList.nBufArrSize = m_sOutPortDef.nBufferCountActual;

	pthread_mutex_unlock(&m_outBufMutex);

    if(m_decoder != NULL)
    {
    	libcedarv_exit(m_decoder);
    	m_decoder = NULL;
    }

    pthread_mutex_destroy(&m_inBufMutex);
    pthread_mutex_destroy(&m_outBufMutex);

    pthread_mutex_destroy(&m_pipeMutex);
    sem_destroy(&m_vdrv_cmd_lock);
    sem_destroy(&m_sem_vbs_input);
    sem_destroy(&m_sem_frame_output);

#if (OPEN_STATISTICS)
    if(mDecodeFrameTotalCount!=0 && mConvertTotalCount!=0)
    {
        mDecodeFrameSmallAverageDuration = (mDecodeFrameTotalDuration + mDecodeOKTotalDuration)/(mDecodeFrameTotalCount);
        mDecodeFrameBigAverageDuration = (mDecodeFrameTotalDuration + mDecodeOKTotalDuration + mDecodeNoFrameTotalDuration + mDecodeNoBitstreamTotalDuration + mDecodeOtherTotalDuration)/(mDecodeFrameTotalCount);
        if(mDecodeNoFrameTotalCount > 0)
        {
            mDecodeNoFrameAverageDuration = (mDecodeNoFrameTotalDuration)/(mDecodeNoFrameTotalCount);
        }
        else
        {
            mDecodeNoFrameAverageDuration = 0;
        }
        if(mDecodeNoBitstreamTotalCount > 0)
        {
            mDecodeNoBitstreamAverageDuration = (mDecodeNoBitstreamTotalDuration)/(mDecodeNoBitstreamTotalCount);
        }
        else
        {
            mDecodeNoBitstreamAverageDuration = 0;
        }
        mConvertAverageDuration = mConvertTotalDuration/mConvertTotalCount;
        ALOGD("decode and convert statistics:\
            \n mDecodeFrameTotalDuration[%lld]ms, mDecodeOKTotalDuration[%lld]ms, mDecodeNoFrameTotalDuration[%lld]ms, mDecodeNoBitstreamTotalDuration[%lld]ms, mDecodeOtherTotalDuration[%lld]ms,\
            \n mDecodeFrameTotalCount[%lld], mDecodeOKTotalCount[%lld], mDecodeNoFrameTotalCount[%lld], mDecodeNoBitstreamTotalCount[%lld], mDecodeOtherTotalCount[%lld],\
            \n mDecodeFrameSmallAverageDuration[%lld]ms, mDecodeFrameBigAverageDuration[%lld]ms, mDecodeNoFrameAverageDuration[%lld]ms, mDecodeNoBitstreamAverageDuration[%lld]ms\
            \n mConvertTotalDuration[%lld]ms, mConvertTotalCount[%lld], mConvertAverageDuration[%lld]ms",
            mDecodeFrameTotalDuration/1000, mDecodeOKTotalDuration/1000, mDecodeNoFrameTotalDuration/1000, mDecodeNoBitstreamTotalDuration/1000, mDecodeOtherTotalDuration/1000,
            mDecodeFrameTotalCount, mDecodeOKTotalCount, mDecodeNoFrameTotalCount, mDecodeNoBitstreamTotalCount, mDecodeOtherTotalCount,
            mDecodeFrameSmallAverageDuration/1000, mDecodeFrameBigAverageDuration/1000, mDecodeNoFrameAverageDuration/1000, mDecodeNoBitstreamAverageDuration/1000,
            mConvertTotalDuration/1000, mConvertTotalCount, mConvertAverageDuration/1000);
    }
#endif
    ALOGD("~omx_dec done!");
}


OMX_ERRORTYPE omx_vdec::component_init(OMX_STRING name)
{
	OMX_ERRORTYPE eRet = OMX_ErrorNone;
    int           err;
    OMX_U32       nIndex;

    ALOGV("(f:%s, l:%d) name = %s", __FUNCTION__, __LINE__, name);
	strncpy((char*)m_cName, name, OMX_MAX_STRINGNAME_SIZE);
	if(!strncmp((char*)m_cName, "OMX.allwinner.video.decoder.mpeg4", OMX_MAX_STRINGNAME_SIZE))
	{
		strncpy((char*)m_cRole, "video_decoder.mpeg4", OMX_MAX_STRINGNAME_SIZE);
		m_eCompressionFormat = OMX_VIDEO_CodingMPEG4;
		m_streamInfo.format  = CEDARV_STREAM_FORMAT_MPEG4;
		m_streamInfo.sub_format = CEDARV_MPEG4_SUB_FORMAT_XVID;
	}
	else if(!strncmp((char*)m_cName, "OMX.allwinner.video.decoder.h263", OMX_MAX_STRINGNAME_SIZE))
	{
		strncpy((char *)m_cRole, "video_decoder.h263", OMX_MAX_STRINGNAME_SIZE);
		ALOGV("\n H263 Decoder selected");
		m_eCompressionFormat = OMX_VIDEO_CodingH263;
		m_streamInfo.format  = CEDARV_STREAM_FORMAT_MPEG4;	//* h263 decoder is integrated into mpeg4 driver.
		m_streamInfo.sub_format = CEDARV_MPEG4_SUB_FORMAT_H263;
	}
	else if(!strncmp((char*)m_cName, "OMX.allwinner.video.decoder.avc", OMX_MAX_STRINGNAME_SIZE))
	{
		strncpy((char *)m_cRole, "video_decoder.avc", OMX_MAX_STRINGNAME_SIZE);
		m_eCompressionFormat = OMX_VIDEO_CodingAVC;
		m_streamInfo.format  = CEDARV_STREAM_FORMAT_H264;
	}
	else if(!strncmp((char*)m_cName, "OMX.allwinner.video.decoder.vc1", OMX_MAX_STRINGNAME_SIZE))
	{
		strncpy((char *)m_cRole, "video_decoder.vc1", OMX_MAX_STRINGNAME_SIZE);
		m_eCompressionFormat = OMX_VIDEO_CodingWMV;
		m_streamInfo.format  = CEDARV_STREAM_FORMAT_VC1;
	}
    else if(!strncmp((char*)m_cName, "OMX.allwinner.video.decoder.mpeg2", OMX_MAX_STRINGNAME_SIZE))
	{
		strncpy((char *)m_cRole, "video_decoder.mpeg2", OMX_MAX_STRINGNAME_SIZE);
		m_eCompressionFormat = OMX_VIDEO_CodingMPEG2;
		m_streamInfo.format  = CEDARV_STREAM_FORMAT_MPEG2;
	}
    else if(!strncmp((char*)m_cName, "OMX.allwinner.video.decoder.vp8", OMX_MAX_STRINGNAME_SIZE))
	{
		strncpy((char *)m_cRole, "video_decoder.vp8", OMX_MAX_STRINGNAME_SIZE);
		m_eCompressionFormat = OMX_VIDEO_CodingVP8; //OMX_VIDEO_CodingVPX
		m_streamInfo.format  = CEDARV_STREAM_FORMAT_VP8;
	}
	else
	{
		ALOGV("\nERROR:Unknown Component\n");
	    eRet = OMX_ErrorInvalidComponentName;
	    return eRet;
	}

    // Initialize component data structures to default values
    OMX_CONF_INIT_STRUCT_PTR(&m_sPortParam, OMX_PORT_PARAM_TYPE);
    m_sPortParam.nPorts           = 0x2;
    m_sPortParam.nStartPortNumber = 0x0;

    // Initialize the video parameters for input port
    OMX_CONF_INIT_STRUCT_PTR(&m_sInPortDef, OMX_PARAM_PORTDEFINITIONTYPE);
    m_sInPortDef.nPortIndex 					 = 0x0;
    m_sInPortDef.bEnabled 						 = OMX_TRUE;
    m_sInPortDef.bPopulated 					 = OMX_FALSE;
    m_sInPortDef.eDomain 						 = OMX_PortDomainVideo;
    m_sInPortDef.format.video.nFrameWidth 		 = 0;
    m_sInPortDef.format.video.nFrameHeight 		 = 0;
    m_sInPortDef.eDir 							 = OMX_DirInput;
    m_sInPortDef.nBufferCountMin 				 = NUM_IN_BUFFERS;
    m_sInPortDef.nBufferCountActual 			 = NUM_IN_BUFFERS;
    m_sInPortDef.nBufferSize 					 = OMX_VIDEO_DEC_INPUT_BUFFER_SIZE;
    m_sInPortDef.format.video.eCompressionFormat = m_eCompressionFormat;
    m_sInPortDef.format.video.cMIMEType 		 = (OMX_STRING)"";

    // Initialize the video parameters for output port
    OMX_CONF_INIT_STRUCT_PTR(&m_sOutPortDef, OMX_PARAM_PORTDEFINITIONTYPE);
    m_sOutPortDef.nPortIndex 				= 0x1;
    m_sOutPortDef.bEnabled 					= OMX_TRUE;
    m_sOutPortDef.bPopulated 				= OMX_FALSE;
    m_sOutPortDef.eDomain 					= OMX_PortDomainVideo;
    m_sOutPortDef.format.video.cMIMEType 	= (OMX_STRING)"YUV420";
    m_sOutPortDef.format.video.nFrameWidth 	= 176;
    m_sOutPortDef.format.video.nFrameHeight = 144;
    m_sOutPortDef.eDir 						= OMX_DirOutput;
    m_sOutPortDef.nBufferCountMin 			= NUM_OUT_BUFFERS;
    m_sOutPortDef.nBufferCountActual 		= NUM_OUT_BUFFERS;
    m_sOutPortDef.nBufferSize 				= (OMX_U32)(m_sOutPortDef.format.video.nFrameWidth*m_sOutPortDef.format.video.nFrameHeight*3/2);
    m_sOutPortDef.format.video.eColorFormat = OMX_COLOR_FormatYUV420Planar;

    // Initialize the video compression format for input port
    OMX_CONF_INIT_STRUCT_PTR(&m_sInPortFormat, OMX_VIDEO_PARAM_PORTFORMATTYPE);
    m_sInPortFormat.nPortIndex         = 0x0;
    m_sInPortFormat.nIndex             = 0x0;
    m_sInPortFormat.eCompressionFormat = m_eCompressionFormat;

    // Initialize the compression format for output port
    OMX_CONF_INIT_STRUCT_PTR(&m_sOutPortFormat, OMX_VIDEO_PARAM_PORTFORMATTYPE);
    m_sOutPortFormat.nPortIndex   = 0x1;
    m_sOutPortFormat.nIndex       = 0x0;
    m_sOutPortFormat.eColorFormat = OMX_COLOR_FormatYUV420Planar;

    OMX_CONF_INIT_STRUCT_PTR(&m_sPriorityMgmt, OMX_PRIORITYMGMTTYPE);

    OMX_CONF_INIT_STRUCT_PTR(&m_sInBufSupplier, OMX_PARAM_BUFFERSUPPLIERTYPE );
    m_sInBufSupplier.nPortIndex = 0x0;

    OMX_CONF_INIT_STRUCT_PTR(&m_sOutBufSupplier, OMX_PARAM_BUFFERSUPPLIERTYPE );
    m_sOutBufSupplier.nPortIndex = 0x1;

    // Initialize the input buffer list
    memset(&(m_sInBufList), 0x0, sizeof(BufferList));

    m_sInBufList.pBufArr = (OMX_BUFFERHEADERTYPE*)malloc(sizeof(OMX_BUFFERHEADERTYPE) * m_sInPortDef.nBufferCountActual);
    if(m_sInBufList.pBufArr == NULL)
    {
    	eRet = OMX_ErrorInsufficientResources;
    	goto EXIT;
    }

    memset(m_sInBufList.pBufArr, 0, sizeof(sizeof(OMX_BUFFERHEADERTYPE) * m_sInPortDef.nBufferCountActual));
    for (nIndex = 0; nIndex < m_sInPortDef.nBufferCountActual; nIndex++)
    {
        OMX_CONF_INIT_STRUCT_PTR (&m_sInBufList.pBufArr[nIndex], OMX_BUFFERHEADERTYPE);
    }


    m_sInBufList.pBufHdrList = (OMX_BUFFERHEADERTYPE**)malloc(sizeof(OMX_BUFFERHEADERTYPE*) * m_sInPortDef.nBufferCountActual);
    if(m_sInBufList.pBufHdrList == NULL)
    {
    	eRet = OMX_ErrorInsufficientResources;
    	goto EXIT;
    }

    m_sInBufList.nSizeOfList       = 0;
    m_sInBufList.nAllocSize        = 0;
    m_sInBufList.nWritePos         = 0;
    m_sInBufList.nReadPos          = 0;
    m_sInBufList.nAllocBySelfFlags = 0;
    m_sInBufList.nSizeOfList       = 0;
    m_sInBufList.nBufArrSize       = m_sInPortDef.nBufferCountActual;
    m_sInBufList.eDir              = OMX_DirInput;

    // Initialize the output buffer list
    memset(&m_sOutBufList, 0x0, sizeof(BufferList));

    m_sOutBufList.pBufArr = (OMX_BUFFERHEADERTYPE*)malloc(sizeof(OMX_BUFFERHEADERTYPE) * m_sOutPortDef.nBufferCountActual);
    if(m_sOutBufList.pBufArr == NULL)
    {
    	eRet = OMX_ErrorInsufficientResources;
    	goto EXIT;
    }

    memset(m_sOutBufList.pBufArr, 0, sizeof(sizeof(OMX_BUFFERHEADERTYPE) * m_sOutPortDef.nBufferCountActual));
    for (nIndex = 0; nIndex < m_sOutPortDef.nBufferCountActual; nIndex++)
    {
        OMX_CONF_INIT_STRUCT_PTR(&m_sOutBufList.pBufArr[nIndex], OMX_BUFFERHEADERTYPE);
    }

    m_sOutBufList.pBufHdrList = (OMX_BUFFERHEADERTYPE**)malloc(sizeof(OMX_BUFFERHEADERTYPE*) * m_sOutPortDef.nBufferCountActual);
    if(m_sOutBufList.pBufHdrList == NULL)
    {
    	eRet = OMX_ErrorInsufficientResources;
    	goto EXIT;
    }

    m_sOutBufList.nSizeOfList       = 0;
    m_sOutBufList.nAllocSize        = 0;
    m_sOutBufList.nWritePos         = 0;
    m_sOutBufList.nReadPos          = 0;
    m_sOutBufList.nAllocBySelfFlags = 0;
    m_sOutBufList.nSizeOfList       = 0;
    m_sOutBufList.nBufArrSize       = m_sOutPortDef.nBufferCountActual;
    m_sOutBufList.eDir              = OMX_DirOutput;

    // Create the pipe used to send commands to the thread
    err = pipe(m_cmdpipe);
    if (err)
    {
    	eRet = OMX_ErrorInsufficientResources;
        goto EXIT;
    }
    // Create the pipe used to send command to the vdrv_thread
    err = pipe(m_vdrv_cmdpipe);
    if (err)
    {
    	eRet = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

	if (ve_mutex_init(&m_cedarv_req_ctx, CEDARV_DECODE) < 0) {
		ALOGE("cedarv_req_ctx init fail!!");
		eRet = OMX_ErrorInsufficientResources;
        goto EXIT;
	}

    // Create the pipe used to send command data to the thread
    err = pipe(m_cmddatapipe);
    if (err)
    {
    	eRet = OMX_ErrorInsufficientResources;
        goto EXIT;
    }
    //* create a decoder.
    m_decoder = libcedarv_init((s32*)&err);
    if(m_decoder == NULL)
    {
    	ALOGV(" can not create video decoder.");
    	eRet = OMX_ErrorInsufficientResources;
    	goto EXIT;
    }

    // Create the component thread
    err = pthread_create(&m_thread_id, NULL, ComponentThread, this);
    if( err || !m_thread_id )
    {
    	eRet = OMX_ErrorInsufficientResources;
        goto EXIT;
    }
    // Create vdrv thread
    err = pthread_create(&m_vdrv_thread_id, NULL, ComponentVdrvThread, this);
    if( err || !m_vdrv_thread_id )
    {
    	eRet = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    mDecodeFrameTotalDuration           = 0;
    mDecodeOKTotalDuration              = 0;
    mDecodeNoFrameTotalDuration         = 0;
    mDecodeNoBitstreamTotalDuration     = 0;
    mDecodeOtherTotalDuration           = 0;
    mDecodeFrameTotalCount              = 0;
    mDecodeOKTotalCount                 = 0;
    mDecodeNoFrameTotalCount            = 0;
    mDecodeNoBitstreamTotalCount        = 0;
    mDecodeOtherTotalCount              = 0;
    mDecodeFrameSmallAverageDuration    = 0;
    mDecodeFrameBigAverageDuration      = 0;
    mDecodeNoFrameAverageDuration       = 0;
    mDecodeNoBitstreamAverageDuration   = 0;

    mConvertTotalDuration               = 0;
    mConvertTotalCount                  = 0;
    mConvertAverageDuration             = 0;

EXIT:
    return eRet;
}


OMX_ERRORTYPE  omx_vdec::get_component_version(OMX_IN OMX_HANDLETYPE hComp,
                                               OMX_OUT OMX_STRING componentName,
                                               OMX_OUT OMX_VERSIONTYPE* componentVersion,
                                               OMX_OUT OMX_VERSIONTYPE* specVersion,
                                               OMX_OUT OMX_UUIDTYPE* componentUUID)
{
    ALOGV("(f:%s, l:%d) ", __FUNCTION__, __LINE__);
    if (!hComp || !componentName || !componentVersion || !specVersion)
    {
        return OMX_ErrorBadParameter;
    }

    strcpy((char*)componentName, (char*)m_cName);

    componentVersion->s.nVersionMajor = 1;
    componentVersion->s.nVersionMinor = 1;
    componentVersion->s.nRevision     = 0;
    componentVersion->s.nStep         = 0;

    specVersion->s.nVersionMajor = 1;
    specVersion->s.nVersionMinor = 1;
    specVersion->s.nRevision     = 0;
    specVersion->s.nStep         = 0;

	return OMX_ErrorNone;
}


OMX_ERRORTYPE  omx_vdec::send_command(OMX_IN OMX_HANDLETYPE  hComp,
                                      OMX_IN OMX_COMMANDTYPE cmd,
                                      OMX_IN OMX_U32         param1,
                                      OMX_IN OMX_PTR         cmdData)
{
    ThrCmdType    eCmd;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    ALOGV("(f:%s, l:%d) ", __FUNCTION__, __LINE__);

    if(m_state == OMX_StateInvalid)
    {
    	ALOGE("ERROR: Send Command in Invalid State\n");
        return OMX_ErrorInvalidState;
    }

    if (cmd == OMX_CommandMarkBuffer && cmdData == NULL)
    {
    	ALOGE("ERROR: Send OMX_CommandMarkBuffer command but pCmdData invalid.");
    	return OMX_ErrorBadParameter;
    }

    switch (cmd)
    {
        case OMX_CommandStateSet:
        	ALOGV(" COMPONENT_SEND_COMMAND: OMX_CommandStateSet");
            eCmd = SetState;
	        break;

        case OMX_CommandFlush:
        	ALOGV(" COMPONENT_SEND_COMMAND: OMX_CommandFlush");
	        eCmd = Flush;
	        if ((int)param1 > 1 && (int)param1 != -1)
	        {
	        	ALOGE("Error: Send OMX_CommandFlush command but param1 invalid.");
	        	return OMX_ErrorBadPortIndex;
	        }
	        break;

        case OMX_CommandPortDisable:
        	ALOGV(" COMPONENT_SEND_COMMAND: OMX_CommandPortDisable");
	        eCmd = StopPort;
	        break;

        case OMX_CommandPortEnable:
        	ALOGV(" COMPONENT_SEND_COMMAND: OMX_CommandPortEnable");
	        eCmd = RestartPort;
	        break;

        case OMX_CommandMarkBuffer:
        	ALOGV(" COMPONENT_SEND_COMMAND: OMX_CommandMarkBuffer");
	        eCmd = MarkBuf;
 	        if (param1 > 0)
	        {
	        	ALOGE("Error: Send OMX_CommandMarkBuffer command but param1 invalid.");
	        	return OMX_ErrorBadPortIndex;
	        }
            break;

        default:
            ALOGW("(f:%s, l:%d) ignore other command[0x%x]", __FUNCTION__, __LINE__, cmd);
            return OMX_ErrorBadParameter;
    }

    post_event(eCmd, param1, cmdData);

    return eError;
}


OMX_ERRORTYPE  omx_vdec::get_parameter(OMX_IN OMX_HANDLETYPE hComp,
                                       OMX_IN OMX_INDEXTYPE  paramIndex,
                                       OMX_INOUT OMX_PTR     paramData)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;

    ALOGV("(f:%s, l:%d) paramIndex = 0x%x", __FUNCTION__, __LINE__, paramIndex);
    if(m_state == OMX_StateInvalid)
    {
    	ALOGV("Get Param in Invalid State\n");
        return OMX_ErrorInvalidState;
    }

    if(paramData == NULL)
    {
    	ALOGV("Get Param in Invalid paramData \n");
        return OMX_ErrorBadParameter;
    }

    switch(paramIndex)
    {
    	case OMX_IndexParamVideoInit:
    	{
    		ALOGV(" COMPONENT_GET_PARAMETER: OMX_IndexParamVideoInit");
	        memcpy(paramData, &m_sPortParam, sizeof(OMX_PORT_PARAM_TYPE));
    		break;
    	}

    	case OMX_IndexParamPortDefinition:
    	{
    		ALOGV(" COMPONENT_GET_PARAMETER: OMX_IndexParamPortDefinition");
	        if (((OMX_PARAM_PORTDEFINITIONTYPE *)(paramData))->nPortIndex == m_sInPortDef.nPortIndex)
	        {
	            memcpy(paramData, &m_sInPortDef, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
                ALOGV(" get_OMX_IndexParamPortDefinition: m_sInPortDef.nPortIndex[%d]", (int)m_sInPortDef.nPortIndex);
	        }
	        else if (((OMX_PARAM_PORTDEFINITIONTYPE*)(paramData))->nPortIndex == m_sOutPortDef.nPortIndex)
	        {
	            memcpy(paramData, &m_sOutPortDef, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
	            ALOGV("(omx_vdec, f:%s, l:%d) OMX_IndexParamPortDefinition, width = %d, height = %d, nPortIndex[%d], nBufferCountActual[%d], nBufferCountMin[%d], nBufferSize[%d]", __FUNCTION__, __LINE__,
                    (int)m_sOutPortDef.format.video.nFrameWidth, (int)m_sOutPortDef.format.video.nFrameHeight,
                    m_sOutPortDef.nPortIndex, m_sOutPortDef.nBufferCountActual, m_sOutPortDef.nBufferCountMin, m_sOutPortDef.nBufferSize);
	        }
	        else
	        {
	            eError = OMX_ErrorBadPortIndex;
                ALOGW(" get_OMX_IndexParamPortDefinition: error. paramData->nPortIndex=[%d]", (int)((OMX_PARAM_PORTDEFINITIONTYPE*)(paramData))->nPortIndex);
	        }

    		break;
    	}

    	case OMX_IndexParamVideoPortFormat:
    	{
    		ALOGV(" COMPONENT_GET_PARAMETER: OMX_IndexParamVideoPortFormat");
 	        if (((OMX_VIDEO_PARAM_PORTFORMATTYPE *)(paramData))->nPortIndex == m_sInPortFormat.nPortIndex)
 	        {
 	            if (((OMX_VIDEO_PARAM_PORTFORMATTYPE*)(paramData))->nIndex > m_sInPortFormat.nIndex)
		            eError = OMX_ErrorNoMore;
		        else
		            memcpy(paramData, &m_sInPortFormat, sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE));
		    }
	        else if (((OMX_VIDEO_PARAM_PORTFORMATTYPE*)(paramData))->nPortIndex == m_sOutPortFormat.nPortIndex)
	        {
	            if (((OMX_VIDEO_PARAM_PORTFORMATTYPE*)(paramData))->nIndex > m_sOutPortFormat.nIndex)
		            eError = OMX_ErrorNoMore;
		        else
		            memcpy(paramData, &m_sOutPortFormat, sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE));
		    }
		    else
		        eError = OMX_ErrorBadPortIndex;

            ALOGV("OMX_IndexParamVideoPortFormat, eError[0x%x]", eError);
	        break;
    	}

    	case OMX_IndexParamStandardComponentRole:
    	{
    		ALOGV(" COMPONENT_GET_PARAMETER: OMX_IndexParamStandardComponentRole");
    		OMX_PARAM_COMPONENTROLETYPE* comp_role;

    		comp_role                    = (OMX_PARAM_COMPONENTROLETYPE *) paramData;
    		comp_role->nVersion.nVersion = OMX_SPEC_VERSION;
    		comp_role->nSize             = sizeof(*comp_role);

    		strncpy((char*)comp_role->cRole, (const char*)m_cRole, OMX_MAX_STRINGNAME_SIZE);
    		break;
    	}

    	case OMX_IndexParamPriorityMgmt:
    	{
    		ALOGV(" COMPONENT_GET_PARAMETER: OMX_IndexParamPriorityMgmt");
	        memcpy(paramData, &m_sPriorityMgmt, sizeof(OMX_PRIORITYMGMTTYPE));
    		break;
    	}

    	case OMX_IndexParamCompBufferSupplier:
    	{
    		ALOGV(" COMPONENT_GET_PARAMETER: OMX_IndexParamCompBufferSupplier");
            OMX_PARAM_BUFFERSUPPLIERTYPE* pBuffSupplierParam = (OMX_PARAM_BUFFERSUPPLIERTYPE*)paramData;

            if (pBuffSupplierParam->nPortIndex == 1)
            {
                pBuffSupplierParam->eBufferSupplier = m_sOutBufSupplier.eBufferSupplier;
            }
            else if (pBuffSupplierParam->nPortIndex == 0)
            {
                pBuffSupplierParam->eBufferSupplier = m_sInBufSupplier.eBufferSupplier;
            }
            else
            {
                eError = OMX_ErrorBadPortIndex;
            }

    		break;
    	}

    	case OMX_IndexParamAudioInit:
    	{
    		ALOGV(" COMPONENT_GET_PARAMETER: OMX_IndexParamAudioInit");
    		OMX_PORT_PARAM_TYPE *audioPortParamType = (OMX_PORT_PARAM_TYPE *) paramData;

    		audioPortParamType->nVersion.nVersion = OMX_SPEC_VERSION;
    		audioPortParamType->nSize             = sizeof(OMX_PORT_PARAM_TYPE);
    		audioPortParamType->nPorts            = 0;
    		audioPortParamType->nStartPortNumber  = 0;

    		break;
    	}

    	case OMX_IndexParamImageInit:
    	{
    		ALOGV(" COMPONENT_GET_PARAMETER: OMX_IndexParamImageInit");
    		OMX_PORT_PARAM_TYPE *imagePortParamType = (OMX_PORT_PARAM_TYPE *) paramData;

    		imagePortParamType->nVersion.nVersion = OMX_SPEC_VERSION;
    		imagePortParamType->nSize             = sizeof(OMX_PORT_PARAM_TYPE);
    		imagePortParamType->nPorts            = 0;
    		imagePortParamType->nStartPortNumber  = 0;

    		break;
    	}

    	case OMX_IndexParamOtherInit:
    	{
    		ALOGV(" COMPONENT_GET_PARAMETER: OMX_IndexParamOtherInit");
    		OMX_PORT_PARAM_TYPE *otherPortParamType = (OMX_PORT_PARAM_TYPE *) paramData;

    		otherPortParamType->nVersion.nVersion = OMX_SPEC_VERSION;
    		otherPortParamType->nSize             = sizeof(OMX_PORT_PARAM_TYPE);
    		otherPortParamType->nPorts            = 0;
    		otherPortParamType->nStartPortNumber  = 0;

    		break;
    	}

    	case OMX_IndexParamVideoAvc:
    	{
    		ALOGV(" COMPONENT_GET_PARAMETER: OMX_IndexParamVideoAvc");
        	ALOGV("get_parameter: OMX_IndexParamVideoAvc, do nothing.\n");
            break;
    	}

    	case OMX_IndexParamVideoH263:
    	{
    		ALOGV(" COMPONENT_GET_PARAMETER: OMX_IndexParamVideoH263");
        	ALOGV("get_parameter: OMX_IndexParamVideoH263, do nothing.\n");
            break;
    	}

    	case OMX_IndexParamVideoMpeg4:
    	{
    		ALOGV(" COMPONENT_GET_PARAMETER: OMX_IndexParamVideoMpeg4");
        	ALOGV("get_parameter: OMX_IndexParamVideoMpeg4, do nothing.\n");
            break;
    	}
        case OMX_IndexParamVideoProfileLevelQuerySupported:
        {
        	VIDEO_PROFILE_LEVEL_TYPE* pProfileLevel = NULL;
        	OMX_U32 nNumberOfProfiles = 0;
        	OMX_VIDEO_PARAM_PROFILELEVELTYPE *pParamProfileLevel = (OMX_VIDEO_PARAM_PROFILELEVELTYPE *)paramData;

        	pParamProfileLevel->nPortIndex = m_sInPortDef.nPortIndex;

        	/* Choose table based on compression format */
        	switch(m_sInPortDef.format.video.eCompressionFormat)
        	{
        	case OMX_VIDEO_CodingH263:
        		pProfileLevel = SupportedH263ProfileLevels;
        		nNumberOfProfiles = sizeof(SupportedH263ProfileLevels) / sizeof (VIDEO_PROFILE_LEVEL_TYPE);
        		break;

        	case OMX_VIDEO_CodingMPEG4:
        		pProfileLevel = SupportedMPEG4ProfileLevels;
        		nNumberOfProfiles = sizeof(SupportedMPEG4ProfileLevels) / sizeof (VIDEO_PROFILE_LEVEL_TYPE);
        		break;

        	case OMX_VIDEO_CodingAVC:
        		pProfileLevel = SupportedAVCProfileLevels;
        		nNumberOfProfiles = sizeof(SupportedAVCProfileLevels) / sizeof (VIDEO_PROFILE_LEVEL_TYPE);
        		break;

        	default:
                ALOGW("OMX_IndexParamVideoProfileLevelQuerySupported, Format[0x%x] not support", m_sInPortDef.format.video.eCompressionFormat);
        		return OMX_ErrorBadParameter;
        	}

        	if(((int)pParamProfileLevel->nProfileIndex < 0) || (pParamProfileLevel->nProfileIndex >= (nNumberOfProfiles - 1)))
        	{
                ALOGW("pParamProfileLevel->nProfileIndex[0x%x] error!", (unsigned int)pParamProfileLevel->nProfileIndex);
        		return OMX_ErrorBadParameter;
        	}

        	/* Point to table entry based on index */
        	pProfileLevel += pParamProfileLevel->nProfileIndex;

        	/* -1 indicates end of table */
        	if(pProfileLevel->nProfile != -1)
        	{
        		pParamProfileLevel->eProfile = pProfileLevel->nProfile;
        		pParamProfileLevel->eLevel = pProfileLevel->nLevel;
        		eError = OMX_ErrorNone;
        	}
        	else
        	{
                ALOGW("pProfileLevel->nProfile error!");
        		eError = OMX_ErrorNoMore;
        	}

        	break;
        }
    	default:
    	{
    		if((AW_VIDEO_EXTENSIONS_INDEXTYPE)paramIndex == AWOMX_IndexParamVideoGetAndroidNativeBufferUsage)
    		{
        		ALOGV(" COMPONENT_GET_PARAMETER: AWOMX_IndexParamVideoGetAndroidNativeBufferUsage");
                break;
    		}
    		else
    		{
        		ALOGV("get_parameter: unknown param %08x\n", paramIndex);
        		eError =OMX_ErrorUnsupportedIndex;
        		break;
    		}
    	}
    }

    return eError;
}



OMX_ERRORTYPE  omx_vdec::set_parameter(OMX_IN OMX_HANDLETYPE hComp, OMX_IN OMX_INDEXTYPE paramIndex,  OMX_IN OMX_PTR paramData)
{
    OMX_ERRORTYPE         eError      = OMX_ErrorNone;
    unsigned int          alignment   = 0;
    unsigned int          buffer_size = 0;
    int                   i;

    ALOGV("(f:%s, l:%d) paramIndex = 0x%x", __FUNCTION__, __LINE__, paramIndex);
    if(m_state == OMX_StateInvalid)
    {
        ALOGV("Set Param in Invalid State\n");
        return OMX_ErrorInvalidState;
    }

    if(paramData == NULL)
    {
    	ALOGV("Get Param in Invalid paramData \n");
    	return OMX_ErrorBadParameter;
    }

    switch(paramIndex)
    {
	    case OMX_IndexParamPortDefinition:
	    {
	    	ALOGV(" COMPONENT_SET_PARAMETER: OMX_IndexParamPortDefinition");
            if (((OMX_PARAM_PORTDEFINITIONTYPE *)(paramData))->nPortIndex == m_sInPortDef.nPortIndex)
            {
                ALOGV("set_OMX_IndexParamPortDefinition, m_sInPortDef.nPortIndex=%d", (int)m_sInPortDef.nPortIndex);
	        	if(((OMX_PARAM_PORTDEFINITIONTYPE *)(paramData))->nBufferCountActual != m_sInPortDef.nBufferCountActual)
	        	{
	        		int nBufCnt;
	        		int nIndex;


	        		pthread_mutex_lock(&m_inBufMutex);

	        		if(m_sInBufList.pBufArr != NULL)
	        			free(m_sInBufList.pBufArr);

	        		if(m_sInBufList.pBufHdrList != NULL)
	        			free(m_sInBufList.pBufHdrList);

	        		nBufCnt = ((OMX_PARAM_PORTDEFINITIONTYPE *)(paramData))->nBufferCountActual;
	        		ALOGV("x allocate %d buffers.", nBufCnt);

	        	    m_sInBufList.pBufArr = (OMX_BUFFERHEADERTYPE*)malloc(sizeof(OMX_BUFFERHEADERTYPE)* nBufCnt);
	        	    m_sInBufList.pBufHdrList = (OMX_BUFFERHEADERTYPE**)malloc(sizeof(OMX_BUFFERHEADERTYPE*)* nBufCnt);
	        	    for (nIndex = 0; nIndex < nBufCnt; nIndex++)
	        	    {
	        	        OMX_CONF_INIT_STRUCT_PTR (&m_sInBufList.pBufArr[nIndex], OMX_BUFFERHEADERTYPE);
	        	    }

	        	    m_sInBufList.nSizeOfList       = 0;
	        	    m_sInBufList.nAllocSize        = 0;
	        	    m_sInBufList.nWritePos         = 0;
	        	    m_sInBufList.nReadPos          = 0;
	        	    m_sInBufList.nAllocBySelfFlags = 0;
	        	    m_sInBufList.nSizeOfList       = 0;
	        	    m_sInBufList.nBufArrSize       = nBufCnt;
	        	    m_sInBufList.eDir              = OMX_DirInput;

	        		pthread_mutex_unlock(&m_inBufMutex);
	        	}

	            memcpy(&m_sInPortDef, paramData, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));

	            m_streamInfo.video_width  = m_sInPortDef.format.video.nFrameWidth;
	            m_streamInfo.video_height = m_sInPortDef.format.video.nFrameHeight;
            }
	        else if (((OMX_PARAM_PORTDEFINITIONTYPE *)(paramData))->nPortIndex == m_sOutPortDef.nPortIndex)
	        {
                ALOGV("set_OMX_IndexParamPortDefinition, m_sOutPortDef.nPortIndex=%d", (int)m_sOutPortDef.nPortIndex);
	        	if(((OMX_PARAM_PORTDEFINITIONTYPE *)(paramData))->nBufferCountActual != m_sOutPortDef.nBufferCountActual)
	        	{
	        		int nBufCnt;
	        		int nIndex;

	        		pthread_mutex_lock(&m_outBufMutex);

	        		if(m_sOutBufList.pBufArr != NULL)
	        			free(m_sOutBufList.pBufArr);

	        		if(m_sOutBufList.pBufHdrList != NULL)
	        			free(m_sOutBufList.pBufHdrList);

	        		nBufCnt = ((OMX_PARAM_PORTDEFINITIONTYPE *)(paramData))->nBufferCountActual;
	        		ALOGV("x allocate %d buffers.", nBufCnt);
	        	    // Initialize the output buffer list
	        	    m_sOutBufList.pBufArr = (OMX_BUFFERHEADERTYPE*) malloc(sizeof(OMX_BUFFERHEADERTYPE) * nBufCnt);
	        	    m_sOutBufList.pBufHdrList = (OMX_BUFFERHEADERTYPE**) malloc(sizeof(OMX_BUFFERHEADERTYPE*) * nBufCnt);
	        	    for (nIndex = 0; nIndex < nBufCnt; nIndex++)
	        	    {
	        	        OMX_CONF_INIT_STRUCT_PTR (&m_sOutBufList.pBufArr[nIndex], OMX_BUFFERHEADERTYPE);
	        	    }

	        	    m_sOutBufList.nSizeOfList       = 0;
	        	    m_sOutBufList.nAllocSize        = 0;
	        	    m_sOutBufList.nWritePos         = 0;
	        	    m_sOutBufList.nReadPos          = 0;
	        	    m_sOutBufList.nAllocBySelfFlags = 0;
	        	    m_sOutBufList.nSizeOfList       = 0;
	        	    m_sOutBufList.nBufArrSize       = nBufCnt;
	        	    m_sOutBufList.eDir              = OMX_DirOutput;

	        		pthread_mutex_unlock(&m_outBufMutex);
	        	}

	            memcpy(&m_sOutPortDef, paramData, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
//	            ALOGV(" width = %d, height = %d", m_sOutPortDef.format.video.nFrameWidth, m_sOutPortDef.format.video.nFrameHeight);
//	            m_sOutPortDef.nBufferSize = m_sOutPortDef.format.video.nFrameWidth * m_sOutPortDef.format.video.nFrameHeight * 3 / 2;
//	            m_sOutPortDef.format.video.nSliceHeight = m_sOutPortDef.format.video.nFrameHeight;
//	            m_sOutPortDef.format.video.nStride = m_sOutPortDef.format.video.nFrameWidth;
                //check some key parameter
            //#if (defined(__CHIP_VERSION_F23) || defined(__CHIP_VERSION_F51) || defined(__CHIP_VERSION_F50))
                int nGpuBufWidth;
                int nGpuBufHeight;
                nGpuBufWidth = (m_sOutPortDef.format.video.nFrameWidth + 15) & ~15;
                nGpuBufHeight = m_sOutPortDef.format.video.nFrameHeight;
                if(nGpuBufHeight%2!=0)
                {
                    ALOGW("fatal error! why frame height[%d] is odd number! add one!", nGpuBufHeight);
                    nGpuBufHeight+=1;
                }
            #if (1 == ADAPT_A10_GPU_RENDER)
                //A10's GPU has a bug, we can avoid it
                if(nGpuBufHeight%8 != 0)
                {
                    ALOGD("(f:%s, l:%d) original gpu buf align_width[%d], height[%d]mod8 = %d", __FUNCTION__, __LINE__, nGpuBufWidth, nGpuBufHeight, nGpuBufHeight%8);
                    if((nGpuBufWidth*nGpuBufHeight)%256 != 0)
                    {
                        ALOGW("(f:%s, l:%d) original gpu buf align_width[%d]*height[%d]mod256 = %d", __FUNCTION__, __LINE__, nGpuBufWidth, nGpuBufHeight, (nGpuBufWidth*nGpuBufHeight)%256);
                        nGpuBufHeight = (nGpuBufHeight+7)&~7;
                        ALOGW("(f:%s, l:%d) change gpu buf height to [%d]", __FUNCTION__, __LINE__, nGpuBufHeight);
                    }
                }
            #endif
                m_sOutPortDef.format.video.nFrameHeight = nGpuBufHeight;
            //#endif
                if(m_sOutPortDef.format.video.nFrameWidth * m_sOutPortDef.format.video.nFrameHeight * 3 / 2 != m_sOutPortDef.nBufferSize)
                {
					ALOGW("set_parameter, OMX_IndexParamPortDefinition, OutPortDef : change nBufferSize[%d] to [%d] to suit frame width[%d] and height[%d]",
                        (int)m_sOutPortDef.nBufferSize,
                        (int)(m_sOutPortDef.format.video.nFrameWidth * m_sOutPortDef.format.video.nFrameHeight * 3 / 2),
                        (int)m_sOutPortDef.format.video.nFrameWidth,
                        (int)m_sOutPortDef.format.video.nFrameHeight);
                    m_sOutPortDef.nBufferSize = m_sOutPortDef.format.video.nFrameWidth * m_sOutPortDef.format.video.nFrameHeight * 3 / 2;
                }

                ALOGV("(omx_vdec, f:%s, l:%d) OMX_IndexParamPortDefinition, width = %d, height = %d, nPortIndex[%d], nBufferCountActual[%d], nBufferCountMin[%d], nBufferSize[%d]", __FUNCTION__, __LINE__,
                    (int)m_sOutPortDef.format.video.nFrameWidth, (int)m_sOutPortDef.format.video.nFrameHeight,
                    m_sOutPortDef.nPortIndex, m_sOutPortDef.nBufferCountActual, m_sOutPortDef.nBufferCountMin, m_sOutPortDef.nBufferSize);
	        }
	        else
	        {
                ALOGW("set_OMX_IndexParamPortDefinition, error, paramPortIndex=%d", (int)((OMX_PARAM_PORTDEFINITIONTYPE *)(paramData))->nPortIndex);
	            eError = OMX_ErrorBadPortIndex;
	        }

	       break;
	    }

	    case OMX_IndexParamVideoPortFormat:
	    {
	    	ALOGV(" COMPONENT_SET_PARAMETER: OMX_IndexParamVideoPortFormat");
#if 0
	        OMX_VIDEO_PARAM_PORTFORMATTYPE *portFmt = (OMX_VIDEO_PARAM_PORTFORMATTYPE *)paramData;
	        if(1 == portFmt->nPortIndex)
	        	m_color_format = portFmt->eColorFormat;
#else

	        if (((OMX_VIDEO_PARAM_PORTFORMATTYPE *)(paramData))->nPortIndex == m_sInPortFormat.nPortIndex)
	        {
	            if (((OMX_VIDEO_PARAM_PORTFORMATTYPE *)(paramData))->nIndex > m_sInPortFormat.nIndex)
		            eError = OMX_ErrorNoMore;
		        else
		            memcpy(&m_sInPortFormat, paramData, sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE));
	        }
	        else if (((OMX_VIDEO_PARAM_PORTFORMATTYPE*)(paramData))->nPortIndex == m_sOutPortFormat.nPortIndex)
	        {
	            if (((OMX_VIDEO_PARAM_PORTFORMATTYPE*)(paramData))->nIndex > m_sOutPortFormat.nIndex)
		            eError = OMX_ErrorNoMore;
		        else
		            memcpy(&m_sOutPortFormat, paramData, sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE));
	        }
	        else
	            eError = OMX_ErrorBadPortIndex;
#endif
	        break;
	    }

	    case OMX_IndexParamStandardComponentRole:
	    {
	    	ALOGV(" COMPONENT_SET_PARAMETER: OMX_IndexParamStandardComponentRole");
    		OMX_PARAM_COMPONENTROLETYPE *comp_role;
    		comp_role = (OMX_PARAM_COMPONENTROLETYPE *) paramData;
    		ALOGV("set_parameter: OMX_IndexParamStandardComponentRole %s\n", comp_role->cRole);

    		if((m_state == OMX_StateLoaded)/* && !BITMASK_PRESENT(&m_flags,OMX_COMPONENT_IDLE_PENDING)*/)
    		{
    			ALOGV("Set Parameter called in valid state");
    		}
    		else
    		{
    			ALOGV("Set Parameter called in Invalid State\n");
    			return OMX_ErrorIncorrectStateOperation;
    		}

    		if(!strncmp((char*)m_cName, "OMX.allwinner.video.decoder.avc", OMX_MAX_STRINGNAME_SIZE))
    		{
    			if(!strncmp((char*)comp_role->cRole, "video_decoder.avc", OMX_MAX_STRINGNAME_SIZE))
    			{
    				strncpy((char*)m_cRole,"video_decoder.avc", OMX_MAX_STRINGNAME_SIZE);
    			}
    			else
    			{
                  	ALOGV("Setparameter: unknown Index %s\n", comp_role->cRole);
                  	eError =OMX_ErrorUnsupportedSetting;
    			}
    		}
    		else if(!strncmp((char*)m_cName, "OMX.allwinner.video.decoder.mpeg4", OMX_MAX_STRINGNAME_SIZE))
    		{
    			if(!strncmp((const char*)comp_role->cRole,"video_decoder.mpeg4",OMX_MAX_STRINGNAME_SIZE))
    			{
    				strncpy((char*)m_cRole,"video_decoder.mpeg4",OMX_MAX_STRINGNAME_SIZE);
    			}
    			else
    			{
    				ALOGV("Setparameter: unknown Index %s\n", comp_role->cRole);
    				eError = OMX_ErrorUnsupportedSetting;
    			}
    		}
    		else if(!strncmp((char*)m_cName, "OMX.allwinner.video.decoder.h263", OMX_MAX_STRINGNAME_SIZE))
    		{
    			if(!strncmp((const char*)comp_role->cRole,"video_decoder.h263", OMX_MAX_STRINGNAME_SIZE))
    			{
    				strncpy((char*)m_cRole,"video_decoder.h263", OMX_MAX_STRINGNAME_SIZE);
    			}
    			else
    			{
    				ALOGV("Setparameter: unknown Index %s\n", comp_role->cRole);
    				eError =OMX_ErrorUnsupportedSetting;
    			}
    		}
    		else if(!strncmp((char*)m_cName, "OMX.allwinner.video.decoder.vc1", OMX_MAX_STRINGNAME_SIZE))
    		{
    			if(!strncmp((const char*)comp_role->cRole,"video_decoder.vc1",OMX_MAX_STRINGNAME_SIZE))
    			{
    				strncpy((char*)m_cRole,"video_decoder.vc1",OMX_MAX_STRINGNAME_SIZE);
    			}
    			else
    			{
    				ALOGV("Setparameter: unknown Index %s\n", comp_role->cRole);
    				eError =OMX_ErrorUnsupportedSetting;
    			}
    		}
            else if(!strncmp((char*)m_cName, "OMX.allwinner.video.decoder.mpeg2", OMX_MAX_STRINGNAME_SIZE))
    		{
    			if(!strncmp((char*)comp_role->cRole, "video_decoder.mpeg2", OMX_MAX_STRINGNAME_SIZE))
    			{
    				strncpy((char*)m_cRole,"video_decoder.mpeg2", OMX_MAX_STRINGNAME_SIZE);
    			}
    			else
    			{
                  	ALOGV("Setparameter: unknown Index %s\n", comp_role->cRole);
                  	eError =OMX_ErrorUnsupportedSetting;
    			}
    		}
            else if(!strncmp((char*)m_cName, "OMX.allwinner.video.decoder.vp8", OMX_MAX_STRINGNAME_SIZE))
    		{
    			if(!strncmp((char*)comp_role->cRole, "video_decoder.vp8", OMX_MAX_STRINGNAME_SIZE))
    			{
    				strncpy((char*)m_cRole,"video_decoder.vp8", OMX_MAX_STRINGNAME_SIZE);
    			}
    			else
    			{
                  	ALOGV("Setparameter: unknown Index %s\n", comp_role->cRole);
                  	eError =OMX_ErrorUnsupportedSetting;
    			}
    		}
    		else
    		{
    			ALOGV("Setparameter: unknown param %s\n", m_cName);
    			eError = OMX_ErrorInvalidComponentName;
    		}

    		break;
	    }

	    case OMX_IndexParamPriorityMgmt:
	    {
	    	ALOGV(" COMPONENT_SET_PARAMETER: OMX_IndexParamPriorityMgmt");
            if(m_state != OMX_StateLoaded)
            {
            	ALOGV("Set Parameter called in Invalid State\n");
            	return OMX_ErrorIncorrectStateOperation;
            }

            OMX_PRIORITYMGMTTYPE *priorityMgmtype = (OMX_PRIORITYMGMTTYPE*) paramData;

            m_sPriorityMgmt.nGroupID = priorityMgmtype->nGroupID;
            m_sPriorityMgmt.nGroupPriority = priorityMgmtype->nGroupPriority;

            break;
	    }

	    case OMX_IndexParamCompBufferSupplier:
	    {
	    	ALOGV(" COMPONENT_SET_PARAMETER: OMX_IndexParamCompBufferSupplier");
    		OMX_PARAM_BUFFERSUPPLIERTYPE *bufferSupplierType = (OMX_PARAM_BUFFERSUPPLIERTYPE*) paramData;

            ALOGV("set_parameter: OMX_IndexParamCompBufferSupplier %d\n", bufferSupplierType->eBufferSupplier);
            if(bufferSupplierType->nPortIndex == 0)
            	m_sInBufSupplier.eBufferSupplier = bufferSupplierType->eBufferSupplier;
            else if(bufferSupplierType->nPortIndex == 1)
            	m_sOutBufSupplier.eBufferSupplier = bufferSupplierType->eBufferSupplier;
        	else
        		eError = OMX_ErrorBadPortIndex;

        	break;
	    }

	    case OMX_IndexParamVideoAvc:
	    {
	    	ALOGV(" COMPONENT_SET_PARAMETER: OMX_IndexParamVideoAvc");
      		ALOGV("set_parameter: OMX_IndexParamVideoAvc, do nothing.\n");
	    	break;
	    }

	    case OMX_IndexParamVideoH263:
	    {
	    	ALOGV(" COMPONENT_SET_PARAMETER: OMX_IndexParamVideoH263");
     		ALOGV("set_parameter: OMX_IndexParamVideoH263, do nothing.\n");
	    	break;
	    }

	    case OMX_IndexParamVideoMpeg4:
	    {
	    	ALOGV(" COMPONENT_SET_PARAMETER: OMX_IndexParamVideoMpeg4");
     		ALOGV("set_parameter: OMX_IndexParamVideoMpeg4, do nothing.\n");
	    	break;
	    }

	    default:
	    {
	    	if((AW_VIDEO_EXTENSIONS_INDEXTYPE)paramIndex == AWOMX_IndexParamVideoUseAndroidNativeBuffer2)
	    	{
		    	ALOGV(" COMPONENT_SET_PARAMETER: AWOMX_IndexParamVideoUseAndroidNativeBuffer2");
	     		ALOGV("set_parameter: AWOMX_IndexParamVideoUseAndroidNativeBuffer2, do nothing.\n");
	     		m_useAndroidBuffer = OMX_TRUE;
		    	break;
	    	}
	    	else if((AW_VIDEO_EXTENSIONS_INDEXTYPE)paramIndex == AWOMX_IndexParamVideoEnableAndroidNativeBuffers)
	    	{
		    	ALOGV(" COMPONENT_SET_PARAMETER: AWOMX_IndexParamVideoEnableAndroidNativeBuffers");
	     		ALOGV("set_parameter: AWOMX_IndexParamVideoEnableAndroidNativeBuffers, set m_useAndroidBuffer to OMX_TRUE\n");
                EnableAndroidNativeBuffersParams *EnableAndroidBufferParams =  (EnableAndroidNativeBuffersParams*) paramData;
                ALOGV(" enbleParam = %d\n",EnableAndroidBufferParams->enable);
                if(1==EnableAndroidBufferParams->enable)
                {
                    m_useAndroidBuffer = OMX_TRUE;
                }
		    	break;
	    	}
	    	else
	    	{
	    		ALOGV("Setparameter: unknown param %d\n", paramIndex);
	    		eError = OMX_ErrorUnsupportedIndex;
	    		break;
	    	}
	    }
    }

    return eError;
}



OMX_ERRORTYPE  omx_vdec::get_config(OMX_IN OMX_HANDLETYPE hComp, OMX_IN OMX_INDEXTYPE configIndex, OMX_INOUT OMX_PTR configData)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;

    ALOGV("(f:%s, l:%d) index = %d", __FUNCTION__, __LINE__, configIndex);
	if (m_state == OMX_StateInvalid)
	{
		ALOGV("get_config in Invalid State\n");
		return OMX_ErrorInvalidState;
	}

	switch (configIndex)
	{
    	default:
    	{
    		ALOGV("get_config: unknown param %d\n",configIndex);
    		eError = OMX_ErrorUnsupportedIndex;
    	}
	}

	return eError;
}



OMX_ERRORTYPE omx_vdec::set_config(OMX_IN OMX_HANDLETYPE hComp, OMX_IN OMX_INDEXTYPE configIndex, OMX_IN OMX_PTR configData)
{

	//ALOGV(" COMPONENT_SET_CONFIG: index = %d", configIndex);
	ALOGV("(f:%s, l:%d) index = %d", __FUNCTION__, __LINE__, configIndex);

	if(m_state == OMX_StateInvalid)
	{
		ALOGV("set_config in Invalid State\n");
		return OMX_ErrorInvalidState;
	}

	OMX_ERRORTYPE eError = OMX_ErrorNone;

	if (m_state == OMX_StateExecuting)
	{
		ALOGV("set_config: Ignore in Executing state\n");
		return eError;
	}

	switch(configIndex)
	{
		default:
		{
			eError = OMX_ErrorUnsupportedIndex;
		}
	}

	return eError;
}


OMX_ERRORTYPE  omx_vdec::get_extension_index(OMX_IN OMX_HANDLETYPE hComp, OMX_IN OMX_STRING paramName, OMX_OUT OMX_INDEXTYPE* indexType)
{
	unsigned int  nIndex;
    OMX_ERRORTYPE eError = OMX_ErrorUndefined;

	//ALOGV(" COMPONENT_GET_EXTENSION_INDEX: param name = %s", paramName);
    ALOGV("(f:%s, l:%d) param name = %s", __FUNCTION__, __LINE__, paramName);
    if(m_state == OMX_StateInvalid)
    {
    	ALOGV("Get Extension Index in Invalid State\n");
        return OMX_ErrorInvalidState;
    }

    if(hComp == NULL)
    	return OMX_ErrorBadParameter;

    for(nIndex = 0; nIndex < sizeof(sVideoDecCustomParams)/sizeof(VIDDEC_CUSTOM_PARAM); nIndex++)
    {
        if(strcmp((char *)paramName, (char *)&(sVideoDecCustomParams[nIndex].cCustomParamName)) == 0)
        {
            *indexType = sVideoDecCustomParams[nIndex].nCustomParamIndex;
            eError = OMX_ErrorNone;
            break;
        }
    }

    return eError;
}



OMX_ERRORTYPE omx_vdec::get_state(OMX_IN OMX_HANDLETYPE hComp, OMX_OUT OMX_STATETYPE* state)
{
	//ALOGV(" COMPONENT_GET_STATE");
	ALOGV("(f:%s, l:%d) ", __FUNCTION__, __LINE__);

	if(hComp == NULL || state == NULL)
		return OMX_ErrorBadParameter;

	*state = m_state;
    ALOGV("COMPONENT_GET_STATE, state[0x%x]", m_state);
	return OMX_ErrorNone;
}


OMX_ERRORTYPE omx_vdec::component_tunnel_request(OMX_IN    OMX_HANDLETYPE       hComp,
                                                 OMX_IN    OMX_U32              port,
                                                 OMX_IN    OMX_HANDLETYPE       peerComponent,
                                                 OMX_IN    OMX_U32              peerPort,
                                                 OMX_INOUT OMX_TUNNELSETUPTYPE* tunnelSetup)
{
	ALOGV(" COMPONENT_TUNNEL_REQUEST");

	ALOGW("Error: component_tunnel_request Not Implemented\n");
	return OMX_ErrorNotImplemented;
}



OMX_ERRORTYPE  omx_vdec::use_buffer(OMX_IN    OMX_HANDLETYPE          hComponent,
                  			        OMX_INOUT OMX_BUFFERHEADERTYPE**  ppBufferHdr,
                  			        OMX_IN    OMX_U32                 nPortIndex,
                  			        OMX_IN    OMX_PTR                 pAppPrivate,
                  			        OMX_IN    OMX_U32                 nSizeBytes,
                  			        OMX_IN    OMX_U8*                 pBuffer)
{
    OMX_PARAM_PORTDEFINITIONTYPE*   pPortDef;
    OMX_U32                         nIndex = 0x0;

	//ALOGV(" COMPONENT_USE_BUFFER");
    ALOGV("(f:%s, l:%d) PortIndex[%d], nSizeBytes[%d], pBuffer[%p]", __FUNCTION__, __LINE__, (int)nPortIndex, (int)nSizeBytes, pBuffer);
	if(hComponent == NULL || ppBufferHdr == NULL || pBuffer == NULL)
	{
		return OMX_ErrorBadParameter;
	}

    if (nPortIndex == m_sInPortDef.nPortIndex)
        pPortDef = &m_sInPortDef;
    else if (nPortIndex == m_sOutPortDef.nPortIndex)
        pPortDef = &m_sOutPortDef;
    else
    	return OMX_ErrorBadParameter;

//    if (!pPortDef->bEnabled)
//    {
//        ALOGW("PortIndex[%d] is disable! Can't use_buffer!", (int)nPortIndex);
//    	return OMX_ErrorIncorrectStateOperation;
//    }
    if (m_state!=OMX_StateLoaded && m_state!=OMX_StateWaitForResources && pPortDef->bEnabled!=OMX_FALSE)
    {
        ALOGW("pPortDef[%d]->bEnabled=%d, m_state=0x%x, Can't use_buffer!", (int)nPortIndex, pPortDef->bEnabled, m_state);
    	return OMX_ErrorIncorrectStateOperation;
    }
    ALOGV("pPortDef[%d]->bEnabled=%d, m_state=0x%x, can use_buffer.", (int)nPortIndex, pPortDef->bEnabled, m_state);

    if (nSizeBytes != pPortDef->nBufferSize || pPortDef->bPopulated)
    	return OMX_ErrorBadParameter;

    // Find an empty position in the BufferList and allocate memory for the buffer header.
    // Use the buffer passed by the client to initialize the actual buffer
    // inside the buffer header.
    if (nPortIndex == m_sInPortDef.nPortIndex)
    {
        ALOGV("use_buffer, m_sInPortDef.nPortIndex=[%d]", (int)m_sInPortDef.nPortIndex);
    	pthread_mutex_lock(&m_inBufMutex);

    	if((OMX_S32)m_sInBufList.nAllocSize >= m_sInBufList.nBufArrSize)
        {
        	pthread_mutex_unlock(&m_inBufMutex);
        	return OMX_ErrorInsufficientResources;
        }

    	nIndex = m_sInBufList.nAllocSize;
    	m_sInBufList.nAllocSize++;

    	m_sInBufList.pBufArr[nIndex].pBuffer = pBuffer;
    	m_sInBufList.pBufArr[nIndex].nAllocLen   = nSizeBytes;
    	m_sInBufList.pBufArr[nIndex].pAppPrivate = pAppPrivate;
    	m_sInBufList.pBufArr[nIndex].nInputPortIndex = nPortIndex;
    	m_sInBufList.pBufArr[nIndex].nOutputPortIndex = 0xFFFFFFFE;
        *ppBufferHdr = &m_sInBufList.pBufArr[nIndex];
        if (m_sInBufList.nAllocSize == pPortDef->nBufferCountActual)
        	pPortDef->bPopulated = OMX_TRUE;

    	pthread_mutex_unlock(&m_inBufMutex);
    }
    else
    {
        ALOGV("use_buffer, m_sOutPortDef.nPortIndex=[%d]", (int)m_sOutPortDef.nPortIndex);
    	pthread_mutex_lock(&m_outBufMutex);

    	if((OMX_S32)m_sOutBufList.nAllocSize >= m_sOutBufList.nBufArrSize)
        {
        	pthread_mutex_unlock(&m_outBufMutex);
        	return OMX_ErrorInsufficientResources;
        }

    	nIndex = m_sOutBufList.nAllocSize;
    	m_sOutBufList.nAllocSize++;

    	m_sOutBufList.pBufArr[nIndex].pBuffer = pBuffer;
    	m_sOutBufList.pBufArr[nIndex].nAllocLen   = nSizeBytes;
    	m_sOutBufList.pBufArr[nIndex].pAppPrivate = pAppPrivate;
    	m_sOutBufList.pBufArr[nIndex].nInputPortIndex = 0xFFFFFFFE;
    	m_sOutBufList.pBufArr[nIndex].nOutputPortIndex = nPortIndex;
        *ppBufferHdr = &m_sOutBufList.pBufArr[nIndex];
        if (m_sOutBufList.nAllocSize == pPortDef->nBufferCountActual)
        	pPortDef->bPopulated = OMX_TRUE;

    	pthread_mutex_unlock(&m_outBufMutex);
    }

    return OMX_ErrorNone;
}


OMX_ERRORTYPE omx_vdec::allocate_buffer(OMX_IN    OMX_HANDLETYPE         hComponent,
        								OMX_INOUT OMX_BUFFERHEADERTYPE** ppBufferHdr,
        								OMX_IN    OMX_U32                nPortIndex,
        								OMX_IN    OMX_PTR                pAppPrivate,
        								OMX_IN    OMX_U32                nSizeBytes)
{
	OMX_S8                        nIndex = 0x0;
	OMX_PARAM_PORTDEFINITIONTYPE* pPortDef;

	//ALOGV(" COMPONENT_ALLOCATE_BUFFER");
    ALOGV("(f:%s, l:%d) nPortIndex[%d], nSizeBytes[%d]", __FUNCTION__, __LINE__, (int)nPortIndex, (int)nSizeBytes);
	if(hComponent == NULL || ppBufferHdr == NULL)
		return OMX_ErrorBadParameter;

    if (nPortIndex == m_sInPortDef.nPortIndex)
        pPortDef = &m_sInPortDef;
    else
    {
        if (nPortIndex == m_sOutPortDef.nPortIndex)
	        pPortDef = &m_sOutPortDef;
        else
        	return OMX_ErrorBadParameter;
    }

//    if (!pPortDef->bEnabled)
//    	return OMX_ErrorIncorrectStateOperation;
    if (m_state!=OMX_StateLoaded && m_state!=OMX_StateWaitForResources && pPortDef->bEnabled!=OMX_FALSE)
    {
        ALOGW("pPortDef[%d]->bEnabled=%d, m_state=0x%x, Can't allocate_buffer!", (int)nPortIndex, pPortDef->bEnabled, m_state);
    	return OMX_ErrorIncorrectStateOperation;
    }
    ALOGV("pPortDef[%d]->bEnabled=%d, m_state=0x%x, can allocate_buffer.", (int)nPortIndex, pPortDef->bEnabled, m_state);

    if (nSizeBytes != pPortDef->nBufferSize || pPortDef->bPopulated)
    	return OMX_ErrorBadParameter;

    // Find an empty position in the BufferList and allocate memory for the buffer header
    // and the actual buffer
    if (nPortIndex == m_sInPortDef.nPortIndex)
    {
        ALOGV("allocate_buffer, m_sInPortDef.nPortIndex[%d]", (int)m_sInPortDef.nPortIndex);
    	pthread_mutex_lock(&m_inBufMutex);

    	if((OMX_S32)m_sInBufList.nAllocSize >= m_sInBufList.nBufArrSize)
        {
        	pthread_mutex_unlock(&m_inBufMutex);
        	return OMX_ErrorInsufficientResources;
        }

    	nIndex = m_sInBufList.nAllocSize;

        m_sInBufList.pBufArr[nIndex].pBuffer = (OMX_U8*)malloc(nSizeBytes);

        if (!m_sInBufList.pBufArr[nIndex].pBuffer)
        {
        	pthread_mutex_unlock(&m_inBufMutex);
            return OMX_ErrorInsufficientResources;
        }

        m_sInBufList.nAllocBySelfFlags |= (1<<nIndex);

    	m_sInBufList.pBufArr[nIndex].nAllocLen   = nSizeBytes;
    	m_sInBufList.pBufArr[nIndex].pAppPrivate = pAppPrivate;
    	m_sInBufList.pBufArr[nIndex].nInputPortIndex = nPortIndex;
    	m_sInBufList.pBufArr[nIndex].nOutputPortIndex = 0xFFFFFFFE;
        *ppBufferHdr = &m_sInBufList.pBufArr[nIndex];

        m_sInBufList.nAllocSize++;
        if (m_sInBufList.nAllocSize == pPortDef->nBufferCountActual)
        	pPortDef->bPopulated = OMX_TRUE;

    	pthread_mutex_unlock(&m_inBufMutex);
    }
    else
    {
        ALOGV("allocate_buffer, m_sOutPortDef.nPortIndex[%d]", (int)m_sOutPortDef.nPortIndex);
    	pthread_mutex_lock(&m_outBufMutex);

    	if((OMX_S32)m_sOutBufList.nAllocSize >= m_sOutBufList.nBufArrSize)
        {
        	pthread_mutex_unlock(&m_outBufMutex);
        	return OMX_ErrorInsufficientResources;
        }

    	nIndex = m_sOutBufList.nAllocSize;

    	m_sOutBufList.pBufArr[nIndex].pBuffer = (OMX_U8*)malloc(nSizeBytes);

        if (!m_sOutBufList.pBufArr[nIndex].pBuffer)
        {
        	pthread_mutex_unlock(&m_outBufMutex);
            return OMX_ErrorInsufficientResources;
        }

        m_sOutBufList.nAllocBySelfFlags |= (1<<nIndex);

        m_sOutBufList.pBufArr[nIndex].nAllocLen   = nSizeBytes;
        m_sOutBufList.pBufArr[nIndex].pAppPrivate = pAppPrivate;
        m_sOutBufList.pBufArr[nIndex].nInputPortIndex = 0xFFFFFFFE;
        m_sOutBufList.pBufArr[nIndex].nOutputPortIndex = nPortIndex;
        *ppBufferHdr = &m_sOutBufList.pBufArr[nIndex];

        m_sOutBufList.nAllocSize++;
        if (m_sOutBufList.nAllocSize == pPortDef->nBufferCountActual)
        	pPortDef->bPopulated = OMX_TRUE;

    	pthread_mutex_unlock(&m_outBufMutex);
    }

    return OMX_ErrorNone;
}



OMX_ERRORTYPE omx_vdec::free_buffer(OMX_IN  OMX_HANDLETYPE        hComponent,
        							OMX_IN  OMX_U32               nPortIndex,
        							OMX_IN  OMX_BUFFERHEADERTYPE* pBufferHdr)
{
    OMX_PARAM_PORTDEFINITIONTYPE* pPortDef;
    OMX_S32                       nIndex;

	//ALOGV(" COMPONENT_FREE_BUFFER, nPortIndex = %d, pBufferHdr = %x", nPortIndex, pBufferHdr);
    ALOGV("(f:%s, l:%d) nPortIndex = %d, pBufferHdr = %p, m_state=0x%x", __FUNCTION__, __LINE__, (int)nPortIndex, pBufferHdr, m_state);
    if(hComponent == NULL || pBufferHdr == NULL)
    	return OMX_ErrorBadParameter;

    // Match the pBufferHdr to the appropriate entry in the BufferList
    // and free the allocated memory
    if (nPortIndex == m_sInPortDef.nPortIndex)
    {
        pPortDef = &m_sInPortDef;

    	pthread_mutex_lock(&m_inBufMutex);

    	for(nIndex = 0; nIndex < m_sInBufList.nBufArrSize; nIndex++)
    	{
    		if(pBufferHdr == &m_sInBufList.pBufArr[nIndex])
    			break;
    	}

    	if(nIndex == m_sInBufList.nBufArrSize)
    	{
    		pthread_mutex_unlock(&m_inBufMutex);
    		return OMX_ErrorBadParameter;
    	}

    	if(m_sInBufList.nAllocBySelfFlags & (1<<nIndex))
    	{
    		free(m_sInBufList.pBufArr[nIndex].pBuffer);
    		m_sInBufList.pBufArr[nIndex].pBuffer = NULL;
    		m_sInBufList.nAllocBySelfFlags &= ~(1<<nIndex);
    	}

    	m_sInBufList.nAllocSize--;
    	if(m_sInBufList.nAllocSize == 0)
    		pPortDef->bPopulated = OMX_FALSE;

    	pthread_mutex_unlock(&m_inBufMutex);
    }
    else if (nPortIndex == m_sOutPortDef.nPortIndex)
    {
	    pPortDef = &m_sOutPortDef;

    	pthread_mutex_lock(&m_outBufMutex);

    	for(nIndex = 0; nIndex < m_sOutBufList.nBufArrSize; nIndex++)
    	{
    		ALOGV("pBufferHdr = %p, &m_sOutBufList.pBufArr[%d] = %p", pBufferHdr, (int)nIndex, &m_sOutBufList.pBufArr[nIndex]);
    		if(pBufferHdr == &m_sOutBufList.pBufArr[nIndex])
    			break;
    	}

    	ALOGV("index = %d", (int)nIndex);

    	if(nIndex == m_sOutBufList.nBufArrSize)
    	{
    		pthread_mutex_unlock(&m_outBufMutex);
    		return OMX_ErrorBadParameter;
    	}

    	if(m_sOutBufList.nAllocBySelfFlags & (1<<nIndex))
    	{
    		free(m_sOutBufList.pBufArr[nIndex].pBuffer);
    		m_sOutBufList.pBufArr[nIndex].pBuffer = NULL;
    		m_sOutBufList.nAllocBySelfFlags &= ~(1<<nIndex);
    	}

    	m_sOutBufList.nAllocSize--;
    	if(m_sOutBufList.nAllocSize == 0)
    		pPortDef->bPopulated = OMX_FALSE;

    	pthread_mutex_unlock(&m_outBufMutex);
    }
    else
        return OMX_ErrorBadParameter;

    return OMX_ErrorNone;
}



OMX_ERRORTYPE  omx_vdec::empty_this_buffer(OMX_IN OMX_HANDLETYPE hComponent, OMX_IN OMX_BUFFERHEADERTYPE* pBufferHdr)
{
    ThrCmdType eCmd   = EmptyBuf;

    ALOGV("(f:%s, l:%d) ", __FUNCTION__, __LINE__);
    if(hComponent == NULL || pBufferHdr == NULL)
    	return OMX_ErrorBadParameter;

    if (!m_sInPortDef.bEnabled)
    	return OMX_ErrorIncorrectStateOperation;

    if (pBufferHdr->nInputPortIndex != 0x0  || pBufferHdr->nOutputPortIndex != OMX_NOPORT)
        return OMX_ErrorBadPortIndex;

    if (m_state != OMX_StateExecuting && m_state != OMX_StatePause)
        return OMX_ErrorIncorrectStateOperation;

    //fwrite(pBufferHdr->pBuffer, 1, pBufferHdr->nFilledLen, ph264File);
    //DBG_WARNING("BHF[0x%x],len[%d]", pBufferHdr->nFlags, pBufferHdr->nFilledLen);
    // Put the command and data in the pipe
//    write(m_cmdpipe[1], &eCmd, sizeof(eCmd));
//    write(m_cmddatapipe[1], &pBufferHdr, sizeof(OMX_BUFFERHEADERTYPE*));

    post_event(eCmd, 0, (OMX_PTR)pBufferHdr);

    return OMX_ErrorNone;
}



OMX_ERRORTYPE  omx_vdec::fill_this_buffer(OMX_IN OMX_HANDLETYPE hComponent, OMX_IN OMX_BUFFERHEADERTYPE* pBufferHdr)
{
    ThrCmdType eCmd = FillBuf;

//	ALOGV(" COMPONENT_FILL_THIS_BUFFER");
    ALOGV("(f:%s, l:%d) ", __FUNCTION__, __LINE__);
    if(hComponent == NULL || pBufferHdr == NULL)
    	return OMX_ErrorBadParameter;

    if (!m_sOutPortDef.bEnabled)
    	return OMX_ErrorIncorrectStateOperation;

    if (pBufferHdr->nOutputPortIndex != 0x1 || pBufferHdr->nInputPortIndex != OMX_NOPORT)
        return OMX_ErrorBadPortIndex;

    if (m_state != OMX_StateExecuting && m_state != OMX_StatePause)
        return OMX_ErrorIncorrectStateOperation;

    // Put the command and data in the pipe
//    write(m_cmdpipe[1], &eCmd, sizeof(eCmd));
//    write(m_cmddatapipe[1], &pBufferHdr, sizeof(OMX_BUFFERHEADERTYPE*));

    post_event(eCmd, 0, (OMX_PTR)pBufferHdr);
    return OMX_ErrorNone;
}



OMX_ERRORTYPE  omx_vdec::set_callbacks(OMX_IN OMX_HANDLETYPE        hComp,
                                           OMX_IN OMX_CALLBACKTYPE* callbacks,
                                           OMX_IN OMX_PTR           appData)
{
	ALOGV("(f:%s, l:%d) ", __FUNCTION__, __LINE__);

    if(hComp == NULL || callbacks == NULL || appData == NULL)
    	return OMX_ErrorBadParameter;
    memcpy(&m_Callbacks, callbacks, sizeof(OMX_CALLBACKTYPE));
    m_pAppData = appData;

    return OMX_ErrorNone;
}



OMX_ERRORTYPE  omx_vdec::component_deinit(OMX_IN OMX_HANDLETYPE hComp)
{
	//ALOGV(" COMPONENT_DEINIT");
    ALOGV("(f:%s, l:%d) ", __FUNCTION__, __LINE__);
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    ThrCmdType    eCmd   = Stop;
    OMX_S32       nIndex = 0;

    // In case the client crashes, check for nAllocSize parameter.
    // If this is greater than zero, there are elements in the list that are not free'd.
    // In that case, free the elements.
    if (m_sInBufList.nAllocSize > 0)
    {
    	for(nIndex=0; nIndex<m_sInBufList.nBufArrSize; nIndex++)
    	{
    		if(m_sInBufList.pBufArr[nIndex].pBuffer != NULL)
    		{
    			if(m_sInBufList.nAllocBySelfFlags & (1<<nIndex))
    			{
    				free(m_sInBufList.pBufArr[nIndex].pBuffer);
    				m_sInBufList.pBufArr[nIndex].pBuffer = NULL;
    			}
    		}
    	}

        if (m_sInBufList.pBufArr != NULL)
        	free(m_sInBufList.pBufArr);

        if (m_sInBufList.pBufHdrList != NULL)
        	free(m_sInBufList.pBufHdrList);

    	memset(&m_sInBufList, 0, sizeof(struct _BufferList));
    	m_sInBufList.nBufArrSize = m_sInPortDef.nBufferCountActual;
    }

    if (m_sOutBufList.nAllocSize > 0)
    {
    	for(nIndex=0; nIndex<m_sOutBufList.nBufArrSize; nIndex++)
    	{
    		if(m_sOutBufList.pBufArr[nIndex].pBuffer != NULL)
    		{
    			if(m_sOutBufList.nAllocBySelfFlags & (1<<nIndex))
    			{
    				free(m_sOutBufList.pBufArr[nIndex].pBuffer);
    				m_sOutBufList.pBufArr[nIndex].pBuffer = NULL;
    			}
    		}
    	}

        if (m_sOutBufList.pBufArr != NULL)
        	free(m_sOutBufList.pBufArr);

        if (m_sOutBufList.pBufHdrList != NULL)
        	free(m_sOutBufList.pBufHdrList);

    	memset(&m_sOutBufList, 0, sizeof(struct _BufferList));
    	m_sOutBufList.nBufArrSize = m_sOutPortDef.nBufferCountActual;
    }

    // Put the command and data in the pipe
//    write(m_cmdpipe[1], &eCmd, sizeof(eCmd));
//    write(m_cmddatapipe[1], &eCmd, sizeof(eCmd));
    post_event(eCmd, eCmd, NULL);

    // Wait for thread to exit so we can get the status into "error"
    pthread_join(m_thread_id, (void**)&eError);
    pthread_join(m_vdrv_thread_id, (void**)&eError);
    ALOGD("(f:%s, l:%d) two threads exit!", __FUNCTION__, __LINE__);
    // close the pipe handles
    close(m_cmdpipe[0]);
    close(m_cmdpipe[1]);
    close(m_cmddatapipe[0]);
    close(m_cmddatapipe[1]);
    close(m_vdrv_cmdpipe[0]);
    close(m_vdrv_cmdpipe[1]);

    if(m_decoder != NULL)
    {
	    if (ve_mutex_lock(&m_cedarv_req_ctx) < 0)
        //if (RequestCedarVFrameLevel(&m2_cedarv_req_ctx) < 0)
		{
			 ALOGW("Request VE failed\n");
		}

    	libcedarv_exit(m_decoder);
    	m_decoder = NULL;

		ve_mutex_unlock(&m_cedarv_req_ctx);
        //ReleaseCedarVFrameLevel(&m2_cedarv_req_ctx);
    }
	ve_mutex_destroy(&m_cedarv_req_ctx);
    //ReleaseCedarVResource(&m2_cedarv_req_ctx);
    return eError;
}


OMX_ERRORTYPE  omx_vdec::use_EGL_image(OMX_IN OMX_HANDLETYPE               hComp,
                                          OMX_INOUT OMX_BUFFERHEADERTYPE** bufferHdr,
                                          OMX_IN OMX_U32                   port,
                                          OMX_IN OMX_PTR                   appData,
                                          OMX_IN void*                     eglImage)
{
	ALOGW("Error : use_EGL_image:  Not Implemented \n");
    return OMX_ErrorNotImplemented;
}


OMX_ERRORTYPE  omx_vdec::component_role_enum(OMX_IN  OMX_HANDLETYPE hComp,
                                             OMX_OUT OMX_U8*        role,
                                             OMX_IN  OMX_U32        index)
{
	//ALOGV(" COMPONENT_ROLE_ENUM");
    ALOGV("(f:%s, l:%d) ", __FUNCTION__, __LINE__);
	OMX_ERRORTYPE eRet = OMX_ErrorNone;

	if(!strncmp((char*)m_cName, "OMX.allwinner.video.decoder.mpeg4", OMX_MAX_STRINGNAME_SIZE))
	{
		if((0 == index) && role)
		{
			strncpy((char *)role, "video_decoder.mpeg4", OMX_MAX_STRINGNAME_SIZE);
			ALOGV("component_role_enum: role %s\n", role);
		}
		else
		{
			eRet = OMX_ErrorNoMore;
		}
	}
	else if(!strncmp((char*)m_cName, "OMX.allwinner.video.decoder.h263", OMX_MAX_STRINGNAME_SIZE))
	{
		if((0 == index) && role)
		{
			strncpy((char *)role, "video_decoder.h263",OMX_MAX_STRINGNAME_SIZE);
			ALOGV("component_role_enum: role %s\n",role);
		}
		else
		{
			ALOGV("\n No more roles \n");
			eRet = OMX_ErrorNoMore;
		}
	}
	else if(!strncmp((char*)m_cName, "OMX.allwinner.video.decoder.avc", OMX_MAX_STRINGNAME_SIZE))
	{
		if((0 == index) && role)
		{
			strncpy((char *)role, "video_decoder.avc",OMX_MAX_STRINGNAME_SIZE);
			ALOGV("component_role_enum: role %s\n",role);
		}
		else
		{
			ALOGV("\n No more roles \n");
			eRet = OMX_ErrorNoMore;
		}
	}
	else if(!strncmp((char*)m_cName, "OMX.allwinner.video.decoder.vc1", OMX_MAX_STRINGNAME_SIZE))
	{
		if((0 == index) && role)
		{
			strncpy((char *)role, "video_decoder.vc1",OMX_MAX_STRINGNAME_SIZE);
			ALOGV("component_role_enum: role %s\n",role);
		}
		else
		{
			ALOGV("\n No more roles \n");
			eRet = OMX_ErrorNoMore;
		}
	}
    else if(!strncmp((char*)m_cName, "OMX.allwinner.video.decoder.vp8", OMX_MAX_STRINGNAME_SIZE))
	{
		if((0 == index) && role)
		{
			strncpy((char *)role, "video_decoder.vp8",OMX_MAX_STRINGNAME_SIZE);
			ALOGV("component_role_enum: role %s\n",role);
		}
		else
		{
			ALOGV("\n No more roles \n");
			eRet = OMX_ErrorNoMore;
		}
	}
	else
	{
		ALOGE("\nERROR:Querying Role on Unknown Component\n");
		eRet = OMX_ErrorInvalidComponentName;
	}

	return eRet;
}

OMX_ERRORTYPE omx_vdec::send_vdrv_feedback_msg(OMX_IN OMX_VDRV_FEEDBACK_MSGTYPE nMsg,
                                                   OMX_IN OMX_U32         param1,
                                                   OMX_IN OMX_PTR         cmdData)
{
    ThrCmdType    eCmd;
    OMX_ERRORTYPE eError = OMX_ErrorNone;
    ALOGV("(f:%s, l:%d) ", __FUNCTION__, __LINE__);

    if(m_state == OMX_StateInvalid)
    {
    	ALOGE("ERROR: Send Command in Invalid State\n");
        return OMX_ErrorInvalidState;
    }

    switch (nMsg)
    {
        case OMX_VdrvFeedbackMsg_NotifyEos:
        	ALOGV("(omx_vdec, f:%s, l:%d) send OMX_VdrvFeedbackMsg_NotifyEos", __FUNCTION__, __LINE__);
            eCmd = VdrvNotifyEos;
	        break;

        default:
            ALOGW("(omx_vdec, f:%s, l:%d) send unknown feedback message[0x%x]", __FUNCTION__, __LINE__, nMsg);
            return OMX_ErrorUndefined;
    }

    post_event(eCmd, param1, cmdData);

    return eError;
}

OMX_ERRORTYPE omx_vdec::post_event(OMX_IN ThrCmdType eCmd,
                                   OMX_IN OMX_U32         param1,
                                   OMX_IN OMX_PTR         cmdData)
{
    pthread_mutex_lock(&m_pipeMutex);
    write(m_cmdpipe[1], &eCmd, sizeof(eCmd));

    // In case of MarkBuf, the pCmdData parameter is used to carry the data.
    // In other cases, the nParam1 parameter carries the data.
    if(eCmd == MarkBuf || eCmd == EmptyBuf || eCmd == FillBuf)
        write(m_cmddatapipe[1], &cmdData, sizeof(OMX_PTR));
    else
        write(m_cmddatapipe[1], &param1, sizeof(param1));

    pthread_mutex_unlock(&m_pipeMutex);

    return OMX_ErrorNone;
}

/****************************************************************************
physical address to virtual address
****************************************************************************/
int convertAddress_Phy2Vir(cedarv_picture_t *picture)
{
    //pict->pixel_format == CEDARV_PIXEL_FORMAT_AW_YUV422
    if(picture->y)
    {
        picture->y = (u8*)cedarv_address_phy2vir(picture->y);
    }
    if(picture->u)
    {
        picture->u = (u8*)cedarv_address_phy2vir(picture->u);
    }
    if(picture->v)
    {
        picture->v = (u8*)cedarv_address_phy2vir(picture->v);
    }
    return 0;
}

/*******************************************************************************
Function name: detectPipeDataToRead
Description:

Parameters:

Return:
    1:data ready
    0:no data
Time: 2013/9/30
*******************************************************************************/
int waitPipeDataToRead(int nPipeFd, int nTimeUs)
{
    int                     i;
    fd_set                  rfds;
    struct timeval          timeout;
    FD_ZERO(&rfds);
    FD_SET(nPipeFd, &rfds);

    // Check for new command
    timeout.tv_sec  = 0;
    timeout.tv_usec = nTimeUs;

    i = select(nPipeFd+1, &rfds, NULL, NULL, &timeout);

    if (FD_ISSET(nPipeFd, &rfds))
    {
        return 1;
    }
    else
    {
        return 0;
    }
}
/*
 *  Component Thread
 *    The ComponentThread function is exeuted in a separate pThread and
 *    is used to implement the actual component functions.
 */
 /*****************************************************************************/
static void* ComponentThread(void* pThreadData)
{
	int                     decodeResult;
    cedarv_picture_t        picture;

    int                     i;
    int                     fd1;
    fd_set                  rfds;
    OMX_U32                 cmddata;
    ThrCmdType              cmd;

    // Variables related to decoder buffer handling
    OMX_BUFFERHEADERTYPE*   pInBufHdr   = NULL;
    OMX_BUFFERHEADERTYPE*   pOutBufHdr  = NULL;
    OMX_MARKTYPE*           pMarkBuf    = NULL;
    OMX_U8*                 pInBuf      = NULL;
    OMX_U32                 nInBufSize;

    // Variables related to decoder timeouts
    OMX_U32                 nTimeout;
    OMX_BOOL                port_setting_match;
    OMX_BOOL                eos;
    int                     nVdrvNotifyEosFlag = 0;

    OMX_PTR                 pMarkData;
    OMX_HANDLETYPE          hMarkTargetComponent;

    struct timeval          timeout;

    port_setting_match   = OMX_TRUE;
    eos                  = OMX_FALSE;
    pMarkData            = NULL;
    hMarkTargetComponent = NULL;

    int64_t         nTimeUs1;
    int64_t         nTimeUs2;

    int nSemVal;
    int nRetSemGetValue;
    // Recover the pointer to my component specific data
    omx_vdec* pSelf = (omx_vdec*)pThreadData;


    while (1)
    {
        fd1 = pSelf->m_cmdpipe[0];
        FD_ZERO(&rfds);
        FD_SET(fd1, &rfds);

        // Check for new command
        timeout.tv_sec  = 0;
        timeout.tv_usec = 0;

        i = select(pSelf->m_cmdpipe[0]+1, &rfds, NULL, NULL, &timeout);

        if (FD_ISSET(pSelf->m_cmdpipe[0], &rfds))
        {
            // retrieve command and data from pipe
            read(pSelf->m_cmdpipe[0], &cmd, sizeof(cmd));
	        read(pSelf->m_cmddatapipe[0], &cmddata, sizeof(cmddata));

            // State transition command
	        if (cmd == SetState)
	        {
	        	ALOGD(" set state command, cmd = %d, cmddata = %d.", (int)cmd, (int)cmddata);
                // If the parameter states a transition to the same state
                // raise a same state transition error.
                if (pSelf->m_state == (OMX_STATETYPE)(cmddata))
                {
                	pSelf->m_Callbacks.EventHandler(&pSelf->m_cmp, pSelf->m_pAppData, OMX_EventError, OMX_ErrorSameState, 0 , NULL);
                }
                else
                {
	                // transitions/callbacks made based on state transition table
                    // cmddata contains the target state
	                switch ((OMX_STATETYPE)(cmddata))
	                {
             	        case OMX_StateInvalid:
             	        	pSelf->m_state = OMX_StateInvalid;
             	        	pSelf->m_Callbacks.EventHandler(&pSelf->m_cmp, pSelf->m_pAppData, OMX_EventError, OMX_ErrorInvalidState, 0 , NULL);
             	        	pSelf->m_Callbacks.EventHandler(&pSelf->m_cmp, pSelf->m_pAppData, OMX_EventCmdComplete, OMX_CommandStateSet, pSelf->m_state, NULL);
			                break;

		                case OMX_StateLoaded:
                            if (pSelf->m_state == OMX_StateIdle || pSelf->m_state == OMX_StateWaitForResources)
                            {
			                    nTimeout = 0x0;
			                    while (1)
			                    {
                                    // Transition happens only when the ports are unpopulated
			                        if (!pSelf->m_sInPortDef.bPopulated && !pSelf->m_sOutPortDef.bPopulated)
			                        {
			                        	pSelf->m_state = OMX_StateLoaded;
			                        	pSelf->m_Callbacks.EventHandler(&pSelf->m_cmp, pSelf->m_pAppData, OMX_EventCmdComplete, OMX_CommandStateSet, pSelf->m_state, NULL);

 			                            //* close decoder
			                        	//* TODO.

				                        break;
 	                                }
				                    else if (nTimeout++ > OMX_MAX_TIMEOUTS)
				                    {
				                    	pSelf->m_Callbacks.EventHandler(&pSelf->m_cmp, pSelf->m_pAppData, OMX_EventError, OMX_ErrorInsufficientResources, 0 , NULL);
 	                                    ALOGW("Transition to loaded failed\n");
				                        break;
				                    }

			                        usleep(OMX_TIMEOUT*1000);
			                    }

                                post_message_to_vdrv(pSelf, OMX_VdrvCommand_CloseVdecLib);
                                ALOGD("(f:%s, l:%d) wait for OMX_VdrvCommand_CloseVdecLib", __FUNCTION__, __LINE__);
                                sem_wait(&pSelf->m_vdrv_cmd_lock);
                                ALOGD("(f:%s, l:%d) wait for OMX_VdrvCommand_CloseVdecLib done!", __FUNCTION__, __LINE__);
		                    }
			                else
			                {
			                	pSelf->m_Callbacks.EventHandler(&pSelf->m_cmp, pSelf->m_pAppData, OMX_EventError, OMX_ErrorIncorrectStateTransition, 0 , NULL);
			                }

			                break;

                        case OMX_StateIdle:
		                    if (pSelf->m_state == OMX_StateInvalid)
		                    	pSelf->m_Callbacks.EventHandler(&pSelf->m_cmp, pSelf->m_pAppData, OMX_EventError, OMX_ErrorIncorrectStateTransition, 0 , NULL);
		                    else
		                    {
			                    // Return buffers if currently in pause and executing
			                    if (pSelf->m_state == OMX_StatePause || pSelf->m_state == OMX_StateExecuting)
			                    {
			                    	pthread_mutex_lock(&pSelf->m_inBufMutex);

                                    while (pSelf->m_sInBufList.nSizeOfList > 0)
                                    {
                                    	pSelf->m_sInBufList.nSizeOfList--;
                                    	pSelf->m_Callbacks.EmptyBufferDone(&pSelf->m_cmp,
                                    			                           pSelf->m_pAppData,
                                    			                           pSelf->m_sInBufList.pBufHdrList[pSelf->m_sInBufList.nReadPos++]);

                                    	if (pSelf->m_sInBufList.nReadPos >= pSelf->m_sInBufList.nBufArrSize)
                                    		pSelf->m_sInBufList.nReadPos = 0;
                                    }

			                    	pthread_mutex_unlock(&pSelf->m_inBufMutex);


			                    	pthread_mutex_lock(&pSelf->m_outBufMutex);

                                    while (pSelf->m_sOutBufList.nSizeOfList > 0)
                                    {
                                    	pSelf->m_sOutBufList.nSizeOfList--;
                                    	pSelf->m_Callbacks.FillBufferDone(&pSelf->m_cmp,
                                    			                           pSelf->m_pAppData,
                                    			                           pSelf->m_sOutBufList.pBufHdrList[pSelf->m_sOutBufList.nReadPos++]);

                                    	if (pSelf->m_sOutBufList.nReadPos >= pSelf->m_sOutBufList.nBufArrSize)
                                    		pSelf->m_sOutBufList.nReadPos = 0;
                                    }

			                    	pthread_mutex_unlock(&pSelf->m_outBufMutex);
			                    }
			                    else    //OMX_StateLoaded -> OMX_StateIdle
			                    {
                                    post_message_to_vdrv(pSelf, OMX_VdrvCommand_PrepareVdecLib);
                                    ALOGD("(f:%s, l:%d) wait for OMX_VdrvCommand_PrepareVdecLib", __FUNCTION__, __LINE__);
                                    sem_wait(&pSelf->m_vdrv_cmd_lock);
                                    ALOGD("(f:%s, l:%d) wait for OMX_VdrvCommand_PrepareVdecLib done!", __FUNCTION__, __LINE__);
			                    }

			                    nTimeout = 0x0;
			                    while (1)
			                    {
			                        // Ports have to be populated before transition completes
			                        if ((!pSelf->m_sInPortDef.bEnabled && !pSelf->m_sOutPortDef.bEnabled)   ||
                                        (pSelf->m_sInPortDef.bPopulated && pSelf->m_sOutPortDef.bPopulated))
                                    {
			                        	pSelf->m_state = OMX_StateIdle;
                                        //wake up vdrvThread
                                        sem_post(&pSelf->m_sem_vbs_input);
                                        sem_post(&pSelf->m_sem_frame_output);
			                        	pSelf->m_Callbacks.EventHandler(&pSelf->m_cmp, pSelf->m_pAppData, OMX_EventCmdComplete, OMX_CommandStateSet, pSelf->m_state, NULL);

                                        //* Open decoder
			                        	//* TODO.
 		                                break;
			                        }
			                        else if (nTimeout++ > OMX_MAX_TIMEOUTS)
			                        {
			                        	pSelf->m_Callbacks.EventHandler(&pSelf->m_cmp, pSelf->m_pAppData, OMX_EventError, OMX_ErrorInsufficientResources, 0 , NULL);
                		                ALOGW("Idle transition failed\n");
	                                    break;
				                    }

			                        usleep(OMX_TIMEOUT*1000);
			                    }
		                    }

		                    break;

		                case OMX_StateExecuting:
                            // Transition can only happen from pause or idle state
                            if (pSelf->m_state == OMX_StateIdle || pSelf->m_state == OMX_StatePause)
                            {
                                // Return buffers if currently in pause
			                    if (pSelf->m_state == OMX_StatePause)
			                    {
			                    	pthread_mutex_lock(&pSelf->m_inBufMutex);

                                    while (pSelf->m_sInBufList.nSizeOfList > 0)
                                    {
                                    	pSelf->m_sInBufList.nSizeOfList--;
                                    	pSelf->m_Callbacks.EmptyBufferDone(&pSelf->m_cmp,
                                    			                           pSelf->m_pAppData,
                                    			                           pSelf->m_sInBufList.pBufHdrList[pSelf->m_sInBufList.nReadPos++]);

                                    	if (pSelf->m_sInBufList.nReadPos >= pSelf->m_sInBufList.nBufArrSize)
                                    		pSelf->m_sInBufList.nReadPos = 0;
                                    }

			                    	pthread_mutex_unlock(&pSelf->m_inBufMutex);

			                    	pthread_mutex_lock(&pSelf->m_outBufMutex);

                                    while (pSelf->m_sOutBufList.nSizeOfList > 0)
                                    {
                                    	pSelf->m_sOutBufList.nSizeOfList--;
                                    	pSelf->m_Callbacks.FillBufferDone(&pSelf->m_cmp,
                                    			                           pSelf->m_pAppData,
                                    			                           pSelf->m_sOutBufList.pBufHdrList[pSelf->m_sOutBufList.nReadPos++]);

                                    	if (pSelf->m_sOutBufList.nReadPos >= pSelf->m_sOutBufList.nBufArrSize)
                                    		pSelf->m_sOutBufList.nReadPos = 0;
                                    }

			                    	pthread_mutex_unlock(&pSelf->m_outBufMutex);
			                    }

			                    pSelf->m_state = OMX_StateExecuting;
			                    pSelf->m_Callbacks.EventHandler(&pSelf->m_cmp, pSelf->m_pAppData, OMX_EventCmdComplete, OMX_CommandStateSet, pSelf->m_state, NULL);
			                }
			                else
			                	pSelf->m_Callbacks.EventHandler(&pSelf->m_cmp, pSelf->m_pAppData, OMX_EventError, OMX_ErrorIncorrectStateTransition, 0 , NULL);

                            eos                  = OMX_FALSE;
                            pMarkData            = NULL;
                            hMarkTargetComponent = NULL;

			                break;

                        case OMX_StatePause:
                            // Transition can only happen from idle or executing state
		                    if (pSelf->m_state == OMX_StateIdle || pSelf->m_state == OMX_StateExecuting)
		                    {
		                    	pSelf->m_state = OMX_StatePause;
		                    	pSelf->m_Callbacks.EventHandler(&pSelf->m_cmp, pSelf->m_pAppData, OMX_EventCmdComplete, OMX_CommandStateSet, pSelf->m_state, NULL);
		                    }
			                else
			                	pSelf->m_Callbacks.EventHandler(&pSelf->m_cmp, pSelf->m_pAppData, OMX_EventError, OMX_ErrorIncorrectStateTransition, 0 , NULL);

		                    break;

                        case OMX_StateWaitForResources:
		                    if (pSelf->m_state == OMX_StateLoaded)
		                    {
		                    	pSelf->m_state = OMX_StateWaitForResources;
		                    	pSelf->m_Callbacks.EventHandler(&pSelf->m_cmp, pSelf->m_pAppData, OMX_EventCmdComplete, OMX_CommandStateSet, pSelf->m_state, NULL);
			                }
			                else
			                	pSelf->m_Callbacks.EventHandler(&pSelf->m_cmp, pSelf->m_pAppData, OMX_EventError, OMX_ErrorIncorrectStateTransition, 0 , NULL);

		                    break;

                        default:
		                	pSelf->m_Callbacks.EventHandler(&pSelf->m_cmp, pSelf->m_pAppData, OMX_EventError, OMX_ErrorIncorrectStateTransition, 0 , NULL);
		                	break;

		            }
                }
	        }
	        else if (cmd == StopPort)
	        {
	        	ALOGD(" stop port command, cmddata = %d.", (int)cmddata);
	            // Stop Port(s)
	            // cmddata contains the port index to be stopped.
                // It is assumed that 0 is input and 1 is output port for this component
                // The cmddata value -1 means that both input and output ports will be stopped.
	            if (cmddata == 0x0 || (OMX_S32)cmddata == -1)
	            {
	                // Return all input buffers
                	pthread_mutex_lock(&pSelf->m_inBufMutex);

                    while (pSelf->m_sInBufList.nSizeOfList > 0)
                    {
                    	pSelf->m_sInBufList.nSizeOfList--;
                    	pSelf->m_Callbacks.EmptyBufferDone(&pSelf->m_cmp,
                    			                           pSelf->m_pAppData,
                    			                           pSelf->m_sInBufList.pBufHdrList[pSelf->m_sInBufList.nReadPos++]);

                    	if (pSelf->m_sInBufList.nReadPos >= pSelf->m_sInBufList.nBufArrSize)
                    		pSelf->m_sInBufList.nReadPos = 0;
                    }

                	pthread_mutex_unlock(&pSelf->m_inBufMutex);

 		            // Disable port
					pSelf->m_sInPortDef.bEnabled = OMX_FALSE;
		        }

	            if (cmddata == 0x1 || (OMX_S32)cmddata == -1)
	            {
		            // Return all output buffers
                	pthread_mutex_lock(&pSelf->m_outBufMutex);

                    while (pSelf->m_sOutBufList.nSizeOfList > 0)
                    {
                    	pSelf->m_sOutBufList.nSizeOfList--;
                    	pSelf->m_Callbacks.FillBufferDone(&pSelf->m_cmp,
                    			                           pSelf->m_pAppData,
                    			                           pSelf->m_sOutBufList.pBufHdrList[pSelf->m_sOutBufList.nReadPos++]);

                    	if (pSelf->m_sOutBufList.nReadPos >= pSelf->m_sOutBufList.nBufArrSize)
                    		pSelf->m_sOutBufList.nReadPos = 0;
                    }

                	pthread_mutex_unlock(&pSelf->m_outBufMutex);

       	            // Disable port
					pSelf->m_sOutPortDef.bEnabled = OMX_FALSE;
		        }

		        // Wait for all buffers to be freed
		        nTimeout = 0x0;
		        while (1)
		        {
		            if (cmddata == 0x0 && !pSelf->m_sInPortDef.bPopulated)
		            {
		                // Return cmdcomplete event if input unpopulated
		            	pSelf->m_Callbacks.EventHandler(&pSelf->m_cmp, pSelf->m_pAppData, OMX_EventCmdComplete, OMX_CommandPortDisable, 0x0, NULL);
			            break;
		            }

		            if (cmddata == 0x1 && !pSelf->m_sOutPortDef.bPopulated)
		            {
		                // Return cmdcomplete event if output unpopulated
		            	pSelf->m_Callbacks.EventHandler(&pSelf->m_cmp, pSelf->m_pAppData, OMX_EventCmdComplete, OMX_CommandPortDisable, 0x1, NULL);
			            break;
		            }

		            if ((OMX_S32)cmddata == -1 &&  !pSelf->m_sInPortDef.bPopulated && !pSelf->m_sOutPortDef.bPopulated)
		            {
            		    // Return cmdcomplete event if inout & output unpopulated
		            	pSelf->m_Callbacks.EventHandler(&pSelf->m_cmp, pSelf->m_pAppData, OMX_EventCmdComplete, OMX_CommandPortDisable, 0x0, NULL);
		            	pSelf->m_Callbacks.EventHandler(&pSelf->m_cmp, pSelf->m_pAppData, OMX_EventCmdComplete, OMX_CommandPortDisable, 0x1, NULL);
			            break;
		            }

		            if (nTimeout++ > OMX_MAX_TIMEOUTS)
		            {
		            	pSelf->m_Callbacks.EventHandler(&pSelf->m_cmp, pSelf->m_pAppData, OMX_EventError, OMX_ErrorPortUnresponsiveDuringDeallocation, 0 , NULL);
       		            break;
		            }

                    usleep(OMX_TIMEOUT*1000);
		        }
	        }
	        else if (cmd == RestartPort)
	        {
	        	ALOGD(" restart port command.pSelf->m_state[%d]", pSelf->m_state);
                // Restart Port(s)
                // cmddata contains the port index to be restarted.
                // It is assumed that 0 is input and 1 is output port for this component.
                // The cmddata value -1 means both input and output ports will be restarted.
//                if (cmddata == 0x0 || (OMX_S32)cmddata == -1)
//                	pSelf->m_sInPortDef.bEnabled = OMX_TRUE;
//
//	            if (cmddata == 0x1 || (OMX_S32)cmddata == -1)
//	            	pSelf->m_sOutPortDef.bEnabled = OMX_TRUE;

 	            // Wait for port to be populated
		        nTimeout = 0x0;
		        while (1)
		        {
                    // Return cmdcomplete event if input port populated
		            if (cmddata == 0x0 && (pSelf->m_state == OMX_StateLoaded || pSelf->m_sInPortDef.bPopulated))
		            {
                        pSelf->m_sInPortDef.bEnabled = OMX_TRUE;
		            	pSelf->m_Callbacks.EventHandler(&pSelf->m_cmp, pSelf->m_pAppData, OMX_EventCmdComplete, OMX_CommandPortEnable, 0x0, NULL);
			            break;
		            }
                    // Return cmdcomplete event if output port populated
		            else if (cmddata == 0x1 && (pSelf->m_state == OMX_StateLoaded || pSelf->m_sOutPortDef.bPopulated))
		            {
                        pSelf->m_sOutPortDef.bEnabled = OMX_TRUE;
		            	pSelf->m_Callbacks.EventHandler(&pSelf->m_cmp, pSelf->m_pAppData, OMX_EventCmdComplete, OMX_CommandPortEnable, 0x1, NULL);
                        break;
		            }
                    // Return cmdcomplete event if input and output ports populated
		            else if ((OMX_S32)cmddata == -1 && (pSelf->m_state == OMX_StateLoaded || (pSelf->m_sInPortDef.bPopulated && pSelf->m_sOutPortDef.bPopulated)))
		            {
                        pSelf->m_sInPortDef.bEnabled = OMX_TRUE;
                        pSelf->m_sOutPortDef.bEnabled = OMX_TRUE;
		            	pSelf->m_Callbacks.EventHandler(&pSelf->m_cmp, pSelf->m_pAppData, OMX_EventCmdComplete, OMX_CommandPortEnable, 0x0, NULL);
		            	pSelf->m_Callbacks.EventHandler(&pSelf->m_cmp, pSelf->m_pAppData, OMX_EventCmdComplete, OMX_CommandPortEnable, 0x1, NULL);
			            break;
		            }
		            else if (nTimeout++ > OMX_MAX_TIMEOUTS)
		            {
		            	pSelf->m_Callbacks.EventHandler(&pSelf->m_cmp, pSelf->m_pAppData, OMX_EventError, OMX_ErrorPortUnresponsiveDuringAllocation, 0, NULL);
                        break;
	                }

                    usleep(OMX_TIMEOUT*1000);
		        }

		        if(port_setting_match == OMX_FALSE)
		        	port_setting_match = OMX_TRUE;
	        }
	        else if (cmd == Flush)
	        {
	        	ALOGD(" flush command.");
	            // Flush port(s)
                // cmddata contains the port index to be flushed.
                // It is assumed that 0 is input and 1 is output port for this component
                // The cmddata value -1 means that both input and output ports will be flushed.
                if(cmddata == OMX_ALL)  //if request flush input and output port, we reset decoder!
                {
                    ALOGD(" flush all port! we reset decoder!");
                    post_message_to_vdrv(pSelf, OMX_VdrvCommand_FlushVdecLib);
                    sem_post(&pSelf->m_sem_vbs_input);
                    sem_post(&pSelf->m_sem_frame_output);
                    ALOGD("(f:%s, l:%d) wait for OMX_VdrvCommand_FlushVdecLib", __FUNCTION__, __LINE__);
                    sem_wait(&pSelf->m_vdrv_cmd_lock);
                    ALOGD("(f:%s, l:%d) wait for OMX_VdrvCommand_FlushVdecLib done!", __FUNCTION__, __LINE__);
                }
	            if (cmddata == 0x0 || (OMX_S32)cmddata == -1)
	            {
	                // Return all input buffers and send cmdcomplete
                	pthread_mutex_lock(&pSelf->m_inBufMutex);

                    while (pSelf->m_sInBufList.nSizeOfList > 0)
                    {
                    	pSelf->m_sInBufList.nSizeOfList--;
                    	pSelf->m_Callbacks.EmptyBufferDone(&pSelf->m_cmp,
                    			                           pSelf->m_pAppData,
                    			                           pSelf->m_sInBufList.pBufHdrList[pSelf->m_sInBufList.nReadPos++]);

                    	if (pSelf->m_sInBufList.nReadPos >= pSelf->m_sInBufList.nBufArrSize)
                    		pSelf->m_sInBufList.nReadPos = 0;
                    }

                	pthread_mutex_unlock(&pSelf->m_inBufMutex);

					pSelf->m_Callbacks.EventHandler(&pSelf->m_cmp, pSelf->m_pAppData, OMX_EventCmdComplete, OMX_CommandFlush, 0x0, NULL);
		        }

	            if (cmddata == 0x1 || (OMX_S32)cmddata == -1)
	            {
	                // Return all output buffers and send cmdcomplete
                	pthread_mutex_lock(&pSelf->m_outBufMutex);

                    while (pSelf->m_sOutBufList.nSizeOfList > 0)
                    {
                    	pSelf->m_sOutBufList.nSizeOfList--;
                    	pSelf->m_Callbacks.FillBufferDone(&pSelf->m_cmp,
                    			                           pSelf->m_pAppData,
                    			                           pSelf->m_sOutBufList.pBufHdrList[pSelf->m_sOutBufList.nReadPos++]);

                    	if (pSelf->m_sOutBufList.nReadPos >= pSelf->m_sOutBufList.nBufArrSize)
                    		pSelf->m_sOutBufList.nReadPos = 0;
                    }

                	pthread_mutex_unlock(&pSelf->m_outBufMutex);

					pSelf->m_Callbacks.EventHandler(&pSelf->m_cmp, pSelf->m_pAppData, OMX_EventCmdComplete, OMX_CommandFlush, 0x1, NULL);
		        }
	        }
	        else if (cmd == Stop)
	        {
	        	ALOGD(" stop command.");
                post_message_to_vdrv(pSelf, OMX_VdrvCommand_Stop);
                ALOGD("(f:%s, l:%d) wait for OMX_VdrvCommand_Stop", __FUNCTION__, __LINE__);
                sem_wait(&pSelf->m_vdrv_cmd_lock);
                ALOGD("(f:%s, l:%d) wait for OMX_VdrvCommand_Stop done!", __FUNCTION__, __LINE__);
  		        // Kill thread
 	            goto EXIT;
	        }
	        else if (cmd == FillBuf)
	        {
	            // Fill buffer
	        	pthread_mutex_lock(&pSelf->m_outBufMutex);
	        	//if (pSelf->m_sOutBufList.nSizeOfList <= pSelf->m_sOutBufList.nAllocSize)
	        	if (pSelf->m_sOutBufList.nSizeOfList < pSelf->m_sOutBufList.nAllocSize)
	        	{
	        		pSelf->m_sOutBufList.nSizeOfList++;
	        		pSelf->m_sOutBufList.pBufHdrList[pSelf->m_sOutBufList.nWritePos++] = ((OMX_BUFFERHEADERTYPE*) cmddata);

	                if (pSelf->m_sOutBufList.nWritePos >= (OMX_S32)pSelf->m_sOutBufList.nAllocSize)
	                	pSelf->m_sOutBufList.nWritePos = 0;
	        	}
	        	pthread_mutex_unlock(&pSelf->m_outBufMutex);
            }
	        else if (cmd == EmptyBuf)
	        {
                OMX_BUFFERHEADERTYPE* pTmpInBufHeader = (OMX_BUFFERHEADERTYPE*) cmddata;
                OMX_TICKS   nInterval;
	            // Empty buffer
	    	    pthread_mutex_lock(&pSelf->m_inBufMutex);
                if(0 == pTmpInBufHeader->nTimeStamp)
                {
                    ALOGV("why cur vbs input nTimeStamp=0? prevPts=%lld", pSelf->m_nInportPrevTimeStamp);
                }
                else
                {
                    if(pSelf->m_nInportPrevTimeStamp)
                    {
                        //compare pts to decide if jump
                        nInterval = pTmpInBufHeader->nTimeStamp - pSelf->m_nInportPrevTimeStamp;
                        if(nInterval < -AWOMX_PTS_JUMP_THRESH || nInterval > AWOMX_PTS_JUMP_THRESH)
                        {
                            ALOGD("pts jump [%lld]us, prev[%lld],cur[%lld],need reset vdeclib", nInterval, pSelf->m_nInportPrevTimeStamp, pTmpInBufHeader->nTimeStamp);
                            pSelf->m_JumpFlag = OMX_TRUE;
                        }

                        pSelf->m_nInportPrevTimeStamp = pTmpInBufHeader->nTimeStamp;
                    }
                    else //first InBuf data
                    {
                        pSelf->m_nInportPrevTimeStamp = pTmpInBufHeader->nTimeStamp;
                    }
                }

	        	//if (pSelf->m_sInBufList.nSizeOfList <= pSelf->m_sInBufList.nAllocSize)
                if (pSelf->m_sInBufList.nSizeOfList < pSelf->m_sInBufList.nAllocSize)
	        	{
	        		pSelf->m_sInBufList.nSizeOfList++;
	        		pSelf->m_sInBufList.pBufHdrList[pSelf->m_sInBufList.nWritePos++] = ((OMX_BUFFERHEADERTYPE*) cmddata);

	                if (pSelf->m_sInBufList.nWritePos >= (OMX_S32)pSelf->m_sInBufList.nAllocSize)
	                	pSelf->m_sInBufList.nWritePos = 0;
	        	}
                ALOGV("(omx_vdec, f:%s, l:%d) nTimeStamp[%lld], nAllocLen[%d], nFilledLen[%d], nOffset[%d], nFlags[0x%x], nOutputPortIndex[%d], nInputPortIndex[%d]", __FUNCTION__, __LINE__,
                    pTmpInBufHeader->nTimeStamp, pTmpInBufHeader->nAllocLen, pTmpInBufHeader->nFilledLen, pTmpInBufHeader->nOffset, pTmpInBufHeader->nFlags, pTmpInBufHeader->nOutputPortIndex, pTmpInBufHeader->nInputPortIndex);
	    	    pthread_mutex_unlock(&pSelf->m_inBufMutex);
        	    // Mark current buffer if there is outstanding command
		        if (pMarkBuf)
		        {
		            ((OMX_BUFFERHEADERTYPE *)(cmddata))->hMarkTargetComponent = &pSelf->m_cmp;
		            ((OMX_BUFFERHEADERTYPE *)(cmddata))->pMarkData = pMarkBuf->pMarkData;
		            pMarkBuf = NULL;
		        }
		    }
	        else if (cmd == MarkBuf)
	        {
	            if (!pMarkBuf)
	                pMarkBuf = (OMX_MARKTYPE *)(cmddata);
	        }
            else if(cmd == VdrvNotifyEos)
            {
                nVdrvNotifyEosFlag = 1;
            }
            else
            {
                ALOGW("(f:%s, l:%d) ", __FUNCTION__, __LINE__);
            }
	    }

        // Buffer processing
        // Only happens when the component is in executing state.
        if (pSelf->m_state == OMX_StateExecuting  &&
        	pSelf->m_sInPortDef.bEnabled          &&
        	pSelf->m_sOutPortDef.bEnabled         &&
        	port_setting_match)
        {
            if(OMX_TRUE == pSelf->m_JumpFlag)
            {
                ALOGD("reset vdeclib for jump!");
                //pSelf->m_decoder->ioctrl(pSelf->m_decoder, CEDARV_COMMAND_JUMP, 0);
                pSelf->m_JumpFlag = OMX_FALSE;
            }
            //fill buffer first
       		while(0==pSelf->m_decoder->picture_ready(pSelf->m_decoder) && pSelf->m_sInBufList.nSizeOfList>0)
    		{
            	u8* pBuf0;
            	u8* pBuf1;
            	u32 size0;
            	u32 size1;
            	u32 require_size;
            	u8* pData;
            	cedarv_stream_data_info_t dataInfo;


    			pInBufHdr = pSelf->m_sInBufList.pBufHdrList[pSelf->m_sInBufList.nReadPos];

                if(pInBufHdr == NULL)
                {
                    ALOGD("(f:%s, l:%d) fatal error! pInBufHdr is NULL, check code!", __FUNCTION__, __LINE__);
                    //usleep(20*1000);
                	break;
                }

                //* add stream to decoder.
                require_size = pInBufHdr->nFilledLen;
                //DBG_WARNING("vbs data size[%d]", require_size);
                if(pSelf->m_decoder->request_write(pSelf->m_decoder, require_size, &pBuf0, &size0, &pBuf1, &size1) != 0)
                {
                    ALOGV("(f:%s, l:%d)req vbs fail! maybe vbs buffer is full! require_size[%d]", __FUNCTION__, __LINE__, require_size);
    				//* report a fatal error.
                    //pSelf->m_Callbacks.EventHandler(&pSelf->m_cmp, pSelf->m_pAppData, OMX_EventError, OMX_ErrorHardware, 0, NULL);
                    break;
                }
                //DBG_WARNING("req vbs size0[%d], size1[%d]!", size0, size1);
                dataInfo.lengh = require_size;
                dataInfo.flags = CEDARV_FLAG_FIRST_PART | CEDARV_FLAG_LAST_PART;
                if(pInBufHdr->nTimeStamp >= 0)
                {
                	dataInfo.pts = pInBufHdr->nTimeStamp;
                	dataInfo.flags |= CEDARV_FLAG_PTS_VALID;
                }

                if(require_size <= size0)
                {
                	pData = pInBufHdr->pBuffer + pInBufHdr->nOffset;
                	memcpy(pBuf0, pData, require_size);
                }
                else
                {
                	pData = pInBufHdr->pBuffer + pInBufHdr->nOffset;
                	memcpy(pBuf0, pData, size0);
                	pData += size0;
                	memcpy(pBuf1, pData, require_size - size0);
                }

                pSelf->m_decoder->update_data(pSelf->m_decoder, &dataInfo);
                //DBG_WARNING("EmptyBufferDone is called");
                pSelf->m_Callbacks.EmptyBufferDone(&pSelf->m_cmp, pSelf->m_pAppData, pInBufHdr);
                //notify vdrvThread vbs input
                nRetSemGetValue=sem_getvalue(&pSelf->m_sem_vbs_input, &nSemVal);
                if(0 == nRetSemGetValue)
                {
                    if(0 == nSemVal)
                    {
                        sem_post(&pSelf->m_sem_vbs_input);
                    }
                    else
                    {
                        //ALOGD("(f:%s, l:%d) Be careful, sema vbs_input[%d]!=0", __FUNCTION__, __LINE__, nSemVal);
                    }
                }
                else
                {
                    ALOGW("(f:%s, l:%d) fatal error, why sem getvalue of m_sem_vbs_input[%d] fail!", __FUNCTION__, __LINE__, nRetSemGetValue);
                }

      	        // Check for EOS flag
                if (pInBufHdr)
                {
                    if (pInBufHdr->nFlags & OMX_BUFFERFLAG_EOS)
                    {
    	                // Copy flag to output buffer header
                    	eos = OMX_TRUE;
                        post_message_to_vdrv(pSelf, OMX_VdrvCommand_EndOfStream);
                        sem_post(&pSelf->m_sem_vbs_input);
                        sem_post(&pSelf->m_sem_frame_output);
                        ALOGD("(f:%s, l:%d) wait for OMX_VdrvCommand_EndOfStream", __FUNCTION__, __LINE__);
                        sem_wait(&pSelf->m_vdrv_cmd_lock);
                        ALOGD("(f:%s, l:%d) wait for OMX_VdrvCommand_EndOfStream done!", __FUNCTION__, __LINE__);

        	        	ALOGD(" set up eos flag.");

     	                // Trigger event handler
                        pSelf->m_Callbacks.EventHandler(&pSelf->m_cmp, pSelf->m_pAppData, OMX_EventBufferFlag, 0x1, pInBufHdr->nFlags, NULL);

     		            // Clear flag
    		            pInBufHdr->nFlags = 0;
    	            }

    	            // Check for mark buffers
    	            if (pInBufHdr->pMarkData)
    	            {
                        // Copy mark to output buffer header
    	                if (pOutBufHdr)
    	                {
    	                    pMarkData = pInBufHdr->pMarkData;
                            // Copy handle to output buffer header
                            hMarkTargetComponent = pInBufHdr->hMarkTargetComponent;
                        }
     		        }

    		        // Trigger event handler
    	            if (pInBufHdr->hMarkTargetComponent == &pSelf->m_cmp && pInBufHdr->pMarkData)
    	            	pSelf->m_Callbacks.EventHandler(&pSelf->m_cmp, pSelf->m_pAppData, OMX_EventMark, 0, 0, pInBufHdr->pMarkData);
                }

                //* check if there is a input bit stream.
                pthread_mutex_lock(&pSelf->m_inBufMutex);

            	pSelf->m_sInBufList.nSizeOfList--;
                pSelf->m_sInBufList.nReadPos++;
            	if (pSelf->m_sInBufList.nReadPos >= (OMX_S32)pSelf->m_sInBufList.nAllocSize)
            		pSelf->m_sInBufList.nReadPos = 0;

                pthread_mutex_unlock(&pSelf->m_inBufMutex);

    		}
        	//* 1. check if there is a picture to output.
        	if(pSelf->m_decoder->picture_ready(pSelf->m_decoder))
        	{
        		int width_align;
        		int height_align;
                int nGpuBufWidth;
                int nGpuBufHeight;
                //DBG_WARNING("====HasPic====");
            	//* check if the picture size changed.
            	pSelf->m_decoder->display_dump_picture(pSelf->m_decoder, &picture);

            	//height_align = (picture.display_height + 7) & (~7);
				//width_align  = (picture.display_width + 15) & (~15);
                height_align = picture.display_height;
				width_align  = picture.display_width;
                nGpuBufWidth = (picture.display_width + 15) & ~15;
                nGpuBufHeight = picture.display_height;
                //#if (defined(__CHIP_VERSION_F23) || defined(__CHIP_VERSION_F51) || defined(__CHIP_VERSION_F50))
                if(nGpuBufHeight%2!=0)
                {
                    ALOGW("Be careful! why frame height[%d] is odd number! add one!", nGpuBufHeight);
                    nGpuBufHeight+=1;
                }
            #if (1 == ADAPT_A10_GPU_RENDER)
                //A10's GPU has a bug, we can avoid it
                if(nGpuBufHeight%8 != 0)
                {
                    if((nGpuBufWidth*nGpuBufHeight)%256 != 0)
                    {
                        nGpuBufHeight = (nGpuBufHeight+7)&~7;
                    }
                }
            #endif
                height_align = nGpuBufHeight;
                //#endif

            	if((OMX_U32)width_align != pSelf->m_sOutPortDef.format.video.nFrameWidth || (OMX_U32)height_align != pSelf->m_sOutPortDef.format.video.nFrameHeight)
            	{
            		ALOGW(" port setting changed., org_height = %d, org_width = %d, new_height = %d, new_width = %d.",
            				(int)pSelf->m_sOutPortDef.format.video.nFrameHeight,
            				(int)pSelf->m_sOutPortDef.format.video.nFrameWidth,
            				(int)height_align, (int)width_align);

            		pSelf->m_sOutPortDef.format.video.nFrameHeight = height_align;
            		pSelf->m_sOutPortDef.format.video.nFrameWidth = width_align;
                    pSelf->m_sOutPortDef.nBufferSize = height_align*width_align *3/2;
            		port_setting_match = OMX_FALSE;
					pSelf->m_Callbacks.EventHandler(&pSelf->m_cmp, pSelf->m_pAppData, OMX_EventPortSettingsChanged, 0x01/*pSelf->m_sOutPortDef.nPortIndex*/, 0, NULL);
					continue;
            	}


                pthread_mutex_lock(&pSelf->m_outBufMutex);

                if(pSelf->m_sOutBufList.nSizeOfList > 0)
                {
                	pSelf->m_sOutBufList.nSizeOfList--;
                	pOutBufHdr = pSelf->m_sOutBufList.pBufHdrList[pSelf->m_sOutBufList.nReadPos++];
                	if (pSelf->m_sOutBufList.nReadPos >= (OMX_S32)pSelf->m_sOutBufList.nAllocSize)
                		pSelf->m_sOutBufList.nReadPos = 0;
                }
                else
                	pOutBufHdr = NULL;

                pthread_mutex_unlock(&pSelf->m_outBufMutex);

                //* if no output buffer, wait for some time.
            	if(pOutBufHdr == NULL)
            	{
//                    int64_t tm1, tm2, tm3;
//                    tm1 = OMX_GetNowUs();
                    if(0 == waitPipeDataToRead(pSelf->m_cmdpipe[0], 40*1000))
                    {
//                        tm2 = OMX_GetNowUs();
//                        ALOGD("(f:%s, l:%d) no output frame, wait 40ms for msg, interval=[%lld]ms", __FUNCTION__, __LINE__, (tm2-tm1)/1000);
                    }
                    else
                    {
//                        tm3 = OMX_GetNowUs();
//                        ALOGD("(f:%s, l:%d) no output frame, wait <40ms for msg coming, interval=[%lld]ms", __FUNCTION__, __LINE__, (tm3-tm1)/1000);
                    }
                    continue;
                    //goto __ProcessInPortBuf;
            	}

            	pSelf->m_decoder->display_request(pSelf->m_decoder, &picture);
                convertAddress_Phy2Vir(&picture);

            	//* transform picture color format.

            	if(pSelf->m_useAndroidBuffer == OMX_FALSE)
            	{
            		//ALOGD("(omx_vdec, f:%s, l:%d) m_useAndroidBuffer==OMX_FALSE", __FUNCTION__, __LINE__);
                	TransformToYUVPlaner(&picture, pOutBufHdr->pBuffer,0);
            	}
            	else
            	{
                    //ALOGD("(omx_vdec, f:%s, l:%d) m_useAndroidBuffer==OMX_TRUE", __FUNCTION__, __LINE__);
                  #if (OPEN_STATISTICS)
                    nTimeUs1 = OMX_GetNowUs();
                  #endif
                	void* dst;

                	android::GraphicBufferMapper &mapper = android::GraphicBufferMapper::get();

                	//android::Rect bounds(picture.width, picture.height);
                	//when lock gui buffer, we must according gui buffer's width and height, not by decoder!
                    android::Rect bounds(pSelf->m_sOutPortDef.format.video.nFrameWidth, pSelf->m_sOutPortDef.format.video.nFrameHeight);
                    if(0 != mapper.lock((buffer_handle_t)pOutBufHdr->pBuffer, GRALLOC_USAGE_SW_WRITE_OFTEN, bounds, &dst))
                    {
                        ALOGW("Lock GUIBuf fail!");
                    }
                	TransformToYUVPlaner(&picture, dst,1);
                    if(0 != mapper.unlock((buffer_handle_t)pOutBufHdr->pBuffer))
                    {
                        ALOGW("Unlock GUIBuf fail!");
                    }
                  #if (OPEN_STATISTICS)
                    nTimeUs2 = OMX_GetNowUs();
                    pSelf->mConvertTotalDuration += (nTimeUs2-nTimeUs1);
                    pSelf->mConvertTotalCount++;
                  #endif
            	}

            	pOutBufHdr->nTimeStamp = picture.pts;
            	//pOutBufHdr->nFilledLen = picture.display_width*picture.display_height*3/2;
                pOutBufHdr->nFilledLen = pSelf->m_sOutPortDef.format.video.nFrameWidth * pSelf->m_sOutPortDef.format.video.nFrameHeight * 3 / 2;
            	pOutBufHdr->nOffset    = 0;

                pSelf->m_decoder->display_release(pSelf->m_decoder, picture.id);
                //notify vdrvThread free frame
                nRetSemGetValue=sem_getvalue(&pSelf->m_sem_frame_output, &nSemVal);
                if(0 == nRetSemGetValue)
                {
                    if(0 == nSemVal)
                    {
                        sem_post(&pSelf->m_sem_frame_output);
                    }
                    else
                    {
                        //ALOGD("(f:%s, l:%d) Be careful, sema frame_output[%d]!=0", __FUNCTION__, __LINE__, nSemVal);
                    }
                }
                else
                {
                    ALOGW("(f:%s, l:%d) fatal error, why sem getvalue of m_sem_frame_output[%d] fail!", __FUNCTION__, __LINE__, nRetSemGetValue);
                }

                if(eos && nVdrvNotifyEosFlag)
                {
                	if(!pSelf->m_decoder->picture_ready(pSelf->m_decoder))
                	{
                        ALOGD(" set up eos flag to picture.");
                		pOutBufHdr->nFlags |= OMX_BUFFERFLAG_EOS;;
                	}
                }

	            // Check for mark buffers
	            if (pMarkData != NULL && hMarkTargetComponent != NULL)
	            {
                	if(!pSelf->m_decoder->picture_ready(pSelf->m_decoder))
                	{
                		// Copy mark to output buffer header
                		pOutBufHdr->pMarkData = pInBufHdr->pMarkData;
                		// Copy handle to output buffer header
                		pOutBufHdr->hMarkTargetComponent = pInBufHdr->hMarkTargetComponent;

                		pMarkData = NULL;
                		hMarkTargetComponent = NULL;
                	}
 		        }
                ALOGV("(omx_vdec, f:%s, l:%d) FillBufferDone is called, nSizeOfList[%d], pts[%lld], nAllocLen[%d], nFilledLen[%d], nOffset[%d], nFlags[0x%x], nOutputPortIndex[%d], nInputPortIndex[%d]", __FUNCTION__, __LINE__,
                    pSelf->m_sOutBufList.nSizeOfList, pOutBufHdr->nTimeStamp, pOutBufHdr->nAllocLen, pOutBufHdr->nFilledLen, pOutBufHdr->nOffset, pOutBufHdr->nFlags, pOutBufHdr->nOutputPortIndex, pOutBufHdr->nInputPortIndex);
                pSelf->m_Callbacks.FillBufferDone(&pSelf->m_cmp, pSelf->m_pAppData, pOutBufHdr);
                pOutBufHdr = NULL;

                continue;
        	}
            else
            {
                if(nVdrvNotifyEosFlag)
                {
                    if(eos!=OMX_TRUE)
                    {
                        ALOGW("fatal error! why eos[%d], nVdrvNotifyEosFlag[%d]?", eos, nVdrvNotifyEosFlag);
                    }
                    //set eof flag, MediaCodec use this flag
                	//to determine whether a playback is finished.
                	pthread_mutex_lock(&pSelf->m_outBufMutex);
                	while(pSelf->m_sOutBufList.nSizeOfList > 0)
                	{
                		pSelf->m_sOutBufList.nSizeOfList--;
                		pOutBufHdr = pSelf->m_sOutBufList.pBufHdrList[pSelf->m_sOutBufList.nReadPos++];
                		if (pSelf->m_sOutBufList.nReadPos >= (OMX_S32)pSelf->m_sOutBufList.nAllocSize)
                			pSelf->m_sOutBufList.nReadPos = 0;
                		CHECK(pOutBufHdr != NULL);
                        if(pSelf->m_sOutBufList.nSizeOfList == 0)
                        {
                            ALOGD("(f:%s, l:%d), EOS msg send with FillBufferDone", __FUNCTION__, __LINE__);
                    		pOutBufHdr->nFlags |= OMX_BUFFERFLAG_EOS;
                        }
                		pSelf->m_Callbacks.FillBufferDone(&pSelf->m_cmp, pSelf->m_pAppData, pOutBufHdr);
                		pOutBufHdr = NULL;
                	}
                	pthread_mutex_unlock(&pSelf->m_outBufMutex);
                }
//                int64_t tm1, tm2, tm3;
//                tm1 = OMX_GetNowUs();
                if(0 == waitPipeDataToRead(pSelf->m_cmdpipe[0], 20*1000))
                {
//                    tm2 = OMX_GetNowUs();
//                    ALOGD("(f:%s, l:%d) no vdeclib frame, wait 20ms for msg, interval=[%lld]ms", __FUNCTION__, __LINE__, (tm2-tm1)/1000);
                }
                else
                {
//                    tm3 = OMX_GetNowUs();
//                    ALOGD("(f:%s, l:%d) no vdeclib frame, wait <20ms for msg coming, interval=[%lld]ms", __FUNCTION__, __LINE__, (tm3-tm1)/1000);
                }
            }
        }
    }

EXIT:
    return (void*)OMX_ErrorNone;
}

static void* ComponentVdrvThread(void* pThreadData)
{
	int                     decodeResult;
    //cedarv_picture_t        picture;

    int                     i;
    int                     fd1;
    fd_set                  rfds;
    //OMX_U32                 cmddata;
    OMX_VDRV_COMMANDTYPE    cmd;

    // Variables related to decoder buffer handling
    //OMX_BUFFERHEADERTYPE*   pInBufHdr   = NULL;
    //OMX_BUFFERHEADERTYPE*   pOutBufHdr  = NULL;
    //OMX_MARKTYPE*           pMarkBuf    = NULL;
    //OMX_U8*                 pInBuf      = NULL;
    //OMX_U32                 nInBufSize;

    // Variables related to decoder timeouts
    //OMX_U32                 nTimeout;
    //OMX_BOOL                port_setting_match;
    OMX_BOOL                nEosFlag = OMX_FALSE;

    //OMX_PTR                 pMarkData;
    //OMX_HANDLETYPE          hMarkTargetComponent;

    struct timeval          timeout;

    //port_setting_match   = OMX_TRUE;
    //pMarkData            = NULL;
    //hMarkTargetComponent = NULL;

    int64_t         nTimeUs1;
    int64_t         nTimeUs2;

    int nSemVal;
    int nRetSemGetValue;
    int nStopFlag = 0;
    // Recover the pointer to my component specific data
    omx_vdec* pSelf = (omx_vdec*)pThreadData;


    while (1)
    {
        fd1 = pSelf->m_vdrv_cmdpipe[0];
        FD_ZERO(&rfds);
        FD_SET(fd1, &rfds);

        // Check for new command
        timeout.tv_sec  = 0;
        timeout.tv_usec = 0;

        i = select(pSelf->m_vdrv_cmdpipe[0]+1, &rfds, NULL, NULL, &timeout);

        if (FD_ISSET(pSelf->m_vdrv_cmdpipe[0], &rfds))
        {
            // retrieve command and data from pipe
            read(pSelf->m_vdrv_cmdpipe[0], &cmd, sizeof(cmd));
            ALOGD("(f:%s, l:%d) vdrvThread receive cmd[0x%x]", __FUNCTION__, __LINE__, cmd);
            // State transition command
	        if (cmd == OMX_VdrvCommand_PrepareVdecLib)  //now omx_vdec's m_state = OMX_StateLoaded, OMX_StateLoaded->OMX_StateIdle
	        {
	        	ALOGD("(f:%s, l:%d)(OMX_VdrvCommand_PrepareVdecLib)", __FUNCTION__, __LINE__);
                // If the parameter states a transition to the same state
                // raise a same state transition error.
                if (pSelf->m_state != OMX_StateLoaded)
                {
                	ALOGW("fatal error! when vdrv OMX_VdrvCommand_PrepareVdecLib, m_state[0x%x] should be OMX_StateLoaded", pSelf->m_state);
                }
                int result;

            	//* set video stream info.
            	pSelf->m_streamInfo.is_pts_correct = 1;
            	if(pSelf->m_decoder->set_vstream_info(pSelf->m_decoder, &(pSelf->m_streamInfo)) != 0)
            	{
                	pSelf->m_Callbacks.EventHandler(&pSelf->m_cmp, pSelf->m_pAppData, OMX_EventError, OMX_ErrorHardware, 0 , NULL);
	                ALOGW("Idle transition failed, set_vstream_info() return fail.\n");
	                //break;
                    goto __EXECUTE_CMD_DONE;
            	}


				if (ve_mutex_lock(&pSelf->m_cedarv_req_ctx) < 0) {
                //if (RequestCedarVFrameLevel(&pSelf->m2_cedarv_req_ctx) < 0) {

					ALOGW("reqest ve resource failed\n");
					//break;
					goto __EXECUTE_CMD_DONE;
				}
//                ALOGD("set max_output width[%d], height[%d]", 2048, 1288);
//                pSelf->m_decoder->ioctrl(pSelf->m_decoder, CEDARV_COMMAND_SET_MAX_OUTPUT_WIDTH, 2048);  //4096, 2048
//        		pSelf->m_decoder->ioctrl(pSelf->m_decoder, CEDARV_COMMAND_SET_MAX_OUTPUT_HEIGHT, 1288); //2304, 1288
            	//* open and start decoder.

				result = pSelf->m_decoder->open(pSelf->m_decoder);
            	if(result != 0)
            	{
            		ve_mutex_unlock(&pSelf->m_cedarv_req_ctx);
                    //ReleaseCedarVFrameLevel(&pSelf->m2_cedarv_req_ctx);

                	pSelf->m_Callbacks.EventHandler(&pSelf->m_cmp, pSelf->m_pAppData, OMX_EventError, OMX_ErrorHardware, 0 , NULL);
	                ALOGW("Idle transition failed, open decoder return fail, result: %d\n", result);
	                //break;
	                goto __EXECUTE_CMD_DONE;
            	}

            	pSelf->m_decoder->ioctrl(pSelf->m_decoder, CEDARV_COMMAND_PLAY, 0);

				ve_mutex_unlock(&pSelf->m_cedarv_req_ctx);
                //ReleaseCedarVFrameLevel(&pSelf->m2_cedarv_req_ctx);
	        }
	        else if (cmd == OMX_VdrvCommand_CloseVdecLib)   // now omx_vdec's m_state = OMX_StateLoaded, OMX_StateIdle->OMX_StateLoaded
	        {
                ALOGD("(f:%s, l:%d)(OMX_VdrvCommand_CloseVdecLib)", __FUNCTION__, __LINE__);
	            //* stop and close decoder.
                if(pSelf->m_decoder != NULL)
                {
    		        if (ve_mutex_lock(&pSelf->m_cedarv_req_ctx) < 0)
                    //if (RequestCedarVFrameLevel(&pSelf->m2_cedarv_req_ctx) < 0)
    				{
    					 ALOGW("Request VE failed\n");
    					 //break;
    					 goto __EXECUTE_CMD_DONE;
    				}

                	pSelf->m_decoder->ioctrl(pSelf->m_decoder, CEDARV_COMMAND_STOP, 0);
                	pSelf->m_decoder->close(pSelf->m_decoder);

    				ve_mutex_unlock(&pSelf->m_cedarv_req_ctx);
                    //ReleaseCedarVFrameLevel(&pSelf->m2_cedarv_req_ctx);
                }
	        }

	        else if (cmd == OMX_VdrvCommand_FlushVdecLib)
	        {
                ALOGD("(f:%s, l:%d)(OMX_VdrvCommand_FlushVdecLib)", __FUNCTION__, __LINE__);
                if(pSelf->m_decoder)
                {
    				if (ve_mutex_lock(&pSelf->m_cedarv_req_ctx) < 0) {

						ALOGW("reqest ve resource failed\n");
						//break;
						goto __EXECUTE_CMD_DONE;
					}
                    pSelf->m_decoder->ioctrl(pSelf->m_decoder, CEDARV_COMMAND_JUMP, 0);

					ve_mutex_unlock(&pSelf->m_cedarv_req_ctx);
                }
                else
                {
                    ALOGW(" fatal error, m_decoder is not malloc when flush all ports!");
                }
	        }
	        else if (cmd == OMX_VdrvCommand_Stop)
	        {
                ALOGD("(f:%s, l:%d)(OMX_VdrvCommand_Stop)", __FUNCTION__, __LINE__);
                nStopFlag = 1;
	        }
	        else if (cmd == OMX_VdrvCommand_EndOfStream)
	        {
                ALOGD("(f:%s, l:%d)(OMX_VdrvCommand_EndOfStream)", __FUNCTION__, __LINE__);
                nEosFlag = OMX_TRUE;
	        }
	        else
	        {
                ALOGW("(f:%s, l:%d)fatal error! unknown OMX_VDRV_COMMANDTYPE[0x%x]", __FUNCTION__, __LINE__, cmd);
	        }
__EXECUTE_CMD_DONE:
            nRetSemGetValue=sem_getvalue(&pSelf->m_vdrv_cmd_lock, &nSemVal);
            if(0 == nRetSemGetValue)
            {
                if(0 == nSemVal)
                {
                    sem_post(&pSelf->m_vdrv_cmd_lock);
                }
                else
                {
                    ALOGW("(f:%s, l:%d) fatal error, why nSemVal[%d]!=0", __FUNCTION__, __LINE__, nSemVal);
                }
            }
            else
            {
                ALOGW("(f:%s, l:%d) fatal error, why sem getvalue[%d] fail!", __FUNCTION__, __LINE__, nRetSemGetValue);
            }
	    }
        if(nStopFlag)
        {
            ALOGD("vdrvThread detect nStopFlag[%d], exit!", nStopFlag);
            goto EXIT;
        }
        // Buffer processing
        // Only happens when the component is in executing state.
        if (pSelf->m_state == OMX_StateExecuting || pSelf->m_state == OMX_StatePause)
        {
        	//* 1. check if there is a picture to output.
			if (ve_mutex_lock(&pSelf->m_cedarv_req_ctx) < 0) {
            //if (RequestCedarVFrameLevel(&pSelf->m2_cedarv_req_ctx) < 0) {
			    ALOGW("ve_mutex_lock fail!");
				usleep(5*1000);
				continue;
			}
          #if (OPEN_STATISTICS)
			nTimeUs1 = OMX_GetNowUs();
          #endif
			decodeResult = pSelf->m_decoder->decode(pSelf->m_decoder);
          #if (OPEN_STATISTICS)
            nTimeUs2 = OMX_GetNowUs();
            if(decodeResult == CEDARV_RESULT_FRAME_DECODED || decodeResult == CEDARV_RESULT_KEYFRAME_DECODED)
            {
                pSelf->mDecodeFrameTotalDuration += (nTimeUs2-nTimeUs1);
                pSelf->mDecodeFrameTotalCount++;
            }
            else if(decodeResult == CEDARV_RESULT_OK)
            {
                pSelf->mDecodeOKTotalDuration += (nTimeUs2-nTimeUs1);
                pSelf->mDecodeOKTotalCount++;
            }
            else if(decodeResult == CEDARV_RESULT_NO_FRAME_BUFFER)
            {
                pSelf->mDecodeNoFrameTotalDuration += (nTimeUs2-nTimeUs1);
                pSelf->mDecodeNoFrameTotalCount++;
            }
            else if(decodeResult == CEDARV_RESULT_NO_BITSTREAM)
            {
                pSelf->mDecodeNoBitstreamTotalDuration += (nTimeUs2-nTimeUs1);
                pSelf->mDecodeNoBitstreamTotalCount++;
            }
            else
            {
                pSelf->mDecodeOtherTotalDuration += (nTimeUs2-nTimeUs1);
                pSelf->mDecodeOtherTotalCount++;
            }
          #endif
			ve_mutex_unlock(&pSelf->m_cedarv_req_ctx);
            //ReleaseCedarVFrameLevel(&pSelf->m2_cedarv_req_ctx);

			if(decodeResult == CEDARV_RESULT_FRAME_DECODED ||
			   decodeResult == CEDARV_RESULT_KEYFRAME_DECODED ||
			   decodeResult == CEDARV_RESULT_OK)
			{
			    //break;
			}
            else if(decodeResult == CEDARV_RESULT_NO_FRAME_BUFFER)
            {
                ALOGV("(f:%s, l:%d) no_frame_buffer, wait!", __FUNCTION__, __LINE__);
                sem_wait(&pSelf->m_sem_frame_output);
                ALOGV("(f:%s, l:%d) no_frame_buffer, wait_done!", __FUNCTION__, __LINE__);
            }
			else if(decodeResult == CEDARV_RESULT_NO_BITSTREAM)
			{
                if(nEosFlag)
                {
                	//set eof flag, MediaCodec use this flag
                	//to determine whether a playback is finished.
                	ALOGD("(f:%s, l:%d) when eos, vdrvtask meet no_bitstream, all frames have decoded, notify ComponentThread eos!", __FUNCTION__, __LINE__);
                    pSelf->send_vdrv_feedback_msg(OMX_VdrvFeedbackMsg_NotifyEos, 0, NULL);
                    ALOGD("(f:%s, l:%d) no_vbsinput_buffer, eos wait!", __FUNCTION__, __LINE__);
                    sem_wait(&pSelf->m_sem_vbs_input);
                    ALOGD("(f:%s, l:%d) no_vbsinput_buffer, eos wait_done!", __FUNCTION__, __LINE__);
                }
                else
                {
                    ALOGV("(f:%s, l:%d) no_vbsinput_buffer, wait!", __FUNCTION__, __LINE__);
                    sem_wait(&pSelf->m_sem_vbs_input);
                    ALOGV("(f:%s, l:%d) no_vbsinput_buffer, wait_done!", __FUNCTION__, __LINE__);
                }
			}
			else if(decodeResult < 0)
			{
                ALOGW("decode fatal error[%d]", decodeResult);
				//* report a fatal error.
                pSelf->m_Callbacks.EventHandler(&pSelf->m_cmp, pSelf->m_pAppData, OMX_EventError, OMX_ErrorHardware, 0, NULL);
                //break;
			}
            else
            {
                ALOGD("decode ret[%d], ignore? why?", decodeResult);
            }
        }
        else
        {
//            int64_t tm1, tm2, tm3;
//            tm1 = OMX_GetNowUs();
            if(0 == waitPipeDataToRead(pSelf->m_vdrv_cmdpipe[0], 20*1000))
            {
//                tm2 = OMX_GetNowUs();
//                ALOGD("(f:%s, l:%d) vdrvThread m_state[0x%x] is not OMX_StateExecuting or OMX_StatePause, wait 20ms for msg, interval=[%lld]ms", __FUNCTION__, __LINE__, pSelf->m_state, (tm2-tm1)/1000);
            }
            else
            {
//                tm3 = OMX_GetNowUs();
//                ALOGD("(f:%s, l:%d) vdrvThread m_state[0x%x] is not OMX_StateExecuting or OMX_StatePause, wait <20ms for msg coming, interval=[%lld]ms", __FUNCTION__, __LINE__, pSelf->m_state, (tm3-tm1)/1000);
            }
        }
    }

EXIT:
    return (void*)OMX_ErrorNone;
}

