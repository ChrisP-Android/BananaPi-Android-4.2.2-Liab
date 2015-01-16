/*
*************************************************************************************
*                         			eBsp
*					   Operation System Adapter Layer
*
*				(c) Copyright 2006-2010, All winners Co,Ld.
*							All	Rights Reserved
*
* File Name 	: OSAL_Clock.h
*
* Author 		: javen
*
* Description 	: ????ϵͳ??????
*
* History 		:
*      <author>    		<time>       	<version >    		<desc>
*       javen     	   	2010-09-07          1.0         create this word
*		holi			2010-12-03			1.1			ʵ???˾????Ľӿ?
*************************************************************************************
*/

#ifndef  __OSAL_CLOCK_H__
#define  __OSAL_CLOCK_H__

//#define __OSAL_CLOCK_MASK__


typedef enum
{
        SYS_CLK_PLL3,
        SYS_CLK_PLL7,
        SYS_CLK_PLL3X2,
        SYS_CLK_PLL5P,
        SYS_CLK_PLL6,
        SYS_CLK_PLL6M,
        SYS_CLK_PLL7X2,

        MOD_CLK_DEBE0,
        MOD_CLK_DEBE1,
        MOD_CLK_DEFE0,
        MOD_CLK_DEFE1,
        MOD_CLK_DEMIX,
        MOD_CLK_LCD0CH0,
        MOD_CLK_LCD0CH1_S1,
        MOD_CLK_LCD0CH1_S2,
        MOD_CLK_LCD1CH0,
        MOD_CLK_LCD1CH1_S1,
        MOD_CLK_LCD1CH1_S2,
        MOD_CLK_HDMI,
        MOD_CLK_HDMI_DDC,
        MOD_CLK_MIPIDSIS,
        MOD_CLK_MIPIDSIP,
        MOD_CLK_IEPDRC0,
        MOD_CLK_IEPDRC1,
        MOD_CLK_IEPDEU0,
        MOD_CLK_IEPDEU1,
        MOD_CLK_LVDS,

        AHB_CLK_MIPIDSI,
        AHB_CLK_LCD0,
        AHB_CLK_LCD1,
        AHB_CLK_CSI0,
        AHB_CLK_CSI1,
        AHB_CLK_HDMI,
        AHB_CLK_HDMI1,
        AHB_CLK_DEBE0,
        AHB_CLK_DEBE1,
        AHB_CLK_DEFE0,
        AHB_CLK_DEFE1,
        AHB_CLK_DEU0,
        AHB_CLK_DEU1,
        AHB_CLK_DRC0,
        AHB_CLK_DRC1,
        AHB_CLK_TVE0, 
        AHB_CLK_TVE1, 

        DRAM_CLK_DRC0,
        DRAM_CLK_DRC1,
        DRAM_CLK_DEU0,
        DRAM_CLK_DEU1,
        DRAM_CLK_DEFE0,
        DRAM_CLK_DEFE1,
        DRAM_CLK_DEBE0,
        DRAM_CLK_DEBE1,
}__disp_clk_id_t;

typedef struct
{
        __disp_clk_id_t       id;     /* clock id         */
        char        *name;  /* clock name       */
}__disp_clk_t;

/*
*********************************************************************************************************
*                                   SET SOURCE CLOCK FREQUENCY
*
* Description: 
*		set source clock frequency;
*
* Arguments  : 
*		nSclkNo  	:	source clock number;
*       nFreq   	:	frequency, the source clock will change to;
*
* Returns    : result;
*
* Note       :
*********************************************************************************************************
*/
__s32 OSAL_CCMU_SetSrcFreq(__u32 nSclkNo, __u32 nFreq);



/*
*********************************************************************************************************
*                                   GET SOURCE CLOCK FREQUENCY
*
* Description: 
*		get source clock frequency;
*
* Arguments  : 
*		nSclkNo  	:	source clock number need get frequency;
*
* Returns    : 
*		frequency of the source clock;
*
* Note       :
*********************************************************************************************************
*/
__u32 OSAL_CCMU_GetSrcFreq(__u32 nSclkNo);



/*
*********************************************************************************************************
*                                   OPEN MODULE CLK
* Description: 
*		open module clk;
*
* Arguments  : 
*		nMclkNo	:	number of module clock which need be open;
*
* Returns    : 
*		EBSP_TRUE/EBSP_FALSE
*
* Note       :
*********************************************************************************************************
*/
__hdle OSAL_CCMU_OpenMclk(__s32 nMclkNo);


/*
*********************************************************************************************************
*                                    CLOSE MODULE CLK
* Description: 
*		close module clk;
*
* Arguments  : 
*		hMclk	:	handle
*
* Returns    : 
*		EBSP_TRUE/EBSP_FALSE
*
* Note       :
*********************************************************************************************************
*/
__s32  OSAL_CCMU_CloseMclk(__hdle hMclk);

/*
*********************************************************************************************************
*                                   GET MODULE SRC
* Description: 
*		set module src;
*
* Arguments  : 
*		nMclkNo	:	number of module clock which need be open;
*       nSclkNo	:	call-back function for process clock change;
*
* Returns    : 
*		EBSP_TRUE/EBSP_FALSE
*
* Note       :
*********************************************************************************************************
*/
__s32 OSAL_CCMU_SetMclkSrc(__hdle hMclk, __u32 nSclkNo);





/*
*********************************************************************************************************
*                                  GET MODULE SRC
*
* Description: 
*		get module src;
*
* Arguments  : 
*		nMclkNo	:	handle of the module clock;
*
* Returns    : 
*		src no
*
* Note       :
*********************************************************************************************************
*/
__s32 OSAL_CCMU_GetMclkSrc(__hdle hMclk);




/*
*********************************************************************************************************
*                                   SET MODUEL CLOCK FREQUENCY
*
* Description: 
*		set module clock frequency;
*
* Arguments  : 
*		nSclkNo  :	number of source clock which the module clock will use;
*		nDiv     :	division for the module clock;
*
* Returns    : 
*		EBSP_TRUE/EBSP_FALSE
*
* Note       :
*********************************************************************************************************
*/
__s32 OSAL_CCMU_SetMclkDiv(__hdle hMclk, __s32 nDiv);



/*
*********************************************************************************************************
*                                   GET MODUEL CLOCK FREQUENCY
*
* Description: 
*		get module clock requency;
*
* Arguments  : 
*		hMclk    	:	module clock handle;
*
* Returns    : 
*		frequency of the module clock;
*
* Note       :
*********************************************************************************************************
*/
__u32 OSAL_CCMU_GetMclkDiv(__hdle hMclk);



/*
*********************************************************************************************************
*                                   MODUEL CLOCK ON/OFF
*
* Description: 
*		module clock on/off;
*
* Arguments  : 
*		nMclkNo		:	module clock handle;
*       bOnOff   	:	on or off;
*
* Returns    : 
*		EBSP_TRUE/EBSP_FALSE
*
* Note       :
*********************************************************************************************************
*/
__s32 OSAL_CCMU_MclkOnOff(__hdle hMclk, __s32 bOnOff);

__s32 OSAL_CCMU_MclkReset(__hdle hMclk, __s32 bReset);


/*
//??һ??
__s32  esCLK_SetSrcFreq(__s32 nSclkNo, __u32 nFreq);
__u32  esCLK_GetSrcFreq(__s32 nSclkNo);

__hdle esCLK_OpenMclk(__s32 nMclkNo, __pCB_ClkCtl_t pCb);
__s32  esCLK_CloseMclk(__hdle hMclk);

__s32  esCLK_SetMclkSrc(__s32 nMclkNo, __s32 nSclkNo);
__s32  esCLK_GetMclkSrc(__s32 nMclkNo);

__s32  esCLK_SetMclkDiv(__s32 nMclkNo, __s32 nDiv);
__u32  esCLK_GetMclkDiv(__s32 nMclkNo);

__s32  esCLK_MclkOnOff(__s32 nMclkNo, __s32 bOnOff);

//======================================================================================

//?ڶ???
__s32 esCLK_reg_cb(__s32 nMclkNo, __pCB_ClkCtl_t pCb);	//__hdle esCLK_OpenMclk(__s32 nMclkNo, __pCB_ClkCtl_t pCb);
__s32  esCLK_unreg_cb(__s32 nMclkNo);					//__s32  esCLK_CloseMclk(__hdle hMclk);

//------------------------------------------------------

					__s32  esCLK_SetSrcFreq(__s32 nSclkNo, __u32 nFreq);
					__u32  esCLK_GetSrcFreq(__s32 nSclkNo);


__hdle esCLK_OpenMclk(__s32 nMclkNo);
__s32  esCLK_CloseMclk(__hdle hMclk);



__s32  esCLK_SetMclkSrc(__hdle hMclk, __s32 nSclkNo);	//__s32  esCLK_SetMclkSrc(__s32 nMclkNo, __s32 nSclkNo);
__s32  esCLK_GetMclkSrc(__hdle hMclk);					//__s32  esCLK_GetMclkSrc(__s32 nMclkNo);

__s32  esCLK_SetMclkDiv(__hdle hMclk, __s32 nDiv);
__u32  esCLK_GetMclkDiv(__hdle hMclk);

__s32  esCLK_MclkOnOff(__hdle hMclk, __s32 bOnOff);


*/

#endif   //__OSAL_CLOCK_H__

