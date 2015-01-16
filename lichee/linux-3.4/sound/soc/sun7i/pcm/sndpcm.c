/*
 * sound\soc\sun7i\i2s\sndi2s.c
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

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/initval.h>
#include <mach/sys_config.h>
#include <linux/io.h>

#include "sndpcm.h"

struct sndpcm_priv {
	int sysclk;
	int dai_fmt;

	struct snd_pcm_substream *master_substream;
	struct snd_pcm_substream *slave_substream;
};

static int pcm_used = 0;
#define sndi2s_RATES  (SNDRV_PCM_RATE_8000_192000|SNDRV_PCM_RATE_KNOT)
#define sndi2s_FORMATS (SNDRV_PCM_FMTBIT_S8 | SNDRV_PCM_FMTBIT_S16_LE | \
		                     SNDRV_PCM_FMTBIT_S18_3LE | SNDRV_PCM_FMTBIT_S20_3LE)

//hdmi_audio_t hdmi_parameter;

static int sndpcm_mute(struct snd_soc_dai *dai, int mute)
{
	return 0;
}

static int sndpcm_startup(struct snd_pcm_substream *substream,
	struct snd_soc_dai *dai)
{
	return 0;
}

static void sndpcm_shutdown(struct snd_pcm_substream *substream,
	struct snd_soc_dai *dai)
{
	
}

static int sndpcm_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params,
	struct snd_soc_dai *dai)
{
	//hdmi_parameter.sample_rate = params_rate(params);

	return 0;
}

static int sndpcm_set_dai_sysclk(struct snd_soc_dai *codec_dai,
				  int clk_id, unsigned int freq, int dir)
{
	return 0;
}

static int sndpcm_set_dai_clkdiv(struct snd_soc_dai *codec_dai, int div_id, int div)
{

//	hdmi_parameter.fs_between = div;
	
	return 0;
}

static int sndpcm_set_dai_fmt(struct snd_soc_dai *codec_dai,
			       unsigned int fmt)
{
	return 0;
}

struct snd_soc_dai_ops sndpcm_dai_ops = {
	.startup = sndpcm_startup,
	.shutdown = sndpcm_shutdown,
	.hw_params = sndpcm_hw_params,
	.digital_mute = sndpcm_mute,
	.set_sysclk = sndpcm_set_dai_sysclk,
	.set_clkdiv = sndpcm_set_dai_clkdiv,
	.set_fmt = sndpcm_set_dai_fmt,
};

struct snd_soc_dai_driver sndpcm_dai = {
	.name = "sndpcm",
	/* playback capabilities */
	.playback = {
		.stream_name = "Playback",
		.channels_min = 1,
		.channels_max = 2,
		.rates = sndi2s_RATES,
		.formats = sndi2s_FORMATS,
	},
	.capture = {
		.stream_name = "Capture",
		.channels_min = 1,
		.channels_max = 2,
		.rates = sndi2s_RATES,
		.formats = sndi2s_FORMATS,
	},
	/* pcm operations */
	.ops = &sndpcm_dai_ops,
	
};
EXPORT_SYMBOL(sndpcm_dai);
	
static int sndpcm_soc_probe(struct snd_soc_codec *codec)
{
	struct sndpcm_priv *sndpcm;

	sndpcm = kzalloc(sizeof(struct sndpcm_priv), GFP_KERNEL);
	if(sndpcm == NULL){		
		return -ENOMEM;
	}		
	snd_soc_codec_set_drvdata(codec, sndpcm);

	return 0;
}

/* power down chip */
static int sndpcm_soc_remove(struct snd_soc_codec *codec)
{
	struct sndhdmi_priv *sndpcm = snd_soc_codec_get_drvdata(codec);

	kfree(sndpcm);

	return 0;
}

static struct snd_soc_codec_driver soc_codec_dev_sndpcm = {
	.probe 	=	sndpcm_soc_probe,
	.remove =   sndpcm_soc_remove,
};

static int __devinit sndpcm_codec_probe(struct platform_device *pdev)
{
	return snd_soc_register_codec(&pdev->dev, &soc_codec_dev_sndpcm, &sndpcm_dai, 1);	
}

static int __devexit sndpcm_codec_remove(struct platform_device *pdev)
{
	snd_soc_unregister_codec(&pdev->dev);
	return 0;
}
/*data relating*/
static struct platform_device sndpcm_codec_device = {
	.name = "sun7i-pcm-codec",
};

/*method relating*/
static struct platform_driver sndpcm_codec_driver = {
	.driver = {
		.name = "sun7i-pcm-codec",
		.owner = THIS_MODULE,
	},
	.probe = sndpcm_codec_probe,
	.remove = __devexit_p(sndpcm_codec_remove),
};

static int __init sndpcm_codec_init(void)
{	
	int err = 0;
	script_item_u val;
	script_item_value_type_e  type;
	printk("func:%s(line:%d)sndpcm.c\n",__func__,__LINE__);
	type = script_get_item("pcm_para", "pcm_used", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        printk("[PCM] type err!\n");
    }
	pcm_used = val.val;
	printk("func:%s(line:%d)sndpcm.c,pcm_used:%d\n",__func__,__LINE__,pcm_used);
	if (pcm_used) {
		if((err = platform_device_register(&sndpcm_codec_device)) < 0)
			return err;
	
		if ((err = platform_driver_register(&sndpcm_codec_driver)) < 0)
			return err;
	} else {
       printk("[PCM]sndi2s cannot find any using configuration for controllers, return directly!\n");
       return 0;
    }
	
	return 0;
}
module_init(sndpcm_codec_init);

static void __exit sndpcm_codec_exit(void)
{
	if (pcm_used) {	
		pcm_used = 0;
		platform_driver_unregister(&sndpcm_codec_driver);
	}
}
module_exit(sndpcm_codec_exit);

MODULE_DESCRIPTION("SNDPCM ALSA soc codec driver");
MODULE_AUTHOR("Zoltan Devai, Christian Pellegrin <chripell@evolware.org>");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:sun7i-pcm-codec");
