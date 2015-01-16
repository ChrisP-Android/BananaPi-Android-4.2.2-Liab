#include "hdmi_core.h"
#include "../aw/include/api/api.h"
#include "../aw/include/core/audio.h"
#include "../aw/include/core/video.h"
const unsigned char vName[] = "allwinner";
const unsigned char pName[] = "hdmi with hdcp";

videoParams_t  	video;
audioParams_t  	audio;
productParams_t product;
hdcpParams_t 	hdcp;
dtd_t           dtd;

__s32		hdmi_state = HDMI_State_Idle;
__bool		video_enable = 0;
__u32		video_mode = HDMI720P_50;
__s32		cts_enable = 0;
__s32		hdcp_enable = 0;

HDMI_AUDIO_INFO audio_info;
__u8		EDID_Buf[1024];
__u8 		Device_Support_VIC[512];
__u8		isHDMI = 0;
__u8		YCbCr444_Support = 0;
__u32		rgb_only = 0;
__s32		HPD = 0;
__u32		hdmi_print = 0;

__u32		hdmi_pll = 0;//0:video pll 0; 1:video pll 1
__u32		hdmi_clk = 297000000;

HDMI_VIDE_INFO video_timing[] = 
{
    //VIC                 PCLK  AVI_PR INPUTX INPUTY   HT   HBP   HFP HPSW  VT VBP VFP VPSW
    {HDMI1440_480I     ,  13500000, 1,  720,  240,  858, 119,  19, 62,  525, 18, 4, 3},
    {HDMI1440_576I     ,  13500000, 1,  720,  288,  864, 132,  12, 63,  625, 22, 2, 3},
    {HDMI480P          ,  27000000, 0,  720,  480,  858, 122,  16, 62, 1050, 36, 9, 6},
    {HDMI576P          ,  27000000, 0,  720,  576,  864, 132,  12, 64, 1250, 44, 5, 5},
    {HDMI720P_50       ,  74250000, 0, 1280,  720, 1980, 260, 440, 40, 1500, 25, 5, 5},
    {HDMI720P_60       ,  74250000, 0, 1280,  720, 1650, 260, 110, 40, 1500, 25, 5, 5},
    {HDMI1080I_50      ,  74250000, 0, 1920,  540, 2640, 192, 528, 44, 1125, 20, 2, 5},
    {HDMI1080I_60      ,  74250000, 0, 1920,  540, 2200, 192,  88, 44, 1125, 20, 2, 5},
    {HDMI1080P_50      , 148500000, 0, 1920, 1080, 2640, 192, 528, 44, 2250, 41, 4, 5},
    {HDMI1080P_60      , 148500000, 0, 1920, 1080, 2200, 192,  88, 44, 2250, 41, 4, 5},
    {HDMI1080P_24      ,  74250000, 0, 1920, 1080, 2750, 192, 638, 44, 2250, 41, 4, 5},
    {HDMI1080P_25 	   ,  74250000, 0, 1920, 1080, 2640, 192, 528, 44, 2250, 41, 4, 5},
    {HDMI1080P_30 	   ,  74250000, 0, 1920, 1080, 2200, 192,  88, 44, 2250, 41, 4, 5},
    {HDMI1080P_24_3D_FP, 148500000, 0, 1920, 2160, 2750, 192, 638, 44, 4500, 41, 4, 5},
    {HDMI720P_50_3D_FP , 148500000, 0, 1280, 1440, 1980, 260, 440, 40, 3000, 25, 5, 5},
    {HDMI720P_60_3D_FP , 148500000, 0, 1280, 1440, 1650, 260, 110, 40, 3000, 25, 5, 5},
};

__u32 hdmi_ishdcp(void)
{
    return 1;
}

__s32 hdmi_core_initial(void)
{
    hdmi_state      = HDMI_State_Idle;
    video_mode      = HDMI720P_50;
    memset(&audio_info,0,sizeof(HDMI_AUDIO_INFO));
    memset(Device_Support_VIC,0,sizeof(Device_Support_VIC));
    HDMI_WUINT32(0x004,0x80000000);				//start hdmi controller
    HDMI_WUINT32(0x200,0xe0000000);   			//power enable
    return 0;
}

__s32 main_Hpd_Check(void)
{
    __s32 i,times;
    times    = 0;

    for(i=0;i<3;i++)
    {
        hdmi_delay_ms(200);
        if( HDMI_RUINT32(0x00c)&0x01)
            times++;
    }
    if(times == 3)
        return 1;
    else
        return 0;
}

__s32 hdmi_main_task_loop(void)
{
    static __u32 times = 0;

    HPD = main_Hpd_Check();
    if( !HPD )
    {
        if((times++) >= 10)
        {
            times = 0;
            __inf("unplug state\n");
        }
        if(hdmi_state > HDMI_State_Wait_Hpd)
        {
            __inf("plugout\n");
            hdmi_state = HDMI_State_Wait_Hpd;
            Hdmi_hpd_event();
            HDMI_HDCP_SW(0);
        }
    }
    switch(hdmi_state)
    {
        case HDMI_State_Idle:
            hdmi_state = HDMI_State_Wait_Hpd;
            return 0;

        case HDMI_State_Wait_Hpd:
            if(HPD)
            {
                hdmi_state = HDMI_State_EDID_Parse;
                __inf("plugin\n");
            }else
            {
                return 0;
            }

        case HDMI_State_Rx_Sense:

        case HDMI_State_EDID_Parse:
            HDMI_WUINT32(0x004,0x80000000);
            HDMI_WUINT32(0x208,(1<<31)+ (1<<30)+ (1<<29)+ (3<<27)+ (0<<26)+
                    (1<<25)+ (0<<24)+ (0<<23)+ (4<<20)+ (7<<17)+
                    (15<<12)+ (7<<8)+ (0x0f<<4)+(8<<0) );
            HDMI_WUINT32(0x200,0xfe800000);   			//txen enable
            HDMI_WUINT32(0x204,0x00D8C850);   			//ckss = 1
            HDMI_WUINT32(0x20c, 1 << 21);
            ParseEDID();
            HDMI_RUINT32(0x5f0);
            hdmi_state = HDMI_State_Wait_Video_config;
            Hdmi_hpd_event();

        case HDMI_State_Wait_Video_config:
            if(video_enable)
            {
                HDMI_WUINT32(0x010,0x80000000);
                HDMI_WUINT32(0x024,0x03e00000);
                hdmi_state = 	HDMI_State_Video_config;
                HDMI_HDCP_SW((1<<7) + (1<<4) + (1<<0));         //switch to hdmi_hdcp mode
            }else
            {
                HDMI_WUINT32(0x010,0x00000000);
                return 0;
            }
        case HDMI_State_Video_config:
            access_Initialize((u8 *)0xf1ce0000);
            api_Initialize(0, 1, 2400, 1);	//force hpd
            if(video_enable == 1)
            {
                video_config(video_mode);
                hdmi_state = 	HDMI_State_Audio_config;
            }
        case HDMI_State_Audio_config:
            audio_config();
            hdmi_state = 	HDMI_State_Playback;
        case HDMI_State_Playback:
            return 0;
        default:
            __wrn(" unkonwn hdmi state, set to idle\n");
            hdmi_state = HDMI_State_Idle;
            return 0;
    }
}

__s32 Hpd_Check(void)
{
    if(HPD == 0)
    {
        return 0;
    }else if(hdmi_state >= HDMI_State_Wait_Video_config)
    {
        return 1;
    }else
    {
        return 0;
    }
}

__s32 get_video_info(__s32 vic)
{
    __s32 i,count;
    count = sizeof(video_timing);
    for(i=0;i<count;i++)
    {
        if(vic == video_timing[i].VIC)
        return i;
    }
    __wrn("can't find the video timing parameters\n");	
    return -1;
}

__s32 get_audio_info(__s32 sample_rate)
{
    //ACR_N 32000 44100 48000 88200 96000 176400 192000
    //		4096  6272  6144  12544 12288  25088  24576
    __inf("sample_rate:%d in get_audio_info\n", sample_rate);

    switch(sample_rate)
    {
        case 32000 :{	audio_info.ACR_N = 4096 ;
            audio_info.CH_STATUS0 = (3 <<24);
            audio_info.CH_STATUS1 = 0x0000000b;
            break;}
        case 44100 :{	audio_info.ACR_N = 6272 ;
            audio_info.CH_STATUS0 = (0 <<24);
            audio_info.CH_STATUS1 = 0x0000000b;
            break;}
        case 48000 :{	audio_info.ACR_N = 6144 ;
            audio_info.CH_STATUS0 = (2 <<24);
            audio_info.CH_STATUS1 = 0x0000000b;
            break;}
        case 88200 :{	audio_info.ACR_N = 12544;
            audio_info.CH_STATUS0 = (8 <<24);
            audio_info.CH_STATUS1 = 0x0000000b;
            break;}
        case 96000 :{	audio_info.ACR_N = 12288;
            audio_info.CH_STATUS0 = (10<<24);
            audio_info.CH_STATUS1 = 0x0000000b;
            break;}
        case 176400:{	audio_info.ACR_N = 25088;
            audio_info.CH_STATUS0 = (12<<24);
            audio_info.CH_STATUS1 = 0x0000000b;
            break;}
        case 192000:{	audio_info.ACR_N = 24576;
            audio_info.CH_STATUS0 = (14<<24);
            audio_info.CH_STATUS1 = 0x0000000b;
            break;}
        default:    {	__wrn("un-support sample_rate,value=%d\n",sample_rate);
                return -1;}
    }
            
    if((video_mode == HDMI1440_480I) || (video_mode == HDMI1440_576I) ||
        (video_mode == HDMI480P)     || (video_mode == HDMI576P))
    {
        audio_info.CTS =   ((27000000/100) *(audio_info.ACR_N /128)) / (sample_rate/100);
    }
    else if( (video_mode == HDMI720P_50 )||(video_mode == HDMI720P_60 ) ||
                 (video_mode == HDMI1080I_50)||(video_mode == HDMI1080I_60) ||
                 (video_mode == HDMI1080P_24)||(video_mode == HDMI1080P_25) ||
                 (video_mode == HDMI1080P_30))
    {
        audio_info.CTS =   ((74250000/100) *(audio_info.ACR_N /128)) / (sample_rate/100);
    }
    else if( (video_mode == HDMI1080P_50)||(video_mode == HDMI1080P_60)       ||
            (video_mode == HDMI1080P_24_3D_FP)||(video_mode == HDMI720P_50_3D_FP) ||
            (video_mode == HDMI720P_60_3D_FP) )
    {
        audio_info.CTS =   ((148500000/100) *(audio_info.ACR_N /128)) / (sample_rate/100);
    }
    else
    {
        __wrn("unkonwn video format when configure audio\n");
        return -1;
    }
    __inf("audio CTS calc:%d\n",audio_info.CTS);
    return 0;
}

__s32 video_config(__u32 vic)
{
    __s32 clk_div = 1,reg_val;
    //cts_enable = 0;
    //isHDMI = 1;
    //YCbCr444_Support = 0;
    //hdcp_enable = 0;
    __inf("video_config, vic:%d,cts_enable:%d,isHDMI:%d,YCbCr444_Support:%d,hdcp_enable:%d\n",
    vic,cts_enable,isHDMI,YCbCr444_Support,hdcp_enable);

    dtd_Fill(&dtd,vic,6000);
    videoParams_Reset(&video);
    if((cts_enable==1) && (isHDMI == 0))
    {
        videoParams_SetHdmi(&video, FALSE);
    }
    else
    {
        videoParams_SetHdmi(&video, TRUE);
    }

    if(cts_enable &&(!YCbCr444_Support))
    {
        videoParams_SetEncodingIn(&video, RGB);
        videoParams_SetEncodingOut(&video, RGB);
    }
    else
    {
        videoParams_SetEncodingIn(&video, YCC444);
        videoParams_SetEncodingOut(&video, YCC444);
    }
    videoParams_SetColorResolution(&video, 8);
    videoParams_SetPixelRepetitionFactor(&video, 0);
    videoParams_SetDtd(&video, &dtd);
    videoParams_SetPixelPackingDefaultPhase(&video, 0);
    videoParams_SetColorimetry(&video, 1);

    audioParams_Reset(&audio);
    audioParams_SetInterfaceType(&audio, I2S);
    audioParams_SetCodingType(&audio, PCM);
    audioParams_SetChannelAllocation(&audio, 0);
    audioParams_SetPacketType(&audio, AUDIO_SAMPLE);
    audioParams_SetSampleSize(&audio, 16);
    audioParams_SetSamplingFrequency(&audio, 44100);
    audioParams_SetLevelShiftValue(&audio, 0);
    audioParams_SetDownMixInhibitFlag(&audio, 0);
    audioParams_SetClockFsFactor(&audio, 64);

    productParams_Reset(&product);
    productParams_SetVendorName(&product, vName,sizeof(vName) - 1);
    productParams_SetProductName(&product, pName,sizeof(pName) - 1);
    productParams_SetSourceType(&product, 0x0A);

    hdcpParams_Reset(&hdcp);
    hdcpParams_SetEnable11Feature(&hdcp,FALSE);
    hdcpParams_SetEnhancedLinkVerification(&hdcp,TRUE);
    hdcpParams_SetI2cFastMode(&hdcp,FALSE);
    hdcpParams_SetRiCheck(&hdcp,TRUE);

    if((video_enable == 1) && (hdcp_enable == 1))
    {
        api_Configure(&video,&audio,&product,&hdcp);            			//hdmi full
        //clear ksv memory size bit

        reg_val = api_CoreRead(0x5016)| 0x01;
        api_CoreWrite(reg_val&0xff,0x5016);
        while((api_CoreRead(0x5016)| 0x02) == 0);
        api_CoreWrite(0x00,0x52b9);
        api_CoreWrite(0x00,0x52ba);
        api_CoreRead(0x52b9);
        api_CoreRead(0x52ba);
        api_CoreWrite(reg_val&0xfe,0x5016);

        //write an if necessary
        api_CoreWrite(0x01,0x7805);
        api_CoreWrite(0x03,0x7806);  //an0
        api_CoreWrite(0x04,0x7807);
        api_CoreWrite(0x07,0x7808);
        api_CoreWrite(0x0c,0x7809);
        api_CoreWrite(0x13,0x780a);
        api_CoreWrite(0x1c,0x780b);
        api_CoreWrite(0x27,0x780c);
        api_CoreWrite(0x34,0x780d);	//an7
        __inf("hdmi full function\n");
    }
    else if(video_enable==1)
    {
        api_Configure(&video,&audio,&product,(hdcpParams_t *)0);		//video + audio
        __inf("hdmi video + audio\n");
    }

        //////////////////////
        //hdmi pll setting
    if((vic == HDMI1440_480I) || (vic == HDMI1440_576I) ||
            (vic == HDMI480P)     || (vic == HDMI576P))
    {
        clk_div = hdmi_clk/27000000;
    }
    else if((vic == HDMI720P_50 ) || (vic == HDMI720P_60 ) ||
                (vic == HDMI1080I_50) || (vic == HDMI1080I_60) ||
                (vic == HDMI1080P_24) || (vic == HDMI1080P_25) ||
                (vic == HDMI1080P_30))
    {
        clk_div = hdmi_clk/74250000;
    }
    else if((vic == HDMI1080P_50) || (vic == HDMI1080P_60)          ||
                (vic == HDMI1080P_24_3D_FP)||(vic == HDMI720P_50_3D_FP) ||
                (vic == HDMI720P_60_3D_FP))
    {
        clk_div = hdmi_clk/148500000;
    }

    clk_div &= 0x0f;
    HDMI_WUINT32(0x208,(1<<31)+ (1<<30)+ (1<<29)+ (3<<27)+ (0<<26)+
            (1<<25)+ (0<<24)+ (0<<23)+ (4<<20)+ (7<<17)+
            (15<<12)+ (7<<8)+ (clk_div<<4)+(8<<0) );
    // tx driver setting
    HDMI_WUINT32(0x200,0xfe800000);   			//txen enable
    HDMI_WUINT32(0x204,0x00D8C850);   			//ckss = 1

    HDMI_WUINT32(0x20c, hdmi_pll << 21);
    return 0;
}

__s32 audio_config(void)
{
    __inf("audio_config, sample_rate:%d\n", audio_info.sample_rate);
    if(!audio_info.audio_en)
    {
        return 0;
    }
    audioParams_Reset(&audio);
    audioParams_SetInterfaceType(&audio, I2S);
    audioParams_SetCodingType(&audio, PCM);
    audioParams_SetChannelAllocation(&audio, 0);
    audioParams_SetPacketType(&audio, AUDIO_SAMPLE);
    audioParams_SetSampleSize(&audio, 16);
    audioParams_SetSamplingFrequency(&audio, audio_info.sample_rate);
    audioParams_SetLevelShiftValue(&audio, 0);
    audioParams_SetDownMixInhibitFlag(&audio, 0);
    audioParams_SetClockFsFactor(&audio, 64);
    audio_Configure(0, &audio, videoParams_GetPixelClock(&video), videoParams_GetRatioClock(&video));
    return 0;
}

