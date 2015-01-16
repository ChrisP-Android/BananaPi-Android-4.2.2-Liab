#ifndef __IOCTL_CMD_H__
#define __IOCTL_CMD_H__

enum IOCTL_CMD {
	IOCTL_MALLOC_CONSECUTIVE_ADDR = 0x100,
	IOCTL_FREE_CONSECUTIVE_ADDR     	 ,
	IOCTL_REGISTER_REMAP				 ,
	IOCTL_REGISTER_UNMAP				 ,
	IOCTL_ENABLE_INT					 ,	
	IOCTL_DISABLE_INT					 ,	
	IOCTL_INS_ISR						 ,	
	IOCTL_UNINS_ISR						 ,	
	IOCTL_WAIT_VE						 ,	
	IOCTL_GET_CLK						 ,	
	IOCTL_PUT_CLK						 ,	
	IOCTL_GET_CLK_FREQ					 ,	
	IOCTL_SET_CLK_FREQ					 ,	
	IOCTL_SET_SRC_CLK_FREQ				 ,	
	IOCTL_ENABLE_CLK					 ,	
	IOCTL_DISABLE_CLK					 ,	
	IOCTL_TEST_WRITE_REG				 ,
	IOCTL_PHYMEMRESET		 ,
	IOCTL_GETPHYMEMSIZE      ,
	IOCTL_PHYMEMALLOC		 ,
	IOCTL_PHYMEMFREE     	 ,
	IOCTL_CONFIG_AVS2 ,
	IOCTL_GETVALUE_AVS2 ,
	IOCTL_PAUSE_AVS2 ,
	IOCTL_START_AVS2 ,
	IOCTL_RESET_AVS2 ,
	IOCTL_RESPHYMEMGET,
	IOCTL_RESPHYMEMSIZEGET,
	IOCTL_ADJUSTG_AVS2,
	IOCTL_ADJUSTL_AVS2,
	IOCTL_ADJUST_AVS2,
};

enum reg_map{
	REG_MAP_CCMU = 0X01,
	REG_MAP_MACC = 0X02,
	REG_MAP_SRAM = 0X04,
	REG_MAP_MEMC = 0X08,
};

#define CLK_NAME_LEN	32


struct malloc_para{
	char* vir_addr;
	char* phy_addr;
	char* proc_addr;
	int   size;
};

struct iomap_para{
	volatile char*	regs_sram;
	volatile char*	regs_ccmu;
	volatile char*	regs_macc;
	volatile char*	regs_mpeg;
	volatile char*	regs_vc1;
	volatile char*	regs_memc;
};

struct clk_para{
	char	clk_name[CLK_NAME_LEN];
	uint	handle;
	int		clk_rate;
};


#endif

