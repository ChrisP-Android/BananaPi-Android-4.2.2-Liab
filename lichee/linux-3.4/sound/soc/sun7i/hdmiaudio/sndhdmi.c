/*
 * sound\soc\sun7i\hdmiaudio\sndhdmi.c
 * (C) Copyright 2010-2016
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
#include <linux/delay.h>
#include <linux/slab.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/initval.h>
#include <linux/io.h>
#include <linux/drv_hdmi.h>
#include <mach/sys_config.h>

__audio_hdmi_func 	g_hdmi_func;
hdmi_audio_t 		hdmi_para;

static int hdmi_used = 0;
#define SNDHDMI_RATES  (SNDRV_PCM_RATE_8000_192000|SNDRV_PCM_RATE_KNOT)
#define SNDHDMI_FORMATS (SNDRV_PCM_FMTBIT_S8 | SNDRV_PCM_FMTBIT_S16_LE | \
		                     SNDRV_PCM_FMTBIT_S18_3LE | SNDRV_PCM_FMTBIT_S20_3LE)

/*the struct just for register the hdmiaudio codec node*/
struct sndhdmi_priv {
	int sysclk;
	int dai_fmt;

	struct snd_pcm_substream *master_substream;
	struct snd_pcm_substream *slave_substream;
};

void audio_set_hdmi_func(__audio_hdmi_func *hdmi_func)
{
	if (hdmi_func) {
		g_hdmi_func.hdmi_audio_enable 	= hdmi_func->hdmi_audio_enable;
		g_hdmi_func.hdmi_set_audio_para = hdmi_func->hdmi_set_audio_para;
	} else {
		printk("error:%s,line:%d\n", __func__, __LINE__);
	}
}
EXPORT_SYMBOL(audio_set_hdmi_func);

static int sndhdmi_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params,
	struct snd_soc_dai *dai)
{
	if ((!substream)||(!params)) {
		printk("error:%s,line:%d\n", __func__, __LINE__);
		return -EAGAIN;
	}

	hdmi_para.sample_rate = params_rate(params);
	hdmi_para.channel_num = params_channels(params);
	if (4 == hdmi_para.channel_num) {
		hdmi_para.channel_num 	= 2;
		hdmi_para.data_raw 		= 1;
	} else {
		hdmi_para.data_raw = 0;
	}
	g_hdmi_func.hdmi_set_audio_para(&hdmi_para);

	return 0;
}

static int sndhdmi_set_dai_fmt(struct snd_soc_dai *codec_dai,
			       unsigned int fmt)
{
	return 0;
}

static int sndhdmi_trigger(struct snd_pcm_substream *substream,
                              int cmd, struct snd_soc_dai *dai)
{
	int ret = 0;

	switch (cmd) {
		case SNDRV_PCM_TRIGGER_START:
		case SNDRV_PCM_TRIGGER_RESUME:
		case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
			if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
			} else {
				g_hdmi_func.hdmi_audio_enable(1, 1);
				
			}
		break;
		case SNDRV_PCM_TRIGGER_STOP:
		case SNDRV_PCM_TRIGGER_SUSPEND:
		case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
			if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
			} else {
				g_hdmi_func.hdmi_audio_enable(0, 1);
			}
			
			break;
		default:
			ret = -EINVAL;
			break;
	}
	return ret;
}

/*codec dai operation*/
struct snd_soc_dai_ops sndhdmi_dai_ops = {
	.hw_params 	= sndhdmi_hw_params,
	.set_fmt 	= sndhdmi_set_dai_fmt,
	.trigger 	= sndhdmi_trigger,
};

/*codec dai*/
struct snd_soc_dai_driver sndhdmi_dai = {
	.name 		= "sndhdmi",
	/* playback capabilities */
	.playback 	= {
		.stream_name 	= "Playback",
		.channels_min 	= 1,
		.channels_max 	= 4,
		.rates 			= SNDHDMI_RATES,
		.formats 		= SNDHDMI_FORMATS,
	},
	/* pcm operations */
	.ops 		= &sndhdmi_dai_ops,
};
EXPORT_SYMBOL(sndhdmi_dai);

static int sndhdmi_soc_probe(struct snd_soc_codec *codec)
{
	struct sndhdmi_priv *sndhdmi = NULL;

	if (!codec) {
		printk("error:%s,line:%d\n", __func__, __LINE__);
		return -EAGAIN;
	}

	sndhdmi = kzalloc(sizeof(struct sndhdmi_priv), GFP_KERNEL);
	if (sndhdmi == NULL) {
		printk("error at:%s,%d\n",__func__,__LINE__);
		return -ENOMEM;
	}

	snd_soc_codec_set_drvdata(codec, sndhdmi);

	return 0;
}

static int sndhdmi_soc_remove(struct snd_soc_codec *codec)
{
	struct sndhdmi_priv *sndhdmi = NULL;
	if (!codec) {
		printk("error:%s,line:%d\n", __func__, __LINE__);
		return -EAGAIN;
	}
	sndhdmi = snd_soc_codec_get_drvdata(codec);

	kfree(sndhdmi);

	return 0;
}

static struct snd_soc_codec_driver soc_codec_dev_sndhdmi = {
	.probe 	=	sndhdmi_soc_probe,
	.remove =   sndhdmi_soc_remove,
};

static int __devinit sndhdmi_codec_probe(struct platform_device *pdev)
{	
	if (!pdev) {
		printk("error:%s,line:%d\n", __func__, __LINE__);
		return -EAGAIN;
	}
	return snd_soc_register_codec(&pdev->dev, &soc_codec_dev_sndhdmi, &sndhdmi_dai, 1);	
}

static int __devexit sndhdmi_codec_remove(struct platform_device *pdev)
{
	if (!pdev) {
		printk("error:%s,line:%d\n", __func__, __LINE__);
		return -EAGAIN;
	}
	snd_soc_unregister_codec(&pdev->dev);
	return 0;
}

/*data relating*/
static struct platform_device sndhdmi_codec_device = {
	.name = "sun7i-hdmiaudio-codec",
};
static struct platform_driver sndhdmi_codec_driver = {
	.driver = {
		.name = "sun7i-hdmiaudio-codec",
		.owner = THIS_MODULE,
	},
	.probe 	= sndhdmi_codec_probe,
	.remove = __devexit_p(sndhdmi_codec_remove),
};

static int __init sndhdmi_codec_init(void)
{
	int err = 0;
	script_item_u val;
	script_item_value_type_e  type;
	type = script_get_item("hdmi_para", "hdcp_enable", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        printk("[hdcp] type err!\n");
    }

	hdmi_used =!val.val;
	if (hdmi_used){
		if ((err = platform_device_register(&sndhdmi_codec_device)) < 0)
			return err;

		if ((err = platform_driver_register(&sndhdmi_codec_driver)) < 0)
			return err;

		return 0;
	}
}
module_init(sndhdmi_codec_init);

static void __exit sndhdmi_codec_exit(void)
{
	platform_driver_unregister(&sndhdmi_codec_driver);
}
module_exit(sndhdmi_codec_exit);

MODULE_DESCRIPTION("SNDHDMI ALSA soc codec driver");
MODULE_AUTHOR("huangxin, <huangxin@Reuuimllatech.com>");
MODULE_LICENSE("GPL");
