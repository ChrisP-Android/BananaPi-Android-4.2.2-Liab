/*
 * linux-3.3\drivers\rtc\rtc-sun7i-regs.h
 * (C) Copyright 2010-2016
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * liugang <liugang@allwinnertech.com>
 *
 * some simple description for this code
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#ifndef __RTC_SUNXI_REGS__
#define __RTC_SUNXI_REGS__

typedef struct {
	u32   osc32k_src_sel:       1; /* XXX */
	u32   resv0:                1; /* XXX */
	u32   ext_losc_gsm:         2; /* XXX */
	u32   resv1:                3; /* XXX */
	u32   rtc_yymmdd_acce:      1; /* XXX */
	u32   rtc_hhmmss_acce:      1; /* XXX */
	u32   alm_ddhhmmss_acce:    1; /* XXX */
	u32   resv2:                4; /* XXX */
	u32   clk32k_auto_swt_en:   1; /* XXX */
	u32   clk32k_auto_swt_pend: 1; /* XXX */
	u32   key_field:            16; /* XXX */
}losc_ctrl;

typedef struct {
	u32   day:                5; /* XXX */
	u32   resv0:              3; /* XXX */
	u32   month:              4; /* XXX */
	u32   resv1:              4; /* XXX */
	u32   year:               8; /* XXX */
	u32   leap:               1; /* XXX */
	u32   resv2:              5; /* XXX */
	u32   rtc_sim_ctrl:       1; /* XXX */
	u32   rtc_test_mode_ctrl: 1; /* XXX */
}rtc_yy_mm_dd;

typedef struct {
	u32   second:       6; /* XXX */
	u32   resv0:        2; /* XXX */
	u32   minute:       6; /* XXX */
	u32   resv1:        2; /* XXX */
	u32   hour:         5; /* XXX */
	u32   resv2:        8; /* XXX */
	u32   wk_no:        3; /* XXX */
}rtc_hh_mm_ss;

typedef struct {
	u32   second:       6; /* XXX */
	u32   resv0:        2; /* XXX */
	u32   minute:       6; /* XXX */
	u32   resv1:        2; /* XXX */
	u32   hour:         5; /* XXX */
	u32   resv2:        3; /* spec bug, is 2 but actually shouldbe 3, 2013-1-26 */
	u32   day:          8; /* XXX */
}alarm_cnt_dd_hh_mm_ss;

typedef struct {
	u32   second:       6; /* XXX */
	u32   resv0:        2; /* XXX */
	u32   minute:       6; /* XXX */
	u32   resv1:        2; /* XXX */
	u32   hour:         5; /* XXX */
	u32   resv2:        11;/* XXX */
}alarm_wk_hh_mm_ss;

typedef struct {
	u32   wk0_alm_en:    1; /* only enable will pendbit set when condition meet */
	u32   wk1_alm_en:    1; /* XXX */
	u32   wk2_alm_en:    1; /* XXX */
	u32   wk3_alm_en:    1; /* XXX */
	u32   wk4_alm_en:    1; /* XXX */
	u32   wk5_alm_en:    1; /* XXX */
	u32   wk6_alm_en:    1; /* XXX */
	u32   resv0:         1; /* XXX */
	u32   alm_cnt_en:    1; /* XXX */
	u32   resv1:         23;/* XXX */
}alarm_enable;

typedef struct {
	u32   alarm_cnt_irq_en:  1; /* XXX */
	u32   alarm_wk_irq_en:   1; /* XXX */
	u32   resv0:             30;/* XXX */
}alarm_irq_enable;

typedef struct {
	u32   cnt_irq_pend:  1; /* alarm week(0/1/2/3/4/5/6) irq pending */
	u32   wk_irq_pend:   1; /* alarm counter irq pending */
	u32   resv0:         30;/* reserve */
}alarm_irq_status;

typedef struct {
	u32   tmr_gp_data:   32; /* timer general purpose reg value canbe stored if RTCVDD larger than 1.0v */
}timer_general_purpose;

typedef struct {
	u32   alarm_wakeup:   1; /* configuration of alarm wakeup output
				  * 0: diable alarm wakeup output;
				  * 1: enable alarm wakeup output;
				  */
	u32   resv0:         31; /* reserve */
}alarm_config;

typedef struct {
	volatile losc_ctrl	         losc_ctl;          /* offset 0x100, losc control reg */
	volatile rtc_yy_mm_dd	         r_yy_mm_dd;        /* XXX */
	volatile rtc_hh_mm_ss	         r_hh_mm_ss;        /* XXX */
	volatile alarm_cnt_dd_hh_mm_ss	 a_cnt_dd_hh_mm_ss; /* XXX */
	volatile alarm_wk_hh_mm_ss	 a_wk_hh_mm_ss;     /* XXX */
	volatile alarm_enable	         a_enable;          /* XXX */
	volatile alarm_irq_enable	 a_irq_enable;      /* XXX */
	volatile alarm_irq_status	 a_irq_status;      /* XXX */
	volatile timer_general_purpose	 timer_gen_purpose; /* XXX */
	volatile alarm_config	         a_config;          /* XXX */
}rtc_alarm_reg;

#endif  /* __RTC_SUNXI_REGS__ */
