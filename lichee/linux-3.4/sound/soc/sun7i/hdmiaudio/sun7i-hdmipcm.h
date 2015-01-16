/*
 * sound\soc\sun7i\hdmiaudio\sun7i-hdmipcm.h
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

#ifndef SUN7I_HDMIPCM_H_
#define SUN7I_HDMIPCM_H_

#define SUN6I_HDMIBASE 		0x01c16000
#define SUN6I_HDMIAUDIO_TX	0x400

	struct sun7i_dma_params {
		char *name;		
		dma_addr_t dma_addr;	
	};
	

#endif //SUN7I_HDMIPCM_H_
