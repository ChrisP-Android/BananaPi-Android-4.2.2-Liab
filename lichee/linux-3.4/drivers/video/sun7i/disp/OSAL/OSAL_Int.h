/*
*************************************************************************************
*                         			eBsp
*					   Operation System Adapter Layer
*
*				(c) Copyright 2006-2010, All winners Co,Ld.
*							All	Rights Reserved
*
* File Name 	: OSAL_Int.h
*
* Author 		: javen
*
* Description 	: ?жϲ???
*
* History 		:
*      <author>    		<time>       	<version >    		<desc>
*       javen     	   2010-09-07          1.0         create this word
*
*************************************************************************************
*/
#ifndef  __OSAL_INT_H__
#define  __OSAL_INT_H__

#define __LINUX_OSAL__

#ifndef __LINUX_OSAL__
typedef __s32 (*ISRCallback)( void *);
#else
typedef int (*ISRCallback)( int, void* );
#endif

/*
*******************************************************************************
*                     OSAL_RegISR
*
* Description:
*    ע???жϷ???????
*
* Parameters:
*    irqno    	    ??input.  ?жϺ?
*    flags    	    ??input.  ?ж????ͣ?Ĭ??ֵΪ0??
*    Handler  	    ??input.  ?жϴ??????????ڣ??????ж??¼?????
*    pArg 	        ??input.  ????
*    DataSize 	    ??input.  ?????ĳ???
*    prio	        ??input.  ?ж????ȼ?

* 
* Return value:
*     ???سɹ?????ʧ?ܡ?
*
* note:
*    ?жϴ??���??ԭ?ͣ?typedef __s32 (*ISRCallback)( void *pArg)??
*
*******************************************************************************
*/
int OSAL_RegISR(__u32 IrqNo,
				__u32 Flags,
				ISRCallback Handler,
				void *pArg,
				__u32 DataSize,
				__u32 Prio);

/*
*******************************************************************************
*                     OSAL_UnRegISR
*
* Description:
*    ע???жϷ???????
*
* Parameters:
*    irqno    	??input.  ?жϺ?
*    handler  	??input.  ?жϴ??????????ڣ??????ж??¼?????
*    Argment 	??input.  ????
* 
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/
void OSAL_UnRegISR(__u32 IrqNo, ISRCallback Handler, void *pArg);

/*
*******************************************************************************
*                     OSAL_InterruptEnable
*
* Description:
*    ?ж?ʹ??
*
* Parameters:
*    irqno ??input.  ?жϺ?
* 
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/
void OSAL_InterruptEnable(__u32 IrqNo);

/*
*******************************************************************************
*                     OSAL_InterruptDisable
*
* Description:
*    ?жϽ?ֹ
*
* Parameters:
*     irqno ??input.  ?жϺ?
* 
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/
void OSAL_InterruptDisable(__u32 IrqNo);

#endif   //__OSAL_INT_H__


