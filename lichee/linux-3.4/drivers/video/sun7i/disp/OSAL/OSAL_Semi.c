/*
*************************************************************************************
*                         			eBsp
*					   Operation System Adapter Layer
*
*				(c) Copyright 2006-2010, All winners Co,Ld.
*							All	Rights Reserved
*
* File Name 	: OSAL_Semi.h
*
* Author 		: javen
*
* Description 	: ?藕?量????
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
*                     eBase_CreateSemaphore
*
* Description:
*    ?????藕?量
*
* Parameters:
*    Count  :  input.  ?藕?量?某?始值??
* 
* Return value:
*    ?晒????????藕?量??????失?埽?????NULL??
*
* note:
*    void
*
*******************************************************************************
*/
OSAL_SemHdle OSAL_CreateSemaphore(__u32 Count)
{
	return 0;
}

/*
*******************************************************************************
*                     OSAL_DeleteSemaphore
*
* Description:
*    删???藕?量
*
* Parameters:
*    SemHdle  :  input.  OSAL_CreateSemaphore ?????? ?藕?量????
* 
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/
void OSAL_DeleteSemaphore(OSAL_SemHdle SemHdle)
{


}

/*
*******************************************************************************
*                     OSAL_SemPend
*
* Description:
*    ???藕?量
*
* Parameters:
*    SemHdle  :  input.  OSAL_CreateSemaphore ?????? ?藕?量????
* 
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/
void OSAL_SemPend(OSAL_SemHdle SemHdle, __u16 TimeOut)
{

}

/*
*******************************************************************************
*                     OSAL_SemPost
*
* Description:
*    ?藕?量????
*
* Parameters:
*    SemHdle  :  input.  OSAL_CreateSemaphore ?????? ?藕?量????
* 
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/
void OSAL_SemPost(OSAL_SemHdle SemHdle)
{

}




