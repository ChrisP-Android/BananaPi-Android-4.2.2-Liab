/*
 * sound\soc\sun7i\spdif\sun7i_spdma.c
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

#include "sun7i_spdif.h"
#include "sun7i_spdma.h"

static volatile unsigned int dmasrc = 0;
static volatile unsigned int dmadst = 0;

static const struct snd_pcm_hardware sun7i_pcm_hardware = {
	.info			= SNDRV_PCM_INFO_INTERLEAVED | SNDRV_PCM_INFO_BLOCK_TRANSFER |
				      SNDRV_PCM_INFO_MMAP | SNDRV_PCM_INFO_MMAP_VALID |
				      SNDRV_PCM_INFO_PAUSE | SNDRV_PCM_INFO_RESUME,
	.formats		= SNDRV_PCM_FMTBIT_S16_LE,
	.rates			= SNDRV_PCM_RATE_8000_192000 | SNDRV_PCM_RATE_KNOT,
	.rate_min		= 8000,
	.rate_max		= 192000,
	.channels_min		= 1,
	.channels_max		= 4,
	.buffer_bytes_max	= 128*1024,  //1024*1024  /* value must be (2^n)Kbyte size */
	.period_bytes_min	= 1024*4,//1024*4,
	.period_bytes_max	= 1024*32,//1024*128,
	.periods_min		= 1,//8,
	.periods_max		= 8,//8,
	.fifo_size		= 32,//32,
};

struct sun7i_runtime_data {
	spinlock_t lock;
	int state;
	unsigned int dma_loaded;
	unsigned int dma_limit;
	unsigned int dma_period;
	dma_addr_t 	dma_start;
	dma_addr_t 	dma_pos;
	dma_addr_t 	dma_end;
	dma_hdl_t	dma_hdl;
	bool		play_dma_flag;
	dma_cb_t 	play_done_cb;
	struct sun7i_dma_params *params;
};

static void sun7i_pcm_enqueue(struct snd_pcm_substream *substream)
{
	struct sun7i_runtime_data *prtd = substream->runtime->private_data;
	dma_addr_t pos = prtd->dma_pos;
	unsigned int limit;
	int ret;
	unsigned long len = prtd->dma_period;
  	limit = prtd->dma_limit;
  	while (prtd->dma_loaded < limit) {
		if ((pos + len) > prtd->dma_end) {
			len  = prtd->dma_end - pos;
		}
		ret = sw_dma_enqueue(prtd->dma_hdl, pos, prtd->params->dma_addr, len);
		if (ret == 0) {
			prtd->dma_loaded++;
			pos += prtd->dma_period;
			if(pos >= prtd->dma_end)
				pos = prtd->dma_start;
		} else {
			break;
		}
		}
	prtd->dma_pos = pos;
}

static void sun7i_audio_buffdone(dma_hdl_t dma_hdl, void *parg)
{
	struct sun7i_runtime_data *prtd;
	struct snd_pcm_substream *substream = parg;
	prtd = substream->runtime->private_data;
	if (substream) {
		snd_pcm_period_elapsed(substream);
	}	
	spin_lock(&prtd->lock);
	{
		prtd->dma_loaded--;
		sun7i_pcm_enqueue(substream);
	}
	spin_unlock(&prtd->lock);
}

static void sun7i_audio_conti_halfdone(dma_hdl_t dma_hdl, void *parg)
{
	struct sun7i_runtime_data *prtd;
	struct snd_pcm_substream *substream = parg;
	prtd = substream->runtime->private_data;
	if (substream) {
		snd_pcm_period_elapsed(substream);
	}
}

static int sun7i_pcm_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct sun7i_runtime_data *prtd = runtime->private_data;
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	unsigned long totbytes = params_buffer_bytes(params);
	struct sun7i_dma_params *dma = 
					snd_soc_dai_get_dma_data(rtd->cpu_dai, substream);

	if (!dma)
		return 0;
		
	if (prtd->params == NULL) {
		prtd->params = dma;
		/*
		 * requeset audio dma handle(we don't care about the channel!)
		 */
		prtd->dma_hdl = sw_dma_request(prtd->params->name, CHAN_NORAML);
		if (NULL == prtd->dma_hdl) {
			printk(KERN_ERR "failed to request spdif dma handle\n");
			return -EINVAL;
		}
	}
	/*
	* set callback
	*/
	memset(&prtd->play_done_cb, 0, sizeof(prtd->play_done_cb));
	if (params_channels(params) == 4) {
		prtd->play_done_cb.func = sun7i_audio_conti_halfdone;
		prtd->play_done_cb.parg = substream;
		printk("%s, line:%d\n", __func__, __LINE__);
		/*use the full buffer callback, maybe we should use the half buffer callback?*/
		if (0 != sw_dma_ctl(prtd->dma_hdl, DMA_OP_SET_FD_CB, (void *)&(prtd->play_done_cb))) {
			printk(KERN_ERR "failed to set dma buffer done!!!\n");
			sw_dma_release(prtd->dma_hdl);
			return -EINVAL;
		}
		if (0 != sw_dma_ctl(prtd->dma_hdl, DMA_OP_SET_HD_CB, (void *)&(prtd->play_done_cb))) {
			printk(KERN_ERR "failed to set dma buffer done!!!\n");
			sw_dma_release(prtd->dma_hdl);
			return -EINVAL;
		}
	} else {
		prtd->play_done_cb.func = sun7i_audio_buffdone;
		prtd->play_done_cb.parg = substream;
printk("%s, line:%d\n", __func__, __LINE__);
		/*use the full buffer callback, maybe we should use the half buffer callback?*/
		if (0 != sw_dma_ctl(prtd->dma_hdl, DMA_OP_SET_FD_CB, (void *)&(prtd->play_done_cb))) {
			printk(KERN_ERR "failed to set dma buffer done!!!\n");
			sw_dma_release(prtd->dma_hdl);
			return -EINVAL;
		}
	}

	snd_pcm_set_runtime_buffer(substream, &substream->dma_buffer);

	runtime->dma_bytes = totbytes;

	spin_lock_irq(&prtd->lock);
	prtd->dma_loaded = 0;
	if (params_channels(params) == 4) {
		prtd->dma_limit = 1;
	} else {
		prtd->dma_limit = 4;//runtime->hw.periods_min;
	}
	prtd->dma_period = params_period_bytes(params);
	prtd->dma_start = runtime->dma_addr;
	prtd->dma_pos = prtd->dma_start;
	prtd->dma_end = prtd->dma_start + totbytes;
	spin_unlock_irq(&prtd->lock);
	return 0;
}

static int sun7i_pcm_hw_free(struct snd_pcm_substream *substream)
{
	struct sun7i_runtime_data *prtd = substream->runtime->private_data;
	
	snd_pcm_set_runtime_buffer(substream, NULL);
  
	if (prtd->params) {
		/*
		 * stop play dma transfer
		 */
		if (0 != sw_dma_ctl(prtd->dma_hdl, DMA_OP_STOP, NULL)) {
			return -EINVAL;
		}
		/*
		*	release play dma handle
		*/
		if (0 != sw_dma_release(prtd->dma_hdl)) {
			return -EINVAL;
		}
		prtd->dma_hdl = (dma_hdl_t)NULL;
		prtd->params = NULL;
	}
	return 0;
}

static int sun7i_pcm_prepare(struct snd_pcm_substream *substream)
{
	struct sun7i_runtime_data *prtd = substream->runtime->private_data;
	dma_config_t spdif_dma_conf;
	int ret = 0;

	if (!prtd->params)
		return 0;
		
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK){
		//spdif_dma_conf.drqsrc_type  = DRQ_TYPE_SDRAM;
		//spdif_dma_conf.drqdst_type  = DRQ_TYPE_SPDIF;
		/* config para */
		memset(&spdif_dma_conf, 0, sizeof(spdif_dma_conf));
		spdif_dma_conf.xfer_type.src_data_width = DATA_WIDTH_16BIT;
		spdif_dma_conf.xfer_type.src_bst_len = DATA_BRST_4;
		spdif_dma_conf.xfer_type.dst_data_width = DATA_WIDTH_16BIT;
		spdif_dma_conf.xfer_type.dst_bst_len = DATA_BRST_4;
		spdif_dma_conf.address_type.src_addr_mode = NDMA_ADDR_INCREMENT;
		spdif_dma_conf.address_type.dst_addr_mode = NDMA_ADDR_NOCHANGE;
		if (substream->runtime->channels== 4) {
			printk("%s, line:%d\n", __func__, __LINE__);
			spdif_dma_conf.bconti_mode = true;
			spdif_dma_conf.irq_spt = CHAN_IRQ_FD|CHAN_IRQ_HD;
			strcpy(substream->pcm->card->id, "sndspdifraw");
			printk("%s, line:%d, substream->pcm->card->id:%s\n", __func__, __LINE__, substream->pcm->card->id);
		} else {
			printk("%s, line:%d\n", __func__, __LINE__);
		spdif_dma_conf.bconti_mode = false;
		spdif_dma_conf.irq_spt = CHAN_IRQ_FD;
		strcpy(substream->pcm->card->id, "sndspdif");
		}
		spdif_dma_conf.src_drq_type = N_SRC_SDRAM;
		spdif_dma_conf.dst_drq_type = N_DST_SPDIF_TX;//DRQDST_SPDIFTX;

		if (0 != sw_dma_config(prtd->dma_hdl, &spdif_dma_conf)) {
			printk("err:%s,line:%d\n", __func__, __LINE__);
			return -EINVAL;
		}
	} else {
		return -EINVAL;
	}
	/* flush the DMA channel */
	//sw_dma_ctrl(prtd->params->channel, SW_DMAOP_FLUSH);
	prtd->dma_loaded = 0;
	//prtd->dma_pos = prtd->dma_start;

	/* enqueue dma buffers */
	sun7i_pcm_enqueue(substream);

	return ret;	
}

static int sun7i_pcm_trigger(struct snd_pcm_substream *substream, int cmd)
{
	struct sun7i_runtime_data *prtd = substream->runtime->private_data;
	int ret ;
	spin_lock(&prtd->lock);

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:	
		printk("%s, line:%d\n", __func__, __LINE__);
	    /*
		* start dma transfer
		*/
		if (0 != sw_dma_ctl(prtd->dma_hdl, DMA_OP_START, NULL)) {
			printk("%s err, dma start err\n", __FUNCTION__);
			return -EINVAL;
		}
		printk("spdif:start\n");
		break;
		
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		printk("%s, line:%d\n", __func__, __LINE__);
		/*
		* stop play dma transfer
		*/
		if (0 != sw_dma_ctl(prtd->dma_hdl, DMA_OP_STOP, NULL)) {
			printk("%s err, dma stop err\n", __FUNCTION__);
			return -EINVAL;
		}
		strcpy(substream->pcm->card->id, "sndspdif");
		printk("%s, line:%d\n", __func__, __LINE__);
		break;

	default:
		ret = -EINVAL;
		break;
	}

	spin_unlock(&prtd->lock);
	return 0;
}

static snd_pcm_uframes_t sun7i_pcm_pointer(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct sun7i_runtime_data *prtd = runtime->private_data;
	unsigned long res = 0;
	snd_pcm_uframes_t offset = 0;
	
	spin_lock(&prtd->lock);
	sw_dma_getposition(prtd->dma_hdl, (dma_addr_t*)&dmasrc, (dma_addr_t*)&dmadst);

	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE){
		res = dmadst - prtd->dma_start;
	} else {
		offset = bytes_to_frames(runtime, dmasrc + prtd->dma_period - runtime->dma_addr);
	}
	spin_unlock(&prtd->lock);

	if(offset >= runtime->buffer_size)
		offset = 0;
		return offset;
}

static int sun7i_pcm_open(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct sun7i_runtime_data *prtd;

	snd_pcm_hw_constraint_integer(runtime, SNDRV_PCM_HW_PARAM_PERIODS);
	snd_soc_set_runtime_hwparams(substream, &sun7i_pcm_hardware);
	
	prtd = kzalloc(sizeof(struct sun7i_runtime_data), GFP_KERNEL);
	if (prtd == NULL)
		return -ENOMEM;
		
	spin_lock_init(&prtd->lock);

	runtime->private_data = prtd;
	return 0;
}

static int sun7i_pcm_close(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct sun7i_runtime_data *prtd = runtime->private_data;
	kfree(prtd);
	
	return 0;
}

static int sun7i_pcm_mmap(struct snd_pcm_substream *substream,
	struct vm_area_struct *vma)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	
	return dma_mmap_writecombine(substream->pcm->card->dev, vma,
				     runtime->dma_area,
				     runtime->dma_addr,
				     runtime->dma_bytes);
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
	size_t size = sun7i_pcm_hardware.buffer_bytes_max;

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
	.ops  		=   &sun7i_pcm_ops,
	.pcm_new	=	sun7i_pcm_new,
	.pcm_free	=	sun7i_pcm_free_dma_buffers,
};

static int __devinit sun7i_spdif_pcm_probe(struct platform_device *pdev)
{	
	return snd_soc_register_platform(&pdev->dev, &sun7i_soc_platform);
}

static int __devexit sun7i_spdif_pcm_remove(struct platform_device *pdev)
{
	snd_soc_unregister_platform(&pdev->dev);
	return 0;
}

/*data relating*/
static struct platform_device sun7i_spdif_pcm_device = {
	.name = "sun7i-spdif-pcm-audio",
};

/*method relating*/
static struct platform_driver sun7i_spdif_pcm_driver = {
	.probe = sun7i_spdif_pcm_probe,
	.remove = __devexit_p(sun7i_spdif_pcm_remove),
	.driver = {
		.name = "sun7i-spdif-pcm-audio",
		.owner = THIS_MODULE,
	},
};

static int __init sun7i_soc_platform_spdif_init(void)
{
	int err = 0;	
	if((err = platform_device_register(&sun7i_spdif_pcm_device)) < 0)
		return err;

	if ((err = platform_driver_register(&sun7i_spdif_pcm_driver)) < 0)
		return err;
	return 0;	
}
module_init(sun7i_soc_platform_spdif_init);

static void __exit sun7i_soc_platform_spdif_exit(void)
{
	return platform_driver_unregister(&sun7i_spdif_pcm_driver);
}
module_exit(sun7i_soc_platform_spdif_exit);

MODULE_AUTHOR("All winner");
MODULE_DESCRIPTION("SUN7I SPDIF DMA module");
MODULE_LICENSE("GPL");
