/*
*************************************************************************************
*                         			eBsp
*					   Operation System Adapter Layer
*
*				(c) Copyright 2006-2010, All winners Co,Ld.
*							All	Rights Reserved
*
* File Name 	: OSAL_time.h
*
* Author 		: javen
*
* Description 	: Time????
*
* History 		:
*      <author>    		<time>       	<version >    		<desc>
*       javen     	   2010-09-07          1.0         create this word
*
*************************************************************************************
*/
#include "OSAL.h"

/*
*******************************************************************************
*                     OSAL_CreateTimer
*
* Description:
*    ??ʼ??һ??timer
*
* Parameters:
*    Period     :  input. ????ʱ??
*    EventType  :  input. ?¼??????????ͣ?һ?λ??Ƕ??Ρ?
*    CallBack   :  input. ?ص?????
*    pArg       :  input. ?ص??????Ĳ???
* 
* Return value:
*    ????timer????
*
* note:
*    void
*
*******************************************************************************
*/
__hdle OSAL_CreateTimer(__u32 Period, __u32 EventType, TIMECALLBACK CallBack, void *pArg)
{
    return 0;
}

/*
*******************************************************************************
*                     OSAL_DelTimer
*
* Description:
*    ɾ??timer
*
* Parameters:
*    HTimer  :  input. OSAL_InitTimer????timer????
* 
* Return value:
*    ???سɹ?????ʧ??
*
* note:
*    void
*
*******************************************************************************
*/
__s32 OSAL_DelTimer(__hdle HTimer)
{
    return 0;
}

/*
*******************************************************************************
*                     OSAL_StartTimer
*
* Description:
*    ??ʼtimer??ʱ
*
* Parameters:
*    HTimer  :  input. OSAL_InitTimer????timer????
* 
* Return value:
*    ???سɹ?????ʧ??
*
* note:
*    void
*
*******************************************************************************
*/
__s32 OSAL_StartTimer(__hdle HTimer)
{
	return 0;
}

/* ˯?? *//* ??λ?????? */
void OSAL_Sleep(__u32 Milliseconds)
{

}


