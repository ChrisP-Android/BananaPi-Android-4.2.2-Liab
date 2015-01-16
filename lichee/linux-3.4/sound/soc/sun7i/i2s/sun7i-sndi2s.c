/*
 * sound\soc\sun7i\i2s\sun7i_sndi2s.c
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
#include <linux/clk.h>
#include <linux/mutex.h>

#include <sound/pcm.h>
#include <sound/soc.h>
#include <sound/pcm_params.h>
#include <sound/soc-dapm.h>
#include <mach/sys_config.h>
#include <linux/io.h>

#include "sun7i-i2s.h"
#include "sun7i-i2sdma.h"
#undef I2S_DBG
#if (1)
    #define I2S_DBG(format,args...)  printk("[SWITCH] "format,##args)    
#else
    #define I2S_DBG(...)    
#endif

static bool i2s_pcm_select 	= 1;

static int i2s_used 		= 0;
static int i2s_master 		= 0;
static int audio_format 	= 0;
static int signal_inversion = 0;

/*
*	i2s_pcm_select == 0:-->	pcm
*	i2s_pcm_select == 1:-->	i2s
*/

static int sun7i_i2s_set_audio_mode(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	i2s_pcm_select = ucontrol->value.integer.value[0];

	if (i2s_pcm_select) {
		audio_format 		= 1;
		signal_inversion 	= 1;
		i2s_master 			= 4;
	} else {
		audio_format 		= 4;
		signal_inversion 	= 3;
		i2s_master 			= 1;
	}
	return 0;
}

static int sun7i_i2s_get_audio_mode(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = i2s_pcm_select;
	return 0;
}
 
   /* I2s Or Pcm Audio Mode Select */
static const struct snd_kcontrol_new sun7i_i2s_controls[] = {
	SOC_SINGLE_BOOL_EXT("I2s Or Pcm Audio Mode Select format", 0,
			sun7i_i2s_get_audio_mode, sun7i_i2s_set_audio_mode),
};

//set mclk and bclk division
static int sun7i_sndi2s_hw_params(struct snd_pcm_substream *substream,
					struct snd_pcm_hw_params *params)
{
	int ret  = 0;
	u32 freq = 22579200;
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	unsigned long sample_rate = params_rate(params);

	switch (sample_rate) {
		case 8000:
		case 16000:
		case 32000:
		case 64000:
		case 128000:
		case 12000:
		case 24000:
		case 48000:
		case 96000:
		case 192000:
			freq = 24576000;
			break;
	}
	ret = snd_soc_dai_set_sysclk(cpu_dai, 0 , freq, i2s_pcm_select);
	if (ret < 0) {
		return ret;
	}

	ret = snd_soc_dai_set_fmt(codec_dai, SND_SOC_DAIFMT_DSP_A |
			SND_SOC_DAIFMT_IB_NF | SND_SOC_DAIFMT_CBM_CFM);
	if (ret < 0)
		return ret;
	/*
	* codec clk & FRM master. AP as slave
	*/
	ret = snd_soc_dai_set_fmt(cpu_dai, (audio_format | (signal_inversion<<8) | (i2s_master<<12)));
	if (ret < 0) {
		return ret;
	}
		
	ret = snd_soc_dai_set_clkdiv(cpu_dai, 0, sample_rate);
	if (ret < 0) {
		return ret;
	}
		
	/*
	*	audio_format == SND_SOC_DAIFMT_DSP_A
	*	signal_inversion<<8 == SND_SOC_DAIFMT_IB_NF
	*	i2s_master<<12 == SND_SOC_DAIFMT_CBM_CFM
	*/
	I2S_DBG("%s,line:%d,audio_format:%d,SND_SOC_DAIFMT_DSP_A:%d\n",\
			__func__, __LINE__, audio_format, SND_SOC_DAIFMT_DSP_A);
	I2S_DBG("%s,line:%d,signal_inversion:%d,signal_inversion<<8:%d,SND_SOC_DAIFMT_IB_NF:%d\n",\
			__func__, __LINE__, signal_inversion, signal_inversion<<8, SND_SOC_DAIFMT_IB_NF);
	I2S_DBG("%s,line:%d,i2s_master:%d,i2s_master<<12:%d,SND_SOC_DAIFMT_CBM_CFM:%d\n",\
			__func__, __LINE__, i2s_master, i2s_master<<12, SND_SOC_DAIFMT_CBM_CFM);

	return 0;
}

/*
 * Card initialization
 */
static int sun7i_i2s_init(struct snd_soc_pcm_runtime *rtd)
{
	struct snd_soc_codec *codec = rtd->codec;
	struct snd_soc_card *card = rtd->card;
	int ret;
		
	/* Add virtual switch */
	ret = snd_soc_add_codec_controls(codec, sun7i_i2s_controls,
					ARRAY_SIZE(sun7i_i2s_controls));
	if (ret) {
		dev_warn(card->dev,
				"Failed to register audio mode control, "
				"will continue without it.\n");
	}
	return 0;
}

static struct snd_soc_ops sun7i_sndi2s_ops = {
	.hw_params 		= sun7i_sndi2s_hw_params,
};

static struct snd_soc_dai_link sun7i_sndi2s_dai_link = {
	.name 			= "I2S",
	.stream_name 	= "SUN7I-I2S",
	.cpu_dai_name 	= "sun7i-i2s.0",
	.codec_dai_name = "sndi2s",
	.init 			= sun7i_i2s_init,
	.platform_name 	= "sun7i-i2s-pcm-audio.0",
	.codec_name 	= "sun7i-i2s-codec.0",
	.ops 			= &sun7i_sndi2s_ops,
};

static struct snd_soc_card snd_soc_sun7i_sndi2s = {
	.name = "sndi2s",
	.owner 		= THIS_MODULE,
	.dai_link 	= &sun7i_sndi2s_dai_link,
	.num_links = 1,
};

static struct platform_device *sun7i_sndi2s_device;

static int __init sun7i_sndi2s_init(void)
{
	int ret; 
	static script_item_u   val;
	script_item_value_type_e  type;
	type=script_get_item("i2s_para", "i2s_used", &val);
	if(SCIRPT_ITEM_VALUE_TYPE_INT != type){
		pr_info("i2s_used  type err\n");
	}
    i2s_used=val.val;
	type = script_get_item("i2s_para", "i2s_master", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        printk("[I2S] i2s_master type err!\n");
    }
	i2s_master = val.val;
	type = script_get_item("i2s_para", "audio_format", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        printk("[I2S] audio_format type err!\n");
    }
	audio_format = val.val;
	type = script_get_item("i2s_para", "signal_inversion", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        printk("[I2S] signal_inversion type err!\n");
    }
	signal_inversion = val.val;
    if (i2s_used) {
		sun7i_sndi2s_device = platform_device_alloc("soc-audio", 2);
		if(!sun7i_sndi2s_device)
			return -ENOMEM;
		platform_set_drvdata(sun7i_sndi2s_device, &snd_soc_sun7i_sndi2s);
		ret = platform_device_add(sun7i_sndi2s_device);		
		if (ret) {			
			platform_device_put(sun7i_sndi2s_device);
		}
	}else{
		printk("[I2S]sun7i_sndi2s cannot find any using configuration for controllers, return directly!\n");
        return 0;
	}
	return ret;
}

static void __exit sun7i_sndi2s_exit(void)
{
	if(i2s_used) {
		i2s_used = 0;
		platform_device_unregister(sun7i_sndi2s_device);
	}
}

module_init(sun7i_sndi2s_init);
module_exit(sun7i_sndi2s_exit);

MODULE_AUTHOR("ALL WINNER");
MODULE_DESCRIPTION("SUN7I_sndi2s ALSA SoC audio driver");
MODULE_LICENSE("GPL");
