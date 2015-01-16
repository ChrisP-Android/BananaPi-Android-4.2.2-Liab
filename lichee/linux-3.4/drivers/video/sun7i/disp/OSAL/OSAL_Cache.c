/*
*************************************************************************************
*                         			eBsp
*					   Operation System Adapter Layer
*
*				(c) Copyright 2006-2010, All winners Co,Ld.
*							All	Rights Reserved
*
* File Name 	: OSAL_Cache.h
*
* Author 		: javen
*
* Description 	: Cache????
*
* History 		:
*      <author>    		<time>       	<version >    		<desc>
*       javen     	   2010-09-07          1.0         create this word
*
*************************************************************************************
*/
#include "OSAL.h"
#include "asm/cacheflush.h"
#include <asm/system.h>


/* ˢ?±???λ */
#define  CACHE_FLUSH_I_CACHE_REGION				0  /* ????I-cache?д?????????һ????????cache?? 			*/
#define  CACHE_FLUSH_D_CACHE_REGION				1  /* ????D-cache?д?????????һ????????cache?? 			*/
#define  CACHE_FLUSH_CACHE_REGION				2  /* ????D-cache??I-cache?д?????????һ????????cache?? */
#define  CACHE_CLEAN_D_CACHE_REGION				3  /* ????D-cache?д?????????һ????????cache?? 			*/
#define  CACHE_CLEAN_FLUSH_D_CACHE_REGION	 	4  /* ???���????D-cache?д?????????һ????????cache?? 	*/
#define  CACHE_CLEAN_FLUSH_CACHE_REGION			5  /* ???���????D-cache??????��????I-cache 				*/

/*
*******************************************************************************
*                     OSAL_CacheRangeFlush
*
* Description:
*    Cache????
*
* Parameters:
*    Address    :  Ҫ??ˢ?µ???????ʼ??ַ
*    Length     :  ??ˢ?µĴ?С
*    Flags      :  ˢ?±???λ
*    
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/
void OSAL_CacheRangeFlush(void*Address, __u32 Length, __u32 Flags)
{

    //flush_cache_range(NULL, (unsigned long)Address, ((unsigned long)Address)+Length);
        if(Address == NULL || Length == 0)
        {
            return;
        }
        
        switch(Flags)
        {
        case CACHE_FLUSH_I_CACHE_REGION:
           
            break;
            
        case CACHE_FLUSH_D_CACHE_REGION:
            //__cpuc_flush_dcache_area(phys_to_virt((phys_addr_t)Address), Length);
           // flush_cach
            break;
            
        case CACHE_FLUSH_CACHE_REGION:
            
            break;
            
        case CACHE_CLEAN_D_CACHE_REGION:
            //clean_dcache_area((unsigned long)Address, Length);
            break;
            
        case CACHE_CLEAN_FLUSH_D_CACHE_REGION:
            //__cpuc_flush_dcache_area(phys_to_virt((phys_addr_t)Address), Length);
            //smp_mb();
            //mb();
            break;        
            
        case CACHE_CLEAN_FLUSH_CACHE_REGION:
            
            break;
            
        default:
           
            break;
        }
        return;
}



