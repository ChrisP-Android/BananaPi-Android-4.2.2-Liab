#ifndef __BSP_TVD_H__
#define __BSP_TVD_H__

#include <linux/kernel.h>

typedef enum
{
        TVD_PL_YUV422,
        TVD_PL_YUV420,
        TVD_MB_YUV420, 
}tvd_fmt_t;

typedef enum
{
        TVD_CVBS,
        TVD_YPBPR_I,
        TVD_YPBPR_P,
}tvd_interface_t;

typedef enum
{
        TVD_NTSC,
        TVD_PAL,
        TVD_SECAM,
}tvd_system_t;

typedef enum
{
        TVD_FRAME_DONE,
        TVD_LOCK,
        TVD_UNLOCK,
}tvd_irq_t;

void TVD_init(void * addr);

//s32 TVD_set_mode(tvd_mode_t mode);

//void TVD_det_enable();
//void TVD_det_disable();
//__u32 TVD_det_finish();
//tvd_mode_t TVD_det_mode();

void  TVD_irq_enable(__u32 id,tvd_irq_t irq);
void  TVD_irq_disable(__u32 id,tvd_irq_t irq);
__s32 TVD_irq_status_get(__u32 id,tvd_irq_t irq);
void  TVD_irq_status_clear(__u32 id,tvd_irq_t irq);

void  TVD_capture_on(__u32 id);
void  TVD_capture_off(__u32 id);

void  TVD_set_addr_y(__u32 id,__u32 addr);
void  TVD_set_addr_c(__u32 id,__u32 addr);

void  TVD_set_width(__u32 id,__u32 w);
void  TVD_set_width_jump(__u32 id,__u32 j);
void  TVD_set_height(__u32 id,__u32 h);

void  TVD_set_fmt(__u32 id,tvd_fmt_t fmt);
void  TVD_config(__u32 interface, __u32 system);
__u32 TVD_get_status(__u32 id);
void  TVD_set_color(__u32 id,__u32 luma,__u32 contrast,__u32 saturation,__u32 hue);
void  TVD_uv_swap(__u8 uv_swap);

#endif
