#include <linux/module.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/version.h>
#include <linux/mutex.h>
#include <linux/videodev2.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/freezer.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-common.h>
#include <media/v4l2-mediabus.h>

#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <media/videobuf-core.h>
#include <media/v4l2-subdev.h>
#include <media/videobuf-dma-contig.h>
#include <linux/moduleparam.h>
#include <mach/sys_config.h>
#include <mach/clock.h>
#include <mach/irqs.h>
#include <linux/regulator/consumer.h>

#include <linux/types.h>

#define TVD_REGS_BASE   0x01c08000
#define TVD_REG_SIZE    0x1000

#define TVD_MAJOR_VERSION 1
#define TVD_MINOR_VERSION 0
#define TVD_RELEASE 0
#define TVD_VERSION KERNEL_VERSION(TVD_MAJOR_VERSION, TVD_MINOR_VERSION, TVD_RELEASE)
#define TVD_MODULE_NAME "tvd"

#define NUM_INPUTS 2

#define MIN_WIDTH  (32)
#define MIN_HEIGHT (32)
#define MAX_WIDTH  (4096)
#define MAX_HEIGHT (4096)
#define MAX_BUFFER (32*1024*1024)

//#define USE_DMA_CONTIG

#define BLINK_DELAY   HZ/2

static unsigned video_nr = 1;
static unsigned first_flag = 0;
//static struct timer_list timer;

typedef enum
{
        TVD_UV_SWAP,
        TVD_COLOR_SET,
}tvd_param_t;

struct fmt {
        u8              name[32];
        __u32           fourcc;          /* v4l2 format id */
        tvd_fmt_t       output_fmt;	
        int   	        depth;
        __u32           width;
        __u32           height;
};

static struct fmt tvd_fmt = {
        .name           = "planar YUV420",
        .fourcc   	= V4L2_PIX_FMT_NV12,
        .output_fmt	= TVD_PL_YUV420,
        .depth    	= 12,
        .width          = 720,//may be change
        .height         = 480,//may be change
};

static struct fmt formats[] = {
	{
		.name           = "planar YUV420",
		.fourcc   	= V4L2_PIX_FMT_NV12,
		.output_fmt	= TVD_PL_YUV420,
		.depth    	= 12,
		.width          = 720,
		.height         = 480,
	},
	{
		.name     	= "planar YUV420",
		.fourcc   	= V4L2_PIX_FMT_NV12,
		.output_fmt	= TVD_PL_YUV420,
		.depth    	= 12,
		.width          = 720,
		.height         = 960,
	},
	{
		.name     	= "planar YUV420",
		.fourcc   	= V4L2_PIX_FMT_NV12,
		.output_fmt	= TVD_PL_YUV420,
		.depth    	= 12,
		.width          = 720,
		.height         = 1920,
	},
	{
		.name     	= "planar YUV420",
		.fourcc   	= V4L2_PIX_FMT_NV12,
		.output_fmt	= TVD_PL_YUV420,
		.depth    	= 12,
		.width          = 1440,
		.height         = 480,
	},
	{
		.name     	= "planar YUV420",
		.fourcc   	= V4L2_PIX_FMT_NV12,
		.output_fmt	= TVD_PL_YUV420,
		.depth    	= 12,
		.width          = 2880,
		.height         = 480,
	},
	{
		.name     	= "planar YUV420",
		.fourcc   	= V4L2_PIX_FMT_NV12,
		.output_fmt	= TVD_PL_YUV420,
		.depth    	= 12,
		.width          = 1440,
		.height         = 960,
	},
	{
		.name     	= "planar YUV420",
		.fourcc   	= V4L2_PIX_FMT_NV12,
		.output_fmt	= TVD_PL_YUV420,
		.depth          = 12,
		.width          = 720,
		.height         = 576,
	},
	{
		.name     	= "planar YUV420",
		.fourcc   	= V4L2_PIX_FMT_NV21,
		.output_fmt	= TVD_PL_YUV420,
		.depth    	= 12,
		.width          = 720,
		.height         = 480,
	},
	{
		.name     	= "planar YUV420",
		.fourcc   	= V4L2_PIX_FMT_NV21,
		.output_fmt	= TVD_PL_YUV420,
		.depth    	= 12,
		.width          = 720,
		.height         = 960,
	},
	{
		.name     	= "planar YUV420",
		.fourcc   	= V4L2_PIX_FMT_NV21,
		.output_fmt	= TVD_PL_YUV420,
		.depth    	= 12,
		.width          = 720,
		.height         = 1920,
	},
	{
		.name     	= "planar YUV420",
		.fourcc   	= V4L2_PIX_FMT_NV21,
		.output_fmt	= TVD_PL_YUV420,
		.depth          = 12,
		.width          = 1440,
		.height         = 480,
	},
	{
		.name     	= "planar YUV420",
		.fourcc   	= V4L2_PIX_FMT_NV21,
		.output_fmt	= TVD_PL_YUV420,
		.depth    	= 12,
		.width          = 2880,
		.height         = 480,
	},
	{
		.name     	= "planar YUV420",
		.fourcc   	= V4L2_PIX_FMT_NV21,
		.output_fmt	= TVD_PL_YUV420,
		.depth    	= 12,
		.width          = 1440,
		.height         = 960,
	},
	{
		.name     	= "planar YUV420",
		.fourcc   	= V4L2_PIX_FMT_NV21,
		.output_fmt	= TVD_PL_YUV420,
		.depth    	= 12,
		.width          = 720,
		.height         = 576,
	},
};

/* buffer for one video frame */
struct buffer {
	struct videobuf_buffer  vb;
	struct fmt              *fmt;
};

struct dmaqueue {
	struct list_head active;
	int frame;      /* Counters to control fps rate */
	int ini_jiffies;
};

struct buf_addr {
	dma_addr_t	y;
	dma_addr_t	c;
};

static LIST_HEAD(devlist);

struct tvd_dev {
	struct list_head       	devlist;
	struct v4l2_device      v4l2_dev;
	struct v4l2_subdev	*sd;
	struct platform_device	*pdev;

	int			id;
	
	spinlock_t              slock;

	/* various device info */
	struct video_device     *vfd;

	struct dmaqueue         vidq;

	/* Several counters */
	unsigned 	        ms;
	unsigned long           jiffies;

	/* Input Number */
	int	                input;

	/* video capture */
	struct fmt              *fmt;
	unsigned int            width;
	unsigned int            height;
	unsigned int		frame_size;
	struct videobuf_queue   vb_vidq;

	/*  */
	unsigned int            interface;
	unsigned int            system;
	unsigned int            format;
	unsigned int            row;
	unsigned int            column;
	//unsigned int channel_en[4];
	unsigned int            channel_index[4];	
	unsigned int            channel_offset_y[4];
	unsigned int            channel_offset_c[4];
	unsigned int            channel_irq;

	/*working state*/
	unsigned long 	        generating;
	int			opened;

	//clock
	struct clk		*ahb_clk;
	struct clk		*module1_clk;
        struct clk		*module2_clk;
	struct clk		*dram_clk;

	int			irq;
	void __iomem		*regs;
	struct resource		*regs_res; //??
	
	struct buf_addr		buf_addr;

        //luma contrast saturation hue 
        unsigned int            luma;
        unsigned int            contrast;
        unsigned int            saturation;
        unsigned int            hue;

        unsigned int            uv_swap;
        //fps
        struct v4l2_fract       fps;
        
};

