/*
*********************************************************************************************************
*											        eBIOS
*						            the Easy Portable/Player Develop Kits
*									           dma sub system
*
*						        (c) Copyright 2006-2008, David China
*											All	Rights Reserved
*
* File    : clk_for_nand.c
* By      : Richard
* Version : V1.00
*********************************************************************************************************
*/
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/list.h>
#include <linux/fs.h>
#include <linux/blkdev.h>
#include <linux/blkpg.h>
#include <linux/spinlock.h>
#include <linux/hdreg.h>
#include <linux/init.h>
#include <linux/semaphore.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/interrupt.h>
#include <asm/uaccess.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/mutex.h>
#include <mach/clock.h>
#include <mach/platform.h> 
#include <mach/hardware.h> 
#include <mach/sys_config.h>
#include <linux/dma-mapping.h>
#include <mach/dma.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <asm/cacheflush.h>
#include <mach/gpio.h>
#include <linux/gpio.h>
//#include "nand_lib.h"
#include "nand_blk.h"

static struct clk *ahb_nand_clk = NULL;
static struct clk *mod_nand_clk = NULL;

extern __u32 NAND_DMASingleMap(__u32 rw, __u32 buff_addr, __u32 len);
extern __u32 NAND_DMASingleUnmap(__u32 rw, __u32 buff_addr, __u32 len);

static __u32 dma_phy_address = 0;
static __u32 dma_len_temp = 0;
static __u32 rw_flag = 0x1234;
#define NAND_READ 0x5555
#define NAND_WRITE 0xAAAA

static DECLARE_WAIT_QUEUE_HEAD(DMA_wait);
dma_hdl_t dma_hdle = (dma_hdl_t)NULL;

int seq=0;
int nand_handle=0;
dma_cb_t done_cb;
dma_config_t dma_config;

static int nanddma_completed_flag = 1;

static int dma_start_flag = 0;



static DECLARE_WAIT_QUEUE_HEAD(NAND_RB_WAIT_CH0);
static DECLARE_WAIT_QUEUE_HEAD(NAND_RB_WAIT_CH1);



#ifdef __LINUX_NAND_SUPPORT_INT__
static int nandrb_ready_flag[2] = {1, 1};
static int nanddma_ready_flag[2] = {1, 1};


#endif

//#define RB_INT_MSG_ON
#ifdef  RB_INT_MSG_ON
#define dbg_rbint(fmt, args...) printk(fmt, ## args)
#else
#define dbg_rbint(fmt, ...)  ({})
#endif

#define RB_INT_WRN_ON
#ifdef  RB_INT_WRN_ON
#define dbg_rbint_wrn(fmt, args...) printk(fmt, ## args)
#else
#define dbg_rbint_wrn(fmt, ...)  ({})
#endif

//#define DMA_INT_MSG_ON
#ifdef  DMA_INT_MSG_ON
#define dbg_dmaint(fmt, args...) printk(fmt, ## args)
#else
#define dbg_dmaint(fmt, ...)  ({})
#endif

#define DMA_INT_WRN_ON
#ifdef  DMA_INT_WRN_ON
#define dbg_dmaint_wrn(fmt, args...) printk(fmt, ## args)
#else
#define dbg_dmaint_wrn(fmt, ...)  ({})
#endif

//for rb int
extern void NFC_RbIntEnable(void);
extern void NFC_RbIntDisable(void);
extern void NFC_RbIntClearStatus(void);
extern __u32 NFC_RbIntGetStatus(void);
extern __u32 NFC_GetRbSelect(void);
extern __u32 NFC_GetRbStatus(__u32 rb);
extern __u32 NFC_RbIntOccur(void);

extern void NFC_DmaIntEnable(void);
extern void NFC_DmaIntDisable(void);
extern void NFC_DmaIntClearStatus(void);
extern __u32 NFC_DmaIntGetStatus(void);
extern __u32 NFC_DmaIntOccur(void);

extern __u32 NAND_GetCurrentCH(void);
extern __u32 NAND_SetCurrentCH(__u32 nand_index);


#ifdef  __LINUX_SUPPORT_DMA_INT__
void nanddma_buffdone(dma_hdl_t dma_hdle, void *parg)
{
	nanddma_completed_flag = 1;
	wake_up( &DMA_wait );
	//printk("buffer done. nanddma_completed_flag: %d\n", nanddma_completed_flag);
}

__s32 NAND_WaitDmaFinish(void)
{
	__u32 rw;
	__u32 buff_addr;
	__u32 len;
	
	wait_event(DMA_wait, nanddma_completed_flag);

	if(rw_flag==(__u32)NAND_READ)
		rw = 0;
	else
		rw = 1;
	buff_addr = dma_phy_address;
	len = dma_len_temp;
	NAND_DMASingleUnmap(rw, buff_addr, len);

	rw_flag = 0x1234;
	
    return 0;
}


#else
void nanddma_buffdone(dma_hdl_t dma_hdle, void *parg)
{
	return 0;
}

__s32 NAND_WaitDmaFinish(void)
{
	
    return 0;
}

#endif
int  nanddma_opfn(dma_hdl_t dma_hdle, void *parg)
{
	return 0;
}

void* NAND_RequestDMA(void)
{
	dma_hdle = sw_dma_request("NAND_DMA",CHAN_DEDICATE);
	if(dma_hdle == NULL)
	{
		printk("[NAND DMA] request dma fail\n");
		return dma_hdle;
	}
	printk("[NAND DMA] request dma success\n");
	
	/* set full done callback */
	done_cb.func = nanddma_buffdone;
	done_cb.parg = NULL;
	if(0 != sw_dma_ctl(dma_hdle, DMA_OP_SET_FD_CB, (void *)&done_cb)) {
		printk("[NAND DMA] set fulldone_cb fail\n");
	}
	printk("[NAND DMA] set fulldone_cb success\n");

	return dma_hdle;

}


__s32  NAND_ReleaseDMA(void)
{
    if(dma_start_flag==1)
    {
        dma_start_flag=0;
        if(0 != sw_dma_ctl(dma_hdle, DMA_OP_STOP, NULL)) 
    	{
    		printk("[NAND DMA] stop dma fail\n");
    	}
    }

    if(0!= sw_dma_release(dma_hdle))
    {
        printk("[NAND DMA] release dma fail\n");
    }
	return 0;
}


int NAND_QueryDmaStat(void)
{
	return 0;
}





__s32 NAND_CleanFlushDCacheRegion(__u32 buff_addr, __u32 len)
{
    return 0;
}




__u32 NAND_DMASingleMap(__u32 rw, __u32 buff_addr, __u32 len)
{
    __u32 mem_addr;
//    __u32 nand_index = NAND_GetCurrentCH();

    
    if (rw == 1) 
    {
	    mem_addr = (__u32)dma_map_single(NULL, (void *)buff_addr, len, DMA_TO_DEVICE);
	} 
	else 
    {
	    mem_addr = (__u32)dma_map_single(NULL, (void *)buff_addr, len, DMA_FROM_DEVICE);
	}

//	tmp_dma_phy_addr[nand_index] = mem_addr;
//	tmp_dma_len[nand_index] = len;
	
	return mem_addr;
}

__u32 NAND_DMASingleUnmap(__u32 rw, __u32 buff_addr, __u32 len)
{
	__u32 mem_addr = buff_addr;

	if (rw == 1) 
	{
	    dma_unmap_single(NULL, (dma_addr_t)mem_addr, len, DMA_TO_DEVICE);
	} 
	else 
	{
	    dma_unmap_single(NULL, (dma_addr_t)mem_addr, len, DMA_FROM_DEVICE);
	}
	
	return mem_addr;
}

void NAND_DMAConfigStart(int rw, unsigned int buff_addr, int len)
{
	__u32 buff_phy_addr = 0;
	

//config dma
	if(rw == 0)//read from nand
	{
		/* config para */
		memset(&dma_config, 0, sizeof(dma_config));
		dma_config.xfer_type.src_data_width 	= DATA_WIDTH_32BIT;
		dma_config.xfer_type.src_bst_len 	= DATA_BRST_4;
		dma_config.xfer_type.dst_data_width 	= DATA_WIDTH_32BIT;
		dma_config.xfer_type.dst_bst_len 	= DATA_BRST_4;
		dma_config.address_type.src_addr_mode 	= DDMA_ADDR_IO; 
		dma_config.address_type.dst_addr_mode 	= DDMA_ADDR_LINEAR;
		dma_config.src_drq_type 	= D_DST_NAND;
		dma_config.dst_drq_type 	= D_DST_SDRAM;
		dma_config.bconti_mode 		= false;
		dma_config.irq_spt 		= CHAN_IRQ_FD;

	}
	else //write to nand
	{
		/* config para */
		memset(&dma_config, 0, sizeof(dma_config));
		dma_config.xfer_type.src_data_width 	= DATA_WIDTH_32BIT;
		dma_config.xfer_type.src_bst_len 	= DATA_BRST_4;
		dma_config.xfer_type.dst_data_width 	= DATA_WIDTH_32BIT;
		dma_config.xfer_type.dst_bst_len 	= DATA_BRST_4;
		dma_config.address_type.src_addr_mode 	= DDMA_ADDR_LINEAR;
		dma_config.address_type.dst_addr_mode 	= DDMA_ADDR_IO;
		dma_config.src_drq_type 	= D_DST_SDRAM;
		dma_config.dst_drq_type 	= D_DST_NAND;
		dma_config.bconti_mode 		= false;
		dma_config.irq_spt 		= CHAN_IRQ_FD;
	}


	if(0 != sw_dma_config(dma_hdle, &dma_config)) {
		printk("[NAND DMA] config dma fail\n");
	}


	{
		dma_para_t para;
		para.src_blk_sz 	= 0x7f;
		para.src_wait_cyc 	= 0x07;
		para.dst_blk_sz 	= 0x7f;
		para.dst_wait_cyc 	= 0x07;
		if(0 != sw_dma_ctl(dma_hdle, DMA_OP_SET_PARA_REG, &para))
		{		
			printk("[NAND DMA] set dma para fail\n");
		}
		
	}
	nanddma_completed_flag = 0;
    
//enqueue buf
	if(rw == 0)//read
	{
		buff_phy_addr = NAND_DMASingleMap( rw,  buff_addr, len);
		if(rw_flag != 0x1234)
			printk("[NAND DMA ERR] rw_flag != 0x1234\n");
		rw_flag =(__u32)NAND_READ;
		dma_phy_address = buff_phy_addr;
		dma_len_temp = len;

		if(0 != sw_dma_enqueue(dma_hdle, 0x01c03030, buff_phy_addr, len))
		{
			printk("[NAND DMA] enqueue buffer fail\n");
		}
		
	}
	else//write
	{

		buff_phy_addr = NAND_DMASingleMap( rw,  buff_addr, len);
		if(rw_flag != 0x1234)
			printk("[NAND DMA ERR] rw_flag != 0x1234\n");
		rw_flag = (__u32)NAND_WRITE;
		dma_phy_address = buff_phy_addr;
		dma_len_temp = len;
		/* enqueue  buf */
		if(0 != sw_dma_enqueue(dma_hdle, buff_phy_addr, 0x01c03030, len))
		{
			printk("[NAND DMA] enqueue buffer fail\n");
		}
		
	}
//start dma
    if(dma_start_flag==0)
    {
        dma_start_flag=1;
        printk("[NAND DMA] start dma***************************************** \n");
        if(0 != sw_dma_ctl(dma_hdle, DMA_OP_START, NULL)) 
    	{
    		printk("[NAND DMA] start dma fail\n");
    	}
    }
}





#ifdef __LINUX_SUPPORT_RB_INT__
void NAND_EnRbInt(void)
{
	__u32 nand_index;

	nand_index = NAND_GetCurrentCH();
	if(nand_index >1)
		printk("NAND_ClearDMAInt, nand_index error: 0x%x\n", nand_index);
	
	//clear interrupt
	NFC_RbIntClearStatus();
	
	nandrb_ready_flag[nand_index] = 0;

	//enable interrupt
	NFC_RbIntEnable();

	dbg_rbint("rb int en\n");
}


void NAND_ClearRbInt(void)
{
    __u32 nand_index;

	nand_index = NAND_GetCurrentCH();
	if(nand_index >1)
		printk("NAND_ClearDMAInt, nand_index error: 0x%x\n", nand_index);
	
	//disable interrupt
	NFC_RbIntDisable();;

	dbg_rbint("rb int clear\n");

	//clear interrupt
	NFC_RbIntClearStatus();
	
	//check rb int status
	if(NFC_RbIntGetStatus())
	{
		dbg_rbint_wrn("nand %d clear rb int status error in int clear \n", nand_index);
	}
	
	nandrb_ready_flag[nand_index] = 0;
}


void NAND_RbInterrupt(void)
{
	__u32 nand_index;

	nand_index = NAND_GetCurrentCH();
	if(nand_index >1)
		printk("NAND_ClearDMAInt, nand_index error: 0x%x\n", nand_index);

	dbg_rbint("rb int occor! \n");
	if(!NFC_RbIntGetStatus())
	{
		dbg_rbint_wrn("nand rb int late \n");
	}
    
    NAND_ClearRbInt();
    
    nandrb_ready_flag[nand_index] = 1;
    if(nand_index == 0)
		wake_up( &NAND_RB_WAIT_CH0 );
    else if(nand_index ==1)
    	wake_up( &NAND_RB_WAIT_CH1 );

}

__s32 NAND_WaitRbReady(void)
{
	__u32 rb;
	__u32 nand_index;
	
	nand_index = NAND_GetCurrentCH();
	if(nand_index >1)
		printk("NAND_ClearDMAInt, nand_index error: 0x%x\n", nand_index);

	
	NAND_EnRbInt();
	
	//wait_event(NAND_RB_WAIT, nandrb_ready_flag);
	dbg_rbint("rb wait \n");

	if(nandrb_ready_flag[nand_index])
	{
		dbg_rbint("fast rb int\n");
		NAND_ClearRbInt();
		return 0;
	}

	rb=  NFC_GetRbSelect();
	if(NFC_GetRbStatus(rb))
	{
		dbg_rbint("rb %u fast ready \n", rb);
		NAND_ClearRbInt();
		return 0;
	}

	//printk("NAND_WaitRbReady, ch %d\n", nand_index);

	if(nand_index == 0)
	{
		if(wait_event_timeout(NAND_RB_WAIT_CH0, nandrb_ready_flag[nand_index], 1*HZ)==0)
		{
			dbg_rbint_wrn("nand wait rb int time out, ch: %d\n", nand_index);
			NAND_ClearRbInt();
		}
		else
		{	NAND_ClearRbInt();
			dbg_rbint("nand wait rb ready ok\n");
		}
	}
	else if(nand_index ==1)
	{
		if(wait_event_timeout(NAND_RB_WAIT_CH1, nandrb_ready_flag[nand_index], 1*HZ)==0)
		{
			dbg_rbint_wrn("nand wait rb int time out, ch: %d\n", nand_index);
			NAND_ClearRbInt();
		}
		else
		{	NAND_ClearRbInt();
			dbg_rbint("nand wait rb ready ok\n");
		}
	}
	else
	{
		NAND_ClearRbInt();
	}
		
    return 0;
}
#else
__s32 NAND_WaitRbReady(void)
{
    return 0;
}
#endif

#define NAND_CH0_INT_EN_REG    (0xf1c03000+0x8)
#define NAND_CH1_INT_EN_REG    (0xf1c05000+0x8)
#define NAND_CH0_INT_ST_REG    (0xf1c03000+0x4)
#define NAND_CH1_INT_ST_REG    (0xf1c05000+0x4)
#define NAND_RB_INT_BITMAP     (0x1)
#define NAND_DMA_INT_BITMAP    (0x4)
#define __NAND_REG(x)    (*(volatile unsigned int   *)(x))


void NAND_Interrupt(__u32 nand_index)
{
	if(nand_index >1)
		printk("NAND_Interrupt, nand_index error: 0x%x\n", nand_index);
#ifdef __LINUX_NAND_SUPPORT_INT__   

    //printk("nand interrupt!\n");
#ifdef __LINUX_SUPPORT_RB_INT__    

    if(nand_index == 0)
    {
	   	if((__NAND_REG(NAND_CH0_INT_EN_REG)&NAND_RB_INT_BITMAP)&&(__NAND_REG(NAND_CH0_INT_ST_REG)&NAND_RB_INT_BITMAP))
			NAND_RbInterrupt();	
    }

#endif    


#endif
}


__u32 NAND_VA_TO_PA(__u32 buff_addr)
{
    return (__u32)(__pa((void *)buff_addr));
}

void NAND_PIORequest(__u32 nand_index)
{
	int cnt, i;
	script_item_u *list = NULL;
	
	/* 获取gpio list */
	cnt = script_get_pio_list("nand_para", &list);
	if(0 == cnt) {
		printk("get nand_para gpio list failed\n");
		return;
	}
	/* 申请gpio */
	for(i = 0; i < cnt; i++)
		if(0 != gpio_request(list[i].gpio.gpio, NULL))
			goto end;
	/* 配置gpio list */
	if(0 != sw_gpio_setall_range(&list[0].gpio, cnt))
	{
		printk("sw_gpio_setall_range failed\n");
		goto end;
	}
	return;
end:
	printk("nand:gpio_request failed\n");
	/* 释放gpio */
	while(i--)
		gpio_free(list[i].gpio.gpio);


}

void NAND_PIORelease(__u32 nand_index)
{

	int	cnt, i;
	script_item_u *list = NULL;
	
	/* 获取gpio list */
	cnt = script_get_pio_list("nand_para", &list);
	if(0 == cnt) {
		printk("get nand_para gpio list failed\n");
		return;
	}
	
	for(i = 0; i < cnt; i++)
		gpio_free(list[i].gpio.gpio);
	
}


void NAND_Memset(void* pAddr, unsigned char value, unsigned int len)
{
    memset(pAddr, value, len);   
}

void NAND_Memcpy(void* pAddr_dst, void* pAddr_src, unsigned int len)
{
    memcpy(pAddr_dst, pAddr_src, len);    
}

void* NAND_Malloc(unsigned int Size)
{
     	return kmalloc(Size, GFP_KERNEL);
}

void NAND_Free(void *pAddr, unsigned int Size)
{
    kfree(pAddr);
}

int NAND_Print(const char *fmt, ...)
{
	va_list args;
	int r;

	va_start(args, fmt);
	r = vprintk(fmt, args);
	va_end(args);
	
	return r;
}

void *NAND_IORemap(unsigned int base_addr, unsigned int size)
{
    return (void *)base_addr;
}

__u32 NAND_GetIOBaseAddrCH0(void)
{
	return 0xf1c03000;
}
	
__u32 NAND_GetIOBaseAddrCH1(void)
{
	return 0xf1c05000;
}

DEFINE_SEMAPHORE(nand_physic_mutex);

int NAND_PhysicLockInit(void)
{
    return 0;
}

int NAND_PhysicLock(void)
{
    down(&nand_physic_mutex);
     return 0;
}

int NAND_PhysicUnLock(void)
{
    up(&nand_physic_mutex);
     return 0;
}

int NAND_PhysicLockExit(void)
{
     return 0;
}


int NAND_ClkRequest(__u32 nand_index)
{
    printk("[NAND] nand clk request start\n");
	ahb_nand_clk = clk_get(NULL, CLK_AHB_NAND);
	if(!ahb_nand_clk||IS_ERR(ahb_nand_clk)) {
		return -1;
	}
	mod_nand_clk = clk_get(NULL, CLK_MOD_NFC);
		if(!mod_nand_clk||IS_ERR(mod_nand_clk)) {
		return -1;
	}
	printk("[NAND] nand clk request ok!\n");
	return 0;
}

void NAND_ClkRelease(__u32 nand_index)
{
	clk_put(ahb_nand_clk);
	clk_put(mod_nand_clk);
}


int NAND_AHBEnable(void)
{
	return clk_enable(ahb_nand_clk);
}

int NAND_ClkEnable(void)
{
	return clk_enable(mod_nand_clk);
}

void NAND_AHBDisable(void)
{
	clk_disable(ahb_nand_clk);
}

void NAND_ClkDisable(void)
{
	clk_disable(mod_nand_clk);
}

int NAND_SetClk(__u32 nand_index, __u32 nand_clk)
{
	return clk_set_rate(mod_nand_clk, nand_clk*2000000);
}

int NAND_GetClk(__u32 nand_index)
{
	return (clk_get_rate(mod_nand_clk)/2000000);
}

int NAND_GetPlatform(void)
{
	return 20;	
}

int NAND_get_storagetype()
{
    script_item_value_type_e script_ret;
    script_item_u storage_type;
    
    script_ret = script_get_item("target","storage_type", &storage_type);
    if(script_ret!=SCIRPT_ITEM_VALUE_TYPE_INT)
    {
           printk("nand init fetch storage_type failed\n");
           storage_type.val=0;
           return storage_type.val;
    }

    return storage_type.val;
    
    
}

/*
__u32  NAND_Get_nandp0(void)
{
    script_item_u   nandp0;
    script_item_value_type_e  type;

    type = script_get_item("nand_para", "nand_p0", &nandp0);
    if(SCIRPT_ITEM_VALUE_TYPE_INT != type)
    {
        nand_dbg_err("nand type err! %d",type);
		return 0xffffffff;
    }
	else
		return nandp0.val;
}

__u32 NAND_Getidnumberctl(void)
{
    script_item_u   id_number_ctl;
    script_item_value_type_e  type;

    type = script_get_item("nand_para", "id_number_ctl", &id_number_ctl);
    if(SCIRPT_ITEM_VALUE_TYPE_INT != type)
    {
        nand_dbg_err("nand type err! %d",type);
		return 0x0;
    }
	nand_dbg_err("nand : get id_number_ctl from script,%x \n",id_number_ctl.val);
	return id_number_ctl.val;	
}
*/

__u32 NAND_GetNandExtPara(__u32 para_num)
{	
	script_item_u nand_para;
    script_item_value_type_e type;
	
	if (para_num == 0) {
	    type = script_get_item("nand_para", "nand_p0", &nand_para);
	    if(SCIRPT_ITEM_VALUE_TYPE_INT != type)
	    {
	        printk("nand type err! %d", type);
			return 0xffffffff;
	    }
		else
			return nand_para.val;
	} else if (para_num == 1) {
	    type = script_get_item("nand_para", "nand_p1", &nand_para);
	    if(SCIRPT_ITEM_VALUE_TYPE_INT != type)
	    {
	        printk("nand type err! %d", type);
			return 0xffffffff;
	    }
		else
			return nand_para.val;	
	} else {
		printk("NAND_GetNandExtPara: wrong para num: %d\n", para_num);
		return 0xffffffff;
	}
}

__u32 NAND_GetNandIDNumCtrl(void)
{
    script_item_u id_number_ctl;
    script_item_value_type_e type;

    type = script_get_item("nand_para", "id_number_ctl", &id_number_ctl);
    if(SCIRPT_ITEM_VALUE_TYPE_INT != type)
    {
        printk("nand type err! %d", type);
		return 0;
    } else {
    	printk("nand : get id_number_ctl from script,%x \n",id_number_ctl.val);	
    	return id_number_ctl.val;
    }	
}

