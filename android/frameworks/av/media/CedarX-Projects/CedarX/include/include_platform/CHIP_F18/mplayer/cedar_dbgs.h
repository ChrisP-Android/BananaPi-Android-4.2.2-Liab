/*
**********************************************************************************************************************
*                                                    ePDK
*                                    the Easy Portable/Player Develop Kits
*                                              eDBG Sub-System
*
*                                   (c) Copyright 2007-2009, Steven.ZGJ.China
*                                             All Rights Reserved
*
* Moudle  : debug define module
* File    : dbgs.h
*
* By      : Steven
* Version : v1.0
* Date    : 2008-10-21 9:06:55
**********************************************************************************************************************
*/

#ifndef _DBGS_H_
#define _DBGS_H_

#define __WRN
#define __INF
#define __MSG
#define __HERE

#ifdef __ASSERT
    #define __ast(condition)    (if(!condition)                                                     \
                                 printf("AST:L%d(%s): condition err!", __LINE__, __FILE__)   )
#else
    #define __ast(condition)
#endif

#if defined(__MSG)
	#define __msg(...)    		(printf("MSG:L%d(%s):", __LINE__, __FILE__),                 \
							     printf(__VA_ARGS__)									        )
#else
    #define __msg(...)
#endif
 
#if defined(__WRN)
    #define __wrn(...)          (printf("WRN:L%d(%s):", __LINE__, __FILE__),                 \
    						     printf(__VA_ARGS__)									        )
#else
    #define __wrn(...)
#endif

#if defined(__INF)
    #define __inf(...)           printf(__VA_ARGS__)
#else
    #define __inf(...)
#endif

#if defined(__HERE)
    #define __here            printf("@L%d(%s)\n", __LINE__, __FILE__);
    #define __wait            (printf("@L%d(%s)(press any key:\n", __LINE__, __FILE__),                 \
                              getchar())
#else
    #define __here
    #define __wait
#endif

#define INFORMATION __inf
#define WARNING __wrn
//#define MESSAGE __msg

//#include "memwatch.h"

#endif  /* _DBGS_H_ */

