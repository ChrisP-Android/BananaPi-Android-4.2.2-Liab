/*
 * sound\soc\sun7i\hdmiaudio\sun7i-hdmiaudio.h
 * (C) Copyright 2007-2011
 * Reuuimlla Technology Co., Ltd. <www.reuuimllatech.com>
 * chenpailin <chenpailin@Reuuimllatech.com>
 *
 * some simple description for this code
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */
#ifndef SUN7I_HDMIAUIDO_H_
#define SUN7I_HDMIAUIDO_H_
#include <linux/drv_hdmi.h>

extern hdmi_audio_t hdmi_para;
extern __audio_hdmi_func g_hdmi_func;

/*------------------------------------------------------------*/
/* Clock dividers */
#define SUN7I_DIV_MCLK	0
#define SUN7I_DIV_BCLK	1

#define SUN7I_HDMIAUDIOCLKD_MCLK_MASK   0x0f
#define SUN7I_HDMIAUDIOCLKD_MCLK_OFFS   0
#define SUN7I_HDMIAUDIOCLKD_BCLK_MASK   0x070
#define SUN7I_HDMIAUDIOCLKD_BCLK_OFFS   4
#define SUN7I_HDMIAUDIOCLKD_MCLKEN_OFFS 7

unsigned int sun7i_hdmiaudio_get_clockrate(void);
extern struct sun7i_hdmiaudio_info sun7i_hdmiaudio;

extern void sun7i_snd_txctrl_hdmiaudio(struct snd_pcm_substream *substream, int on);
extern void sun7i_snd_rxctrl_hdmiaudio(struct snd_pcm_substream *substream, int on);

struct sun7i_hdmiaudio_info {
	void __iomem   *regs;    /* IIS BASE */
	/*void __iomem   *ccmregs;  //CCM BASE
	void __iomem   *ioregs;   //IO BASE*/
};

extern struct sun7i_hdmiaudio_info sun7i_hdmiaudio;
#endif
