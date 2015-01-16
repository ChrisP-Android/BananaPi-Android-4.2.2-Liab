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
#include <linux/drv_hdmi.h>
#include "sndhdcp.h"
__audio_hdmi_func 	g_hdcp_func;
hdmi_audio_t 		hdcp_para;

struct sndhdcp_priv {
	int sysclk;
	int dai_fmt;

	struct snd_pcm_substream *master_substream;
	struct snd_pcm_substream *slave_substream;
};

static int hdcp_used = 0;
#define sndi2s_RATES  (SNDRV_PCM_RATE_8000_192000|SNDRV_PCM_RATE_KNOT)
#define sndi2s_FORMATS (SNDRV_PCM_FMTBIT_S8 | SNDRV_PCM_FMTBIT_S16_LE | \
		                     SNDRV_PCM_FMTBIT_S18_3LE | SNDRV_PCM_FMTBIT_S20_3LE)

//hdmi_audio_t hdcp_parameter;
void audio_set_hdcp_func(__audio_hdmi_func *hdmi_func)
{
	if (hdmi_func) {
		g_hdcp_func.hdmi_audio_enable 	= hdmi_func->hdmi_audio_enable;
		g_hdcp_func.hdmi_set_audio_para = hdmi_func->hdmi_set_audio_para;
	} else {
		printk("error:%s,line:%d\n", __func__, __LINE__);
	}
}
EXPORT_SYMBOL(audio_set_hdcp_func);

static int sndhdcp_mute(struct snd_soc_dai *dai, int mute)
{
	return 0;
}

static int sndhdcp_startup(struct snd_pcm_substream *substream,
	struct snd_soc_dai *dai)
{
	return 0;
}

static void sndhdcp_shutdown(struct snd_pcm_substream *substream,
	
	struct snd_soc_dai *dai)
{
	
}

static int sndhdcp_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params,
	struct snd_soc_dai *dai)
{
	//hdcp_parameter.sample_rate = params_rate(params);
		if ((!substream)||(!params)) {
		printk("error:%s,line:%d\n", __func__, __LINE__);
		return -EAGAIN;
	}

	hdcp_para.sample_rate = params_rate(params);
	hdcp_para.channel_num = params_channels(params);
	if (4 == hdcp_para.channel_num) {
		hdcp_para.channel_num 	= 2;
		hdcp_para.data_raw 		= 1;
	} else {
		hdcp_para.data_raw = 0;
	}
	g_hdcp_func.hdmi_set_audio_para(&hdcp_para);

	return 0;
}

static int sndhdcp_set_dai_sysclk(struct snd_soc_dai *codec_dai,
				  int clk_id, unsigned int freq, int dir)
{
	return 0;
}

static int sndhdcp_set_dai_clkdiv(struct snd_soc_dai *codec_dai, int div_id, int div)
{

//	hdcp_parameter.fs_between = div;
	
	return 0;
}


static int sndhdcp_trigger(struct snd_pcm_substream *substream,
                              int cmd, struct snd_soc_dai *dai)
{
	int ret = 0;

	switch (cmd) {
		case SNDRV_PCM_TRIGGER_START:
		case SNDRV_PCM_TRIGGER_RESUME:
		case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
			if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
			} else {
				g_hdcp_func.hdmi_audio_enable(1, 1);
				
			}
		break;
		case SNDRV_PCM_TRIGGER_STOP:
		case SNDRV_PCM_TRIGGER_SUSPEND:
		case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
			if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
			} else {
				g_hdcp_func.hdmi_audio_enable(0, 1);
			}
			
			break;
		default:
			ret = -EINVAL;
			break;
	}
	return ret;
}
static int sndhdcp_set_dai_fmt(struct snd_soc_dai *codec_dai,
			       unsigned int fmt)
{
	return 0;
}

struct snd_soc_dai_ops sndhdcp_dai_ops = {
	.startup = sndhdcp_startup,
	.shutdown = sndhdcp_shutdown,
	.hw_params = sndhdcp_hw_params,
	.digital_mute = sndhdcp_mute,
	.set_sysclk = sndhdcp_set_dai_sysclk,
	.set_clkdiv = sndhdcp_set_dai_clkdiv,
	.set_fmt = sndhdcp_set_dai_fmt,
		.trigger 	= sndhdcp_trigger,
};

struct snd_soc_dai_driver sndhdcp_dai = {
	.name = "sndhdcp",
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
	.ops = &sndhdcp_dai_ops,
	
};
EXPORT_SYMBOL(sndhdcp_dai);
	
static int sndhdcp_soc_probe(struct snd_soc_codec *codec)
{
	struct sndhdcp_priv *sndi2s;

	sndi2s = kzalloc(sizeof(struct sndhdcp_priv), GFP_KERNEL);
	if(sndi2s == NULL){		
		return -ENOMEM;
	}		
	snd_soc_codec_set_drvdata(codec, sndi2s);

	return 0;
}

/* power down chip */
static int sndhdcp_soc_remove(struct snd_soc_codec *codec)
{
	struct sndhdmi_priv *sndi2s = snd_soc_codec_get_drvdata(codec);

	kfree(sndi2s);

	return 0;
}

static struct snd_soc_codec_driver soc_codec_dev_sndhdcp = {
	.probe 	=	sndhdcp_soc_probe,
	.remove =   sndhdcp_soc_remove,
};

static int __devinit sndi2s_codec_probe(struct platform_device *pdev)
{
	return snd_soc_register_codec(&pdev->dev, &soc_codec_dev_sndhdcp, &sndhdcp_dai, 1);	
}

static int __devexit sndi2s_codec_remove(struct platform_device *pdev)
{
	snd_soc_unregister_codec(&pdev->dev);
	return 0;
}
/*data relating*/
static struct platform_device sndhdcp_codec_device = {
	.name = "sun7i-hdcp-codec",
};

/*method relating*/
static struct platform_driver sndhdcp_codec_driver = {
	.driver = {
		.name = "sun7i-hdcp-codec",
		.owner = THIS_MODULE,
	},
	.probe = sndi2s_codec_probe,
	.remove = __devexit_p(sndi2s_codec_remove),
};

static int __init sndhdcp_codec_init(void)
{	
	int err = 0;
	script_item_u val;
	script_item_value_type_e  type;
	type = script_get_item("hdmi_para", "hdcp_enable", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        printk("[hdcp] type err!\n");
    }

	hdcp_used =val.val;
	if (hdcp_used) {
		if((err = platform_device_register(&sndhdcp_codec_device)) < 0)
			return err;
	
		if ((err = platform_driver_register(&sndhdcp_codec_driver)) < 0)
			return err;
	} else {
       printk("[hdcp]sndhdcp cannot find any using configuration for controllers, return directly!\n");
       return 0;
    }
	
	return 0;
}
module_init(sndhdcp_codec_init);

static void __exit sndhdcp_codec_exit(void)
{
	if (hdcp_used) {	
		hdcp_used = 0;
		platform_driver_unregister(&sndhdcp_codec_driver);
	}
}
module_exit(sndhdcp_codec_exit);

MODULE_DESCRIPTION("SNDHDCP ALSA soc codec driver");
MODULE_AUTHOR("Zoltan Devai, Christian Pellegrin <chripell@evolware.org>");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:sun7i-hdcp-codec");
