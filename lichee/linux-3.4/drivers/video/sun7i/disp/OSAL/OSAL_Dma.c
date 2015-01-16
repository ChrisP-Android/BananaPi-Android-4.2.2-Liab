/*
*************************************************************************************
*                         			eBsp
*					   Operation System Adapter Layer
*
*				(c) Copyright 2006-2010, All winners Co,Ld.
*							All	Rights Reserved
*
* File Name 	: OSAL_Dma.h
*
* Author 		: javen
*
* Description 	: Dma????
*
* History 		:
*      <author>    		<time>       	<version >    		<desc>
*       javen     	   	2010-09-07          1.0         create this word
*		holi			2010-12-04			1.1			?????Ĳ??????֣???ȫ??CSP_para????·
*************************************************************************************
*/


#include "OSAL.h"



/*
*******************************************************************************
*                     OSAL_DmaRequest
*
* Description:
*    ????DMAͨ?��?
*
* Parameters:
*	 user_name 	:	ģ???�����??ͳ??
*    DmaType  	:  	input. DMA???͡?Normal or Dedicated
* 
* Return value:
*    ?ɹ?????DMA??????ʧ?ܷ???NULL??
*
* note:
*    void
*
*******************************************************************************
*/
__hdle OSAL_DmaRequest(u8 * user_name ,__u32 DmaType)
{
	return 0;
}

/*
*******************************************************************************
*                     OSAL_DmaRelease
*
* Description:
*    ????DMAͨ?��?
*
* Parameters:
*    hDMA ?? input. cspRequestDma?????ľ?????
* 
* Return value:
*    ?ɹ?????EBSP_OK??ʧ?ܷ???EBSP_FAIL??
*
* note:
*    void
*
*******************************************************************************
*/
__s32 OSAL_DmaRelease(__hdle hDMA)
{
	return 0;
}


/*
*******************************************************************************
*                     OSAL_DmaEnableINT
*
* Description:
*    ʹ??DMA?ж?
*
* Parameters:
*    hDMA 	    :  input. cspRequestDma?????ľ?????
*    IrqType    :  input. ???????͡?end_irq or half_irq??
* 
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/
__s32 OSAL_DmaEnableINT(__hdle hDMA, __s32 IrqType)
{
	return 0;
}

/*
*******************************************************************************
*                     OSAL_DmaDisableINT
*
* Description:
*    ??ֹDMA?ж?
*
* Parameters:
*    hDMA 	    :  input. cspRequestDma?????ľ?????
*    IrqType    :  input. ???????͡?end_irq or half_irq??
* 
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/
__s32 OSAL_DmaDisableINT(__hdle hDMA, __s32 IrqType)
{
	return 0;
	}

/*
*******************************************************************************
*                     eBsp_DmaRegIrq
*
* Description:
*    ע???жϴ??���????
*
* Parameters:
*    hDMA 	    :  input. cspRequestDma?????ľ?????
*    IrqType    :  input. ?ж????͡?end_irq or half_irq??
*    pCallBack  :  input. ?жϻص???????
*    pArg		:  input. ?жϻص??????Ĳ?????
* 
* Return value:
*    ?ɹ?????DMA??????ʧ?ܷ???NULL??
*
* note:
*    ?ص???????ԭ?ͣ?typedef void (*DmaCallback)(void *pArg);
*
*******************************************************************************
*/
__s32 OSAL_DmaRegIrq(__hdle hDMA, __u32 IrqType, DmaCallback pCallBack, void *pArg)
{
	return 0;
	}

/*
*******************************************************************************
*                     FunctionName
*
* Description:
*    ע???жϴ??���????
*
* Parameters:
*    hDMA 	    :  input. cspRequestDma?????ľ?????
*    IrqType    :  input. ???????͡?end_irq or half_irq??
*    pCallBack  :  input. ?жϻص???????
* 
* Return value:
*    ?ɹ?????DMA??????ʧ?ܷ???NULL??
*
* note:
*    void
*
*******************************************************************************
*/
__s32 OSAL_DmaUnRegIrq(__hdle hDMA, __u32 IrqType, DmaCallback pCallBack)
{
	return 0;
	}

/*
*******************************************************************************
*                     OSAL_DmaConfig
*
* Description:
*    ????DMA ͨ?��????????á?
*
* Parameters:
*    hDMA 	     :  input. cspRequestDma?????ľ?????
*    p_cfg       :  input.  DMA???á?,ʵ?????ݽṹ??????struct CSP_dma_config{}
* 
* Return value:
*    ?ɹ?????EBSP_OK??ʧ?ܷ???EBSP_FAIL??
*
* note:
*    void
*
*******************************************************************************
*/
__s32 OSAL_DmaConfig(__hdle hDMA, void * p_cfg)
{
	return 0;
	}

/*
*******************************************************************************
*                     OSAL_DmaStart
*
* Description:
*    ??ʼ DMA ???䡣
*
* Parameters:
*    hDMA 	 		 :  input. cspRequestDma?????ľ?????
*    SrcAddr		 :  input. Դ??ַ
*    DestAddr		 :  input. Ŀ????ַ
*    TransferLength  :  input. ???䳤??
* 
* Return value:
*    ?ɹ?????EBSP_OK??ʧ?ܷ???EBSP_FAIL??
*
* note:
*    void
*
*******************************************************************************
*/
__s32 OSAL_DmaStart(__hdle hDMA, __u32 SrcAddr, __u32 DestAddr, __u32 TransferLength)
{
	return 0;
	}

/*
*******************************************************************************
*                     OSAL_DmaStop
*
* Description:
*    ֹͣ????DMA ???䡣
*
* Parameters:
*    hDMA ?? input. cspRequestDma?????ľ?????
* 
* Return value:
*    ?ɹ?????EBSP_OK??ʧ?ܷ???EBSP_FAIL??
*
* note:
*    void
*
*******************************************************************************
*/
__s32 OSAL_DmaStop(__hdle hDMA)
{
	return 0;
	}

/*
*******************************************************************************
*                     OSAL_DmaRestart
*
* Description:
*    ??????һ??DMA???䡣
*
* Parameters:
*    hDMA 	?? input. cspRequestDma?????ľ?????
* 
* Return value:
*    ?ɹ?????EBSP_OK??ʧ?ܷ???EBSP_FAIL??
*
* note:
*    void
*
*******************************************************************************
*/
__s32 OSAL_DmaRestart(__hdle hDMA)
{
	return 0;
	}

/*
*******************************************************************************
*                     OSAL_DmaQueryChannelNo
*
* Description:
*    ??ѯDMA??ͨ?��š?
*
* Parameters:
*    hDMA  ?? input. cspRequestDma?????ľ?????
* 
* Return value:
*    ????DMAͨ?��š?
*
* note:
*    void
*
*******************************************************************************
*/
__u32 OSAL_DmaQueryChannelNo(__hdle hDMA)
{
	return 0;
	}

/*
*******************************************************************************
*                     OSAL_DmaQueryStatus
*
* Description:
*    ??ѯDMA??ͨ?��?״̬??Busy or Idle??
*
* Parameters:
*    hDMA ?? input. cspRequestDma?????ľ?????
* 
* Return value:
*    ???ص?ǰDMAͨ?��?״̬??1??busy??0??idle??
*
* note:
*    void
*
*******************************************************************************
*/
__u32 OSAL_DmaQueryStatus(__hdle hDMA)
{
	return 0;
	}

/*
*******************************************************************************
*                     OSAL_DmaQueryLeftCount
*
* Description:
*    ??ѯDMA??ʣ???ֽ?????
*
* Parameters:
*    hDMA  :  input. cspRequestDma?????ľ?????
* 
* Return value:
*    ???ص?ǰDMA??ʣ???ֽ?????
*
* note:
*    void
*
*******************************************************************************
*/
__u32 OSAL_DmaQueryLeftCount(__hdle hDMA)
{
	return 0;
	}

/*
*******************************************************************************
*                     OSAL_DmaQueryConfig
*
* Description:
*    ??ѯDMAͨ?��????á?
*
* Parameters:
*    hDMA 	   :  input. cspRequestDma?????ľ?????
*    RegAddr   :  input. ?Ĵ?????ַ
*    RegWidth  :  input. ?Ĵ???????
*    RegValue  :  output. ?Ĵ???ֵ
* 
* Return value:
*    ?ɹ?????EBSP_OK??ʧ?ܷ???EBSP_FAIL??
*
* note:
*    void
*
*******************************************************************************
*/
__s32 OSAL_DmaQueryConfig(__hdle hDMA, __u32 RegAddr, __u32 RegWidth, __u32 *RegValue)
{
	return 0;
	}
/*
*******************************************************************************
*                     eBsp_DmaPause
*
* Description:
*    ??ͣDMA???䡣
*
* Parameters:
*    hDMA  ?? input. cspRequestDma?????ľ?????
* 
* Return value:
*    ?ɹ?????EBSP_OK??ʧ?ܷ???EBSP_FAIL??
*
* note:
*    void
*
*******************************************************************************
*/
__s32 OSAL_DmaPause(__hdle hDMA){
	return 0;
	}
/*
*******************************************************************************
*                     eBsp_DmaProceed
*
* Description:
*    ????csp_DmaPause ??ͣ??DMA???䡣
*
* Parameters:
*    hDMA  ?? input. cspRequestDma?????ľ?????
* 
* Return value:
*    ?ɹ?????EBSP_OK??ʧ?ܷ???EBSP_FAIL??
*
* note:
*    void
*
*******************************************************************************
*/
__s32 OSAL_DmaProceed(__hdle hDMA){
	return 0;
	}
/*
*******************************************************************************
*                     OSAL_DmaChangeMode
*
* Description:
*    ?л? DMA ?Ĵ???ģʽ??
*
* Parameters:
*    hDMA  ?? input. cspRequestDma?????ľ?????
*    mode  :  input. ????ģʽ
* 
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/
__s32 OSAL_DmaChangeMode(__hdle hDMA, __s32 mode){
	return 0;
	}

