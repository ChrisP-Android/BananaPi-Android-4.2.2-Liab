#include "dev_disp.h"


extern struct device	*display_dev;
extern fb_info_t g_fbi;

extern __s32 disp_video_set_dit_mode(__u32 scaler_index, __u32 mode);
extern __s32 disp_video_get_dit_mode(__u32 scaler_index);

static __u32 sel;
static __u32 hid;
//#define DISP_CMD_CALLED_BUFFER_LEN 100
//static __u32 disp_cmd_called[DISP_CMD_CALLED_BUFFER_LEN];
//static __u32 disp_cmd_called_index;

#define ____SEPARATOR_GLABOL_NODE____

static ssize_t disp_sel_show(struct device *dev,struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", sel);
}

static ssize_t disp_sel_store(struct device *dev,struct device_attribute *attr,const char *buf, size_t count)
{
	int err;
        unsigned long val;
    
	err = strict_strtoul(buf, 10, &val);
	if (err) {
		printk("Invalid size\n");
		return err;
	}

        if((val>1))
        {
                printk("Invalid value, 0/1 is expected!\n");
        }else
        {
                printk("%ld\n", val);
                sel = val;
	}
    
	return count;
}

static ssize_t disp_hid_show(struct device *dev,struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", HANDTOID(hid));
}

static ssize_t disp_hid_store(struct device *dev,struct device_attribute *attr,const char *buf, size_t count)
{
	int err;
        unsigned long val;
    
	err = strict_strtoul(buf, 10, &val);
	if (err) {
		printk("Invalid size\n");
		return err;
	}

        if((val>3) || (val < 0))
        {
                printk("Invalid value, 0~3 is expected!\n");
        }else
        {
                printk("%ld\n", val);
                hid = IDTOHAND(val);
	}
    
	return count;
}
static DEVICE_ATTR(sel, S_IRUGO|S_IWUSR|S_IWGRP,disp_sel_show, disp_sel_store);

static DEVICE_ATTR(hid, S_IRUGO|S_IWUSR|S_IWGRP,disp_hid_show, disp_hid_store);

static ssize_t disp_sys_status_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
  ssize_t count = 0;
	int num_screens, screen_id;
	int num_layers, layer_id;
	int hpd;

	num_screens = 2;
	for(screen_id=0; screen_id < num_screens; screen_id ++) {
		count += sprintf(buf + count, "screen %d:\n", screen_id);
		/* output */
		if(BSP_disp_get_output_type(screen_id) == DISP_OUTPUT_TYPE_LCD) {
			count += sprintf(buf + count, "\tlcd output\tbacklight(%3d)", BSP_disp_lcd_get_bright(screen_id));
			count += sprintf(buf + count, "\t%4dx%4d\n", BSP_disp_get_screen_width(screen_id), 			BSP_disp_get_screen_height(screen_id));
		} else if(BSP_disp_get_output_type(screen_id) == DISP_OUTPUT_TYPE_HDMI) {
			count += sprintf(buf + count, "\thdmi output");
			if(BSP_disp_hdmi_get_mode(screen_id) == DISP_TV_MOD_720P_50HZ) {
				count += sprintf(buf + count, "\t%16s", "720p50hz");
			} else if(BSP_disp_hdmi_get_mode(screen_id) == DISP_TV_MOD_720P_60HZ) {
				count += sprintf(buf + count, "\t%16s", "720p60hz");
			} else if(BSP_disp_hdmi_get_mode(screen_id) == DISP_TV_MOD_1080P_60HZ) {
				count += sprintf(buf + count, "\t%16s", "1080p60hz");
			} else if(BSP_disp_hdmi_get_mode(screen_id) == DISP_TV_MOD_1080P_50HZ) {
				count += sprintf(buf + count, "\t%16s", "1080p50hz");
			} else if(BSP_disp_hdmi_get_mode(screen_id) == DISP_TV_MOD_1080I_50HZ) {
				count += sprintf(buf + count, "\t%16s", "1080i50hz");
			} else if(BSP_disp_hdmi_get_mode(screen_id) == DISP_TV_MOD_1080I_60HZ) {
				count += sprintf(buf + count, "\t%16s", "1080i60hz");
			}
			count += sprintf(buf + count, "\t%4dx%4d\n", BSP_disp_get_screen_width(screen_id), BSP_disp_get_screen_height(screen_id));
		}

		hpd = BSP_disp_hdmi_get_hpd_status(screen_id); 
		count += sprintf(buf + count, "\t%11s\n", hpd? "hdmi plugin":"hdmi unplug");
		count += sprintf(buf + count, "    type  |  status  |  id  | pipe | z | pre_mult |    alpha   | colorkey |  format  | framebuffer |  	   source crop      |       frame       |   trd   |         address\n");
		count += sprintf(buf + count, "----------+--------+------+------+---+----------+------------+----------+----------+-------------+-----------------------+-------------------+---------+-----------------------------\n");
		num_layers = 4;
		/* layer info */
		for(layer_id=0; layer_id<num_layers; layer_id++) {
			__disp_layer_info_t layer_para;
			int ret;

			ret = BSP_disp_layer_get_para(screen_id, IDTOHAND(layer_id), &layer_para);
			if(ret == 0) {
				count += sprintf(buf + count, " %8s |", (layer_para.mode == DISP_LAYER_WORK_MODE_SCALER)? "SCALER":"NORAML");
				count += sprintf(buf + count, " %8s |", BSP_disp_layer_is_open(screen_id, IDTOHAND(layer_id))? "enable":"disable");
				count += sprintf(buf + count, " %4d |", layer_id);
				count += sprintf(buf + count, " %4d |", layer_para.pipe);
				count += sprintf(buf + count, " %1d |", layer_para.prio);
				count += sprintf(buf + count, " %8s |", (layer_para.fb.pre_multiply)? "Y":"N");
				count += sprintf(buf + count, " %5s(%3d) |", (layer_para.alpha_en)? "globl":"pixel", layer_para.alpha_val);
				count += sprintf(buf + count, " %8s |", (layer_para.ck_enable)? "enable":"disable");
				count += sprintf(buf + count, " %2d,%2d,%2d |", layer_para.fb.mode, layer_para.fb.format, layer_para.fb.seq);
				count += sprintf(buf + count, " [%4d,%4d] |", layer_para.fb.size.width, layer_para.fb.size.height);
				count += sprintf(buf + count, " [%4d,%4d,%4d,%4d] |", layer_para.src_win.x, layer_para.src_win.y, layer_para.src_win.width, layer_para.src_win.height);
				count += sprintf(buf + count, " [%4d,%4d,%4d,%4d] |", layer_para.scn_win.x, layer_para.scn_win.y, layer_para.scn_win.width, layer_para.scn_win.height);
				count += sprintf(buf + count, " [%1d%1d,%1d%1d] |", layer_para.fb.b_trd_src, layer_para.fb.trd_mode, layer_para.b_trd_out, layer_para.out_trd_mode);
				count += sprintf(buf + count, " [%8x,%8x,%8x] |", layer_para.fb.addr[0], layer_para.fb.addr[1], layer_para.fb.addr[2]);
				count += sprintf(buf + count, "\n");
			}
		}
		//count += sprintf(buf + count, "\n\tsmart backlight: %s\n", BSP_disp_drc_get_enable(screen_id)? "enable":"disable");

	}

	return count;
}

static ssize_t disp_sys_status_store(struct device *dev,
        struct device_attribute *attr,
        const char *buf, size_t count)
{
  return count;
}

static DEVICE_ATTR(sys_status, S_IRUGO|S_IWUSR|S_IWGRP,
    disp_sys_status_show, disp_sys_status_store);


#define ____SEPARATOR_REG_DUMP____
static ssize_t disp_reg_dump_show(struct device *dev,struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", "there is nothing here!");
}

static ssize_t disp_reg_dump_store(struct device *dev,struct device_attribute *attr,const char *buf, size_t count)
{
        if (count < 1)
        return -EINVAL;

        if(strnicmp(buf, "be0", 3) == 0)
        {
                BSP_disp_print_reg(1, DISP_REG_IMAGE0);
        }
        else if(strnicmp(buf, "be1", 3) == 0)
        {
                BSP_disp_print_reg(1, DISP_REG_IMAGE1);
        }
        else if(strnicmp(buf, "fe0", 3) == 0)
        {
                BSP_disp_print_reg(1, DISP_REG_SCALER0);
        }
        else if(strnicmp(buf, "fe1", 3) == 0)
        {
                BSP_disp_print_reg(1, DISP_REG_SCALER1);
        }
        else if(strnicmp(buf, "lcd0", 4) == 0)
        {
                BSP_disp_print_reg(1, DISP_REG_LCDC0);
        }
        else if(strnicmp(buf, "lcd1", 4) == 0)
        {
                BSP_disp_print_reg(1, DISP_REG_LCDC1);
        }
        else if(strnicmp(buf, "tve0", 4) == 0)
        {
                BSP_disp_print_reg(1, DISP_REG_TVEC0);
        }
        else if(strnicmp(buf, "tve1", 4) == 0)
        {
                BSP_disp_print_reg(1, DISP_REG_TVEC1);
        }
        else if(strnicmp(buf, "hdmi", 4) == 0)
        {
                BSP_disp_print_reg(1, DISP_REG_HDMI);
        }
        else if(strnicmp(buf, "ccmu", 4) == 0)
        {
                BSP_disp_print_reg(1, DISP_REG_CCMU);
        }
        else if(strnicmp(buf, "pioc", 4) == 0)
        {
                BSP_disp_print_reg(1, DISP_REG_PIOC);
        }
        else if(strnicmp(buf, "pwm", 3) == 0)
        {
                BSP_disp_print_reg(1, DISP_REG_PWM);
        }
        else
        {
                printk("Invalid para!\n");
        }
            
        return count;

}

static DEVICE_ATTR(reg_dump, S_IRUGO|S_IWUSR|S_IWGRP,disp_reg_dump_show, disp_reg_dump_store);


#define ____SEPARATOR_PRINT_CMD_LEVEL____
extern __u32 disp_print_cmd_level;
static ssize_t disp_print_cmd_level_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", disp_print_cmd_level);
}

static ssize_t disp_print_cmd_level_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int err;
        unsigned long val;

        err = strict_strtoul(buf, 10, &val);
        if (err) {
        	printk("Invalid size\n");
        	return err;
        }

        if((val != 0) && (val != 1))
        {
                printk("Invalid value, 0/1 is expected!\n");
        }else
        {
                disp_print_cmd_level = val;
        }
    
	return count;
}

static DEVICE_ATTR(print_cmd_level, S_IRUGO|S_IWUSR|S_IWGRP,
		disp_print_cmd_level_show, disp_print_cmd_level_store);

#define ____SEPARATOR_CMD_PRINT_LEVEL____
extern __u32 disp_cmd_print;
static ssize_t disp_cmd_print_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%x\n", disp_cmd_print);
}

static ssize_t disp_cmd_print_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int err;
        unsigned long val;
    
	err = strict_strtoul(buf, 16, &val);
	if (err) {
		printk("Invalid size\n");
		return err;
	}

   
        disp_cmd_print = val;    
    
	return count;
}

static DEVICE_ATTR(cmd_print, S_IRUGO|S_IWUSR|S_IWGRP,disp_cmd_print_show, disp_cmd_print_store);



#define ____SEPARATOR_PRINT_LEVEL____
static ssize_t disp_debug_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "debug=%s\n", bsp_disp_get_print_level()?"on" : "off");
}

static ssize_t disp_debug_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	if (count < 1)
        return -EINVAL;

        if (strnicmp(buf, "on", 2) == 0 || strnicmp(buf, "1", 1) == 0)
        {
                bsp_disp_set_print_level(1);
        }
        else if (strnicmp(buf, "off", 3) == 0 || strnicmp(buf, "0", 1) == 0)
        {
                bsp_disp_set_print_level(0);
        }
        else
        {
                return -EINVAL;
        }
    
	return count;
}

static DEVICE_ATTR(debug, S_IRUGO|S_IWUSR|S_IWGRP,disp_debug_show, disp_debug_store);



#define ____SEPARATOR_LAYER_PARA____
static ssize_t disp_layer_para_show(struct device *dev,struct device_attribute *attr, char *buf)
{
        __disp_layer_info_t layer_para;
        __s32 ret = 0;

        ret = BSP_disp_layer_get_para(sel, hid, &layer_para);
        if(ret != 0)
        {
                return sprintf(buf, "screen%d layer%d is not init\n", sel, HANDTOID(hid));
        }
        else
        {
                return sprintf(buf, "=== screen%d layer%d para ====\nmode: %d\nfb.size=<%dx%d>\nfb.fmt=<%d, %d, %d>\ntrd_src=<%d, %d> trd_out=<%d, %d>\npipe:%d\tprio: %d\nalpha: <%d, %d>\tcolor_key_en: %d\nsrc_window:<%d,%d,%d,%d>\nscreen_window:<%d,%d,%d,%d>\n======= screen%d layer%d para ====\n", 
                sel, HANDTOID(hid),layer_para.mode, layer_para.fb.size.width, 
                layer_para.fb.size.height, layer_para.fb.mode, layer_para.fb.format, 
                layer_para.fb.seq, layer_para.fb.b_trd_src,  layer_para.fb.trd_mode, 
                layer_para.b_trd_out, layer_para.out_trd_mode, layer_para.pipe, 
                layer_para.prio, layer_para.alpha_en, layer_para.alpha_val, 
                layer_para.ck_enable, layer_para.src_win.x, layer_para.src_win.y, 
                layer_para.src_win.width, layer_para.src_win.height, layer_para.scn_win.x, layer_para.scn_win.y, 
                layer_para.scn_win.width, layer_para.scn_win.height, sel, HANDTOID(hid));
        }
}

static ssize_t disp_layer_para_store(struct device *dev,struct device_attribute *attr,const char *buf, size_t count)
{
        printk("there no room for anything\n");
        
	return count;
}

static DEVICE_ATTR(layer_para, S_IRUGO|S_IWUSR|S_IWGRP,disp_layer_para_show, disp_layer_para_store);


#define ____SEPARATOR_SCRIPT_DUMP____
static ssize_t disp_script_dump_show(struct device *dev,struct device_attribute *attr, char *buf)
{
        if(sel == 0)
        {
                script_dump_mainkey("disp_init");
                script_dump_mainkey("lcd0_para");
                script_dump_mainkey("hdmi_para");
                script_dump_mainkey("power_sply");
                script_dump_mainkey("clock");
                script_dump_mainkey("dram_para");
        }else if(sel == 1)
        {
                script_dump_mainkey("lcd1_para");
        }

        return sprintf(buf, "%s\n", "oh yeah!");
}

static ssize_t disp_script_dump_store(struct device *dev,struct device_attribute *attr,const char *buf, size_t count)
{
        char main_key[32];

        if(strlen(buf) == 0) {
        	printk("Invalid para\n");
        	return -1;
        }

        memcpy(main_key, buf, strlen(buf)+1);

        if(sel == 0)
        {
                script_dump_mainkey("disp_init");
                script_dump_mainkey("lcd0_para");
                script_dump_mainkey("hdmi_para");
                script_dump_mainkey("power_sply");
                script_dump_mainkey("clock");
                script_dump_mainkey("dram_para");
        }else if(sel == 1)
        {
                script_dump_mainkey("lcd1_para");
        }

	return count;
}

static DEVICE_ATTR(script_dump, S_IRUGO|S_IWUSR|S_IWGRP,disp_script_dump_show, disp_script_dump_store);


#define ____SEPARATOR_LCD____
static ssize_t disp_lcd_show(struct device *dev,struct device_attribute *attr, char *buf)
{
        if(BSP_disp_get_output_type(sel) == DISP_OUTPUT_TYPE_LCD)
        {
                return sprintf(buf, "screen%d lcd on!\n", sel);
        }
        else
        {
                return sprintf(buf, "screen%d lcd off!\n", sel);
        }
}

static ssize_t disp_lcd_store(struct device *dev,struct device_attribute *attr,const char *buf, size_t count)
{
	int err;
        unsigned long val;
    
	err = strict_strtoul(buf, 10, &val);
	if (err) {
		printk("Invalid size\n");
		return err;
	}

        if((val==0))
        {
                DRV_lcd_close(sel);
        }else
        {
                BSP_disp_hdmi_close(sel);
                DRV_lcd_open(sel);
	}
    
	return count;
}

static DEVICE_ATTR(lcd, S_IRUGO|S_IWUSR|S_IWGRP,disp_lcd_show, disp_lcd_store);

static ssize_t disp_lcd_bl_show(struct device *dev,struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", BSP_disp_lcd_get_bright(sel));
}

static ssize_t disp_lcd_bl_store(struct device *dev,struct device_attribute *attr,const char *buf, size_t count)
{
	int err;
        unsigned long val;
    
	err = strict_strtoul(buf, 10, &val);
	if (err) {
		printk("Invalid size\n");
		return err;
	}

        if((val < 0) || (val > 255))
        {
                printk("Invalid value, 0~255 is expected!\n");
        }else
        {
                BSP_disp_lcd_set_bright(sel, val);
	}
    
	return count;
}

static DEVICE_ATTR(lcd_bl, S_IRUGO|S_IWUSR|S_IWGRP,disp_lcd_bl_show, disp_lcd_bl_store);





#define ____SEPARATOR_HDMI____
static ssize_t disp_hdmi_show(struct device *dev,struct device_attribute *attr, char *buf)
{
        if(BSP_disp_get_output_type(sel) == DISP_OUTPUT_TYPE_HDMI)
        {
                return sprintf(buf, "screen%d hdmi on, mode=%d\n", sel, BSP_disp_hdmi_get_mode(sel));
        }
        else
        {
                return sprintf(buf, "screen%d hdmi off!\n", sel);
        }
}

static ssize_t disp_hdmi_store(struct device *dev,struct device_attribute *attr,const char *buf, size_t count)
{
	int err;
        unsigned long val;
    
	err = strict_strtoul(buf, 10, &val);
	if (err) {
		printk("Invalid size\n");
		return err;
	}

        if((val==0xff))
        {
                BSP_disp_hdmi_close(sel);
        }else
        {
                BSP_disp_hdmi_close(sel);
                if(BSP_disp_hdmi_set_mode(sel,(__disp_tv_mode_t)val) == 0)
                {
                        BSP_disp_hdmi_open(sel);
                }
	}
    
	return count;
}

static DEVICE_ATTR(hdmi, S_IRUGO|S_IWUSR|S_IWGRP,disp_hdmi_show, disp_hdmi_store);

static ssize_t disp_hdmi_hpd_show(struct device *dev,struct device_attribute *attr, char *buf)
{

        return sprintf(buf, "screen%d hdmi hpd=%d\n", sel, BSP_disp_hdmi_get_hpd_status(sel));
}

static ssize_t disp_hdmi_hpd_store(struct device *dev,struct device_attribute *attr,const char *buf, size_t count)
{
	return count;
}

static DEVICE_ATTR(hdmi_hpd, S_IRUGO|S_IWUSR|S_IWGRP,disp_hdmi_hpd_show, disp_hdmi_hpd_store);





#define ____SEPARATOR_LAYER____
static ssize_t disp_layer_mode_show(struct device *dev,struct device_attribute *attr, char *buf)
{
        __disp_layer_info_t para;
        int ret;

        ret = BSP_disp_layer_get_para(sel, hid, &para);
        if(0 == ret)
        {
                return sprintf(buf, "%d\n", para.mode);
        }else
        {
                return sprintf(buf, "%s\n", "not used!");
        }
}

static ssize_t disp_layer_mode_store(struct device *dev,struct device_attribute *attr,const char *buf, size_t count)
{
	int err;
        unsigned long val;
        int ret;
        __disp_layer_info_t para;
    
	err = strict_strtoul(buf, 10, &val);
	if (err) {
		printk("Invalid size\n");
		return err;
	}

        if((val>4))
        {
                printk("Invalid value, <5 is expected!\n");

        }else
        {
                ret = BSP_disp_layer_get_para(sel, hid, &para);
                if(0 == ret)
                {
                        para.mode = (__disp_layer_work_mode_t)val;
                        if(para.mode == DISP_LAYER_WORK_MODE_SCALER)
                        {
                                para.scn_win.width = BSP_disp_get_screen_width(sel);
                                para.scn_win.height = BSP_disp_get_screen_height(sel);
                        }
                        BSP_disp_layer_set_para(sel, hid, &para);
                }else
                {
                    printk("not used!\n");
                }
	}
    
	return count;
}

static DEVICE_ATTR(layer_mode, S_IRUGO|S_IWUSR|S_IWGRP,disp_layer_mode_show, disp_layer_mode_store);




#define ____SEPARATOR_DEU_NODE____
static ssize_t disp_vpp_enable_show(struct device *dev,struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", BSP_disp_layer_get_vpp_enable(sel, hid));
}

static ssize_t disp_vpp_enable_store(struct device *dev,struct device_attribute *attr,const char *buf, size_t count)
{
	int err;
        unsigned long val;
    
	err = strict_strtoul(buf, 10, &val);
	if (err) {
		printk("Invalid size\n");
		return err;
	}

        if((val>1))
        {
                printk("Invalid value, 0/1 is expected!\n");
        }else
        {
                printk("%ld\n", val);
                BSP_disp_layer_vpp_enable(sel, hid, (unsigned int)val);
	}
    
	return count;
}


static ssize_t disp_vpp_luma_sharp_level_show(struct device *dev,struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", BSP_disp_layer_get_luma_sharp_level(sel, hid));
}

static ssize_t disp_vpp_luma_sharp_level_store(struct device *dev,struct device_attribute *attr,const char *buf, size_t count)
{
	int err;
        unsigned long val;
    
	err = strict_strtoul(buf, 10, &val);
	if (err) {
		printk("Invalid size\n");
		return err;
	}

        if(val > 4)
        {
                printk("Invalid value, 0~4 is expected!\n");
        }else
        {
                printk("%ld\n", val);
                BSP_disp_layer_set_luma_sharp_level(sel, hid, (unsigned int)val);
	}
    
	return count;
}

static ssize_t disp_vpp_chroma_sharp_level_show(struct device *dev,struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", BSP_disp_layer_get_chroma_sharp_level(sel, hid));
}

static ssize_t disp_vpp_chroma_sharp_level_store(struct device *dev,struct device_attribute *attr,const char *buf, size_t count)
{
	int err;
        unsigned long val;
    
	err = strict_strtoul(buf, 10, &val);
	if (err) {
		printk("Invalid size\n");
		return err;
	}

        if(val > 4)
        {
                printk("Invalid value, 0~4 is expected!\n");
        }else
        {
                printk("%ld\n", val);
                BSP_disp_layer_set_chroma_sharp_level(sel, hid, (unsigned int)val);
	}
    
	return count;
}

static ssize_t disp_vpp_black_exten_level_show(struct device *dev,struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", BSP_disp_layer_get_black_exten_level(sel, hid));
}

static ssize_t disp_vpp_black_exten_level_store(struct device *dev,struct device_attribute *attr,const char *buf, size_t count)
{
	int err;
        unsigned long val;
    
	err = strict_strtoul(buf, 10, &val);
	if (err) {
		printk("Invalid size\n");
		return err;
	}

        if(val > 4)
        {
                printk("Invalid value, 0~4 is expected!\n");
        }else
        {
                printk("%ld\n", val);
                BSP_disp_layer_set_black_exten_level(sel, hid, (unsigned int)val);
	}
    
	return count;
}

static ssize_t disp_vpp_white_exten_level_show(struct device *dev,struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", BSP_disp_layer_get_white_exten_level(sel, hid));
}

static ssize_t disp_vpp_white_exten_level_store(struct device *dev,struct device_attribute *attr,const char *buf, size_t count)
{
	int err;
        unsigned long val;
    
	err = strict_strtoul(buf, 10, &val);
	if (err) {
		printk("Invalid size\n");
		return err;
	}

        if(val > 4)
        {
                printk("Invalid value, 0~4 is expected!\n");
        }else
        {
                printk("%ld\n", val);
                BSP_disp_layer_set_white_exten_level(sel, hid, (unsigned int)val);
	}
    
	return count;
}

static DEVICE_ATTR(vpp_en, S_IRUGO|S_IWUSR|S_IWGRP,disp_vpp_enable_show, disp_vpp_enable_store);

static DEVICE_ATTR(vpp_luma_level, S_IRUGO|S_IWUSR|S_IWGRP,disp_vpp_luma_sharp_level_show, disp_vpp_luma_sharp_level_store);

static DEVICE_ATTR(vpp_chroma_level, S_IRUGO|S_IWUSR|S_IWGRP,disp_vpp_chroma_sharp_level_show, disp_vpp_chroma_sharp_level_store);

static DEVICE_ATTR(vpp_black_level, S_IRUGO|S_IWUSR|S_IWGRP,disp_vpp_black_exten_level_show, disp_vpp_black_exten_level_store);

static DEVICE_ATTR(vpp_white_level, S_IRUGO|S_IWUSR|S_IWGRP,disp_vpp_white_exten_level_show, disp_vpp_white_exten_level_store);





#define ____SEPARATOR_LAYER_ENHANCE_NODE____
static ssize_t disp_layer_enhance_enable_show(struct device *dev,struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", BSP_disp_layer_get_enhance_enable(sel, hid));
}

static ssize_t disp_layer_enhance_enable_store(struct device *dev,struct device_attribute *attr,const char *buf, size_t count)
{
	int err;
        unsigned long bright_val;
    
	err = strict_strtoul(buf, 10, &bright_val);
        printk("string=%s, int=%ld\n", buf, bright_val);
	if (err) {
		printk("Invalid size\n");
		return err;
	}

        if(bright_val > 100)
        {
                printk("Invalid value, 0~100 is expected!\n");
        }else
        {
                printk("%ld\n", bright_val);
                BSP_disp_layer_enhance_enable(sel, hid, (unsigned int)bright_val);
	}
    
	return count;
}


static ssize_t disp_layer_bright_show(struct device *dev,struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", BSP_disp_layer_get_bright(sel, hid));
}

static ssize_t disp_layer_bright_store(struct device *dev,struct device_attribute *attr,const char *buf, size_t count)
{
	int err;
        unsigned long bright_val;
    
	err = strict_strtoul(buf, 10, &bright_val);
        printk("string=%s, int=%ld\n", buf, bright_val);
	if (err) {
		printk("Invalid size\n");
		return err;
	}

        if(bright_val > 100)
        {
                printk("Invalid value, 0~100 is expected!\n");
        }else
        {
                printk("%ld\n", bright_val);
                BSP_disp_layer_set_bright(sel, hid, (unsigned int)bright_val);
	}
    
	return count;
}

static ssize_t disp_layer_contrast_show(struct device *dev,struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", BSP_disp_layer_get_contrast(sel, hid));
}

static ssize_t disp_layer_contrast_store(struct device *dev,struct device_attribute *attr,const char *buf, size_t count)
{
	int err;
        unsigned long contrast_val;
    
	err = strict_strtoul(buf, 10, &contrast_val);

	if (err) {
		printk("Invalid size\n");
		return err;
	}

        if(contrast_val > 100)
        {
                printk("Invalid value, 0~100 is expected!\n");
        }else
        {
                printk("%ld\n", contrast_val);
                BSP_disp_layer_set_contrast(sel, hid, (unsigned int)contrast_val);
	}
    
	return count;
}

static ssize_t disp_layer_saturation_show(struct device *dev,struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", BSP_disp_layer_get_saturation(sel, hid));
}

static ssize_t disp_layer_saturation_store(struct device *dev,struct device_attribute *attr,const char *buf, size_t count)
{
	int err;
        unsigned long saturation_val;
    
	err = strict_strtoul(buf, 10, &saturation_val);

	if (err) {
		printk("Invalid size\n");
		return err;
	}

        if(saturation_val > 100)
        {
                printk("Invalid value, 0~100 is expected!\n");
        }else
        {
                printk("%ld\n", saturation_val);
                BSP_disp_layer_set_saturation(sel, hid,(unsigned int)saturation_val);
	}
    
	return count;
}

static ssize_t disp_layer_hue_show(struct device *dev,struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", BSP_disp_layer_get_hue(sel,hid));
}

static ssize_t disp_layer_hue_store(struct device *dev,struct device_attribute *attr,const char *buf, size_t count)
{
	int err;
        unsigned long hue_val;
    
	err = strict_strtoul(buf, 10, &hue_val);

	if (err) {
		printk("Invalid size\n");
		return err;
	}

        if(hue_val > 100)
        {
                printk("Invalid value, 0~100 is expected!\n");
        }else
        {
                printk("%ld\n", hue_val);
                BSP_disp_layer_set_hue(sel, hid,(unsigned int)hue_val);
	}
    
	return count;
}



#define ____SEPARATOR_SCREEN_ENHANCE_NODE____
static ssize_t disp_enhance_enable_show(struct device *dev,struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", BSP_disp_get_enhance_enable(sel));
}

static ssize_t disp_enhance_enable_store(struct device *dev,struct device_attribute *attr,const char *buf, size_t count)
{
	int err;
        unsigned long bright_val;
    
	err = strict_strtoul(buf, 10, &bright_val);
        printk("string=%s, int=%ld\n", buf, bright_val);
	if (err) {
		printk("Invalid size\n");
		return err;
	}

        if(bright_val > 100)
        {
                printk("Invalid value, 0~100 is expected!\n");
        }else
        {
                printk("%ld\n", bright_val);
                BSP_disp_enhance_enable(sel,(unsigned int)bright_val);
	}
    
	return count;
}



static ssize_t disp_bright_show(struct device *dev,struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", BSP_disp_get_bright(sel));
}

static ssize_t disp_bright_store(struct device *dev,struct device_attribute *attr,const char *buf, size_t count)
{
	int err;
        unsigned long bright_val;
    
	err = strict_strtoul(buf, 10, &bright_val);
        printk("string=%s, int=%ld\n", buf, bright_val);
	if (err) {
		printk("Invalid size\n");
		return err;
	}

        if(bright_val > 100)
        {
                printk("Invalid value, 0~100 is expected!\n");
        }else
        {
                printk("%ld\n", bright_val);
                BSP_disp_set_bright(sel, (unsigned int)bright_val);
	}
    
	return count;
}

static ssize_t disp_contrast_show(struct device *dev,struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", BSP_disp_get_contrast(sel));
}

static ssize_t disp_contrast_store(struct device *dev,struct device_attribute *attr,const char *buf, size_t count)
{
	int err;
        unsigned long contrast_val;
    
	err = strict_strtoul(buf, 10, &contrast_val);

	if (err) {
		printk("Invalid size\n");
		return err;
	}

        if(contrast_val > 100)
        {
                printk("Invalid value, 0~100 is expected!\n");
        }else
        {
                printk("%ld\n", contrast_val);
                BSP_disp_set_contrast(sel, (unsigned int)contrast_val);
	}
    
	return count;
}

static ssize_t disp_saturation_show(struct device *dev,struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", BSP_disp_get_saturation(sel));
}

static ssize_t disp_saturation_store(struct device *dev,struct device_attribute *attr,const char *buf, size_t count)
{
	int err;
        unsigned long saturation_val;
    
	err = strict_strtoul(buf, 10, &saturation_val);

	if (err) {
		printk("Invalid size\n");
		return err;
	}

        if(saturation_val > 100)
        {
                printk("Invalid value, 0~100 is expected!\n");
        }else
        {
                printk("%ld\n", saturation_val);
                BSP_disp_set_saturation(sel, (unsigned int)saturation_val);
	}
    
	return count;
}

static ssize_t disp_hue_show(struct device *dev,struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", BSP_disp_get_hue(sel));
}

static ssize_t disp_hue_store(struct device *dev,struct device_attribute *attr,const char *buf, size_t count)
{
	int err;
        unsigned long hue_val;
    
	err = strict_strtoul(buf, 10, &hue_val);

	if (err) {
		printk("Invalid size\n");
		return err;
	}

        if(hue_val > 100)
        {
                printk("Invalid value, 0~100 is expected!\n");
        }else
        {
                printk("%ld\n", hue_val);
                BSP_disp_set_hue(sel, (unsigned int)hue_val);
	}
    
	return count;
}



static DEVICE_ATTR(layer_enhance_en, S_IRUGO|S_IWUSR|S_IWGRP,disp_layer_enhance_enable_show, disp_layer_enhance_enable_store);

static DEVICE_ATTR(layer_bright, S_IRUGO|S_IWUSR|S_IWGRP,disp_layer_bright_show, disp_layer_bright_store);

static DEVICE_ATTR(layer_contrast, S_IRUGO|S_IWUSR|S_IWGRP,disp_layer_contrast_show, disp_layer_contrast_store);

static DEVICE_ATTR(layer_saturation, S_IRUGO|S_IWUSR|S_IWGRP,disp_layer_saturation_show, disp_layer_saturation_store);

static DEVICE_ATTR(layer_hue, S_IRUGO|S_IWUSR|S_IWGRP,disp_layer_hue_show, disp_layer_hue_store);


static DEVICE_ATTR(screen_enhance_en, S_IRUGO|S_IWUSR|S_IWGRP,disp_enhance_enable_show, disp_enhance_enable_store);

static DEVICE_ATTR(screen_bright, S_IRUGO|S_IWUSR|S_IWGRP,disp_bright_show, disp_bright_store);

static DEVICE_ATTR(screen_contrast, S_IRUGO|S_IWUSR|S_IWGRP,disp_contrast_show, disp_contrast_store);

static DEVICE_ATTR(screen_saturation, S_IRUGO|S_IWUSR|S_IWGRP,disp_saturation_show, disp_saturation_store);

static DEVICE_ATTR(screen_hue, S_IRUGO|S_IWUSR|S_IWGRP,disp_hue_show, disp_hue_store);




#define ____SEPARATOR_COLORBAR____
static ssize_t disp_colorbar_show(struct device *dev,struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", "there is nothing here!");
}

extern __s32 fb_draw_colorbar(__u32 base, __u32 width, __u32 height, struct fb_var_screeninfo *var);

static ssize_t disp_colorbar_store(struct device *dev,struct device_attribute *attr,const char *buf, size_t count)
{
	int err;
        unsigned long val;
    
	err = strict_strtoul(buf, 10, &val);
	if (err) {
		printk("Invalid size\n");
		return err;
	}

        if((val>7)) {
                printk("Invalid value, 0~7 is expected!\n");
        }
        else {
                fb_draw_colorbar((__u32)g_fbi.fbinfo[val]->screen_base, g_fbi.fbinfo[val]->var.xres, 
                g_fbi.fbinfo[val]->var.yres, &(g_fbi.fbinfo[val]->var));;
	}
    
	return count;
}

static DEVICE_ATTR(colorbar, S_IRUGO|S_IWUSR|S_IWGRP,disp_colorbar_show, disp_colorbar_store);



static struct attribute *disp_attributes[] = {
        &dev_attr_screen_enhance_en.attr,
        &dev_attr_screen_bright.attr,
        &dev_attr_screen_contrast.attr,
        &dev_attr_screen_saturation.attr,
        &dev_attr_screen_hue.attr,
        &dev_attr_layer_enhance_en.attr,
        &dev_attr_layer_bright.attr,
        &dev_attr_layer_contrast.attr,
        &dev_attr_layer_saturation.attr,
        &dev_attr_layer_hue.attr,
        &dev_attr_vpp_en.attr,
        &dev_attr_vpp_luma_level.attr,
        &dev_attr_vpp_chroma_level.attr,
        &dev_attr_vpp_black_level.attr,
        &dev_attr_vpp_white_level.attr,
        &dev_attr_sel.attr,
        &dev_attr_hid.attr,
        &dev_attr_reg_dump.attr,
        &dev_attr_layer_mode.attr,
        &dev_attr_lcd.attr,
        &dev_attr_lcd_bl.attr,
        &dev_attr_hdmi.attr,
        &dev_attr_script_dump.attr,
        &dev_attr_colorbar.attr,
        &dev_attr_layer_para.attr,
        &dev_attr_hdmi_hpd.attr,
        &dev_attr_print_cmd_level.attr,
        &dev_attr_cmd_print.attr,
        &dev_attr_debug.attr,
        &dev_attr_sys_status.attr,
        NULL
};

static struct attribute_group disp_attribute_group = {
	.name = "attr",
	.attrs = disp_attributes
};

int disp_attr_node_init(void)
{
        unsigned int ret;

        ret = sysfs_create_group(&display_dev->kobj,&disp_attribute_group);
        sel = 0;
        hid = 100;
        return 0;
}

int disp_attr_node_exit(void)
{
        return 0;
}
