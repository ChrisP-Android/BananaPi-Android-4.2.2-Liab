/*
*********************************************************************************************************
*                                                    LINUX-KERNEL
*                                        newbie Linux Platform Develop Kits
*                                                   Kernel Module
*
*                                    (c) Copyright 2006-2011, kevin.z China
*                                             All Rights Reserved
*
* File    : standby_ir.c
* By      : kevin.z
* Version : v1.0
* Date    : 2011-5-31 14:36
* Descript:
* Update  : date                auther      ver     notes
*********************************************************************************************************
*/
#include  "standby_i.h"
//add by fe3o4 <<<
__standby_ir_reg_t *IR_reg;
__standby_ir_reg_t IR_reg_bak;
extern struct standby_ir_buffer ir_buffer;

#define IR_DMA_BYTES 100
//add end >>>
/*
*********************************************************************************************************
*                           INIT IR FOR STANDBY
*
*Description: init ir for standby;
*
*Arguments  : none
*
*Return     : result;
*               EPDK_OK,    init ir successed;
*               EPDK_FAIL,  init ir failed;
*********************************************************************************************************
*/
//add by fe3o4 <<<
#define DMAC_REG_INT (SW_VA_DMAC_IO_BASE)
#define DMAC_REG_STA (SW_VA_DMAC_IO_BASE + 0x4)
#define DMAC_REG_N1_CFG (SW_VA_DMAC_IO_BASE + 0x120)
#define DMAC_REG_N1_SRC_ADDR (SW_VA_DMAC_IO_BASE + 0x124)
#define DMAC_REG_N1_DST_ADDR (SW_VA_DMAC_IO_BASE + 0x128)
#define DMAC_REG_N1_BYTE_CNT (SW_VA_DMAC_IO_BASE + 0x12c)

__u32  g_dma_cfg = 0;
__u32  g_dma_src = 0;
__u32  g_dma_dst = 0;
__u32  g_dma_byte = 0;

__u32  g_ir_rxint = 0;
//add end >>>
__s32  standby_ir_init(void)
{
	//add by fe3o4 <<<
    __u32   daddr;
    __u32   *dmac_cfg;
    __u32   *dmac_src;
    __u32   *dmac_dst;
    __u32   *dmac_byte;
    __u32   reg_val;

    IR_reg = (__standby_ir_reg_t *)SW_VA_IR0_IO_BASE;
    g_ir_rxint = IR_reg->CIR_RxInt;

//    IR_reg->CIR_RxInt &= ~IR_RXINT_ALL_MASK;
    IR_reg->CIR_RxInt |= IR_RXINT_DRQ_EN;/* 开启DRQ中断 */
    IR_reg->CIR_RxInt |= IR_RXINT_RPEI_EN;/* 开启接收包完成中断 */
    IR_reg->CIR_RxInt &= ~IR_RXINT_RAL_MASK;/*长度满1个byte启动DRQ*/
    IR_reg->CIR_RxSta |= IR_RXSTA_ALL_MASK; /*清除pending位*/
    
    /*back up dma register*/
    dmac_cfg = (__u32   *)DMAC_REG_N1_CFG;
    dmac_src = (__u32   *)DMAC_REG_N1_SRC_ADDR;
    dmac_dst = (__u32   *)DMAC_REG_N1_DST_ADDR;
    dmac_byte = (__u32   *)DMAC_REG_N1_BYTE_CNT;
    g_dma_cfg = *dmac_cfg;
    g_dma_src = *dmac_src;
    g_dma_dst = *dmac_dst;
    g_dma_byte = *dmac_byte;
    
    /* 配置DMA
    *dmac_cfg = 0;
    *dmac_cfg |= (0 << 30);  // 1bit , dma continuous mode disable
    *dmac_cfg |= (2 << 27);  // 3bit , wait for 2 dma clock to request
    *dmac_cfg |= (0 << 25);  // 2bits, dst data width -- 8 bit
    *dmac_cfg |= (0 << 23);  // 2bits, dst bst len -- 1
    *dmac_cfg |= (0 << 21);  // 1bit , dst addr increment
    *dmac_cfg |= (0x15 << 16);  // 5bit(10101) , dma dst drq type: sram
    *dmac_cfg |= (1 << 15);  // 1bit , remain mode
    *dmac_cfg |= (0 << 9);  // 2bit , src data width -- 8 bit
    *dmac_cfg |= (0 << 7);  // 2bit , src bst len -- 1
    *dmac_cfg |= (1 << 5);  // 1bit , src addr no change
    *dmac_cfg |= (0 << 0);  // 5bit , dma dst drq type: IR-RX */
    *dmac_cfg = (0x15 << 16)|(1<<5)|(1<<15)|(2<<27); 
    *dmac_src = (__u32)(0x0fffffff & IR_REG_RXFIFO);/* 设置接收寄存器为源地址 */
    *dmac_dst = (__u32)((__u32)ir_buffer.buf & 0x000fffff); /* 目的物理地址 */
    *dmac_byte = IR_DMA_BYTES;  /* 暂时定在取100个字节数据，这个值比实际的一帧IR数据字节数要少一点  */

    /* 忽略无用的数据 */
    while(IR_RXSTA_RAC_MASK & (IR_reg->CIR_RxSta))
    {
        reg_val = IR_reg->CIR_RxFifo; 
    }
    /* pending清零 */
    IR_reg->CIR_RxSta |= IR_RXSTA_ALL_MASK;

    /* 启动DMA接收数据 */
    *dmac_cfg |= (__u32)((1 << 31) | (0x15 << 16));
	//add end >>>
    return 0;
}


/*
*********************************************************************************************************
*                           EXIT IR FOR STANDBY
*
*Description: exit ir for standby;
*
*Arguments  : none;
*
*Return     : result.
*               EPDK_OK,    exit ir successed;
*               EPDK_FAIL,  exit ir failed;
*********************************************************************************************************
*/
__s32 standby_ir_exit(void)
{
	//add by fe3o4 <<<
    __u32   saddr, daddr;
    __u32   *dmac_cfg;
    __u32   *dmac_src;
    __u32   *dmac_dst;
    __u32   *dmac_byte;
    __u32   reg_val;

    IR_reg = (__standby_ir_reg_t *)SW_VA_IR0_IO_BASE;
    
    dmac_cfg = (__u32   *)DMAC_REG_N1_CFG;
    dmac_src = (__u32   *)DMAC_REG_N1_SRC_ADDR;
    dmac_dst = (__u32   *)DMAC_REG_N1_DST_ADDR;
    dmac_byte = (__u32   *)DMAC_REG_N1_BYTE_CNT;

    if (mem_query_int(INT_SOURCE_IR0) == 0)    /*返回值为0表示有中断*/
    {
        /*等待dma传输完毕*/
        while (dmac_byte != 0 && (IR_reg->CIR_RxSta&IR_RXSTA_RPE_MASK) == 0);
        ir_buffer.dcnt = IR_DMA_BYTES - *dmac_byte;
    }
	/*
    standby_put_dec(ir_buffer.dcnt);
    standby_put_hex_ext(ir_buffer.buf,ir_buffer.dcnt);
    */

    *dmac_cfg = g_dma_cfg;
    *dmac_src = g_dma_src;
    *dmac_dst = g_dma_dst;
    *dmac_byte = g_dma_byte;
    IR_reg->CIR_RxInt = g_ir_rxint;
	//add end >>>
    return 0;
}


/*
*********************************************************************************************************
*                           DETECT IR FOR STANDBY
*
*Description: detect ir for standby;
*
*Arguments  : none
*
*Return     : result;
*               EPDK_OK,    receive some signal;
*               EPDK_FAIL,  no signal;
*********************************************************************************************************
*/
__s32 standby_ir_detect(void)
{
    return 0;
}

/*
*********************************************************************************************************
*                           VERIFY IR SIGNAL FOR STANDBY
*
*Description: verify ir signal for standby;
*
*Arguments  : none
*
*Return     : result;
*               EPDK_OK,    valid ir signal;
*               EPDK_FAIL,  invalid ir signal;
*********************************************************************************************************
*/
__s32 standby_ir_verify(void)
{
    return -1;
}

