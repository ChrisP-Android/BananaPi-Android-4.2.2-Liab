#ifndef __FB_HEAD_H
#define __FB_HEAD_H
#include <disp/fb.h>
#include <disp/drv_display.h>
#include <disp/fb_head.h>

#define uint32_t  unsigned int
struct fb {
	int fd;
	uint32_t w;
	uint32_t h;
	uint32_t bpp;
	uint32_t bpp_byte;
	uint32_t mem_size;
	int b_map;
	void *mem;
};

struct __fbuser
{
		int bits_per_pixel;
		int scaler_mode;
		int scaler_fmt;
		int scaler_seq;
		int br_swap;
};

struct scaler_paratable
{
	int line_w;
	int line_h;
	__disp_pixel_mod_t mode;
	__disp_pixel_fmt_t format;
	__disp_pixel_seq_t seq;
	unsigned char *name;
};

struct test_para
{
	int pausekey;
	int helpkey;
	int b_dual;
	int alpahval;
	unsigned int loop;
	unsigned int switchtype[2];
	unsigned int switchmode[2];
	//pipe 0
	int b_ch1_en;
	int nonstd;	
	struct __fbuser fb_user;
	__disp_rectsz_t size_w;
	__disp_rectsz_t size_o;
	int sel;
	int videotype;
	int videomode ;
	__disp_fb_create_para_t fb_para;
	//pipe1
	int b_ch2_en;
	struct __fbuser fb_user2;
	int nonstd2;
	__disp_rectsz_t size_w2;
	__disp_rectsz_t size_o2;
	int sel2;
	int videotype2;
	int videomode2;
	__disp_fb_create_para_t fb_para2;
	//scaler
	__disp_scaler_para_t scaler_para;
	//scaler in 
	struct scaler_paratable scaler_in;
	int scaler_in_valid;
	//scaler out
	struct scaler_paratable scaler_out;
	int scaler_out_valid;
	
	unsigned char filename[256];
	unsigned char filename2[256];
	int pan_x;
	int pan_y;
	int bkalpha;
	int bkred;
	int bkblue;
	int bkgreen;
	int b_varWindow;
	int b_varPan;
	int b_doublebuffer;
};
#define PR_HEAD " [allwinner]: "
//#define __DEBUG
#ifdef __DEBUG
#define FB_OK(x) {printf("\033[34m");printf(PR_HEAD);printf x;printf("   OK\e[0m\n");}
#define FB_FAIL(x) {printf("\033[35m");printf(PR_HEAD);printf x;printf("FAIL\e[0m\n");}
#define FB_DBG(x) {printf x;}
#define FB_PUT(x) {printf x;}
#define FB_INFO(x) {printf x;}
#else
#define FB_OK(x) {printf("\033[34m");printf(PR_HEAD);printf x;printf("   OK\e[0m\n");}
#define FB_FAIL(x) {printf("\033[35m");printf(PR_HEAD);printf x;printf("   FAIL\e[0m\n");}
#define FB_DBG(x) do{}while(0)
#define FB_PUT(x) {printf x;}
#define FB_INFO(x) {printf x;}
#endif

#endif
