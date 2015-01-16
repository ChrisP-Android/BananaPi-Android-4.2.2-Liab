#ifndef DRIVER_INTERFACE_H
#define DRIVER_INTERFACE_H

#include<linux/ioctl.h>


//define iocltl command
#define TSCDEV_IOC_MAGIC            't'

#define TSCDEV_ENABLE_INT           _IO(TSCDEV_IOC_MAGIC,   0)
#define TSCDEV_DISABLE_INT          _IO(TSCDEV_IOC_MAGIC,   1)
#define TSCDEV_GET_PHYSICS          _IO(TSCDEV_IOC_MAGIC,   2)
#define TSCDEV_RELEASE_SEM          _IO(TSCDEV_IOC_MAGIC,   3)
#define TSCDEV_WAIT_INT             _IOR(TSCDEV_IOC_MAGIC,  4,  unsigned long)
#define TSCDEV_ENABLE_CLK           _IOW(TSCDEV_IOC_MAGIC,  5,  unsigned long)
#define TSCDEV_DISABLE_CLK          _IOW(TSCDEV_IOC_MAGIC,  6,  unsigned long)
#define TSCDEV_PUT_CLK              _IOW(TSCDEV_IOC_MAGIC,  7,  unsigned long)
#define TSCDEV_SET_CLK_FREQ         _IOW(TSCDEV_IOC_MAGIC,  8,  unsigned long)
#define TSCDEV_GET_CLK              _IOWR(TSCDEV_IOC_MAGIC, 9,  unsigned long)
#define TSCDEV_GET_CLK_FREQ         _IOWR(TSCDEV_IOC_MAGIC, 10, unsigned long)
#define TSCDEV_SET_SRC_CLK_FREQ     _IOWR(TSCDEV_IOC_MAGIC, 11, unsigned long)
#define TSCDEV_FOR_TEST             _IO(TSCDEV_IOC_MAGIC,   12)


#define TSCDEV_IOC_MAXNR            (12)


struct intrstatus {
    unsigned int port0chan;
    unsigned int port0pcr;
    unsigned int port1chan;
    unsigned int port1pcr;
};

/*
 * define struct for clock parameters
 */
#define CLK_NAME_LEN (32)
struct clk_para {
    char            clk_name[CLK_NAME_LEN];
    unsigned int    handle;
    int             clk_rate;
};

/*
 * define values of channel information
 */
#define TSF_INTR_THRESHOLD          (1) // defines when to interrupt
//0:1/2 buffer size 1:1/4 buffer size
//2:1/8 buffer size 3:1/16 buffer size
#define VIDEO_NOTIFY_PKT_NUM        (1024)
#define AUDIO_NOTIFY_PKT_NUM        (64)
#define SUBTITLE_NOTIFY_PKT_NUM     (64)
#define SECTION_NOTIFY_PKT_NUM      (64)
#define TS_DATA_NOTIFY_PKT_NUM      (64)

/*
 * define channel max numbers and base offset
 */
#define VIDEO_CHAN_BASE         (0)
#define MAX_VIDEO_CHAN          (1)

#define AUDIO_CHAN_BASE         (VIDEO_CHAN_BASE + MAX_VIDEO_CHAN)
#define MAX_AUDIO_CHAN          (1)

#define SUBTITLE_CHAN_BASE      (AUDIO_CHAN_BASE + MAX_AUDIO_CHAN)
#define MAX_SUBTITLE_CHAN       (1)

#define SECTION_CHAN_BASE       (SUBTITLE_CHAN_BASE + MAX_SUBTITLE_CHAN)
#define MAX_SECTION_CHAN        (12)

#define TS_DATA_CHAN_BASE       (SECTION_CHAN_BASE + MAX_SECTION_CHAN)
#define MAX_TS_DATA_CHAN        (15)

/*
 * define address of Registers
 */
#define REGS_pBASE              (0x01C04000)

// registers offset
#define TSC_REGS_OFFSET         (0x00      )
#define TSG_REGS_OFFSET         (0x40      )
#define TSF0_REGS_OFFSET        (0x80      )
#define TSF1_REGS_OFFSET        (0x100     )

#define TSF0_PCR_STATUS_OFFSET  (0x88      )
#define TSF0_CHAN_STATUS_OFFSET (0x98      )
#define TSF1_PCR_STATUS_OFFSET  (0x108     )
#define TSF1_CHAN_STATUS_OFFSET (0x118     )

//start address
#define TSC_REGS_pBASE          (REGS_pBASE + 0x00 )
#define TSG_REGS_pBASE          (REGS_pBASE + 0x40 )
#define TSF0_REGS_pBASE         (REGS_pBASE + 0x80 )
#define TSF1_REGS_pBASE         (REGS_pBASE + 0x100)

#define REGS_BASE               REGS_pBASE
#define TSC_REGS_BASE           TSC_REGS_pBASE
#define TSG_REGS_BASE           TSG_REGS_pBASE
#define TSF0_REGS_BASE          TSF0_REGS_pBASE
#define TSF1_REGS_BASE          TSF1_REGS_pBASE

//register size
#define REGS_SIZE               (0x1000)

#endif
