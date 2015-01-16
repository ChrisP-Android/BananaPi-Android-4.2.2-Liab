#ifndef __HWCOMPOSER_PRIV_H__
#define __HWCOMPOSER_PRIV_H__

#include <hardware/hardware.h>

#include <fcntl.h>
#include <errno.h>

#include <cutils/log.h>
#include <cutils/atomic.h>

#include <hardware/hwcomposer.h>

#include <EGL/egl.h>

enum
{
    HWC_STATUS_HAVE_FRAME       = 1,
    HWC_STATUS_COMPOSITED       = 2,
    HWC_STATUS_OPENED           = 4,
    HWC_STATUS_HAVE_VIDEO       = 8,
};

typedef struct hwc_context_t
{
    hwc_composer_device_1_t device;
    hwc_procs_t             *procs;
    int                     dispfd;
    int                     mFD_fb[2];
    e_hwc_mode_t            mode;
    screen_para_t           screen_para;
    bool                    b_video_in_valid_area;

    uint32_t                ui_layerhdl[2];
    uint32_t                video_layerhdl[2];
    uint32_t                status[2];
    uint32_t                w;
    uint32_t                h;
    uint32_t                format;
    bool                    cur_3denable;
    e_hwc_3d_src_mode_t     cur_3d_src;
    e_hwc_3d_out_mode_t     cur_3d_out;
    uint32_t                cur_3d_w;
    uint32_t                cur_3d_h;
    e_hwc_format_t          cur_3d_format;
    hwc_rect_t              rect_in;
    hwc_rect_t              rect_out;
    hwc_rect_t              rect_out_active[2];
    __disp_tv_mode_t        org_hdmi_mode;
    __disp_rect_t           org_scn_win;
    libhwclayerpara_t       cur_frame_para;
    bool                    layer_para_set;
    bool                    vsync_en;
    pthread_t               vsync_thread;
    bool                    screen_size_change;
}sun4i_hwc_context_t;

#endif
