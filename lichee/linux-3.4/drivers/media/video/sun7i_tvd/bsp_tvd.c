#include "bsp_tvd.h"
#include <linux/delay.h>

#define REG_RD32(reg)    (*((volatile __u32 *)(reg)))         
#define REG_WR32(reg, value) (*((volatile __u32 *)(reg))  = (value))  

static __u32 addr_base = 0;

void TVD_init(void *addr)
{
	__u32 i; 
        addr_base = (__u32)addr;       
        for(i=0;i<4;i++){
    	        REG_WR32(addr_base+0x0130+0x100*i,0x04000000);//reg for set once here    
        }
}

void TVD_config(__u32 interface, __u32 system)
{
	//global reg set here
        REG_WR32(addr_base+0x0500,0x00000f11);  //first, open adc, so the tvd has clock
        REG_WR32(addr_base+0x0000,0x00001f00);  //reset tvd
        msleep(1);                              //need delay here for tvd reset 
        if(interface==TVD_CVBS)                 //composite
        {
	        switch(system)
		{
		        case TVD_NTSC://NTSC
                                REG_WR32(addr_base+0x0008,0x00590000);//ntsc vs pal
                                REG_WR32(addr_base+0x000c,0x00000010);//ntsc vs pal
                                REG_WR32(addr_base+0x0018,0x00002080);
                                REG_WR32(addr_base+0x0024,0x0682810a);
                                REG_WR32(addr_base+0x0028,0x00006440);
                                REG_WR32(addr_base+0x002c,0x0000cb74);//ntsc vs pal
                                REG_WR32(addr_base+0x0030,0x21f07c1f);//ntsc vs pal
                                REG_WR32(addr_base+0x0034,0x20000000);

                                //REG_WR32(addr_base+0x0024,0x05C8B10A);
                                //REG_WR32(addr_base+0x0028,0x00005838);
                                //REG_WR32(addr_base+0x002c,0x0000cb74);
                                //REG_WR32(addr_base+0x0030,0x262E8BA2);
                                //REG_WR32(addr_base+0x0034,0x24000000);

                                REG_WR32(addr_base+0x0044,0x50823925);
                                REG_WR32(addr_base+0x004c,0x0e70106c);
                                REG_WR32(addr_base+0x0050,0x00000a00);
                                REG_WR32(addr_base+0x005c,0x0000006f);
                                REG_WR32(addr_base+0x0070,0x00002050);
                                REG_WR32(addr_base+0x0074,0x000003c3);
                                REG_WR32(addr_base+0x0080,0x00500082);//ntsc vs pal
                                REG_WR32(addr_base+0x0084,0x00610022);//ntsc vs pal
				break;
			case TVD_PAL://PAL
                                REG_WR32(addr_base+0x0008,0x11590102);//ntsc vs pal
                                REG_WR32(addr_base+0x000c,0x0000001e);//ntsc vs pal
                                REG_WR32(addr_base+0x0018,0x00002480);
                                REG_WR32(addr_base+0x0024,0x0682810a);
                                REG_WR32(addr_base+0x0028,0x00006440);
                                REG_WR32(addr_base+0x002c,0x00000d74);//ntsc vs pal
                                REG_WR32(addr_base+0x0030,0x2a098acb);//ntsc vs pal
                                REG_WR32(addr_base+0x0034,0x20000000);
                                REG_WR32(addr_base+0x0044,0x50823925);
                                REG_WR32(addr_base+0x004c,0x0e70106c);
                                REG_WR32(addr_base+0x0050,0x00000a00);
                                REG_WR32(addr_base+0x005c,0x0000006f);
                                REG_WR32(addr_base+0x0070,0x00002050);
                                REG_WR32(addr_base+0x0074,0x000003c3);
                                REG_WR32(addr_base+0x0080,0x00500087);//ntsc vs pal
                                REG_WR32(addr_base+0x0084,0x00c10026);//ntsc vs pal
				break;
                        default:
                                break;
		}
	        REG_WR32(addr_base+0x0504,0x00000000);
		REG_WR32(addr_base+0x052c,0xe0110000);
		//1 channel cvbs
	        //REG_WR32(addr_base+0x0500,0x00000111);	
	        //REG_WR32(addr_base+0x0000,0x00000321);
	        //default open all 4 channels if you don't care power consumption
	        REG_WR32(addr_base+0x0500,0x00000f11);
	        REG_WR32(addr_base+0x0000,0x00001f2f);
	}
	else if(interface==TVD_YPBPR_I)//ypbpr 480i/576i
	{
		switch(system)
		{
			case TVD_NTSC://480i
    				REG_WR32(addr_base+0x0008,0x00594001);
			        REG_WR32(addr_base+0x0018,0x00002080);
			        REG_WR32(addr_base+0x0080,0x00500082);
			        REG_WR32(addr_base+0x0084,0x00610022);
				break;
			case TVD_PAL://576i
                                REG_WR32(addr_base+0x0008,0x10594101);
                                REG_WR32(addr_base+0x0018,0x00002480);
                                REG_WR32(addr_base+0x0080,0x00500087);
                                REG_WR32(addr_base+0x0084,0x00c10026);
				break;
		}
	        REG_WR32(addr_base+0x0504,0x00000000);
			REG_WR32(addr_base+0x052c,0xe0110000);
	        REG_WR32(addr_base+0x0500,0x00020711);
	        REG_WR32(addr_base+0x0000,0x00000321);
	}
	else if(interface==TVD_YPBPR_P)//ypbpr 480p/576p
	{
		switch(system)
		{
			case TVD_NTSC://480p
    				REG_WR32(addr_base+0x0008,0x80594001);
    				REG_WR32(addr_base+0x0034,0x20000000);
    				REG_WR32(addr_base+0x0070,0x00002050);
				break;
			case TVD_PAL://576p not work
			        REG_WR32(addr_base+0x0008,0x90594101);
			        REG_WR32(addr_base+0x0034,0x20000000);
			        REG_WR32(addr_base+0x0070,0x00002050);
			        REG_WR32(addr_base+0x0080,0x00500087);
			        REG_WR32(addr_base+0x0084,0x00c10026);
			    break;
		}
	        REG_WR32(addr_base+0x0504,0x00000000);
	        REG_WR32(addr_base+0x052c,0xe0110000);
	        REG_WR32(addr_base+0x0500,0x00020711);
	        REG_WR32(addr_base+0x0000,0x00000321);
	}	
}

void TVD_set_width(__u32 id,__u32 w)
{
	__u32 reg_val;
	reg_val = REG_RD32(addr_base + 0x134+0x100*id);
	reg_val &= ~(0xfff<<0);
	reg_val |= ((w>720)?720:w)<<0;
        REG_WR32(addr_base+0x0134+0x100*id, reg_val);
}

void TVD_set_width_jump(__u32 id,__u32 j)
{
        REG_WR32(addr_base+0x0138+0x100*id, j); 
}

void TVD_set_height(__u32 id,__u32 h)
{
	__u32 reg_val;
	reg_val = REG_RD32(addr_base + 0x134+0x100*id);
	reg_val &= ~(0x7ff<<16);
	reg_val |= h<<16;
	reg_val |= h<<16;
        REG_WR32(addr_base+0x0134+0x100*id, reg_val);
}

void TVD_irq_enable(__u32 id,tvd_irq_t irq)
{
	__u32 reg_val;
	switch(irq){
		case TVD_FRAME_DONE:	
			reg_val = REG_RD32(addr_base + 0x148);
			reg_val |= 1<<(24+id);
			REG_WR32(addr_base + 0x148, reg_val);
			break;
		case TVD_LOCK:
			break;	
		case TVD_UNLOCK:
			break;	
                default:
                        break;
	}
}

void TVD_irq_disable(__u32 id,tvd_irq_t irq)
{
	__u32 reg_val;
	switch(irq){
		case TVD_FRAME_DONE:	
			reg_val = REG_RD32(addr_base + 0x148);
			reg_val &= ~(1<<(24+id));
			REG_WR32(addr_base + 0x148, reg_val);
			break;
                default:
                        break;
	}	
}

__s32 TVD_irq_status_get(__u32 id,tvd_irq_t irq)
{
	__u32 reg_val, ret = -1;
	switch(irq){
		case TVD_FRAME_DONE:	
			reg_val = REG_RD32(addr_base+0x140);
			ret = (reg_val>>(24+id))&1;
			break;
                default:
                        printk("TVD get irq status error\n");
                        break;
	}	
	return ret;
}

void TVD_irq_status_clear(__u32 id,tvd_irq_t irq)
{
	__u32 reg_val;
	switch(irq){
		case TVD_FRAME_DONE:	
			reg_val = 1<<(24+id);
			REG_WR32(addr_base+0x140, reg_val);
			break;
                default:
                        break;
	}	
}

void TVD_capture_on(__u32 id)
{
	__u32 reg_val;
	reg_val = REG_RD32(addr_base+0x130+0x100*id);
	reg_val |= 1<<0;
	REG_WR32(addr_base+0x130+0x100*id, reg_val);
}
void TVD_capture_off(__u32 id)
{
	__u32 reg_val;
	reg_val = REG_RD32(addr_base+0x130+0x100*id);
	reg_val &= ~(1<<0);
	REG_WR32(addr_base+0x130+0x100*id, reg_val);
}

void TVD_set_addr_y(__u32 id,__u32 addr)
{
	REG_WR32(addr_base + 0x100+0x100*id, addr);
}

void TVD_set_addr_c(__u32 id,__u32 addr)
{
	REG_WR32(addr_base + 0x110+0x100*id, addr);
}

void TVD_set_fmt(__u32 id, tvd_fmt_t fmt)
{
	__u32 reg_val;
	reg_val = REG_RD32(addr_base + 0x130+0x100*id);
	switch(fmt){
		case TVD_PL_YUV422:				
			reg_val &= ~(1<<24);
			reg_val |= 1<<4;
			break;
		case TVD_PL_YUV420:				
			reg_val &= ~(1<<24);
			reg_val &= ~(1<<4);
			break;
		case TVD_MB_YUV420:				
			reg_val |= 1<<24;
			reg_val &= ~(1<<4);
			break;
	}	
	REG_WR32(addr_base + 0x130+0x100*id, reg_val);
}

__u32 TVD_get_status(__u32 id)
{
	__u32 reg_val = 0;
	__u32 det = 0;
	__u32 system = 0;
	reg_val = REG_RD32(addr_base+0x0600+0x20*id);
	if(reg_val&1){
		det = 0;
	}
	else{
		det = 1;
	}
	if(reg_val&(1<<18)){
		system = 1;
	}
	else{
		system = 0;
	}
	return ((det<<0)+(system<<4));//bit0=det bit4=system
}

void TVD_set_color(__u32 id,__u32 luma,__u32 contrast,__u32 saturation,__u32 hue)
{
        __u32 reg_val=0;
        reg_val  = REG_RD32(addr_base+0x0018);
        reg_val &= 0xffff0000;
        reg_val |= contrast;
        reg_val |= luma<<8;
        REG_WR32(addr_base + 0x0018, reg_val);


        reg_val  = REG_RD32(addr_base+0x001c);
        reg_val &= 0xffff0000;
        reg_val |= saturation;
        reg_val |= hue<<8;
        REG_WR32(addr_base + 0x001c, reg_val);
}

void TVD_uv_swap(__u8 uv_swap)
{
        __u32 reg_val=0;
        if(uv_swap)
        {
                reg_val  = REG_RD32(addr_base+0x0014);
                reg_val &= 0xffffffbf;
                reg_val |= 1<<6;
                REG_WR32(addr_base + 0x0014, reg_val);
        }
        else
        {
                reg_val  = REG_RD32(addr_base+0x0014);
                reg_val &= 0xffffffbf;
                REG_WR32(addr_base + 0x0014, reg_val);
        }
}
