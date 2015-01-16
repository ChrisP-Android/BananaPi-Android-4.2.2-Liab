

/*============================================================================
                            O p e n M A X   w r a p p e r s
                             O p e n  M A X   C o r e

*//** @file omx_venc.cpp
  This module contains the implementation of the OpenMAX core & component.

*//*========================================================================*/

//////////////////////////////////////////////////////////////////////////////
//                             Include Files
//////////////////////////////////////////////////////////////////////////////

//#define LOG_NDEBUG 0
#define LOG_TAG "omx_venc"
#include <utils/Log.h>
#include "AWOmxDebug.h"

#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>
#include "omx_venc.h"
#include <fcntl.h>
#include "AWOMX_VideoIndexExtension.h"
#include <utils/CallStack.h>

using namespace android;

#define DEFAULT_BITRATE 1024*1024*2

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
  {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel41},
  {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel42},
  {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel5},
  {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel51},
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


typedef enum VIDENC_CUSTOM_INDEX
{
    VideoEncodeCustomParamStoreMetaDataInBuffers = OMX_IndexVendorStartUnused,
} VIDENC_CUSTOM_INDEX;

typedef enum ENC_INPUT_STEP
{
   STEP_QEQUEST_INPUT_BUFFER,
   STEP_ENCODE
} ENC_INPUT_STEP;


static VIDDEC_CUSTOM_PARAM sVideoEncCustomParams[] =
{
	{"OMX.google.android.index.storeMetaDataInBuffers", (OMX_INDEXTYPE)VideoEncodeCustomParamStoreMetaDataInBuffers}
};

typedef struct OMX_VIDEO_STOREMETADATAINBUFFERSPARAMS {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_BOOL bStoreMetaData;
} OMX_VIDEO_STOREMETADATAINBUFFERSPARAMS;


typedef struct ENCEXTRADATAINFO_t //don't touch it, because it also defined in type.h
{
	char *data;
	int length;
}ENCEXTRADATAINFO_t;

static void* ComponentThread(void* pThreadData);

//* factory function executed by the core to create instances
void *get_omx_component_factory_fn(void)
{
	ALOGD("----get_omx_component_factory_fn");
	return (new omx_venc);
}

static VENC_DEVICE * CedarvEncInit(unsigned int width, unsigned int height, unsigned int avg_bit_rate, unsigned int framerate, unsigned level)
{
	int ret = -1;

	VENC_DEVICE *pCedarV = NULL;
	
	pCedarV = H264EncInit(&ret);
	if (ret < 0)
	{
		printf("H264EncInit failed\n");
	}

	__video_encode_format_t enc_fmt;
	enc_fmt.src_width = width;
	enc_fmt.src_height = height;
	enc_fmt.width = width;
	enc_fmt.height = height;
	enc_fmt.frame_rate = framerate * 1000;
	enc_fmt.color_format = PIXEL_YUV420;
	enc_fmt.color_space = BT601;
	enc_fmt.qp_max = 40;
	enc_fmt.qp_min = 20;
	enc_fmt.avg_bit_rate = avg_bit_rate;
	
	enc_fmt.maxKeyInterval = 25;

    //enc_fmt.profileIdc = 77; /* main profile */
	enc_fmt.profileIdc = 66; /* baseline profile */
	enc_fmt.levelIdc = 31;
	enc_fmt.levelIdc = level;

	pCedarV->IoCtrl(pCedarV, VENC_SET_ENC_INFO_CMD, (unsigned int)(&enc_fmt));

	ret = pCedarV->open(pCedarV);

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

//static FILE *ph264File = NULL;
omx_venc::omx_venc()
{
	m_state              = OMX_StateLoaded;
	m_cRole[0]           = 0;
	m_cName[0]           = 0;
	m_eCompressionFormat = OMX_VIDEO_CodingUnused;
	m_pAppData           = NULL;
	m_thread_id          = 0;
	m_cmdpipe[0]         = 0;
	m_cmdpipe[1]         = 0;
	m_cmddatapipe[0]     = 0;
	m_cmddatapipe[1]     = 0;
	m_pExtraData         = NULL;
	m_extraDataLength    = 0;
	m_firstFrameFlag     = OMX_FALSE;
	m_framerate          = 25;

	m_encoder = NULL;

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

	pthread_mutex_init(&m_inBufMutex, NULL);
	pthread_mutex_init(&m_outBufMutex, NULL);

    ALOGV("omx_enc Create!");
}


omx_venc::~omx_venc()
{
	OMX_S32 nIndex;
    // In case the client crashes, check for nAllocSize parameter.
    // If this is greater than zero, there are elements in the list that are not free'd.
    // In that case, free the elements.

	if(m_pExtraData)
	{
		free(m_pExtraData);
		m_pExtraData = NULL;
	}

	pthread_mutex_lock(&m_inBufMutex);

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

    if(m_encoder != NULL)
    {
    	CedarvEncExit(m_encoder);
		m_encoder = NULL;
    }

    pthread_mutex_destroy(&m_inBufMutex);
    pthread_mutex_destroy(&m_outBufMutex);

    ALOGV("~omx_dec done!");
}

OMX_ERRORTYPE omx_venc::component_init(OMX_STRING name)
{
	OMX_ERRORTYPE eRet = OMX_ErrorNone;
    int           err;
    OMX_U32       nIndex;

	ALOGV(" COMPONENT_INIT, name = %s", name);

	strncpy((char*)m_cName, name, OMX_MAX_STRINGNAME_SIZE);
	
	if(!strncmp((char*)m_cName, "OMX.allwinner.video.encoder.avc", OMX_MAX_STRINGNAME_SIZE))
	{
		strncpy((char*)m_cRole, "video_encoder.avc", OMX_MAX_STRINGNAME_SIZE);
		m_eCompressionFormat = OMX_VIDEO_CodingAVC;
	}
	else if(!strncmp((char*)m_cName, "OMX.allwinner.video.encoder.h263", OMX_MAX_STRINGNAME_SIZE))
	{
		strncpy((char*)m_cRole, "video_encoder.avc", OMX_MAX_STRINGNAME_SIZE);
		m_eCompressionFormat = OMX_VIDEO_CodingH263;
	}
	else if(!strncmp((char*)m_cName, "OMX.allwinner.video.encoder.mpeg4", OMX_MAX_STRINGNAME_SIZE))
	{
		strncpy((char*)m_cRole, "video_encoder.avc", OMX_MAX_STRINGNAME_SIZE);
		m_eCompressionFormat = OMX_VIDEO_CodingMPEG4;
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
    m_sInPortDef.nBufferSize 					 = (OMX_U32)(m_sOutPortDef.format.video.nFrameWidth*m_sOutPortDef.format.video.nFrameHeight*3/2);
    m_sInPortDef.format.video.eColorFormat 		 = OMX_COLOR_FormatYUV420SemiPlanar; //fix it later

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
    m_sOutPortDef.nBufferSize 				= OMX_VIDEO_ENC_INPUT_BUFFER_SIZE;

	m_sOutPortDef.format.video.eCompressionFormat = m_eCompressionFormat;
    m_sOutPortDef.format.video.cMIMEType 		 = (OMX_STRING)"";

    // Initialize the video compression format for input port
    OMX_CONF_INIT_STRUCT_PTR(&m_sInPortFormat, OMX_VIDEO_PARAM_PORTFORMATTYPE);
    m_sInPortFormat.nPortIndex         = 0x0;
    m_sInPortFormat.nIndex             = 0x0;
	m_sInPortFormat.eColorFormat 	   = OMX_COLOR_FormatYUV420SemiPlanar; //fix it later

    // Initialize the compression format for output port
    OMX_CONF_INIT_STRUCT_PTR(&m_sOutPortFormat, OMX_VIDEO_PARAM_PORTFORMATTYPE);
    m_sOutPortFormat.nPortIndex   = 0x1;
    m_sOutPortFormat.nIndex       = 0x0;
    m_sOutPortFormat.eCompressionFormat = m_eCompressionFormat;

    OMX_CONF_INIT_STRUCT_PTR(&m_sPriorityMgmt, OMX_PRIORITYMGMTTYPE);

    OMX_CONF_INIT_STRUCT_PTR(&m_sInBufSupplier, OMX_PARAM_BUFFERSUPPLIERTYPE );
    m_sInBufSupplier.nPortIndex = 0x0;

    OMX_CONF_INIT_STRUCT_PTR(&m_sOutBufSupplier, OMX_PARAM_BUFFERSUPPLIERTYPE );
    m_sOutBufSupplier.nPortIndex = 0x1;

	// Initalize the output bitrate
	OMX_CONF_INIT_STRUCT_PTR(&m_sOutPutBitRateType, OMX_VIDEO_PARAM_BITRATETYPE);
    m_sOutPutBitRateType.nPortIndex = 0x01;
    m_sOutPutBitRateType.eControlRate = OMX_Video_ControlRateDisable;
    m_sOutPutBitRateType.nTargetBitrate = DEFAULT_BITRATE; //default bitrate

    // Initalize the output h264param
	OMX_CONF_INIT_STRUCT_PTR(&m_sH264Type, OMX_VIDEO_PARAM_AVCTYPE);
    m_sH264Type.nPortIndex                = 0x01;
    m_sH264Type.nSliceHeaderSpacing       = 0;
    m_sH264Type.nPFrames                  = -1;
    m_sH264Type.nBFrames                  = -1;
    m_sH264Type.bUseHadamard              = OMX_TRUE; /*OMX_FALSE*/
    m_sH264Type.nRefFrames                = 1; /*-1;  */
    m_sH264Type.nRefIdx10ActiveMinus1     = -1;
    m_sH264Type.nRefIdx11ActiveMinus1     = -1;
    m_sH264Type.bEnableUEP                = OMX_FALSE;
    m_sH264Type.bEnableFMO                = OMX_FALSE;
    m_sH264Type.bEnableASO                = OMX_FALSE;
    m_sH264Type.bEnableRS                 = OMX_FALSE;
    m_sH264Type.eProfile                  = OMX_VIDEO_AVCProfileBaseline; /*0x01;*/
    m_sH264Type.eLevel                    = OMX_VIDEO_AVCLevel1; /*OMX_VIDEO_AVCLevel11;*/
    m_sH264Type.nAllowedPictureTypes      = -1;
    m_sH264Type.bFrameMBsOnly             = OMX_FALSE;
    m_sH264Type.bMBAFF                    = OMX_FALSE;
    m_sH264Type.bEntropyCodingCABAC       = OMX_FALSE;
    m_sH264Type.bWeightedPPrediction      = OMX_FALSE;
    m_sH264Type.nWeightedBipredicitonMode = -1;
    m_sH264Type.bconstIpred               = OMX_FALSE;
    m_sH264Type.bDirect8x8Inference       = OMX_FALSE;
    m_sH264Type.bDirectSpatialTemporal    = OMX_FALSE;
    m_sH264Type.nCabacInitIdc             = -1;
    m_sH264Type.eLoopFilterMode           = OMX_VIDEO_AVCLoopFilterDisable;


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


	if (ve_mutex_init(&m_cedarv_req_ctx, CEDARV_ENCODE) < 0) {
		ALOGE("cedarv_req_ctx init fail!!");
		eRet = OMX_ErrorInsufficientResources;
        goto EXIT;
	}
//	}

    // Create the pipe used to send commands to the thread
    err = pipe(m_cmdpipe);
    if (err)
    {
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

    // Create the component thread
    err = pthread_create(&m_thread_id, NULL, ComponentThread, this);
    if( err || !m_thread_id )
    {
    	eRet = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

EXIT:
    return eRet;
}


OMX_ERRORTYPE  omx_venc::get_component_version(OMX_IN OMX_HANDLETYPE hComp,
                                               OMX_OUT OMX_STRING componentName,
                                               OMX_OUT OMX_VERSIONTYPE* componentVersion,
                                               OMX_OUT OMX_VERSIONTYPE* specVersion,
                                               OMX_OUT OMX_UUIDTYPE* componentUUID)
{

	ALOGV(" COMPONENT_GET_VERSION");

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


OMX_ERRORTYPE  omx_venc::send_command(OMX_IN OMX_HANDLETYPE  hComp,
                                      OMX_IN OMX_COMMANDTYPE cmd,
                                      OMX_IN OMX_U32         param1,
                                      OMX_IN OMX_PTR         cmdData)
{
    ThrCmdType    eCmd;
    OMX_ERRORTYPE eError = OMX_ErrorNone;


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
            break;
    }

    write(m_cmdpipe[1], &eCmd, sizeof(eCmd));

    // In case of MarkBuf, the pCmdData parameter is used to carry the data.
    // In other cases, the nParam1 parameter carries the data.
    if(eCmd == MarkBuf)
        write(m_cmddatapipe[1], &cmdData, sizeof(OMX_PTR));
    else
        write(m_cmddatapipe[1], &param1, sizeof(param1));

    return eError;
}


OMX_ERRORTYPE  omx_venc::get_parameter(OMX_IN OMX_HANDLETYPE hComp,
                                       OMX_IN OMX_INDEXTYPE  paramIndex,
                                       OMX_INOUT OMX_PTR     paramData)
{
    OMX_ERRORTYPE eError = OMX_ErrorNone;


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
//          android::CallStack stack;
//          stack.update(1, 100);
//          stack.dump("get_parameter");
    		ALOGV(" COMPONENT_GET_PARAMETER: OMX_IndexParamPortDefinition");
	        if (((OMX_PARAM_PORTDEFINITIONTYPE *)(paramData))->nPortIndex == m_sInPortDef.nPortIndex)
	            memcpy(paramData, &m_sInPortDef, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
	        else if (((OMX_PARAM_PORTDEFINITIONTYPE*)(paramData))->nPortIndex == m_sOutPortDef.nPortIndex)
	        {
	            memcpy(paramData, &m_sOutPortDef, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
	            ALOGV(" width = %d, height = %d", (int)m_sOutPortDef.format.video.nFrameWidth, (int)m_sOutPortDef.format.video.nFrameHeight);
	        }
	        else
	            eError = OMX_ErrorBadPortIndex;

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
		
        case OMX_IndexParamVideoBitrate:
		{
			ALOGV(" COMPONENT_GET_PARAMETER: OMX_IndexParamVideoBitrate");
            if (((OMX_VIDEO_PARAM_BITRATETYPE*)(paramData))->nPortIndex == m_sOutPutBitRateType.nPortIndex)
            {
                memcpy(paramData,&m_sOutPutBitRateType, sizeof(OMX_VIDEO_PARAM_BITRATETYPE));
            }
            else
            {
                eError = OMX_ErrorBadPortIndex;
            }
            break;			
		}

	    case OMX_IndexParamVideoAvc:
	    {
	    	ALOGV(" COMPONENT_GET_PARAMETER: OMX_IndexParamVideoAvc");
            OMX_VIDEO_PARAM_AVCTYPE* pComponentParam = (OMX_VIDEO_PARAM_AVCTYPE*)paramData;
			
            if (pComponentParam->nPortIndex == m_sH264Type.nPortIndex)
            {
                memcpy(pComponentParam, &m_sH264Type, sizeof(OMX_VIDEO_PARAM_AVCTYPE));
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

        	pParamProfileLevel->nPortIndex = m_sOutPortDef.nPortIndex;

        	/* Choose table based on compression format */
        	switch(m_sOutPortDef.format.video.eCompressionFormat)
        	{

        	case OMX_VIDEO_CodingAVC:
        		pProfileLevel = SupportedAVCProfileLevels;
        		nNumberOfProfiles = sizeof(SupportedAVCProfileLevels) / sizeof (VIDEO_PROFILE_LEVEL_TYPE);
        		break;

        	case OMX_VIDEO_CodingH263:
        		pProfileLevel = SupportedH263ProfileLevels;
        		nNumberOfProfiles = sizeof(SupportedH263ProfileLevels) / sizeof (VIDEO_PROFILE_LEVEL_TYPE);
        		break;
        	default:
        		return OMX_ErrorBadParameter;
        	}

        	if(pParamProfileLevel->nProfileIndex >= (nNumberOfProfiles - 1))
        		return OMX_ErrorBadParameter;

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



OMX_ERRORTYPE  omx_venc::set_parameter(OMX_IN OMX_HANDLETYPE hComp, OMX_IN OMX_INDEXTYPE paramIndex,  OMX_IN OMX_PTR paramData)
{
    OMX_ERRORTYPE         eError      = OMX_ErrorNone;
    unsigned int          alignment   = 0;
    unsigned int          buffer_size = 0;
    int                   i;

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
            	ALOGV("in, nPortIndex: %d", ((OMX_PARAM_PORTDEFINITIONTYPE *)(paramData))->nBufferCountActual);
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
	        		ALOGV(" allocate %d buffers.", (int)nBufCnt);

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
            }
	        else if (((OMX_PARAM_PORTDEFINITIONTYPE *)(paramData))->nPortIndex == m_sOutPortDef.nPortIndex)
	        {
	        	ALOGV("out, nPortIndex: %d", (int)((OMX_PARAM_PORTDEFINITIONTYPE *)(paramData))->nBufferCountActual);
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
	        		ALOGV(" allocate %d buffers.", (int)nBufCnt);
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

                m_sInPortDef.nBufferSize = (OMX_U32)m_sInPortDef.format.video.nFrameWidth*m_sInPortDef.format.video.nFrameHeight*3/2;
	            ALOGV("x width = %d, height = %d", m_sOutPortDef.format.video.nFrameWidth, m_sOutPortDef.format.video.nFrameHeight);
	            m_sOutPortDef.nBufferSize = m_sOutPortDef.format.video.nFrameWidth * m_sOutPortDef.format.video.nFrameHeight * 3 / 2;
	            m_sOutPortDef.format.video.nSliceHeight = m_sOutPortDef.format.video.nFrameHeight;
	            m_sOutPortDef.format.video.nStride = m_sOutPortDef.format.video.nFrameWidth;

 				m_framerate = (m_sInPortDef.format.video.xFramerate >> 16);
				
				ALOGV("m_framerate: %d", (int)m_framerate);
	        }
	        else
	            eError = OMX_ErrorBadPortIndex;

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

    		if(!strncmp((char*)m_cName, "OMX.allwinner.video.encoder.avc", OMX_MAX_STRINGNAME_SIZE))
    		{
    			if(!strncmp((char*)comp_role->cRole, "video_encoder.avc", OMX_MAX_STRINGNAME_SIZE))
    			{
    				strncpy((char*)m_cRole,"video_encoder.avc", OMX_MAX_STRINGNAME_SIZE);
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
		
        case OMX_IndexParamVideoBitrate:
        {
			ALOGV(" COMPONENT_SET_PARAMETER: OMX_IndexParamVideoBitrate"); 
            OMX_VIDEO_PARAM_BITRATETYPE* pComponentParam = (OMX_VIDEO_PARAM_BITRATETYPE*)paramData;
            if (pComponentParam->nPortIndex == m_sOutPutBitRateType.nPortIndex)
            {
                memcpy(&m_sOutPutBitRateType,pComponentParam, sizeof(OMX_VIDEO_PARAM_BITRATETYPE));

                if(!m_sOutPutBitRateType.nTargetBitrate)
                {
                    m_sOutPutBitRateType.nTargetBitrate = DEFAULT_BITRATE;
                }
				
                m_sOutPortDef.format.video.nBitrate = m_sOutPutBitRateType.nTargetBitrate;
            }
            else
            {
                eError = OMX_ErrorBadPortIndex;
            }
            break;
        }

	    case OMX_IndexParamVideoAvc:
	    {
	    	ALOGV(" COMPONENT_SET_PARAMETER: OMX_IndexParamVideoAvc");
            OMX_VIDEO_PARAM_AVCTYPE* pComponentParam = (OMX_VIDEO_PARAM_AVCTYPE*)paramData;
			
            if (pComponentParam->nPortIndex == m_sH264Type.nPortIndex)
            {
                memcpy(&m_sH264Type,pComponentParam, sizeof(OMX_VIDEO_PARAM_AVCTYPE));

                //CalculateBufferSize(pCompPortOut->pPortDef, pComponentPrivate);
            }
            else
            {
                eError = OMX_ErrorBadPortIndex;
            }

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

		case VideoEncodeCustomParamStoreMetaDataInBuffers: //fixed if later
		{
	    	ALOGV(" COMPONENT_SET_PARAMETER: VideoEncodeCustomParamStoreMetaDataInBuffers");
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
	     		ALOGV("set_parameter: AWOMX_IndexParamVideoEnableAndroidNativeBuffers, do nothing.\n");
	     		m_useAndroidBuffer = OMX_TRUE;
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

OMX_ERRORTYPE  omx_venc::get_config(OMX_IN OMX_HANDLETYPE hComp, OMX_IN OMX_INDEXTYPE configIndex, OMX_INOUT OMX_PTR configData)
{
	OMX_ERRORTYPE eError = OMX_ErrorNone;

	ALOGV(" COMPONENT_GET_CONFIG: index = %d", configIndex);

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



OMX_ERRORTYPE omx_venc::set_config(OMX_IN OMX_HANDLETYPE hComp, OMX_IN OMX_INDEXTYPE configIndex, OMX_IN OMX_PTR configData)
{

	ALOGV(" COMPONENT_SET_CONFIG: index = %d", configIndex);

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


OMX_ERRORTYPE  omx_venc::get_extension_index(OMX_IN OMX_HANDLETYPE hComp, OMX_IN OMX_STRING paramName, OMX_OUT OMX_INDEXTYPE* indexType)
{
	unsigned int  nIndex;
    OMX_ERRORTYPE eError = OMX_ErrorUndefined;

	ALOGV(" COMPONENT_GET_EXTENSION_INDEX: param name = %s", paramName);
    if(m_state == OMX_StateInvalid)
    {
    	ALOGV("Get Extension Index in Invalid State\n");
        return OMX_ErrorInvalidState;
    }

    if(hComp == NULL)
    	return OMX_ErrorBadParameter;

    for(nIndex = 0; nIndex < sizeof(sVideoEncCustomParams)/sizeof(VIDDEC_CUSTOM_PARAM); nIndex++)
    {
        if(strcmp((char *)paramName, (char *)&(sVideoEncCustomParams[nIndex].cCustomParamName)) == 0)
        {
            *indexType = sVideoEncCustomParams[nIndex].nCustomParamIndex;
            eError = OMX_ErrorNone;
            break;
        }
    }

    return eError;
}



OMX_ERRORTYPE omx_venc::get_state(OMX_IN OMX_HANDLETYPE hComp, OMX_OUT OMX_STATETYPE* state)
{
	ALOGV(" COMPONENT_GET_STATE");

	if(hComp == NULL || state == NULL)
		return OMX_ErrorBadParameter;

	*state = m_state;
	return OMX_ErrorNone;
}


OMX_ERRORTYPE omx_venc::component_tunnel_request(OMX_IN    OMX_HANDLETYPE       hComp,
                                                 OMX_IN    OMX_U32              port,
                                                 OMX_IN    OMX_HANDLETYPE       peerComponent,
                                                 OMX_IN    OMX_U32              peerPort,
                                                 OMX_INOUT OMX_TUNNELSETUPTYPE* tunnelSetup)
{
	ALOGV(" COMPONENT_TUNNEL_REQUEST");

	ALOGV("Error: component_tunnel_request Not Implemented\n");
	return OMX_ErrorNotImplemented;
}



OMX_ERRORTYPE  omx_venc::use_buffer(OMX_IN    OMX_HANDLETYPE          hComponent,
                  			        OMX_INOUT OMX_BUFFERHEADERTYPE**  ppBufferHdr,
                  			        OMX_IN    OMX_U32                 nPortIndex,
                  			        OMX_IN    OMX_PTR                 pAppPrivate,
                  			        OMX_IN    OMX_U32                 nSizeBytes,
                  			        OMX_IN    OMX_U8*                 pBuffer)
{
    OMX_PARAM_PORTDEFINITIONTYPE*   pPortDef;
    OMX_U32                         nIndex = 0x0;

	ALOGV(" COMPONENT_USE_BUFFER");

	// ALOGD("-------nPortIndex: %d, nSizeBytes: %d", (int)nPortIndex, (int)nSizeBytes);

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

    if (!pPortDef->bEnabled)
    	return OMX_ErrorIncorrectStateOperation;

    if (nSizeBytes != pPortDef->nBufferSize || pPortDef->bPopulated)
    	return OMX_ErrorBadParameter;

    // Find an empty position in the BufferList and allocate memory for the buffer header.
    // Use the buffer passed by the client to initialize the actual buffer
    // inside the buffer header.
    if (nPortIndex == m_sInPortDef.nPortIndex)
    {
    	pthread_mutex_lock(&m_inBufMutex);
        ALOGV("vencInPort: use_buffer");
//        android::CallStack stack;
//        stack.update(1, 100);
//        stack.dump("use_buffer_inPort");

    	if((int)m_sInBufList.nAllocSize >= m_sInBufList.nBufArrSize)
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
    	pthread_mutex_lock(&m_outBufMutex);
        ALOGV("vencOutPort: use_buffer");
//        android::CallStack stack;
//        stack.update(1, 100);
//        stack.dump("use_buffer_outPort");
    
    	if((int)m_sOutBufList.nAllocSize >= m_sOutBufList.nBufArrSize)
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


OMX_ERRORTYPE omx_venc::allocate_buffer(OMX_IN    OMX_HANDLETYPE         hComponent,
        								OMX_INOUT OMX_BUFFERHEADERTYPE** ppBufferHdr,
        								OMX_IN    OMX_U32                nPortIndex,
        								OMX_IN    OMX_PTR                pAppPrivate,
        								OMX_IN    OMX_U32                nSizeBytes)
{
	OMX_S8                        nIndex = 0x0;
	OMX_PARAM_PORTDEFINITIONTYPE* pPortDef;

	ALOGV(" COMPONENT_ALLOCATE_BUFFER");

	if(hComponent == NULL || ppBufferHdr == NULL)
		return OMX_ErrorBadParameter;

    if (nPortIndex == m_sInPortDef.nPortIndex)
    {
        ALOGV("port_in: nPortIndex=%d", nPortIndex);
        pPortDef = &m_sInPortDef;
    }
    else
    {
        ALOGV("port_out: nPortIndex=%d", nPortIndex);
        if (nPortIndex == m_sOutPortDef.nPortIndex)
        {
            ALOGV("port_out: nPortIndex=%d", nPortIndex);
	        pPortDef = &m_sOutPortDef;
        }
        else
        {
            ALOGV("allocate_buffer fatal error! nPortIndex=%d", nPortIndex);
        	return OMX_ErrorBadParameter;
        }
    }

    if (!pPortDef->bEnabled)
    	return OMX_ErrorIncorrectStateOperation;

    if (nSizeBytes != pPortDef->nBufferSize || pPortDef->bPopulated)
    	return OMX_ErrorBadParameter;

    // Find an empty position in the BufferList and allocate memory for the buffer header
    // and the actual buffer
    if (nPortIndex == m_sInPortDef.nPortIndex)
    {
    	pthread_mutex_lock(&m_inBufMutex);
        ALOGV("vencInPort: malloc vbs");
//        android::CallStack stack;
//        stack.update(1, 100);
//        stack.dump("allocate_buffer");

    	if((int)m_sInBufList.nAllocSize >= m_sInBufList.nBufArrSize)
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
        ALOGV("vencOutPort: malloc frame");
//        android::CallStack stack;
//        stack.update(1, 100);
//        stack.dump("allocate_buffer_for_frame");
    	pthread_mutex_lock(&m_outBufMutex);

    	if((int)m_sOutBufList.nAllocSize >= m_sOutBufList.nBufArrSize)
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



OMX_ERRORTYPE omx_venc::free_buffer(OMX_IN  OMX_HANDLETYPE        hComponent,
        							OMX_IN  OMX_U32               nPortIndex,
        							OMX_IN  OMX_BUFFERHEADERTYPE* pBufferHdr)
{
    OMX_PARAM_PORTDEFINITIONTYPE* pPortDef;
    OMX_S32                       nIndex;

	ALOGV(" COMPONENT_FREE_BUFFER, nPortIndex = %d, pBufferHdr = %x", nPortIndex, pBufferHdr);

    if(hComponent == NULL || pBufferHdr == NULL)
    	return OMX_ErrorBadParameter;

    // Match the pBufferHdr to the appropriate entry in the BufferList
    // and free the allocated memory
    if (nPortIndex == m_sInPortDef.nPortIndex)
    {
        pPortDef = &m_sInPortDef;

    	pthread_mutex_lock(&m_inBufMutex);
        ALOGV("vencInPort: free_buffer");
//        android::CallStack stack;
//        stack.update(1, 100);
//        stack.dump("free_buffer_outPort");
    
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
        ALOGV("vencOutPort: free_buffer");
//        android::CallStack stack;
//        stack.update(1, 100);
//        stack.dump("free_buffer_outPort");
    
    	for(nIndex = 0; nIndex < m_sOutBufList.nBufArrSize; nIndex++)
    	{
    		ALOGV("pBufferHdr = %x, &m_sOutBufList.pBufArr[%d] = %x", pBufferHdr, nIndex, &m_sOutBufList.pBufArr[nIndex]);
    		if(pBufferHdr == &m_sOutBufList.pBufArr[nIndex])
    			break;
    	}

    	ALOGV("index = %d", nIndex);

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



OMX_ERRORTYPE  omx_venc::empty_this_buffer(OMX_IN OMX_HANDLETYPE hComponent, OMX_IN OMX_BUFFERHEADERTYPE* pBufferHdr)
{
    ThrCmdType eCmd   = EmptyBuf;

//	ALOGV(" COMPONENT_EMPTY_THIS_BUFFER");

    //ALOGV("vdecInPort: empty_this_buffer");
//    android::CallStack stack;
//    stack.update(1, 100);
//    stack.dump("empty_this_buffer_outPort");
    
    if(hComponent == NULL || pBufferHdr == NULL)
    	return OMX_ErrorBadParameter;

    if (!m_sInPortDef.bEnabled)
    	return OMX_ErrorIncorrectStateOperation;

    if (pBufferHdr->nInputPortIndex != 0x0  || pBufferHdr->nOutputPortIndex != OMX_NOPORT)
        return OMX_ErrorBadPortIndex;

    if (m_state != OMX_StateExecuting && m_state != OMX_StatePause)
        return OMX_ErrorIncorrectStateOperation;

    //fwrite(pBufferHdr->pBuffer, 1, pBufferHdr->nFilledLen, ph264File);
    //ALOGV("BHF[0x%x],len[%d]", pBufferHdr->nFlags, pBufferHdr->nFilledLen);
    // Put the command and data in the pipe
    write(m_cmdpipe[1], &eCmd, sizeof(eCmd));
    write(m_cmddatapipe[1], &pBufferHdr, sizeof(OMX_BUFFERHEADERTYPE*));

    return OMX_ErrorNone;
}



OMX_ERRORTYPE  omx_venc::fill_this_buffer(OMX_IN OMX_HANDLETYPE hComponent, OMX_IN OMX_BUFFERHEADERTYPE* pBufferHdr)
{
    ThrCmdType eCmd = FillBuf;

//	ALOGV(" COMPONENT_FILL_THIS_BUFFER");
    ALOGV("vencOutPort: fill_this_buffer");
//    android::CallStack stack;
//    stack.update(1, 100);
//    stack.dump("fill_this_buffer_outPort");

    if(hComponent == NULL || pBufferHdr == NULL)
    	return OMX_ErrorBadParameter;

    if (!m_sOutPortDef.bEnabled)
    	return OMX_ErrorIncorrectStateOperation;

    if (pBufferHdr->nOutputPortIndex != 0x1 || pBufferHdr->nInputPortIndex != OMX_NOPORT)
        return OMX_ErrorBadPortIndex;

    if (m_state != OMX_StateExecuting && m_state != OMX_StatePause)
        return OMX_ErrorIncorrectStateOperation;

    // Put the command and data in the pipe
    write(m_cmdpipe[1], &eCmd, sizeof(eCmd));
    write(m_cmddatapipe[1], &pBufferHdr, sizeof(OMX_BUFFERHEADERTYPE*));

    return OMX_ErrorNone;
}



OMX_ERRORTYPE  omx_venc::set_callbacks(OMX_IN OMX_HANDLETYPE        hComp,
                                           OMX_IN OMX_CALLBACKTYPE* callbacks,
                                           OMX_IN OMX_PTR           appData)
{
	ALOGV(" COMPONENT_SET_CALLBACKS");
//    android::CallStack stack;
//    stack.update(1, 100);
//    stack.dump("vdec_set_callbacks");
    if(hComp == NULL || callbacks == NULL || appData == NULL)
    	return OMX_ErrorBadParameter;
    memcpy(&m_Callbacks, callbacks, sizeof(OMX_CALLBACKTYPE));
    m_pAppData = appData;

    return OMX_ErrorNone;
}



OMX_ERRORTYPE  omx_venc::component_deinit(OMX_IN OMX_HANDLETYPE hComp)
{
	ALOGV(" COMPONENT_DEINIT");

    OMX_ERRORTYPE eError = OMX_ErrorNone;
    ThrCmdType    eCmd   = Stop;
    OMX_S32       nIndex = 0;

	ALOGD("component_deinit before");
	
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
    write(m_cmdpipe[1], &eCmd, sizeof(eCmd));
    write(m_cmddatapipe[1], &eCmd, sizeof(eCmd));

    // Wait for thread to exit so we can get the status into "error"
    pthread_join(m_thread_id, (void**)&eError);

    // close the pipe handles
    close(m_cmdpipe[0]);
    close(m_cmdpipe[1]);
    close(m_cmddatapipe[0]);
    close(m_cmddatapipe[1]);

	if (m_encoder!= NULL)
	{
		CedarvEncExit(m_encoder);
		m_encoder = NULL;
	}
	
	ve_mutex_destroy(&m_cedarv_req_ctx);

    return eError;
}


OMX_ERRORTYPE  omx_venc::use_EGL_image(OMX_IN OMX_HANDLETYPE               hComp,
                                          OMX_INOUT OMX_BUFFERHEADERTYPE** bufferHdr,
                                          OMX_IN OMX_U32                   port,
                                          OMX_IN OMX_PTR                   appData,
                                          OMX_IN void*                     eglImage)
{
	ALOGV("Error : use_EGL_image:  Not Implemented \n");
    return OMX_ErrorNotImplemented;
}


OMX_ERRORTYPE  omx_venc::component_role_enum(OMX_IN  OMX_HANDLETYPE hComp,
                                             OMX_OUT OMX_U8*        role,
                                             OMX_IN  OMX_U32        index)
{
	ALOGV(" COMPONENT_ROLE_ENUM");

	OMX_ERRORTYPE eRet = OMX_ErrorNone;

	if(!strncmp((char*)m_cName, "OMX.allwinner.video.encoder.avc", OMX_MAX_STRINGNAME_SIZE))
	{
		if((0 == index) && role)
		{
			strncpy((char *)role, "video_encoder.avc", OMX_MAX_STRINGNAME_SIZE);
			ALOGV("component_role_enum: role %s\n", role);
		}
		else
		{
			eRet = OMX_ErrorNoMore;
		}
	}
	else
	{
		ALOGV("\nERROR:Querying Role on Unknown Component\n");
		eRet = OMX_ErrorInvalidComponentName;
	}

	return eRet;
}


/*
 *  Component Thread
 *    The ComponentThread function is exeuted in a separate pThread and
 *    is used to implement the actual component functions.
 */
 /*****************************************************************************/
static void* ComponentThread(void* pThreadData)
{
	int                     encodeResult;

    int                     i;
    int                     fd1;
    fd_set                  rfds;
    OMX_U32                 cmddata;
    ThrCmdType              cmd;

    // Variables related to encoder buffer handling
    OMX_BUFFERHEADERTYPE*   pInBufHdr   = NULL;
    OMX_BUFFERHEADERTYPE*   pOutBufHdr  = NULL;
    OMX_MARKTYPE*           pMarkBuf    = NULL;
    OMX_U8*                 pInBuf      = NULL;
    OMX_U32                 nInBufSize;

    // Variables related to encoder timeouts
    OMX_U32                 nTimeout;
    OMX_BOOL                port_setting_match;
    OMX_BOOL                eos;

    OMX_PTR                 pMarkData;
    OMX_HANDLETYPE          hMarkTargetComponent;

    struct timeval          timeout;

    port_setting_match   = OMX_TRUE;
    eos                  = OMX_FALSE;
    pMarkData            = NULL;
    hMarkTargetComponent = NULL;

    // Recover the pointer to my component specific data
    omx_venc* pSelf = (omx_venc*)pThreadData;

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
	        	ALOGD("x set state command, cmd = %d, cmddata = %d.", (int)cmd, (int)cmddata);
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

			                    //* encoder
			                    if(pSelf->m_encoder!= NULL)
			                    {
			                    	CedarvEncExit(pSelf->m_encoder);
									pSelf->m_encoder = NULL;
			                    }
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
			                    else
			                    {
									if(pSelf->m_encoder == NULL)
									{
										unsigned int width  = pSelf->m_sInPortDef.format.video.nFrameWidth;
										unsigned int height = pSelf->m_sInPortDef.format.video.nFrameHeight;
										unsigned int bitrate = pSelf->m_sOutPortDef.format.video.nBitrate;
										ENCEXTRADATAINFO_t sps_pps_info;
										unsigned int level = 31;

										switch(pSelf->m_sH264Type.eLevel) 
										{
											case OMX_VIDEO_AVCLevel31: 
												level = 31;
												break;
											case OMX_VIDEO_AVCLevel41:
												level = 41;
												break;
											case OMX_VIDEO_AVCLevel42:
												level = 42;
												break;
											default:
												level = 31;
												break;
										}
										memset(&sps_pps_info, 0, sizeof(ENCEXTRADATAINFO_t));

										ALOGD(" init format,width: %d, height: %d, bitrate: %d", width, height, bitrate);
										
										if (ve_mutex_lock(&pSelf->m_cedarv_req_ctx) < 0) {
										
											ALOGE("can not get ve resource");
											goto EXIT;
										}
	
										pSelf->m_encoder = CedarvEncInit(pSelf->m_sInPortDef.format.video.nFrameWidth, pSelf->m_sInPortDef.format.video.nFrameHeight, bitrate, pSelf->m_framerate, level);

										ve_mutex_unlock(&pSelf->m_cedarv_req_ctx);
										
										if(pSelf->m_encoder == NULL)
										{
											ALOGE("can not create video encoder.");
									    	goto EXIT;
										}

										pSelf->m_input_step = STEP_QEQUEST_INPUT_BUFFER;

										pSelf->m_encoder->IoCtrl(pSelf->m_encoder, VENC_GET_SPS_PPS_DATA, (unsigned int)(&sps_pps_info));

										if(sps_pps_info.data == NULL)
										{
											ALOGE("can not get sps pps info");
									    	goto EXIT;										
										}

										pSelf->m_pExtraData = (unsigned char *)malloc(sps_pps_info.length);

										if(pSelf->m_pExtraData == NULL)
										{
										    ALOGE("malloc sps pps info space error");
									    	goto EXIT;
										}

										pSelf->m_extraDataLength = sps_pps_info.length;
										memcpy(pSelf->m_pExtraData, sps_pps_info.data, sps_pps_info.length);	

										pSelf->m_firstFrameFlag = OMX_TRUE;
									}
									else
									{
										ALOGE("error, encode has been init");
									}
			                    }

			                    nTimeout = 0x0;
			                    while (1)
			                    {
			                    	ALOGD("pSelf->m_sInPortDef.bPopulated:%d, pSelf->m_sOutPortDef.bPopulated: %d", pSelf->m_sInPortDef.bPopulated, pSelf->m_sOutPortDef.bPopulated);
			                        // Ports have to be populated before transition completes
			                        if ((!pSelf->m_sInPortDef.bEnabled && !pSelf->m_sOutPortDef.bEnabled)   ||
                                        (pSelf->m_sInPortDef.bPopulated && pSelf->m_sOutPortDef.bPopulated))
                                    {
			                        	pSelf->m_state = OMX_StateIdle;
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
			                    	ALOGE("encode component do not support OMX_StatePause");
									
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

                            eos                  = OMX_FALSE; //if need it
                            pMarkData            = NULL;
                            hMarkTargetComponent = NULL;

			                break;

                        case OMX_StatePause:

							ALOGE("encode component do not support OMX_StatePause");
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

							ALOGE("unknowed OMX_State");
		                	pSelf->m_Callbacks.EventHandler(&pSelf->m_cmp, pSelf->m_pAppData, OMX_EventError, OMX_ErrorIncorrectStateTransition, 0 , NULL);
		                	break;

		            }
                }
	        }
	        else if (cmd == StopPort)
	        {
	        	ALOGD("x stop port command, cmddata = %d.", (int)cmddata);
	            // Stop Port(s)
	            // cmddata contains the port index to be stopped.
                // It is assumed that 0 is input and 1 is output port for this component
                // The cmddata value -1 means that both input and output ports will be stopped.
	            if (cmddata == 0x0 || (int)cmddata == -1)
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

	            if (cmddata == 0x1 || (int)cmddata == -1)
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

		            if ((int)cmddata == -1 &&  !pSelf->m_sInPortDef.bPopulated && !pSelf->m_sOutPortDef.bPopulated)
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
	        	ALOGD("x restart port command.");
                // Restart Port(s)
                // cmddata contains the port index to be restarted.
                // It is assumed that 0 is input and 1 is output port for this component.
                // The cmddata value -1 means both input and output ports will be restarted.
                if (cmddata == 0x0 || (int)cmddata == -1)
                	pSelf->m_sInPortDef.bEnabled = OMX_TRUE;

	            if (cmddata == 0x1 || (int)cmddata == -1)
	            	pSelf->m_sOutPortDef.bEnabled = OMX_TRUE;

 	            // Wait for port to be populated
		        nTimeout = 0x0;
		        while (1)
		        {
                    // Return cmdcomplete event if input port populated
		            if (cmddata == 0x0 && (pSelf->m_state == OMX_StateLoaded || pSelf->m_sInPortDef.bPopulated))
		            {
		            	pSelf->m_Callbacks.EventHandler(&pSelf->m_cmp, pSelf->m_pAppData, OMX_EventCmdComplete, OMX_CommandPortEnable, 0x0, NULL);
			            break;
		            }
                    // Return cmdcomplete event if output port populated
		            else if (cmddata == 0x1 && (pSelf->m_state == OMX_StateLoaded || pSelf->m_sOutPortDef.bPopulated))
		            {
		            	pSelf->m_Callbacks.EventHandler(&pSelf->m_cmp, pSelf->m_pAppData, OMX_EventCmdComplete, OMX_CommandPortEnable, 0x1, NULL);
                        break;
		            }
                    // Return cmdcomplete event if input and output ports populated
		            else if ((int)cmddata == -1 && (pSelf->m_state == OMX_StateLoaded || (pSelf->m_sInPortDef.bPopulated && pSelf->m_sOutPortDef.bPopulated)))
		            {
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
	        	ALOGD("x flush command.");
	            // Flush port(s)
                // cmddata contains the port index to be flushed.
                // It is assumed that 0 is input and 1 is output port for this component
                // The cmddata value -1 means that both input and output ports will be flushed.
	            if (cmddata == 0x0 || (int)cmddata == -1)
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

	            if (cmddata == 0x1 || (int)cmddata == -1)
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
	        	ALOGD("x stop command.");
  		        // Kill thread
 	            goto EXIT;
	        }
	        else if (cmd == FillBuf)
	        {
	            // Fill buffer
	        	pthread_mutex_lock(&pSelf->m_outBufMutex);
	        	if (pSelf->m_sOutBufList.nSizeOfList <= pSelf->m_sOutBufList.nAllocSize)
	        	{
	        		pSelf->m_sOutBufList.nSizeOfList++;
	        		pSelf->m_sOutBufList.pBufHdrList[pSelf->m_sOutBufList.nWritePos++] = ((OMX_BUFFERHEADERTYPE*) cmddata);

	                if (pSelf->m_sOutBufList.nWritePos >= (int)pSelf->m_sOutBufList.nAllocSize)
	                	pSelf->m_sOutBufList.nWritePos = 0;
	        	}
	        	pthread_mutex_unlock(&pSelf->m_outBufMutex);
            }
	        else if (cmd == EmptyBuf)
	        {
	            // Empty buffer
	    	    pthread_mutex_lock(&pSelf->m_inBufMutex);
	        	if (pSelf->m_sInBufList.nSizeOfList <= pSelf->m_sInBufList.nAllocSize)
	        	{
	        		pSelf->m_sInBufList.nSizeOfList++;
	        		pSelf->m_sInBufList.pBufHdrList[pSelf->m_sInBufList.nWritePos++] = ((OMX_BUFFERHEADERTYPE*) cmddata);

	                if (pSelf->m_sInBufList.nWritePos >= (int)pSelf->m_sInBufList.nAllocSize)
	                	pSelf->m_sInBufList.nWritePos = 0;
	        	}
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
	    }

        // Buffer processing
        // Only happens when the component is in executing state.
        if (pSelf->m_state == OMX_StateExecuting  &&
        	pSelf->m_sInPortDef.bEnabled          &&
        	pSelf->m_sOutPortDef.bEnabled         &&
        	port_setting_match)
        {
			
        	//* 1. check if there is bitstream to output.
        	if(pSelf->m_encoder->hasOutputStream(pSelf->m_encoder))
        	{
                pthread_mutex_lock(&pSelf->m_outBufMutex);

                if(pSelf->m_sOutBufList.nSizeOfList > 0)
                {
                	pSelf->m_sOutBufList.nSizeOfList--;
                	pOutBufHdr = pSelf->m_sOutBufList.pBufHdrList[pSelf->m_sOutBufList.nReadPos++];
                	if (pSelf->m_sOutBufList.nReadPos >= (int)pSelf->m_sOutBufList.nAllocSize)
                		pSelf->m_sOutBufList.nReadPos = 0;
                }
                else
                {
                    //ALOGV("NO output bitstream !!!");
                	pOutBufHdr = NULL;
                }

                pthread_mutex_unlock(&pSelf->m_outBufMutex);

                //* if no output buffer, wait for some time.
            	if(pOutBufHdr == NULL)
            	{
            		usleep(10*1000);
                    ALOGV("no output frame,wait 10ms");
            		continue;
            	}
				
				pOutBufHdr->nFlags &= ~OMX_BUFFERFLAG_CODECCONFIG;
				pOutBufHdr->nFlags &= ~OMX_BUFFERFLAG_SYNCFRAME;

				if(pSelf->m_firstFrameFlag)
				{
					pOutBufHdr->nTimeStamp = 0; //fixed it later;
					pOutBufHdr->nFilledLen = pSelf->m_extraDataLength;
					pOutBufHdr->nOffset = 0;
					memcpy(pOutBufHdr->pBuffer, pSelf->m_pExtraData, pOutBufHdr->nFilledLen);
                    pSelf->m_firstFrameFlag = OMX_FALSE;

					pOutBufHdr->nFlags |= OMX_BUFFERFLAG_CODECCONFIG;
					//pOutBufHdr->nFlags |= OMX_BUFFERFLAG_SYNCFRAME;

				    pSelf->m_Callbacks.FillBufferDone(&pSelf->m_cmp, pSelf->m_pAppData, pOutBufHdr);
	                pOutBufHdr = NULL;	
						
		            continue;
				}
				else
				{
				  	memset(&pSelf->m_datainfo, 0, sizeof(__vbv_data_ctrl_info_t));
	            	pSelf->m_encoder->GetBitStreamInfo(pSelf->m_encoder, &pSelf->m_datainfo);
	            	
	            	pOutBufHdr->nTimeStamp = pSelf->m_datainfo.pts; //fix it, add base pts
	            	pOutBufHdr->nFilledLen = pSelf->m_datainfo.uSize0 + pSelf->m_datainfo.uSize1;
				    pOutBufHdr->nOffset = 0;

					pOutBufHdr->nFlags |= OMX_BUFFERFLAG_ENDOFFRAME;
					if(pSelf->m_datainfo.keyFrameFlag)
					{
						pOutBufHdr->nFlags |= OMX_BUFFERFLAG_SYNCFRAME;

					}

            		memcpy(pOutBufHdr->pBuffer, pSelf->m_datainfo.pData0, pSelf->m_datainfo.uSize0);
					if(pSelf->m_datainfo.uSize1){
						memcpy(pOutBufHdr->pBuffer + pSelf->m_datainfo.uSize0, pSelf->m_datainfo.pData1, pSelf->m_datainfo.uSize1);
					}

	                pSelf->m_encoder->ReleaseBitStreamInfo(pSelf->m_encoder, pSelf->m_datainfo.idx);

		            // Check for mark buffers
		            if (pMarkData != NULL && hMarkTargetComponent != NULL)
		            {
	                	if(!pSelf->m_encoder->hasOutputStream(pSelf->m_encoder))
	                	{
	                		// Copy mark to output buffer header
	                		pOutBufHdr->pMarkData = pInBufHdr->pMarkData;
	                		// Copy handle to output buffer header
	                		pOutBufHdr->hMarkTargetComponent = pInBufHdr->hMarkTargetComponent;

	                		pMarkData = NULL;
	                		hMarkTargetComponent = NULL;
	                	}
	 		        }
	                //ALOGV("FillBufferDone is called");
	                pSelf->m_Callbacks.FillBufferDone(&pSelf->m_cmp, pSelf->m_pAppData, pOutBufHdr);
	                pOutBufHdr = NULL;	
				}

                continue;
        	}
        	else
        	{
        		unsigned int size_Y;
				unsigned int size_C;
		        unsigned char* pBufY;
            	unsigned char* pBufC;
				unsigned int phy_y;
				unsigned int phy_c;
		
				if(pSelf->m_input_step == STEP_QEQUEST_INPUT_BUFFER)
				{
					pInBufHdr = NULL;
					
					//* check if there is a input picture.
	                pthread_mutex_lock(&pSelf->m_inBufMutex);

	                if(pSelf->m_sInBufList.nSizeOfList > 0)
	                {
	                	pSelf->m_sInBufList.nSizeOfList--;
	                	pInBufHdr = pSelf->m_sInBufList.pBufHdrList[pSelf->m_sInBufList.nReadPos++];
	                	if (pSelf->m_sInBufList.nReadPos >= (int)pSelf->m_sInBufList.nAllocSize)
	                		pSelf->m_sInBufList.nReadPos = 0;
	                }

	                pthread_mutex_unlock(&pSelf->m_inBufMutex);

	                if(pInBufHdr == NULL)
	                {
	                    //ALOGV("wait vbs input for 10ms");
	                    usleep(10*1000);
	                    //ALOGV("wait vbs input done");
	                	continue;
	                }

					if (pInBufHdr)
					{
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

					// *reqest inputbuffer;
	                pSelf->m_encoder->RequestBuffer(pSelf->m_encoder, &pBufY, &pBufC, &phy_y, &phy_c);

					// *add stream to encode;
					size_Y = pSelf->m_sInPortDef.format.video.nFrameWidth*pSelf->m_sInPortDef.format.video.nFrameHeight;
					size_C = size_Y>>1;
					
					memcpy(pBufY, pInBufHdr->pBuffer + pInBufHdr->nOffset,size_Y);
					memcpy(pBufC, pInBufHdr->pBuffer + pInBufHdr->nOffset + size_Y,size_C);

					pSelf->m_encoder->UpdataBuffer(pSelf->m_encoder);

				 	pSelf->m_Callbacks.EmptyBufferDone(&pSelf->m_cmp, pSelf->m_pAppData, pInBufHdr);

					memset(&pSelf->m_encBuf, 0, sizeof(VEnc_FrmBuf_Info));
					pSelf->m_encBuf.addrY = (unsigned char *)phy_y;
					pSelf->m_encBuf.addrCb = (unsigned char *)phy_c;

					pSelf->m_input_step = STEP_ENCODE;
				}

				if(pSelf->m_input_step == STEP_ENCODE)
				{
					if (ve_mutex_lock(&pSelf->m_cedarv_req_ctx) < 0) {

						usleep(5*1000);
						continue;
					}
					
					pSelf->m_encBuf.addrCr = 0;
					pSelf->m_encBuf.pts_valid = 1;
					pSelf->m_encBuf.pts = pInBufHdr->nTimeStamp;
					pSelf->m_encBuf.pover_overlay = NULL;
					pSelf->m_encBuf.color_fmt = PIXEL_YUV420;
					pSelf->m_encBuf.color_space = BT601;

					encodeResult = pSelf->m_encoder->encode(pSelf->m_encoder, &pSelf->m_encBuf);

					ve_mutex_unlock(&pSelf->m_cedarv_req_ctx);

					pSelf->m_input_step = STEP_QEQUEST_INPUT_BUFFER;
					
	    			if(encodeResult < 0)
	    			{
	                    ALOGE("encode fatal error[%d]", encodeResult);
						
	    				//* report a fatal error.
	                    pSelf->m_Callbacks.EventHandler(&pSelf->m_cmp, pSelf->m_pAppData, OMX_EventError, OMX_ErrorHardware, 0, NULL);
	                    continue;
	    			}
				}
        	}
        }
    }

EXIT:
    return (void*)OMX_ErrorNone;
}



