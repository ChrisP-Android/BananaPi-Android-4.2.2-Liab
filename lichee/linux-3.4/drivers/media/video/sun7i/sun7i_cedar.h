#ifndef _SUN7I_CEDAR_H_
#define _SUN7I_CEDAR_H_

enum IOCTL_CMD {
	IOCTL_UNKOWN = 0x100,
	IOCTL_GET_ENV_INFO,
	IOCTL_WAIT_VE,
	IOCTL_RESET_VE,
	IOCTL_ENABLE_VE,
	IOCTL_DISABLE_VE,
	IOCTL_SET_VE_FREQ,
	
	IOCTL_CONFIG_AVS2 = 0x200,
	IOCTL_GETVALUE_AVS2 ,
	IOCTL_PAUSE_AVS2 ,
	IOCTL_START_AVS2 ,
	IOCTL_RESET_AVS2 ,
	IOCTL_ADJUST_AVS2,
	IOCTL_ENGINE_REQ,
	IOCTL_ENGINE_REL,
	IOCTL_ENGINE_CHECK_DELAY,
	IOCTL_GET_IC_VER,
	IOCTL_ADJUST_AVS2_ABS,
	IOCTL_FLUSH_CACHE,
	IOCTL_SET_REFCOUNT,
	
	IOCTL_READ_REG = 0x300,
	IOCTL_WRITE_REG,
};

struct cedarv_env_infomation{
	unsigned int phymem_start;
	int  phymem_total_size;
	unsigned int  address_macc;
};

struct cedarv_cache_range{
	long start;
	long end;
};

struct __cedarv_task {
	int task_prio;
	int ID;
	unsigned long timeout;	
	unsigned int frametime;
	unsigned int block_mode;
};

struct cedarv_engine_task {
	struct __cedarv_task t;	
	struct list_head list;
	struct task_struct *task_handle;
	unsigned int status;
	unsigned int running;
	unsigned int is_first_task;
};

/* 利用优先级task_prio查询当前运行task的frametime，和比优先级task_prio高的task可能运行的总时间total_time */
struct cedarv_engine_task_info {
	int task_prio;
	unsigned int frametime;
	unsigned int total_time;
};

struct cedarv_regop {
    unsigned int addr;
    unsigned int value;
};

/* reg define */
#define SRAM_REGS_BASE      SW_PA_SRAM_IO_BASE     // SRAM Controller
#define CCMU_REGS_BASE      SW_PA_CCM_IO_BASE      // Clock Control manager unit  OK
#define MACC_REGS_BASE      SW_PA_VE_IO_BASE       // Media ACCelerate
#define SS_REGS_BASE        SW_PA_SS_IO_BASE       // Security System
#define SDRAM_REGS_BASE	    SW_PA_DRAM_IO_BASE     //SDRAM Controller   OK
#define AVS_REGS_BASE       SW_PA_TIMERC_IO_BASE

#define SRAM_REGS_SIZE      (4096)  // 4K
#define CCMU_REGS_SIZE      (1024)  // 1K
#define MACC_REGS_SIZE      (4096)  // 4K
#define SS_REGS_SIZE        (4096)  // 4K

#define MPEG_REGS_BASE      (MACC_REGS_BASE + 0x100) // MPEG engine
#define H264_REGS_BASE      (MACC_REGS_BASE + 0x200) // H264 engine
#define VC1_REGS_BASE       (MACC_REGS_BASE + 0x300) // VC-1 engine

#define SRAM_REG_o_CFG	    (0x00)
#define SRAM_REG_ADDR_CFG   (SRAM_REGS_BASE + SRAM_REG_o_CFG) // SRAM MAP Cfg Reg 0

#endif
