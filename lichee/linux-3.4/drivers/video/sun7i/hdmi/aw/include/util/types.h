/*
 * types.h
 *
 *  Created on: Jun 25, 2010
 *
 *  Synopsys Inc.
 *  SG DWC PT02
 */

#ifndef TYPES_H_
#define TYPES_H_

/*
 * @file: types
 * Define basic type optimised for use in the API so that it can be
 * platform-independent.
 */
#include <linux/types.h>
#include <linux/bitops.h>
#include <linux/types.h>
#ifdef __XMK__
#include "xmk.h"
#include "xutil.h"
#include "pthread.h"
typedef pthread_mutex_t mutex_t;

#else
typedef unsigned char	u8;
typedef unsigned short	u16;
typedef unsigned int	u32;
typedef void* mutex_t;
#endif

//#define BIT(n)		(1 << n)

typedef void (*handler_t)(void *);
typedef void* (*thread_t)(void *);

#define TRUE  1
#define FALSE 0
//#define NULL 0
//typedef long unsigned int size_t;
typedef struct
{
    u8* buffer;
    size_t size;
}
buffer_t;

#endif /* TYPES_H_ */
