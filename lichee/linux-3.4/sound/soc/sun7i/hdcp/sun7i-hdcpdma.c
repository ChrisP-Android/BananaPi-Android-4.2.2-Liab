/*
 * sound\soc\sun7i\i2s\sun7i-i2sdma.c
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
#include <linux/init.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>

#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>

#include <asm/dma.h>
#include <mach/hardware.h>
#include <mach/dma.h>

#include "sun7i-hdcp.h"
#include "sun7i-hdcpdma.h"

static volatile unsigned int capture_dmasrc = 0;
static volatile unsigned int capture_dmadst = 0;
static volatile unsigned int play_dmasrc = 0;
static volatile unsigned int play_dmadst = 0;

static const struct snd_pcm_hardware sun7i_pcm_play_hardware = {
	.info			= SNDRV_PCM_INFO_INTERLEAVED | SNDRV_PCM_INFO_BLOCK_TRANSFER |
				      SNDRV_PCM_INFO_MMAP | SNDRV_PCM_INFO_MMAP_VALID |
				      SNDRV_PCM_INFO_PAUSE | SNDRV_PCM_INFO_RESUME,
	.formats		= SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S20_3LE | SNDRV_PCM_FMTBIT_S24_LE,
	.rates			= SNDRV_PCM_RATE_8000_192000 | SNDRV_PCM_RATE_KNOT,
	.rate_min		= 8000,
	.rate_max		= 192000,
	.channels_min		= 1,
	.channels_max		= 2,
	.buffer_bytes_max	= 128*1024,    /* value must be (2^n)Kbyte size */
	.period_bytes_min	= 1024*4,//1024*4,
	.period_bytes_max	= 1024*32,//1024*32,
	.periods_min		= 4,//4,
	.periods_max		= 8,//8,
	.fifo_size		= 128,//32,
};

static const struct snd_pcm_hardware sun7i_pcm_capture_hardware = {
	.info			= SNDRV_PCM_INFO_INTERLEAVED | SNDRV_PCM_INFO_BLOCK_TRANSFER |
				      SNDRV_PCM_INFO_MMAP | SNDRV_PCM_INFO_MMAP_VALID |
				      SNDRV_PCM_INFO_PAUSE | SNDRV_PCM_INFO_RESUME,
	.formats		= SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S20_3LE | SNDRV_PCM_FMTBIT_S24_LE,
	.rates			= SNDRV_PCM_RATE_8000_192000 | SNDRV_PCM_RATE_KNOT,
	.rate_min		= 8000,
	.rate_max		= 192000,
	.channels_min		= 1,
	.channels_max		= 2,
	.buffer_bytes_max	= 128*1024,    /* value must be (2^n)Kbyte size */
	.period_bytes_min	= 1024*4,//1024*4,
	.period_bytes_max	= 1024*32,//1024*32,
	.periods_min		= 4,//4,
	.periods_max		= 8,//8,
	.fifo_size		= 128,//32,
};

struct sun7i_playback_runtime_data {
	spinlock_t lock;
	int state;
	unsigned int dma_loaded;
	unsigned int dma_limit;
	unsigned int dma_period;
	dma_addr_t dma_start;
	dma_addr_t dma_pos;
	dma_addr_t dma_end;
	dma_hdl_t	dma_hdl;
	bool		play_dma_flag;
	dma_cb_t 	play_done_cb;
	struct sun7i_dma_params *params;
};

struct sun7i_capture_runtime_data {
	spinlock_t lock;
	int state;
	unsigned int dma_loaded;
	unsigned int dma_limit;
	unsigned int dma_period;
	dma_addr_t dma_start;
	dma_addr_t dma_pos;
	dma_addr_t dma_end;
	dma_hdl_t	dma_hdl;
	bool		play_dma_flag;
	dma_cb_t 	play_done_cb;
	struct sun7i_dma_params *params;
};

static void sun7i_pcm_enqueue(struct snd_pcm_substream *substream)
{
	int play_ret = 0, capture_ret = 0;
	struct sun7i_playback_runtime_data *play_prtd = NULL;
	struct sun7i_capture_runtime_data *capture_prtd = NULL;
	dma_addr_t play_pos = 0, capture_pos = 0;
	unsigned long play_len = 0, capture_len = 0;
	unsigned int play_limit = 0, capture_limit = 0;
	
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		play_prtd = substream->runtime->private_data;
		play_pos = play_prtd->dma_pos;
		play_len = play_prtd->dma_period;
		play_limit = play_prtd->dma_limit;
		while (play_prtd->dma_loaded < play_limit) {
			if ((play_pos + play_len) > play_prtd->dma_end) {
				play_len  = play_prtd->dma_end - play_pos;
			}
			
			play_ret = sw_dma_enqueue(play_prtd->dma_hdl, play_pos, play_prtd->params->dma_addr, play_len);
			if (play_ret == 0) {
				play_prtd->dma_loaded++;
				play_pos += play_prtd->dma_period;
				if(play_pos >= play_prtd->dma_end)
					play_pos = play_prtd->dma_start;
			} else {
				break;
			}
		}
		play_prtd->dma_pos = play_pos;
	} else {
		/*pr_info("CAPTUR:sun7i_i2sdma.c::func:%s(line:%d)\n",__func__,__LINE__);*/
		capture_prtd = substream->runtime->private_data;
		capture_pos = capture_prtd->dma_pos;
		capture_len = capture_prtd->dma_period;
		capture_limit = capture_prtd->dma_limit;
		while (capture_prtd->dma_loaded < capture_limit) {
			if ((capture_pos + capture_len) > capture_prtd->dma_end) {
				capture_len  = capture_prtd->dma_end - capture_pos;
			}
			
			capture_ret = sw_dma_enqueue(capture_prtd->dma_hdl, capture_prtd->params->dma_addr, capture_pos, capture_len);
			if (capture_ret == 0) {
			capture_prtd->dma_loaded++;
			capture_pos += capture_prtd->dma_period;
			if (capture_pos >= capture_prtd->dma_end)
				capture_pos = capture_prtd->dma_start;
			} else {
				break;
			}
		}
		capture_prtd->dma_pos = capture_pos;
}
}

static void sun7i_audio_capture_buffdone(dma_hdl_t dma_hdl, void *parg)
{
	struct sun7i_capture_runtime_data *capture_prtd;
	struct snd_pcm_substream *substream = parg;

	/*pr_info("CAPTUR:sun7i_i2sdma.c::func:%s(line:%d)\n",__func__,__LINE__);*/
	capture_prtd = substream->runtime->private_data;
		if (substream && capture_prtd) {
			snd_pcm_period_elapsed(substream);
		}

	spin_lock(&capture_prtd->lock);
	{
		capture_prtd->dma_loaded--;
		sun7i_pcm_enqueue(substream);
	}
	spin_unlock(&capture_prtd->lock);
}

static void sun7i_audio_play_buffdone(dma_hdl_t dma_hdl, void *parg)
{
	struct sun7i_playback_runtime_data *play_prtd;
	struct snd_pcm_substream *substream = parg;
	/*pr_info("sun7i_i2sdma.c::func:%s(line:%d)\n",__func__,__LINE__);*/
	play_prtd = substream->runtime->private_data;
	if ((substream) && (play_prtd)) {
		snd_pcm_period_elapsed(substream);
	}

	spin_lock(&play_prtd->lock);
	{
		play_prtd->dma_loaded--;
		sun7i_pcm_enqueue(substream);
	}
	spin_unlock(&play_prtd->lock);
}

static snd_pcm_uframes_t sun7i_pcm_pointer(struct snd_pcm_substream *substream)
{
	unsigned long play_res = 0, capture_res = 0;
	struct sun7i_playback_runtime_data *play_prtd = NULL;
	struct sun7i_capture_runtime_data *capture_prtd = NULL;
	snd_pcm_uframes_t play_offset = 0;
	struct snd_pcm_runtime *play_runtime = NULL;
	struct snd_pcm_runtime *capture_runtime = NULL;
	
    if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		play_prtd = substream->runtime->private_data;
		play_runtime = substream->runtime;
		spin_lock(&play_prtd->lock);
		sw_dma_getposition(play_prtd->dma_hdl, (dma_addr_t*)&play_dmasrc, (dma_addr_t*)&play_dmadst);
		play_res = play_dmasrc + play_prtd->dma_period - play_prtd->dma_start;
		play_offset = bytes_to_frames(play_runtime, play_res);
		
		spin_unlock(&play_prtd->lock);
		if (play_offset >= substream->runtime->buffer_size) {
			play_offset = 0;
		}
		return play_offset;
    } else {
    	/*pr_info("CAPTUR:sun7i_i2sdma.c::func:%s(line:%d)\n",__func__,__LINE__);*/
		capture_prtd = substream->runtime->private_data;
		spin_lock(&capture_prtd->lock);
		sw_dma_getposition(capture_prtd->dma_hdl, (dma_addr_t*)&capture_dmasrc, (dma_addr_t*)&capture_dmadst);
		capture_res = capture_dmadst + capture_prtd->dma_period - capture_prtd->dma_start;
		spin_unlock(&capture_prtd->lock);
#if 1
		{
			snd_pcm_uframes_t capture_offset = 0;

			capture_runtime = substream->runtime;
			capture_offset = bytes_to_frames(capture_runtime, capture_res);
			if (capture_offset >= substream->runtime->buffer_size) {
				capture_offset = 0;
			}
			return capture_offset;
		}
#else
		if (capture_res >= snd_pcm_lib_buffer_bytes(substream)) {
			if (capture_res == snd_pcm_lib_buffer_bytes(substream))
				capture_res = 0;
		}
		return bytes_to_frames(substream->runtime, capture_res);
#endif
    }
}
static int sun7i_pcm_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params)
{	
	/*pr_info("sun7i_i2sdma.c::func:%s(line:%d)\n",__func__,__LINE__);*/
    struct snd_pcm_runtime *play_runtime = NULL, *capture_runtime = NULL;
    struct sun7i_playback_runtime_data *play_prtd = NULL;
    struct sun7i_capture_runtime_data *capture_prtd = NULL;
    struct snd_soc_pcm_runtime *play_rtd = NULL;
    struct snd_soc_pcm_runtime *capture_rtd = NULL;
    struct sun7i_dma_params *play_dma = NULL;
    struct sun7i_dma_params *capture_dma = NULL;
    unsigned long play_totbytes = 0, capture_totbytes = 0;

    if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		pr_info("play:sun7i_i2sdma.c::func:%s(line:%d)\n",__func__,__LINE__);
		play_runtime = substream->runtime;
		play_prtd = play_runtime ->private_data;
		play_rtd = substream->private_data;
		play_totbytes = params_buffer_bytes(params);
		play_dma = snd_soc_dai_get_dma_data(play_rtd->cpu_dai, substream);
		
		if (!play_dma) {
			return 0;
		}

		if (play_prtd->params == NULL) {
			play_prtd->params = play_dma;
				/*
		 * requeset audio dma handle(we don't care about the channel!)
		 */
		play_prtd->dma_hdl = sw_dma_request(play_prtd->params->name, CHAN_NORAML);
		if (NULL == play_prtd->dma_hdl) {
			printk(KERN_ERR "failed to request spdif dma handle\n");
			return -EINVAL;
		}
			
	}

	/*
	* set callback
	*/
	memset(&play_prtd->play_done_cb, 0, sizeof(play_prtd->play_done_cb));
	play_prtd->play_done_cb.func = sun7i_audio_play_buffdone;
	play_prtd->play_done_cb.parg = substream;
	/*use the full buffer callback, maybe we should use the half buffer callback?*/
	if (0 != sw_dma_ctl(play_prtd->dma_hdl, DMA_OP_SET_FD_CB, (void *)&(play_prtd->play_done_cb))) {
		printk(KERN_ERR "failed to set dma buffer done!!!\n");
		sw_dma_release(play_prtd->dma_hdl);
		return -EINVAL;
	}
		snd_pcm_set_runtime_buffer(substream, &substream->dma_buffer);
		play_runtime->dma_bytes = play_totbytes;
		
		spin_lock_irq(&play_prtd->lock);
		play_prtd->dma_loaded = 0;
		play_prtd->dma_limit = play_runtime->hw.periods_min;
		play_prtd->dma_period = params_period_bytes(params);
		play_prtd->dma_start = play_runtime->dma_addr;
		play_prtd->dma_pos = play_prtd->dma_start;
		play_prtd->dma_end = play_prtd->dma_start + play_totbytes;
		spin_unlock_irq(&play_prtd->lock);
    } else if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
    	pr_info("CAPTUR:sun7i_i2sdma.c::func:%s(line:%d)\n",__func__,__LINE__);
		capture_runtime = substream->runtime;
		capture_prtd = capture_runtime ->private_data;
		capture_rtd = substream->private_data;
		capture_totbytes = params_buffer_bytes(params);
		capture_dma = snd_soc_dai_get_dma_data(capture_rtd->cpu_dai, substream);
		
		if (!capture_dma) {
			return 0;
		}

		if (capture_prtd->params == NULL) {
			capture_prtd->params = capture_dma;
			
				/*
		 * requeset audio dma handle(we don't care about the channel!)
		 */
		capture_prtd->dma_hdl = sw_dma_request(capture_prtd->params->name, CHAN_NORAML);
		if (NULL == capture_prtd->dma_hdl) {
			printk(KERN_ERR "failed to request spdif dma handle\n");
			return -EINVAL;
		}
		}

							/*
	* set callback
	*/
	memset(&capture_prtd->play_done_cb, 0, sizeof(capture_prtd->play_done_cb));
	capture_prtd->play_done_cb.func = sun7i_audio_capture_buffdone;
	capture_prtd->play_done_cb.parg = substream;
	/*use the full buffer callback, maybe we should use the half buffer callback?*/
	if (0 != sw_dma_ctl(capture_prtd->dma_hdl, DMA_OP_SET_FD_CB, (void *)&(capture_prtd->play_done_cb))) {
		printk(KERN_ERR "failed to set dma buffer done!!!\n");
		sw_dma_release(capture_prtd->dma_hdl);
		return -EINVAL;
	}
		snd_pcm_set_runtime_buffer(substream, &substream->dma_buffer);
		capture_runtime->dma_bytes = capture_totbytes;
		
		spin_lock_irq(&capture_prtd->lock);
		capture_prtd->dma_loaded = 0;
		capture_prtd->dma_limit = capture_runtime->hw.periods_min;
		capture_prtd->dma_period = params_period_bytes(params);
		capture_prtd->dma_start = capture_runtime->dma_addr;
		capture_prtd->dma_pos = capture_prtd->dma_start;
		capture_prtd->dma_end = capture_prtd->dma_start + capture_totbytes;
		spin_unlock_irq(&capture_prtd->lock);
    } else {
		return -EINVAL;
    }

	return 0;
}

static int sun7i_pcm_hw_free(struct snd_pcm_substream *substream)
{
	struct sun7i_playback_runtime_data *play_prtd = NULL;
	struct sun7i_capture_runtime_data *capture_prtd = NULL;
	
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
 		play_prtd = substream->runtime->private_data;
 		/* TODO - do we need to ensure DMA flushed */
		if (play_prtd->params) {
	  	
		}
	
		snd_pcm_set_runtime_buffer(substream, NULL);

		if (play_prtd->params) {
			
				/*
		 * stop play dma transfer
		 */
		//printk("play DMA_OP_STOP:sun7i-i2sdma.c::func:%s(line:%d)\n",__func__,__LINE__);
		if (0 != sw_dma_ctl(play_prtd->dma_hdl, DMA_OP_STOP, NULL)) {
			return -EINVAL;
		}
		/*
		*	release play dma handle
		*/
		if (0 != sw_dma_release(play_prtd->dma_hdl)) {
			return -EINVAL;
		}
		play_prtd->dma_hdl = (dma_hdl_t)NULL;
			play_prtd->params = NULL;
		}
   	} else if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
   
		capture_prtd = substream->runtime->private_data;
   		/* TODO - do we need to ensure DMA flushed */
		if (capture_prtd->params) {
	  		
		}

		snd_pcm_set_runtime_buffer(substream, NULL);

		if (capture_prtd->params) {
		
					/*
		 * stop play dma transfer
		 */
		//printk("CAPTURE DMA_OP_STOP:sun7i-i2sdma.c::func:%s(line:%d)\n",__func__,__LINE__);
		if (0 != sw_dma_ctl(capture_prtd->dma_hdl, DMA_OP_STOP, NULL)) {
			return -EINVAL;
		}
		/*
		*	release play dma handle
		*/
		if (0 != sw_dma_release(capture_prtd->dma_hdl)) {
			return -EINVAL;
		}
		capture_prtd->dma_hdl = (dma_hdl_t)NULL;
			capture_prtd->params = NULL;
		}
   	} else {
		return -EINVAL;
	}

	return 0;
}

static int sun7i_pcm_prepare(struct snd_pcm_substream *substream)
{	
	
	dma_config_t codec_play_dma_conf;
	dma_config_t codec_capture_dma_conf;
	int play_ret = 0, capture_ret = 0;
	struct sun7i_playback_runtime_data *play_prtd = NULL;
	struct sun7i_capture_runtime_data *capture_prtd = NULL;
	/*pr_info("sun7i_i2sdma.c::substream->stream%d)\n",substream->stream);*/
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		play_prtd = substream->runtime->private_data;
		if (!play_prtd->params) {
			return 0;
			}
			/*pr_info("play:sun7i_i2sdma.c::func:%s(line:%d)\n",__func__,__LINE__);*/

			memset(&codec_play_dma_conf, 0, sizeof(codec_play_dma_conf));		
			codec_play_dma_conf.xfer_type.src_data_width 	= DATA_WIDTH_16BIT;		
			codec_play_dma_conf.xfer_type.src_bst_len 	= DATA_BRST_1;		
			codec_play_dma_conf.xfer_type.dst_data_width 	= DATA_WIDTH_16BIT;		
			codec_play_dma_conf.xfer_type.dst_bst_len 	= DATA_BRST_1;		
			codec_play_dma_conf.address_type.src_addr_mode 	= NDMA_ADDR_INCREMENT;		
			codec_play_dma_conf.address_type.dst_addr_mode 	= NDMA_ADDR_NOCHANGE;		
			codec_play_dma_conf.src_drq_type 	= N_SRC_SDRAM;		
			codec_play_dma_conf.dst_drq_type 	= N_DST_IIS2_TX;	
			codec_play_dma_conf.bconti_mode 		= false;		
			codec_play_dma_conf.irq_spt 		=  CHAN_IRQ_FD;	
			if(0 !=(play_ret = sw_dma_config(play_prtd->dma_hdl, &codec_play_dma_conf))) {
				printk("err:%s,line:%d\n", __func__, __LINE__);
				return -EINVAL;
				}
		

		/* flush the DMA channel */
		
		play_prtd->dma_loaded = 0;
		play_prtd->dma_pos = play_prtd->dma_start;
		/* enqueue dma buffers */
		sun7i_pcm_enqueue(substream);
		
		return play_ret;
	} else {
		capture_prtd = substream->runtime->private_data;
		
		if (!capture_prtd->params) {
			return 0;
		}
		/*pr_info("CAPTUR:sun7i_i2sdma.c::func:%s(line:%d)\n",__func__,__LINE__);*/
		memset(&codec_capture_dma_conf, 0, sizeof(codec_capture_dma_conf));		
		codec_capture_dma_conf.xfer_type.src_data_width 	= DATA_WIDTH_16BIT;		
		codec_capture_dma_conf.xfer_type.src_bst_len 	= DATA_BRST_1;		
		codec_capture_dma_conf.xfer_type.dst_data_width 	= DATA_WIDTH_16BIT;		
		codec_capture_dma_conf.xfer_type.dst_bst_len 	= DATA_BRST_1;		
		codec_capture_dma_conf.address_type.src_addr_mode 	= NDMA_ADDR_NOCHANGE;		
		codec_capture_dma_conf.address_type.dst_addr_mode 	= NDMA_ADDR_INCREMENT;		
		codec_capture_dma_conf.src_drq_type 	= N_SRC_IIS2_RX;		
		codec_capture_dma_conf.dst_drq_type 	= N_DST_SDRAM;		
		codec_capture_dma_conf.bconti_mode 		= false;		
		codec_capture_dma_conf.irq_spt 		=  CHAN_IRQ_FD;
		
		//capture_ret = sw_dma_config(capture_dma_config->dma_hdl, &capture_dma_config);
		if(0!=(capture_ret = sw_dma_config(capture_prtd->dma_hdl, &codec_capture_dma_conf))){
		printk("err:%s,line:%d\n", __func__, __LINE__);
			return -EINVAL;
			}
		

		/* flush the DMA channel */
		
		capture_prtd->dma_loaded = 0;
		capture_prtd->dma_pos = capture_prtd->dma_start;
		
		/* enqueue dma buffers */
		sun7i_pcm_enqueue(substream);
		
		return capture_ret;
}
}


static int sun7i_pcm_trigger(struct snd_pcm_substream *substream, int cmd)
{
	int ret = 0;
	/*dump_stack();*/
	struct sun7i_playback_runtime_data *play_prtd = NULL;
	struct sun7i_capture_runtime_data *capture_prtd = NULL;
	/*printk("[IIS] func:%s(line:%d),substream->stream == SNDRV_PCM_STREAM_PLAYBACK::%d\n",
							__func__,__LINE__,substream->stream == SNDRV_PCM_STREAM_PLAYBACK);*/
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		play_prtd = substream->runtime->private_data;
		spin_lock(&play_prtd->lock);
		/*printk("[IIS] func:%s(line:%d),cmd::%d\n",__func__,__LINE__,cmd);*/
		switch (cmd) {
		case SNDRV_PCM_TRIGGER_START:
		case SNDRV_PCM_TRIGGER_RESUME:
		case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
			//printk("play dma trigge start:sun7i-i2sdma.c::func:%s(line:%d)\n",__func__,__LINE__);
			//printk("[IIS] 0x01c22400+0x24 = %#x, line= %d\n", readl(0xf1c22400+0x24), __LINE__);
			
			 /*
		* start dma transfer
		*/
		if (0 != sw_dma_ctl(play_prtd->dma_hdl, DMA_OP_START, NULL)) {
			printk("%s err, dma start err\n", __FUNCTION__);
			return -EINVAL;
		}
			break;
		case SNDRV_PCM_TRIGGER_SUSPEND:
		case SNDRV_PCM_TRIGGER_STOP:
		case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
	        //pr_info("play dma stop:sun7i_i2sdma.c::func:%s(line:%d)\n",__func__,__LINE__);
	        //printk("[IIS] 0x01c22400+0x24 = %#x, line= %d\n", readl(0xf1c22400+0x24), __LINE__);
			
			/*
		* stop play dma transfer
		*/
		if (0 != sw_dma_ctl(play_prtd->dma_hdl, DMA_OP_STOP, NULL)) {
			printk("%s err, dma stop err\n", __FUNCTION__);
			return -EINVAL;
		}
			break;
		default:
			ret = -EINVAL;
			break;
		}
		spin_unlock(&play_prtd->lock);
	} else {
		capture_prtd = substream->runtime->private_data;
		spin_lock(&capture_prtd->lock);

		switch (cmd) {
		case SNDRV_PCM_TRIGGER_START:
		case SNDRV_PCM_TRIGGER_RESUME:
		case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
			
			//pr_info("CAPTUR dma start:sun7i_i2sdma.c::func:%s(line:%d)\n",__func__,__LINE__);
				 /*
		* start dma transfer
		*/
		if (0 != sw_dma_ctl(capture_prtd->dma_hdl, DMA_OP_START, NULL)) {
			printk("%s err, dma start err\n", __FUNCTION__);
			return -EINVAL;
		}
			break;
		case SNDRV_PCM_TRIGGER_SUSPEND:
		case SNDRV_PCM_TRIGGER_STOP:
		case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
	       // pr_info("CAPTUR dma stop:sun7i_i2sdma.c::func:%s(line:%d)\n",__func__,__LINE__);
	        //printk("[IIS] 0x01c22400+0x24 = %#x, line= %d\n", readl(0xf1c22400+0x24), __LINE__);
		
				/*
		* stop play dma transfer
		*/
		if (0 != sw_dma_ctl(capture_prtd->dma_hdl, DMA_OP_STOP, NULL)) {
			printk("%s err, dma stop err\n", __FUNCTION__);
			return -EINVAL;
		}
			break;
		default:
			ret = -EINVAL;
			break;
		}
		spin_unlock(&capture_prtd->lock);
	}
	return ret;
}

static int sun7i_pcm_open(struct snd_pcm_substream *substream)
{
	struct sun7i_playback_runtime_data *play_prtd = NULL;
	struct sun7i_capture_runtime_data *capture_prtd = NULL;
	struct snd_pcm_runtime *play_runtime = NULL;
	struct snd_pcm_runtime *capture_runtime = NULL;
	/*pr_info("CAPTUR:sun7i_i2sdma.c::func:%s(line:%d)\n",__func__,__LINE__);*/
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		play_runtime = substream->runtime;

		snd_pcm_hw_constraint_integer(play_runtime, SNDRV_PCM_HW_PARAM_PERIODS);
		snd_soc_set_runtime_hwparams(substream, &sun7i_pcm_play_hardware);
		play_prtd = kzalloc(sizeof(struct sun7i_playback_runtime_data), GFP_KERNEL);
	
		if (play_prtd == NULL) {
			return -ENOMEM;
		}

		spin_lock_init(&play_prtd->lock);

		play_runtime->private_data = play_prtd;
	} else {
		capture_runtime = substream->runtime;

		snd_pcm_hw_constraint_integer(capture_runtime, SNDRV_PCM_HW_PARAM_PERIODS);
		snd_soc_set_runtime_hwparams(substream, &sun7i_pcm_capture_hardware);

		capture_prtd = kzalloc(sizeof(struct sun7i_capture_runtime_data), GFP_KERNEL);
		if (capture_prtd == NULL)
			return -ENOMEM;

		spin_lock_init(&capture_prtd->lock);

		capture_runtime->private_data = capture_prtd;
	}

	return 0;
}

static int sun7i_pcm_close(struct snd_pcm_substream *substream)
{
	struct sun7i_playback_runtime_data *play_prtd = NULL;
	struct sun7i_capture_runtime_data *capture_prtd = NULL;
	struct snd_pcm_runtime *play_runtime = NULL;
	struct snd_pcm_runtime *capture_runtime = NULL;
	/*pr_info("CAPTUR:sun7i_i2sdma.c::func:%s(line:%d)\n",__func__,__LINE__);*/
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		play_runtime = substream->runtime;
		play_prtd = play_runtime->private_data;

		kfree(play_prtd);
	} else {
		capture_runtime = substream->runtime;
		capture_prtd = capture_runtime->private_data;

		kfree(capture_prtd);
	}

	return 0;
}

static int sun7i_pcm_mmap(struct snd_pcm_substream *substream,
	struct vm_area_struct *vma)
{
	struct snd_pcm_runtime *play_runtime = NULL;
	struct snd_pcm_runtime *capture_runtime = NULL;

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		play_runtime = substream->runtime;

		return dma_mmap_writecombine(substream->pcm->card->dev, vma,
					     play_runtime->dma_area,
					     play_runtime->dma_addr,
					     play_runtime->dma_bytes);
	} else {
		capture_runtime = substream->runtime;

		return dma_mmap_writecombine(substream->pcm->card->dev, vma,
					     play_runtime->dma_area,
					     play_runtime->dma_addr,
					     play_runtime->dma_bytes);
	}

}

static struct snd_pcm_ops sun7i_pcm_ops = {
	.open			= sun7i_pcm_open,
	.close			= sun7i_pcm_close,
	.ioctl			= snd_pcm_lib_ioctl,
	.hw_params		= sun7i_pcm_hw_params,
	.hw_free		= sun7i_pcm_hw_free,
	.prepare		= sun7i_pcm_prepare,
	.trigger		= sun7i_pcm_trigger,
	.pointer		= sun7i_pcm_pointer,
	.mmap			= sun7i_pcm_mmap,
};

static int sun7i_pcm_preallocate_dma_buffer(struct snd_pcm *pcm, int stream)
{
	struct snd_pcm_substream *substream = pcm->streams[stream].substream;
	struct snd_dma_buffer *buf = &substream->dma_buffer;
	size_t size = 0;

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		size = sun7i_pcm_play_hardware.buffer_bytes_max;
	} else {
		size = sun7i_pcm_capture_hardware.buffer_bytes_max;
	}

	buf->dev.type = SNDRV_DMA_TYPE_DEV;
	buf->dev.dev = pcm->card->dev;
	buf->private_data = NULL;
	buf->area = dma_alloc_writecombine(pcm->card->dev, size,
					   &buf->addr, GFP_KERNEL);
	if (!buf->area)
		return -ENOMEM;
	buf->bytes = size;
	return 0;
}

static void sun7i_pcm_free_dma_buffers(struct snd_pcm *pcm)
{
	struct snd_pcm_substream *substream;
	struct snd_dma_buffer *buf;
	int stream;
	
	for (stream = 0; stream < 2; stream++) {
		substream = pcm->streams[stream].substream;
		if (!substream)
			continue;

		buf = &substream->dma_buffer;
		if (!buf->area)
			continue;

		dma_free_writecombine(pcm->card->dev, buf->bytes,
				      buf->area, buf->addr);
		buf->area = NULL;
	}
}

static u64 sun7i_pcm_mask = DMA_BIT_MASK(32);

//static int sun7i_pcm_new(struct snd_card *card,
//			   struct snd_soc_dai *dai, struct snd_pcm *pcm)
static int sun7i_pcm_new(struct snd_soc_pcm_runtime *rtd)
{
	struct snd_card *card = rtd->card->snd_card;
	struct snd_pcm *pcm = rtd->pcm;
	
	int ret = 0;	
	if (!card->dev->dma_mask)
		card->dev->dma_mask = &sun7i_pcm_mask;
	if (!card->dev->coherent_dma_mask)
		card->dev->coherent_dma_mask = 0xffffffff;

	
	if (pcm->streams[SNDRV_PCM_STREAM_PLAYBACK].substream) {		
		ret = sun7i_pcm_preallocate_dma_buffer(pcm,
			SNDRV_PCM_STREAM_PLAYBACK);
		if (ret)
			goto out;
	}

	if (pcm->streams[SNDRV_PCM_STREAM_CAPTURE].substream) {
		ret = sun7i_pcm_preallocate_dma_buffer(pcm,
			SNDRV_PCM_STREAM_CAPTURE);
		if (ret)
			goto out;
	}
 out:
	return ret;
}

static struct snd_soc_platform_driver sun7i_soc_platform = {
	.ops		= &sun7i_pcm_ops,
	.pcm_new	= sun7i_pcm_new,
	.pcm_free	= sun7i_pcm_free_dma_buffers,
};

static int __devinit sun7i_hdcp_pcm_probe(struct platform_device *pdev)
{
	return snd_soc_register_platform(&pdev->dev, &sun7i_soc_platform);
}

static int __devexit sun7i_hdcp_pcm_remove(struct platform_device *pdev)
{
	snd_soc_unregister_platform(&pdev->dev);
	return 0;
}

/*data relating*/
static struct platform_device sun7i_hdcp_pcm_device = {
	.name = "sun7i-hdcp-pcm-audio",
};

/*method relating*/
static struct platform_driver sun7i_hdcp_pcm_driver = {
	.probe = sun7i_hdcp_pcm_probe,
	.remove = __devexit_p(sun7i_hdcp_pcm_remove),
	.driver = {
		.name = "sun7i-hdcp-pcm-audio",
		.owner = THIS_MODULE,
	},
};

static int __init sun7i_soc_platform_hdcp_init(void)
{
	int err = 0;	
	if((err = platform_device_register(&sun7i_hdcp_pcm_device)) < 0)
		return err;

	if ((err = platform_driver_register(&sun7i_hdcp_pcm_driver)) < 0)
		return err;
	return 0;	
}
module_init(sun7i_soc_platform_hdcp_init);

static void __exit sun7i_soc_platform_hdcp_exit(void)
{
	return platform_driver_unregister(&sun7i_hdcp_pcm_driver);
}
module_exit(sun7i_soc_platform_hdcp_exit);

MODULE_AUTHOR("All winner");
MODULE_DESCRIPTION("SUN7I PCM DMA module");
MODULE_LICENSE("GPL");

