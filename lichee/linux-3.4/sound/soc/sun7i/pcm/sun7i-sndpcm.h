/*
 * sound\soc\sun7i\i2s\sun7i_sndi2s.h
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
#ifndef SUN7I_SNDI2S_H_
#define SUN7I_SNDI2S_H_

struct sun7i_sndi2s_platform_data {
	int iis_bclk;
	int iis_ws;
	int iis_data;
	void (*power)(int);
	int model;
}
#endif
