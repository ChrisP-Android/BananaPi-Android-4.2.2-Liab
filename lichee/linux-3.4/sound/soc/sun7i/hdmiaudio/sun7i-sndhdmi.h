/*
 * sound\soc\sun7i\hdmiaudio\sun7i-sndhdmi.h
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

#ifndef SUN7I_SNDHDMI_H_
#define SUN7I_SNDHDMI_H_

struct sun7i_sndhdmi_platform_data {
	int hdmiaudio_bclk;
	int hdmiaudio_ws;
	int hdmiaudio_data;
	void (*power)(int);
	int model;
}
#endif