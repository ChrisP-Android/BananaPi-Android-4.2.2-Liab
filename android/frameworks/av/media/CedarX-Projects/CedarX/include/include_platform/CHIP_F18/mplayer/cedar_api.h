/*
*********************************************************************************************************
*                                                    cddar_gel
*                                   the Easy Portable/Player Operation System
*                                               bsp lib sub-system
*                                               cedar general module
*
*                                    (c) Copyright 2010-2011,Ivan China
*                                              All Rights Reserved
*
* File   : cedar_gel.h
* Version: V1.0
* By      :Ivan
* Date   : 2010-07-12 19:33
* 
*********************************************************************************************************
*/

#ifndef _CEDAR_API_H_
#define _CEDAR_API_H_

#include <semaphore.h>
#include <sys/eldk_types.h>
#include <disp/de_type.h>
#include <pthread.h>

#define FRAME_RATE_BASE     1000
#define DISPLAY_TIME_DELAY          (0x0000)        //delay 0 us
#define FRM_SHOW_LOW_THRESHOLD      (10)            //low threshold is -10ms
#define FRM_SHOW_HIGH_THRESHOLD     (20)            //high threshold is 20ms
#define FRM_SHOW_SCALE_CNT          (3)             //һ֡����ʾʱ��ĵȷ���
#define FRM_SHOW_SCALE_LOW          (1)             //low_threshold��ʱ��ȣ���FRM_SHOW_SCALE_CNTΪ��ĸ
#define FRM_SHOW_SCALE_HIGH         (2)             //high_threshold��ʱ���

#define CEDAR_TAG_INF_SIZE  (256)

#ifndef VDRV_TRUE
#define VDRV_TRUE                   (1)
#endif

#ifndef VDRV_FALSE
#define VDRV_FALSE                  (0)
#endif

#ifndef VDRV_SUCCESS
#define VDRV_SUCCESS                (0)
#endif

#ifndef VDRV_NULL
#define VDRV_NULL                   (0)
#endif

#ifndef VDRV_FAIL
#define VDRV_FAIL                   (-1)
#endif

#ifndef NULL
#define NULL 0
#endif
	
typedef struct __VDEC_RMG2_INIT_INFO
{
    s32        bUMV;
    s32        bAP;
    s32        bAIC;
    s32        bDeblocking;
    s32        bSliceStructured;
    s32        bRPS;
    s32        bISD;
    s32        bAIV;
    s32        bMQ;
    s32        iRvSubIdVersion;
    u16        uNum_RPR_Sizes;
    u16        FormatPlus;
    u32        RmCodecID;

}__vdec_rmg2_init_info;
typedef struct __CEDAR_BUF_NODE
{
    u32       addr;       //the address of the buffer malloc from system heap
    u32       size;       //the size of the memory block

    struct __CEDAR_BUF_NODE  *prev;     //prev pointer, point to front node
    struct __CEDAR_BUF_NODE  *next;     //next pointer, point to next node

} __cedar_buf_node_t;


//define the type of video bitstream
#define VIDEO_NONE              CEDAR_VBS_TYPE_NONE
#define VIDEO_UNKNOWN           CEDAR_VBS_TYPE_UNKNOWN
#define VIDEO_JPEG              CEDAR_VBS_TYPE_JPEG
#define VIDEO_MJPEG             CEDAR_VBS_TYPE_MJPEG
#define VIDEO_MPEG1_ES          CEDAR_VBS_TYPE_MPEG1_ES
#define VIDEO_MPEG2_ES          CEDAR_VBS_TYPE_MPEG2_ES
#define VIDEO_XVID              CEDAR_VBS_TYPE_XVID
#define VIDEO_DIVX3             CEDAR_VBS_TYPE_DIVX3
#define VIDEO_DIVX4             CEDAR_VBS_TYPE_DIVX4
#define VIDEO_DIVX5             CEDAR_VBS_TYPE_DIVX5
#define VIDEO_SORENSSON_H263    CEDAR_VBS_TYPE_SORENSSON_H263
#define VIDEO_RMG2              CEDAR_VBS_TYPE_RMG2
#define VIDEO_RMVB8             CEDAR_VBS_TYPE_RMVB8
#define VIDEO_RMVB9             CEDAR_VBS_TYPE_RMVB9
#define VIDEO_H264              CEDAR_VBS_TYPE_H264
#define VIDEO_H264_ES           CEDAR_VBS_TYPE_H264_ES
#define VIDEO_WMV3              CEDAR_VBS_TYPE_WMV3
#define VIDEO_WVC1              CEDAR_VBS_TYPE_WVC1
#define VIDEO_SVC1              CEDAR_VBS_TYPE_SVC1
#define VIDEO_AVS               CEDAR_VBS_TYPE_AVS
#define VIDEO_H263              CEDAR_VBS_TYPE_H263
#define VIDEO_VP6               CEDAR_VBS_TYPE_VP6
#define VIDEO_WMV1              CEDAR_VBS_TYPE_WMV1
#define VIDEO_WMV2              CEDAR_VBS_TYPE_WMV2
#define CEDAR_MAX_PICTURE_WIDTH_MPEG124     (1920)
#define CEDAR_MAX_PICTURE_HIGHT_MPEG124     (1280)

#define CEDAR_MAX_PICTURE_WIDTH_RMVB        (1920)
#define CEDAR_MAX_PICTURE_HIGHT_RMVB        (1280)

#define CEDAR_MAX_PICTURE_WIDTH_H264        (1920)
#define CEDAR_MAX_PICTURE_HIGHT_H264        (1280)

#define CEDAR_MAX_PICTURE_WIDTH_MJPEG       (1920)
#define CEDAR_MAX_PICTURE_HIGHT_MJPEG       (1280)

#define CEDAR_MAX_PICTURE_WIDTH_VC1         (1920)
#define CEDAR_MAX_PICTURE_HIGHT_VC1         (1280)

#define CEDAR_MAX_PICTURE_WIDTH_WMV2        (1920)
#define CEDAR_MAX_PICTURE_HIGHT_WMV2        (1280)

#define CEDAR_MAX_PICTURE_WIDTH_VP6         (1920)
#define CEDAR_MAX_PICTURE_HIGHT_VP6         (1280)

#define MACC_INT_VE_FINISH          (0)
#define MACC_INT_VE_ERROR           (1)
#define MACC_INT_VE_MEMREQ          (2)
#define MACC_INT_VE_DBLK            (3)
//redefine color format
#define PIC_CFMT_YCBCR_444  PIXEL_YUV444
#define PIC_CFMT_YCBCR_422  PIXEL_YUV422
#define PIC_CFMT_YCBCR_420  PIXEL_YUV420
#define PIC_CFMT_YCBCR_411  PIXEL_YUV411

//-----------------------------------------------------------------------------------
#define STC_ID_VALID_CTRL           (1<<0)
#define PTS_VALID_CTRL              (1<<1)
#define FRM_BORDER_CTRL             (1<<2)
#define FIRST_PART_CTRL             (1<<3)
#define LAST_PART_CTRL              (1<<4)
#define SLICE_STRUCTURE_CTRL        (1<<5)
#define MULTIPLE_FRAME_CTRL         (1<<6)
#define FRM_DEC_MODE_VALID_CTRL     (1<<7)
#define FRM_RATE_CTLR               (1<<8)
#define BROKEN_SLICE_FLAG_CTRL      (1<<9)

/*
*******************************************************
cedar 
define the type base number of message  which taken with socket
*******************************************************
*/

#define CEDAR_MSGTYPE_NONE          (0<<16)     /* none message                                                     */
#define CEDAR_PSR2ADEC_MSG_BASE     (1<<16)     /* message base number between parser and audio deocder             */
#define CEDAR_PSR2VDEC_MSG_BASE     (2<<16)     /* message base number between parser and video deocder             */
#define CEDAR_PSR2LDEC_MSG_BASE     (3<<16)     /* message base number between parser and lyric deocder             */
#define CEDAR_ADEC2ARDR_MSG_BASE    (4<<16)     /* message base number between audio decoder and render             */
#define CEDAR_ARDR2APLY_MSG_BASE    (5<<16)     /* message base number between render and audio playback            */
#define CEDAR_VDEC2VPLY_MSG_BASE    (6<<16)     /* message base number between video decoder and video playback     */
#define CEDAR_LDEC2LRDR_MSG_BASE    (7<<16)     /* message base number between lyric decoder and lyric render       */
#define CEDAR_LRDR2LPLY_MSG_BASE    (8<<16)     /* message base number between lyric render and lyric playback      */
#define CEDAR_ADEC2ADRV_MSG_BASE    (9<<16)     /* message base number between audio decoder plug-in and driver     */
#define CEDAR_VDEC2VDRV_MSG_BASE    (10<<16)    /* message base number between video decoder plug-in and driver     */
#define CEDAR_ARDR2ADRV_MSG_BASE    (11<<16)    /* message base number between audio render plug-in and driver      */


//local define of vc1-decoder
// #define VC1FILE_FMT_SMP 0 //dont't change it
// #define VC1FILE_FMT_AP  1
// #define VC1STM_FMT_AP   2

//define the format get data from system call
typedef struct __VIDEO_ENCRYPT_ITEM
{
    char    format[16]; //"mpeg2","mpeg4","rmvb","h264","vc1""wmv1","wmv2","vp6","avs"
    s32   support;
    u32   s_width;
    u32   s_height;
    u32   reserved;
}__video_encrypt_item;

typedef s32   (* __pCBK_t )(void* /*p_arg*/);     /* call back function pointer                                   */

extern pthread_mutex_t	pSemThredMutex;
extern pthread_mutex_t	sys_sem_mutex;

typedef struct __VDEC_2_VDRV_MSG
{
    void        *pData;
    u32       nDataLen;

    void        *pMsg;
    u32       nMsgType;
    u32       nMsgLen;

}__vdec_2_vdrv_msg_t;

typedef enum __VDEC_2_VDRV_MSG_TYPE
{
    VDEC2VDRV_MSG_TYPE_NONE=0,
    VDEC2VDRV_MSG_TYPE_FRMSIZE_CHANGE=CEDAR_VDEC2VDRV_MSG_BASE,
                                                // frame size change
    VDEC2VDRV_MSG_TYPE_I_DECODE,                // decode an I frame
    VDEC2VDRV_MSG_TYPE_REFETCH_DATA,            // refetch video bitstream
    VDEC2VDRV_MSG_TYPE_FATAL_ERR,               // fatal err
    VDEC2VDRV_MSG_TYPE_DEC_END,                 // decode driver finish decode bitstream
    VDEC2VDRV_MSG_TYPE_DATA_FULL,               // bitstream buffer full

    VDEC2VDRV_MSG_TYPE_

} __vdec_2_vdrv_msg_type_t;


typedef enum __VDRV_MSG_TYPE
{
    VDRV_MSG_TYPE_NONE = 0,             // no message
    VDRV_MSG_SIZE_CHANGE,               // picture frame change
    VDRV_MSG_I_DECODE,                  // decode i frame
    VDRV_MSG_DATA_END,                  // vbs decode end
    VDRV_MSG_

} __vdrv_msg_type;

//Aspect ratio
enum AspRatio
{
    ASP_RATIO_DAR_FORBIDDEN = 0,    // aspect ratio forbidden

    ASP_RATIO_SAR,                  // sample aspect ratio
    ASP_RATIO_DAR_4_3,              // Display aspect ratio 3/4
    ASP_RATIO_DAR_16_9,             // Display aspect ratio 9/16
    ASP_RATIO_DAR_221_1,            // Display aspect ratio 1/2.11

    ASP_RATIO_
}; //P126, value must NOT change!

typedef struct VIDEO_CODEC_FORMAT
{
    u32       codec_type;         // video codec type

    u8        ifrm_pts_credible;  // flag marks that if pts for i frame is credible
    u8        pfrm_pts_credible;  // flag marks that if pts for p frame is credible
    u8        bfrm_pts_credible;  // flag marks that if pts for b frame is credible
	u8		all_pts_credible;

    u16       width;              // picture width
    u16       height;             // picture height
    u32       frame_rate;         // frame rate, ��ֵ�Ŵ�1000����
    u32       mic_sec_per_frm;    // frame duration

    s32       avg_bit_rate;       // average bit rate
    s32       max_bit_rate;       // maximum bit rate

    //define private information for video decode
    u16       video_bs_src;       // video bitstream source
    void        *private_inf;       // video bitstream private information pointer
    s16       priv_inf_len;       // video bitstream private information length

} __video_codec_format_t;
typedef struct VDecDataCtrlBlk
{
    u16       CtrlBits;
        //bit 0:    0 -- STC_ID section not valid,
        //          1 -- STC_ID section valid;
        //bit 1:    0 -- PTS section not valid,
        //          1 -- PTS section valid;
        //bit 2:    0 -- frame border not known,
        //          1 -- frame border known;
        //bit 3:    0 -- this data block is not the first part of one frame,
        //          1 -- this data block is the first part of one frame,
        //          bit 3 is valid only when bit 2 is set;
        //bit 4:    0 -- this data block is not the last part of one frame,
        //          1 -- this data block is the last part of one frame,
        //          bit 4 is valid only when bit 2 is set;
        //bit 5:    0 -- the frame data is not in slice structure mode,
        //          1 -- the frame data is in slice structure mode;
        //bit 6:    0 -- single frame data,
        //          1 -- multiple frame data;
        //bit 7:    0 -- frame decode mode is invalid,
        //          1 -- frame decode mode is valid;
        //bit 8:    0 -- frame rate is invalid,
        //          1 -- frame rate is valid;
        //bit 9:    0 -- broken slice flag invalid,
        //          1 -- broken slice flag valid;
        //others:   reserved

    u8        uStcId;
        //0-STC 0, 1-STC 1

    u8        uFrmDecMode;
        //frame decode mode, normal play mode forward/backward mode

    u8        curDataNum;
        //current data number, for distinguish the sequential 2 packet bitstream for mpeg2

    s64       uPTS;
        //Corresponding PTS value for a data block, [63,0] of actual PTS, based on micro-second

    u32       uFrmRate;
        // frame rate, the frame rate maybe known till get bitstream to send to decoder

} __vdec_datctlblk_t;
enum RepeatField
{
    REPEAT_FIELD_NONE,          //means no field should be repeated

    REPEAT_FIELD_TOP,           //means the top field should be repeated
    REPEAT_FIELD_BOTTOM,        //means the bottom field should be repeated

    REPEAT_FIELD_
};
typedef struct Video_Info
{
    u16       width;          //the stored picture width for luma because of mapping
    u16       height;         //the stored picture height for luma because of mapping
    u16       frame_rate;     //the source picture frame rate
    u16       eAspectRatio;   //the source picture aspect ratio
    u16       color_format;   //the source picture color format

} __video_info_t;

typedef struct PanScanInfo
{
    u32 uNumberOfOffset;
    s16 HorizontalOffsets[3];

} __pan_scan_info_t;

typedef struct Rect
{
    s16       uStartX;    // Horizontal start point.
    s16       uStartY;    // Vertical start point.
    s16       uWidth;     // Horizontal size.
    s16       uHeight;    // Vertical size.

} __vdrv_rect_t;

typedef struct Display_Info
{
    bool              bProgressiveSrc;    // Indicating the source is progressive or not
    bool              bTopFieldFirst;     // VPO should check this flag when bProgressiveSrc is FALSE
    enum RepeatField    eRepeatField;       // only check it when frame rate is 24FPS and interlace output
    __video_info_t      pVideoInfo;         // a pointer to structure stored video information
    __pan_scan_info_t   pPanScanInfo;
    __vdrv_rect_t       src_rect;           // source valid size
    __vdrv_rect_t       dst_rect;           // source display size
    u8                top_index;          // frame buffer index containing the top field
    u32               top_y;              // the address of frame buffer, which contains top field luminance
    u32               top_c;              // the address of frame buffer, which contains top field chrominance

    //the following is just for future
    u8                bottom_index;       // frame buffer index containing the bottom field
    u32               bottom_y;           // the address of frame buffer, which contains bottom field luminance
    u32               bottom_c;           // the address of frame buffer, which contains bottom field chrominance

    //time stamp of the frame
    u32               uPts;               // time stamp of the frame (ms?)

} __display_info_t;

//define the result of frame decode
typedef enum __PIC_DEC_RESULT
{
    PIC_DEC_ERR_VE_BUSY         = -1,   // video engine is busy
	PIC_DEC_ERR_VFMT_ERR        = -2,   // the format of the video is not supported, fatal error
		
	PIC_DEC_ERR_NONE            = 0,    // decode one picture successed
	PIC_DEC_ERR_FRMNOTDEC       = 1,    // decode one picture successed
	PIC_DEC_ERR_VBS_UNDERFLOW   = 2,    // video bitstream underflow
	PIC_DEC_ERR_NO_FRAMEBUFFER  = 3,    // there is no free frame buffer for decoder
	PIC_DEC_ERR_DEC_VBSEND      = 4,    // decode all video bitstream end
	PIC_DEC_I_FRAME ,

	
	PIC_DEC_ERR_
		
} __pic_dec_result;


typedef enum __AVS_DRIVER_COMMAND
{                                   // command list for av sync
    DRV_AVS_CMD_START=0x00,         // start avsync
    DRV_AVS_CMD_STOP,               // stop avsync
    DRV_AVS_CMD_PAUSE,              // pause avsync
    DRV_AVS_CMD_CONTI,              // continue
    DRV_AVS_CMD_RESET,              // reset avsync
    DRV_AVS_CMD_FF,                 // fast forward
    DRV_AVS_CMD_RR,                 // fast reverse
    DRV_AVS_CMD_CLEAR_FFRR,         // clear fast forward/reverse
    DRV_AVS_CMD_GET_STATUS,         // get the status of avsync
    DRV_AVS_CMD_JUMP,                 // fast reverse
    DRV_AVS_CMD_CLEAR_JUMP,         // clear fast forward/reverse

    DRV_AVS_CMD_GET_VID_TIME=0x20,  // get video time, based on ms
    DRV_AVS_CMD_SET_VID_TIME,       // set video time, based on ms
    DRV_AVS_CMD_GET_AUD_TIME,       // get audio time, based on ms
    DRV_AVS_CMD_SET_AUD_TIME,       // set audio time, based on ms
    DRV_AVS_CMD_GET_CUR_TIME,       // get play time based on ms

    DRV_AVS_CMD_ENABLE_AVSYNC=0x40, // enable or disable audio video sync operation
    DRV_AVS_CMD_REGIST_CLOCK,       // regist audio or video clock
    DRV_AVS_CMD_CHECK_CLOCK,        // check if audio or video clock is exist
    DRV_AVS_CMD_ENABLE_CLOCK,       // enable/disable audio clock or video clock
    DRV_AVS_CMD_SET_VID_CLK_SPEED,  // set video clock speed(.../-3/-2/-1/1/2/3/...)
    DRV_AVS_CMD_SET_DACDEVHDL,      // set audio device handle, set or clear
    DRV_AVS_CMD_SET_AUD_CHAN_CNT,   // set audio channel count
    DRV_AVS_CMD_SET_PCM_SAMPRATE,   // set audio sample rate, for calculate audio time
    DRV_AVS_CMD_SET_AUD_VPS,        // set audio variable speed, for calculate audio time
    DRV_AVS_CMD_GET_AUD_CACHE_TIME, // get audio cache time for skip video frame
    DRV_AVS_CMD_SET_FORCE_SYNC,     // set force sync, need adjust video clock
    DRV_AVS_CMD_CHK_AUDIO_PLAY_END, // check if all audio sample has play end
//    DRV_AVS_CMD_SET_WAIT_FLG,  //����Ҫ��ȴ�ı��,Ŀǰ��audio playbackģ����,video playbackģ���.aux = 1(��),0(���õ�)
//    DRV_AVS_CMD_GET_WAIT_FLG, //��ѯaudio playbackģ���Ƿ���ͣ
    
    DRV_AVS_CMD_SET_AV_END_FLAG=0x60,   // set audio or video end flag
    DRV_AVS_CMD_GET_AV_END_FLAG,        // get audio or video end flag
    DRV_AVS_CMD_SET_FFRR_FLAG,          // set flag if ff/rr is allowed
    DRV_AVS_CMD_GET_FFRR_FLAG,          // get flag if ff/rr is allowed
    DRV_AVS_CMD_GET_AUDIO_CACHE_STAT,   // ��ѯdac buffer�е�audio��ݵ����������
                                        //ret = 1:buffer�е�audio_data����;0:audio_data��û���ˡ�

    
    DRV_AVS_CMD_

} __avs_driver_command_t;

typedef enum __AVS_TIME_TYPE
{                                   // define parameter type for get and set time
    DRV_AVS_TIME_TOTAL=0,           // total time, base + offset
    DRV_AVS_TIME_BASE,              // base time, video time start value
    DRV_AVS_TIME_OFFSET,            // time offset, play time offset
    DRV_AVS_TIME_

} __avs_time_type_t;

typedef enum __CEDAR_VIDEO_FMT
{
    CEDAR_VBS_TYPE_NONE = 0,            /* û����Ƶ��                           */
    CEDAR_VBS_TYPE_UNKNOWN,              /* ����Ƶ��,���޷�ʶ��                  */

    CEDAR_VBS_TYPE_JPEG,
    CEDAR_VBS_TYPE_MJPEG,
    CEDAR_VBS_TYPE_MPEG1_ES,
    CEDAR_VBS_TYPE_MPEG2_ES,
    CEDAR_VBS_TYPE_XVID,
    CEDAR_VBS_TYPE_DIVX3,
    CEDAR_VBS_TYPE_DIVX4,
    CEDAR_VBS_TYPE_DIVX5,
    CEDAR_VBS_TYPE_SORENSSON_H263,
    CEDAR_VBS_TYPE_RMG2,
    CEDAR_VBS_TYPE_RMVB8,
    CEDAR_VBS_TYPE_RMVB9,
    CEDAR_VBS_TYPE_H264,
	CEDAR_VBS_TYPE_H264_ES,
	CEDAR_VBS_TYPE_WMV3,
    CEDAR_VBS_TYPE_WVC1,
	CEDAR_VBS_TYPE_SVC1, //stream vc1,used for ts
    CEDAR_VBS_TYPE_AVS,
    CEDAR_VBS_TYPE_H263,
    CEDAR_VBS_TYPE_VP6,
    CEDAR_VBS_TYPE_WMV1,
    CEDAR_VBS_TYPE_WMV2,

    CEDAR_VBS_TYPE_

} __cedar_video_fmt_t;
//define decoding status
#define MEDIA_STATUS_TMP_BIT 0x80
enum MEDIA_STATUS
{
    MEDIA_STATUS_IDLE = 0,
    MEDIA_STATUS_PAUSE,
    MEDIA_STATUS_STEP,
    MEDIA_STATUS_PLAY,
    MEDIA_STATUS_FORWARD,
    MEDIA_STATUS_BACKWARD,
    MEDIA_STATUS_STOP,
    MEDIA_STATUS_ABORT,
    MEDIA_STATUS_JUMP,

    MEDIA_STATUS_
};

enum MEDIA_COMMAND
{
    MEDIA_COMMAND_IDLE = 0,
	MEDIA_COMMAND_PAUSE,
	MEDIA_COMMAND_STEP,
	MEDIA_COMMAND_PLAY,
	MEDIA_COMMAND_FORWARD,
	MEDIA_COMMAND_BACKWARD,
	MEDIA_COMMAND_STOP,
	MEDIA_COMMAND_ABORT,
	MEDIA_COMMAND_JUMP,
	MEDIA_COMMAND_DATAEND,
	MEDIA_COMMAND_ROTATE,
};

typedef struct __VDEC_MPEG4_VBS_INF
{
    //video codec format information
    __video_codec_format_t  vCodecInf;

    //rmg2 vbs init information
    __vdec_rmg2_init_info   vRmg2Inf;

} __vdec_mpeg4_vbs_inf_t;

//==============================================================================
//                              PUB VARIABLE
//==============================================================================
#define BVOP_DEC_LATE_THRESHOLD     (60)        //decode B-VOP time late threshold, if the later than
                                                //this value, the B-VOP will be dropped without decoded
#define VOP_DEC_LATE_THRESHOLD      (1000)      //decode vop too late threshold, if the time of vop
                                                //decode is later than this value, the no-key frames
                                                //before next key frame will be dropped all
extern s32 VDrvSendMsg(s32 nMsgType, void *pMsg);
//extern s32 MIoctrl(__mp *mp, u32 cmd, s32 aux, void *pbuffer);


//=============================================================================
//                           Common Codec Definition
//=============================================================================

#define MAX_PIC_NUM_IN_VBV          256
#define MAX_DISP_QUEEN_BUFFER_COUNT 16

typedef struct display_frm_queue
{
    u16       frm_idx_buf[MAX_DISP_QUEEN_BUFFER_COUNT];
    s16       valid_size;
    u16       PicReadIdx;
    u16       PicWriteIdx;

	u32		frame_ctr;
    u32		preValidId;
    u64		preValidPTS;
	s8		preFrameIdx;
} __display_frm_queue_t;

typedef struct VBV_DATA_CTRL_INFO
{
    u32       start_addr;
    s64       pts;
    bool      pts_valid;
    bool      isMultipleFrame;
    u8        stc_id;
    s8        frm_dec_mode;
    u32       stream_len;         // G2 need this field.
	
} __vbv_data_ctrl_info_t;

typedef struct vdec_frm_info
{
    //buffer info
    u32       y_buf;
    u32       c_buf;
    u32       y_buf_size;
    u32       c_buf_size;
	
	//u32		y_buff_phy;
	//u32		c_buff_phy;

    u16       pic_width;          // picture luma coded width
    u16       pic_height;         // picture luma coded height
    u16       pic_store_width;    // picture luma stored width because of mapping
    u16       pic_store_height;   // picture luma stored height because of mapping
    u16       disp_width;         // valid luma display width
    u16       disp_height;        // valid luma display height
    u16       color_format;       // the source picture color format
    u16       frame_rate;
    u16       isFull;             // one picture is decoded
    bool      bProgressive;
    bool      bRepeatField;
    bool      bTopFieldFirst;
	
	bool		bPTSValid;
    s64       uPTS;
    s32       frm_dec_mode;       // frmae decode mode, we need check if the frame
									//can be displayed under current status

	u32		stc_id;
	u32		hasSecBuf;
    enum AspRatio   aspect_ratio;
	
} __vdec_frm_info_t;

typedef struct vdec_frm_manager
{
    u8        reconstruct_buf_idx;
    u8        forward_buf_idx;
    u8        backward_buf_idx;
    u8        display_buf_idx;
    u8        free_buf_idx;
	u8        sec_buf_idx;
	
    u8        push_frm_idx;
	
    u8        de_request_frm_idx;
    u8        de_release_frm_idx;
	
    u8       frame_buf_num;
    
    u32       BFrameNum;
	
    struct vdec_frm_info frm_info[MAX_DISP_QUEEN_BUFFER_COUNT];
} __vdec_frm_manager_t;

typedef struct vdec_vbv_info
{
    u8        *vbv_buf;
    u8        *vbv_buf_end;
    u8        *read_addr;
    u8        *write_addr;
    bool      hasPicBorder;
    bool      hasSliceStructure;
    struct      VBV_DATA_CTRL_INFO vbv_data_info[MAX_PIC_NUM_IN_VBV];
    u16       PicReadIdx;
    u16       PicWriteIdx;
    u16       PicIdxSize;
    s32       valid_size; //for mpeg2
    u32       vbv_buf_size;

	//u8    frame_mode;
	//u8    data_num;
    u8    lastAddr;
} __vdec_vbv_info_t;

typedef struct cedar_quality
{
	u32 vbvbuffer_validsize_percent; //[0:100]
	u32 element_in_vbv;				   //[0:MAX_PIC_NUM_IN_VBV]
}__cedar_quality_t;



typedef struct _PICframe
{
    s32       frametype;
    s16       Index;
    s32       nPTS;
    u32       uFrmDecMode;
}PICframe;

//rv89 struct
typedef struct rv_format_info_struct
{
    u32   ulLength;
    u32   ulMOFTag;
    u32   ulSubMOFTag;
    u16   usWidth;
    u16   usHeight;
    u16   usBitCount;
    u16   usPadWidth;
    u16   usPadHeight;
    u32   ufFramesPerSecond;
    u32   ulOpaqueDataSize;
    u8    *pOpaqueData;
} rv_format_info;

typedef struct VDEC_DEVICE
{
    void        *priv_data;

	bool      bHasOpenFlag;
	bool      enRotation;
    s16       RotationSel;                 //0--no, 1--90, 2--180, 3--270;
    bool      enDeblocking;

	bool		enScaleDown;
	s8		scaleHor;
	s8		scaleVer;

    s8        status;
    s8        pre_status;
    s8        syncstatflag;
	s32		av_delay; //uinit:ms [A-V]

    struct VIDEO_CODEC_FORMAT   vFormat;
    struct vdec_vbv_info        vbvInfo;
    struct vdec_frm_manager     frm_manager;
    struct display_frm_queue    out_frm_queue;
    
    bool      parsing_data_end;
    bool      first_frm_flag;             //0-first frame, !0-not the first frame
    s32       frm_show_low_threshold;     //unit:ms
    s32       frm_show_high_threshold;    //unit:ms

    s16       display_width,display_height; //output
    s32       eAspectRatio;				  //output

	/*configuration of decoder memory*/
	s32		frame_buffer_count_max;     //framebuffer count of decode 
	s32		extern_vbv_buffer_size;            //input bitstream buffer size
	void		*external_vbvbuffer_start;
	bool		use_external_vbvbuffer;
    
	/*function of cedar library*/
    s16       (*open)(struct VDEC_DEVICE *p);
    s16       (*close)(struct VDEC_DEVICE *p);
    s16       (*decode)(struct VDEC_DEVICE *p);
    s16       (*IoCtrl)(struct VDEC_DEVICE *p, u32, u32);
    s16       (*RequestWrite)(struct VDEC_DEVICE *pDev, u32 uSizeRequested, u8 **ppData0, \
								u32 *puSizeGot0, u8 **ppData1, u32 *puSizeGot1);
    s16       (*UpdatePointer)(struct VDEC_DEVICE *pDev,  \
								struct VDecDataCtrlBlk * pVDataCtrlBlk,u32 uDataSize);
    s16       (*DisplayRequest)(struct VDEC_DEVICE *pDev, struct Display_Info *pDisplay_Info);
    s16       (*DisplayRelease)(struct VDEC_DEVICE *pDev, u8 frm_idx);
    s16       (*SetVbsInf)(struct VDEC_DEVICE *pDev, void *pVbsInf);
	void		(*QueryQuality)(struct VDEC_DEVICE *pDev, struct cedar_quality *vq);

	/* external callback function */
    s32       (*SendMsg)(s32 nMstType, void *pMsg);
	void		(*ReleaseFrmBufSem)(void);
	void		(*VDrvFreeVbsBuffer)(void);	
	
	/*h264 member*/
	u8        bHasInitInfo;
	u8        frm_number;
    s32       PicNumb;
    s32       outframe;
#define MAX_FRM_NUMBER_H264DEC 32
    PICframe    PIC_Message[MAX_FRM_NUMBER_H264DEC];
	u8		ReleaseBuf[MAX_FRM_NUMBER_H264DEC];
	u8		ReleaseIdx;
	s32		dec_status;
	
    __video_codec_format_t		codec_fmt;
    s32       nBFrmTimeDeta;
    s32       nForFrmPts;
    s32       nBakFrmPts;
    u32       aspect_ratio;

	/*rv89 member*/
    //rv89dec_t*					core; // decode handler
    rv_format_info              init_fmt_info;
    
	u32						trWrap;
    s32						modeSwitch;
    s32                       needKeyFrm;

	u8*                       fbm_buf;
	u8*                       vbv_buf;
	u8*                       mv_buf;
    u8*                       mbh_buf;

	/*mpeg2*/
    bool  DecodeEnd;
    s64   LastShowFrmPts;
    u32   IsFistFrmFlag;
	bool  ffrrStatusFlag;
	bool  noplayStatusFlag;
	bool  statusChangeSuccess;
	bool  nonfst_play_status;
	bool   preRepeatField;
	bool   sec_play_status;
	bool   first_real_pts;
	bool  is_ts_stream;
	u8     frameNum;
} __vdec_device_t;
struct mem_bak{
	char name[32];
	void *paddr;
};

struct mem_bak_de{
	int fd;
	unsigned long arg01;
	unsigned long arg02;
	unsigned long arg03;
	unsigned long arg11;
	unsigned long arg12;
	unsigned long arg13;
};
struct mem_bak_infor{
	u32   size;
	void *paddr;
};
extern struct mem_bak_infor mem_bak_addr;

extern int osal_init(void **bufferptr,int size);
extern int osal_exit(void);

extern void avs_counter_config();
extern void avs_counter_start();
extern void avs_counter_pause();
extern void avs_counter_reset();
extern s64 avs_counter_get_time_us();
extern void avs_counter_adjust_abs(int val);

#endif  //_CEDAR_MALLOC_H_

