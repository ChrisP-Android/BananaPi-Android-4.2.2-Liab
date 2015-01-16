/*
*********************************************************************************************************
*                                                    LINUX-KERNEL
*                                        newbie Linux Platform Develop Kits
*                                                   Kernel Module
*
*                                    (c) Copyright 2006-2011, kevin.z China
*                                             All Rights Reserved
*
* File    : standby_ir.h
* By      : kevin.z
* Version : v1.0
* Date    : 2011-5-31 15:15
* Descript:
* Update  : date                auther      ver     notes
*********************************************************************************************************
*/

#ifndef __STANDBY_IR_H__
#define __STANDBY_IR_H__

#include "standby_cfg.h"
//add by fe3o4 <<<
#define STANDBY_IR_BUF_SIZE     128

#define IR_RXINT_ROI_EN   (1<<0)  //FIFO overrun
#define IR_RXINT_RPEI_EN  (1<<1)  //receiver packet end interrupt
#define IR_RXINT_RAI_EN   (1<<4)  //RX fifo available interrupt
#define IR_RXINT_DRQ_EN   (1<<5)  //DRQ enable
#define IR_RXINT_ALL_MASK  (IR_RXINT_ROI_EN | IR_RXINT_RPEI_EN | IR_RXINT_RAI_EN | (IR_RXINT_DRQ_EN))

#define IR_RXINT_RAL_MASK   (0x3f<<8)  


#define IR_RXSTA_RPE_MASK   (1<<1) //packet end flag
#define IR_RXSTA_ALL_MASK   (0x1f)
#define IR_RXSTA_RAC_MASK  (0x7f<<8) /* rxFIFO Available Counter.0:no data,64:64 byte in rxFIFO */

#define IR_REG_RXFIFO (SW_VA_IR0_IO_BASE + 0x20)
//define ir controller registers
typedef struct __STANDBY_IR_REG
{
    volatile __u32   CIR_Ctl;
    volatile __u32   CIR_Reserve0[3];
    volatile __u32   CIR_RxCtl;
    volatile __u32   CIR_Reserve1[3];
    volatile __u32   CIR_RxFifo;
    volatile __u32   CIR_Reserve2[2];
    volatile __u32   CIR_RxInt;
    volatile __u32   CIR_RxSta;
    volatile __u32   CIR_Config;
} __standby_ir_reg_t;


struct standby_ir_buffer	
{
	unsigned long dcnt;                  /*Packet Count*/
	unsigned char buf[STANDBY_IR_BUF_SIZE];	
};
//add end >>>

extern __s32 standby_ir_init(void);
extern __s32 standby_ir_exit(void);
extern __s32 standby_ir_detect(void);
extern __s32 standby_ir_verify(void);

#endif  /*__STANDBY_IR_H__*/
