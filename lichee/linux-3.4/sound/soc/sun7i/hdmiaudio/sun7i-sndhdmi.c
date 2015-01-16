/*
 * sound\soc\sun7i\hdmiaudio\sun7i-sndhdmi.c
 * (C) Copyright 2007-2011
 * Reuuimlla Technology Co., Ltd. <www.reuuimllatech.com>
 * huangxin <huangxin@Reuuimllatech.com>
 *
 * some simple description for this code
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#include <linux/module.h>
#include <linux/clk.h>
#include <linux/mutex.h>

#include <sound/pcm.h>
#include <sound/soc.h>
#include <sound/pcm_params.h>
#include <sound/soc-dapm.h>
#include <linux/io.h>

#include "sun7i-hdmipcm.h"




static int sun7i_sndhdmi_hw_params(struct snd_pcm_substream *substream,
					struct snd_pcm_hw_params *params)
{
	int ret = 0;
	struct snd_soc_pcm_runtime *rtd = NULL;
	struct snd_soc_dai *codec_dai 	= NULL;
	struct snd_soc_dai *cpu_dai 	= NULL;
	
	if (!substream) {
		printk("error:%s,line:%d\n", __func__, __LINE__);
		return -EAGAIN;
	}
	rtd 		= substream->private_data;
	codec_dai 	= rtd->codec_dai;
	cpu_dai 	= rtd->cpu_dai;
	
	ret = snd_soc_dai_set_fmt(codec_dai, 0);
	if (ret < 0)
		return ret;

	ret = snd_soc_dai_set_fmt(cpu_dai, 0);
	if (ret < 0)
		return ret;

	return 0;
}

static struct snd_soc_ops sun7i_sndhdmi_ops = {
	.hw_params 	= sun7i_sndhdmi_hw_params,
};

static struct snd_soc_dai_link sun7i_sndhdmi_dai_link = {
	.name 			= "HDMIAUDIO",
	.stream_name 	= "SUN7I-HDMIAUDIO",
	.cpu_dai_name 	= "sun7i-hdmiaudio.0",
	.codec_dai_name = "sndhdmi",
	.platform_name 	= "sun7i-hdmiaudio-pcm-audio.0",
	.codec_name 	= "sun7i-hdmiaudio-codec.0",
	.ops 			= &sun7i_sndhdmi_ops,
};

static struct snd_soc_card snd_soc_sun7i_sndhdmi = {
	.name 		= "sndhdmi",
	.owner 		= THIS_MODULE,
	.dai_link 	= &sun7i_sndhdmi_dai_link,
	.num_links 	= 1,
};

static struct platform_device *sun7i_sndhdmi_device;

static int __init sun7i_sndhdmi_init(void)
{
	int ret;

	sun7i_sndhdmi_device = platform_device_alloc("soc-audio", 0);
		
	if(!sun7i_sndhdmi_device)
		return -ENOMEM;
			
	platform_set_drvdata(sun7i_sndhdmi_device, &snd_soc_sun7i_sndhdmi);
		
	ret = platform_device_add(sun7i_sndhdmi_device);		
		
	if (ret) {			
		platform_device_put(sun7i_sndhdmi_device);
	}

	return ret;
}

static void __exit sun7i_sndhdmi_exit(void)
{
	platform_device_unregister(sun7i_sndhdmi_device);
}

module_init(sun7i_sndhdmi_init);
module_exit(sun7i_sndhdmi_exit);

MODULE_AUTHOR("huangxin");
MODULE_DESCRIPTION("SUN7I_SNDHDMI ALSA SoC audio driver");
MODULE_LICENSE("GPL");
