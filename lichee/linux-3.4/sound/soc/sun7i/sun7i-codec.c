/*
 **************************************************************************************************
 *   Driver for CODEC on M1 soundcard
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License.
 **************************************************************************************************
 */
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/errno.h>
#include <linux/ioctl.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <asm/io.h>
#include <mach/dma.h>
#ifdef CONFIG_PM
#include <linux/pm.h>
#endif
#include <asm/mach-types.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/control.h>
#include <sound/initval.h>
#include <linux/clk.h>
#include <mach/clock.h>
#include <linux/gpio.h>
#include <linux/timer.h>
#include "sun7i-codec.h"
#include <mach/sys_config.h>
#include <mach/system.h>
#include <sound/soc.h>

static int capture_used = 1;
static script_item_u item;
struct clk *codec_apbclk,*codec_pll2clk,*codec_moduleclk;
static volatile unsigned int capture_dmasrc = 0;
static volatile unsigned int capture_dmadst = 0;
static volatile unsigned int play_dmasrc 	= 0;
static volatile unsigned int play_dmadst 	= 0;
static bool codec_speakerout_enabled 		= false;
static bool codec_phoneout_enabled = false;
static bool codec_phonemic_enabled = false;
static bool codec_headsetmic_enabled = false;
static bool codec_dacphoneout_enabled = false;
static bool codec_earpiece_enabled = false;
static bool codec_phone_capture_enabled = false;
static int req_status;
static script_item_value_type_e  type;

typedef struct codec_board_info {
	struct device	*dev;
	struct resource	*codec_base_res;
	struct resource	*codec_base_req;

	spinlock_t	lock;
} codec_board_info_t;

static struct sun7i_pcm_dma_params sun7i_codec_pcm_stereo_play = {
	.name		= "CODEC PCM Stereo PLAY",
	.dma_addr	= CODEC_BASSADDRESS + SUN7I_DAC_TXDATA,
};

static struct sun7i_pcm_dma_params sun7i_codec_pcm_stereo_capture = {
	.name		= "CODEC PCM Stereo CAPTURE",
	.dma_addr	= CODEC_BASSADDRESS + SUN7I_ADC_RXDATA,
};

struct sun7i_playback_runtime_data {
	spinlock_t 		lock;
	int 			state;
	unsigned int 	dma_loaded;
	unsigned int 	dma_limit;
	unsigned int 	dma_period;
	dma_addr_t   	dma_start;
	dma_addr_t   	dma_pos;
	dma_addr_t	 	dma_end;
	dma_hdl_t		dma_hdl;
	bool			play_dma_flag;
	dma_cb_t 		play_done_cb;
	struct sun7i_pcm_dma_params	*params;
};

struct sun7i_capture_runtime_data {
	spinlock_t 		lock;
	int 			state;
	unsigned int 	dma_loaded;
	unsigned int 	dma_limit;
	unsigned int 	dma_period;
	dma_addr_t   	dma_start;
	dma_addr_t   	dma_pos;
	dma_addr_t	 	dma_end;
	dma_hdl_t		dma_hdl;
	bool			play_dma_flag;
	dma_cb_t 		play_done_cb;
	struct sun7i_pcm_dma_params	*params;
};

static struct snd_pcm_hardware sun7i_pcm_playback_hardware =
{
	.info			= (SNDRV_PCM_INFO_INTERLEAVED |
				   SNDRV_PCM_INFO_BLOCK_TRANSFER |
				   SNDRV_PCM_INFO_MMAP | SNDRV_PCM_INFO_MMAP_VALID |
				   SNDRV_PCM_INFO_PAUSE | SNDRV_PCM_INFO_RESUME),
	.formats		= SNDRV_PCM_FMTBIT_S16_LE,
	.rates			= (SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_16000 |SNDRV_PCM_RATE_11025 |\
				   SNDRV_PCM_RATE_22050| SNDRV_PCM_RATE_32000 |\
				   SNDRV_PCM_RATE_44100| SNDRV_PCM_RATE_48000 |SNDRV_PCM_RATE_96000 | SNDRV_PCM_RATE_192000 |\
				   SNDRV_PCM_RATE_KNOT),
	.rate_min		= 8000,
	.rate_max		= 192000,
	.channels_min		= 1,
	.channels_max		= 2,
	.buffer_bytes_max	= 128*1024,
	.period_bytes_min	= 1024*4,
	.period_bytes_max	= 1024*32,
	.periods_min		= 4,
	.periods_max		= 8,
	.fifo_size	     	= 32,
};

static struct snd_pcm_hardware sun7i_pcm_capture_hardware =
{
	.info			= (SNDRV_PCM_INFO_INTERLEAVED |
				   SNDRV_PCM_INFO_BLOCK_TRANSFER |
				   SNDRV_PCM_INFO_MMAP | SNDRV_PCM_INFO_MMAP_VALID |
				   SNDRV_PCM_INFO_PAUSE | SNDRV_PCM_INFO_RESUME),
	.formats		= SNDRV_PCM_FMTBIT_S16_LE,
	.rates			= (SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_16000 |SNDRV_PCM_RATE_11025 |\
				   SNDRV_PCM_RATE_22050| SNDRV_PCM_RATE_32000 |\
				   SNDRV_PCM_RATE_44100| SNDRV_PCM_RATE_48000 | SNDRV_PCM_RATE_96000 |SNDRV_PCM_RATE_192000 |\
				   SNDRV_PCM_RATE_KNOT),
	.rate_min		= 8000,
	.rate_max		= 192000,
	.channels_min		= 1,
	.channels_max		= 2,
	.buffer_bytes_max	= 128*1024,
	.period_bytes_min	= 1024*4,
	.period_bytes_max	= 1024*32,
	.periods_min		= 4,
	.periods_max		= 8,
	.fifo_size	     	= 32,
};

struct sun7i_codec {
	long samplerate;
	struct snd_card *card;
	struct snd_pcm 	*pcm;		
};

static void codec_resume_events(struct work_struct *work);
struct workqueue_struct *resume_work_queue;
static DECLARE_WORK(codec_resume_work, codec_resume_events);

static unsigned int rates[] = {
	8000,11025,12000,16000,
	22050,24000,24000,32000,
	44100,48000,96000,192000
};

static struct snd_pcm_hw_constraint_list hw_constraints_rates = {
	.count	= ARRAY_SIZE(rates),
	.list	= rates,
	.mask	= 0,
};

int codec_wrreg_bits(unsigned short reg, unsigned int	mask,	unsigned int value)
{
	unsigned int old, new;

	old	=	codec_rdreg(reg);
	new	=	(old & ~mask) | value;
	codec_wrreg(reg,new);

	return 0;
}

int snd_codec_info_volsw(struct snd_kcontrol *kcontrol,
		struct	snd_ctl_elem_info	*uinfo)
{
	struct	codec_mixer_control *mc	= (struct codec_mixer_control*)kcontrol->private_value;
	int	max	=	mc->max;
	unsigned int shift  = mc->shift;
	unsigned int rshift = mc->rshift;
	
	if (max	== 1)
		uinfo->type = SNDRV_CTL_ELEM_TYPE_BOOLEAN;
	else
		uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	
	uinfo->count = shift ==	rshift	?	1:	2;
	uinfo->value.integer.min = 0;
	uinfo->value.integer.max = max;
	return	0;
}

int snd_codec_get_volsw(struct snd_kcontrol	*kcontrol,
		struct	snd_ctl_elem_value	*ucontrol)
{
	struct codec_mixer_control *mc= (struct codec_mixer_control*)kcontrol->private_value;
	unsigned int shift = mc->shift;
	unsigned int rshift = mc->rshift;
	int	max = mc->max;
	/*fls(7) = 3,fls(1)=1,fls(0)=0,fls(15)=4,fls(3)=2,fls(23)=5*/
	unsigned int mask = (1 << fls(max)) -1;
	unsigned int invert = mc->invert;
	unsigned int reg = mc->reg;
	
	ucontrol->value.integer.value[0] =	
		(codec_rdreg(reg)>>	shift) & mask;
	if(shift != rshift)
		ucontrol->value.integer.value[1] =
			(codec_rdreg(reg) >> rshift) & mask;

	if (invert) {
		ucontrol->value.integer.value[0] =
			max - ucontrol->value.integer.value[0];
		if (shift != rshift)
			ucontrol->value.integer.value[1] =
				max - ucontrol->value.integer.value[1];
		}
		
		return 0;
}

int snd_codec_put_volsw(struct	snd_kcontrol	*kcontrol,
	struct	snd_ctl_elem_value	*ucontrol)
{
	struct codec_mixer_control *mc= (struct codec_mixer_control*)kcontrol->private_value;
	unsigned int reg = mc->reg;
	unsigned int shift = mc->shift;
	unsigned int rshift = mc->rshift;
	int max = mc->max;
	unsigned int mask = (1<<fls(max))-1;
	unsigned int invert = mc->invert;
	unsigned int	val, val2, val_mask;
	
	val = (ucontrol->value.integer.value[0] & mask);
	if(invert)
		val = max - val;
	val <<= shift;
	val_mask = mask << shift;
	if (shift != rshift) {
		val2	= (ucontrol->value.integer.value[1] & mask);
		if (invert)
			val2	=	max	- val2;
		val_mask |= mask <<rshift;
		val |= val2 <<rshift;
	}
	
	return codec_wrreg_bits(reg,val_mask,val);
}

int codec_wr_control(u32 reg, u32 mask, u32 shift, u32 val)
{
	u32 reg_val;
	reg_val = val << shift;
	mask = mask << shift;
	codec_wrreg_bits(reg, mask, reg_val);
	return 0;
}

int codec_rd_control(u32 reg, u32 bit, u32 *val)
{
	return 0;
}

static  int codec_init(void)
{
	codec_wr_control(SUN7I_DAC_FIFOC , 0x1, 28, 0x1);
	//pa mute
	codec_wr_control(SUN7I_DAC_ACTL, 0x1, PA_MUTE, 0x0);
	//enable PA
	codec_wr_control(SUN7I_ADC_ACTL, 0x1, PA_ENABLE, 0x1);
	codec_wr_control(SUN7I_DAC_FIFOC, 0x3, DRA_LEVEL,0x3);
	/*dither*/
	codec_wr_control(SUN7I_ADC_ACTL, 0x1, 8, 0x0);

	codec_wr_control(SUN7I_DAC_ACTL, 0x6, VOLUME, 0x3b);

	return 0;
}

static int codec_play_open(struct snd_pcm_substream *substream)
{
	//pa mute
	codec_wr_control(SUN7I_DAC_ACTL, 0x1, PA_MUTE, 0x0);
	codec_wr_control(SUN7I_DAC_DPC ,  0x1, DAC_EN, 0x1);
	codec_wr_control(SUN7I_DAC_FIFOC ,0x1, DAC_FIFO_FLUSH, 0x1);
	//set TX FIFO send drq level
	codec_wr_control(SUN7I_DAC_FIFOC ,0x4, TX_TRI_LEVEL, 0xf);
	if (substream->runtime->rate > 32000) {
		codec_wr_control(SUN7I_DAC_FIFOC ,  0x1,28, 0x0);
	} else {
		codec_wr_control(SUN7I_DAC_FIFOC ,  0x1,28, 0x1);
	}
	//set TX FIFO MODE
	codec_wr_control(SUN7I_DAC_FIFOC ,0x1, TX_FIFO_MODE, 0x1);
	//send last sample when dac fifo under run
	codec_wr_control(SUN7I_DAC_FIFOC ,0x1, LAST_SE, 0x0);
	//enable dac analog
	codec_wr_control(SUN7I_DAC_ACTL, 0x1, 	DACAEN_L, 0x1);
	codec_wr_control(SUN7I_DAC_ACTL, 0x1, 	DACAEN_R, 0x1);
	//enable dac to pa
	codec_wr_control(SUN7I_DAC_ACTL, 0x1, 	DACPAS, 0x1);
	return 0;
}

static int codec_capture_open(void)
{
	 //enable mic1 pa
	 codec_wr_control(SUN7I_ADC_ACTL, 0x1, MIC1_EN, 0x1);
	 //mic1 gain 32dB
	 codec_wr_control(SUN7I_MIC_CRT, 0x3,mic1_preg1,0x1);
	  //enable VMIC
	 codec_wr_control(SUN7I_ADC_ACTL, 0x1, VMIC_EN, 0x1);
	 //boost up record effect
	 codec_wr_control(SUN7I_DAC_TUNE, 0x3,8,0x3);
	 //enable adc digital
	 codec_wr_control(SUN7I_ADC_FIFOC, 0x1,ADC_DIG_EN, 0x1);
	 //set RX FIFO mode
	 codec_wr_control(SUN7I_ADC_FIFOC, 0x1, RX_FIFO_MODE, 0x1);
	 //flush RX FIFO
	 codec_wr_control(SUN7I_ADC_FIFOC, 0x1, ADC_FIFO_FLUSH, 0x1);
	 //set RX FIFO rec drq level
	 codec_wr_control(SUN7I_ADC_FIFOC, 0xf, RX_TRI_LEVEL, 0x7);
	 //enable adc analog
	 codec_wr_control(SUN7I_ADC_ACTL, 0x1,  ADC_LF_EN, 0x1);
	 codec_wr_control(SUN7I_ADC_ACTL, 0x1,  ADC_RI_EN, 0x1);
	 return 0;
}

static int codec_play_start(void)
{
	//flush TX FIFO
	codec_wr_control(SUN7I_DAC_FIFOC ,0x1, DAC_FIFO_FLUSH, 0x1);
	//enable dac drq
	codec_wr_control(SUN7I_DAC_FIFOC ,0x1, DAC_DRQ, 0x1);
	return 0;
}

static int codec_play_stop(void)
{	
	//pa mute
	codec_wr_control(SUN7I_DAC_ACTL, 0x1, PA_MUTE, 0x0);
	mdelay(5);
	//disable dac drq
	codec_wr_control(SUN7I_DAC_FIFOC ,0x1, DAC_DRQ, 0x0);

	codec_wr_control(SUN7I_DAC_ACTL, 0x1, 	DACAEN_L, 0x0);
	codec_wr_control(SUN7I_DAC_ACTL, 0x1, 	DACAEN_R, 0x0);	

	codec_wr_control(SUN7I_DAC_DPC ,  0x1, DAC_EN, 0x0);

	return 0;
}

static int codec_capture_start(void)
{
	//enable adc drq
	codec_wr_control(SUN7I_ADC_FIFOC ,0x1, ADC_DRQ, 0x1);
	return 0;
}

static int codec_capture_stop(void)
{
	//disable adc drq
	codec_wr_control(SUN7I_ADC_FIFOC ,0x1, ADC_DRQ, 0x0);
	//disable mic1 pa
	codec_wr_control(SUN7I_ADC_ACTL, 0x1, MIC1_EN, 0x0);

	//disable VMIC
	codec_wr_control(SUN7I_ADC_ACTL, 0x1, VMIC_EN, 0x0);

	codec_wr_control(SUN7I_DAC_TUNE, 0x3,8,0x0);

	//disable adc digital
	codec_wr_control(SUN7I_ADC_FIFOC, 0x1,ADC_DIG_EN, 0x0);
	//set RX FIFO mode
	codec_wr_control(SUN7I_ADC_FIFOC, 0x1, RX_FIFO_MODE, 0x0);
	//flush RX FIFO
	codec_wr_control(SUN7I_ADC_FIFOC, 0x1, ADC_FIFO_FLUSH, 0x0);
	//disable adc1 analog
	 codec_wr_control(SUN7I_ADC_ACTL, 0x1,  ADC_LF_EN, 0x1);
	 codec_wr_control(SUN7I_ADC_ACTL, 0x1,  ADC_RI_EN, 0x1);
	return 0;
}

static int codec_dev_free(struct snd_device *device)
{
	return 0;
};

static int codec_get_speakerout(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{	
	ucontrol->value.integer.value[0] = codec_speakerout_enabled;
	return 0;
}

static int codec_set_speakerout(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	codec_speakerout_enabled = ucontrol->value.integer.value[0];
	if (codec_speakerout_enabled) {
		item.gpio.data = 1;
		/*config gpio info of audio_pa_ctrl open*/
		if (0 != sw_gpio_setall_range(&item.gpio, 1)) {
			printk("sw_gpio_setall_range failed\n");
		}
		mdelay(62);
	}
	return 0;
}

/*
*	codec_earpiece_enabled == 1,mixpas pa-en and pa_unmute you can play the line from earpiece.
*	codec_earpiece_enabled == 0,	
*/
static int codec_set_earpieceout(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	int ret = 0;
	codec_earpiece_enabled = ucontrol->value.integer.value[0];

	if (codec_earpiece_enabled) {
		/*line-in mode*/
		codec_wr_control(SUN7I_ADC_ACTL, 0x1, line_in_mode, 0x0);
		/*EN LINE-IN right channel and DAC left channel TO MIXER*/
		codec_wr_control(SUN7I_DAC_ACTL, 0x1, LINEIN_MIXER_L, 0x1);
		codec_wr_control(SUN7I_DAC_ACTL, 0x1, LINEIN_MIXER_R, 0x1);
		/*EN MIXER*/
		codec_wr_control(SUN7I_DAC_ACTL, 0x1, MIXEN, 0x1);
		/*EN  MIXPAS*/
		codec_wr_control(SUN7I_DAC_ACTL, 0x1, MIXPAS, 0x1);
		/*EN PA*/
		codec_wr_control(SUN7I_ADC_ACTL, 0x1, PA_ENABLE, 0x1);
		/*PA UNMUTE*/
		codec_wr_control(SUN7I_DAC_ACTL, 0x1, PA_MUTE, 0x1);
		codec_speakerout_enabled=0;
	} else {
		/*disable DAC right channel and DAC left channel*/
		codec_wr_control(SUN7I_DAC_ACTL, 0x1, LINEIN_MIXER_L, 0x0);
		codec_wr_control(SUN7I_DAC_ACTL, 0x1, LINEIN_MIXER_R, 0x0);
		/*disable MIXER*/
		codec_wr_control(SUN7I_DAC_ACTL, 0x1, MIXEN, 0x0);
		/*disable  MIXPAS*/
		codec_wr_control(SUN7I_DAC_ACTL, 0x1, MIXPAS, 0x0);
		/*disable PA*/
		codec_wr_control(SUN7I_ADC_ACTL, 0x1, PA_ENABLE, 0x0);
		/*PA MUTE*/
		codec_wr_control(SUN7I_DAC_ACTL, 0x1, PA_MUTE, 0x0);
	}

	return ret;
}

static int codec_get_earpieceout(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = codec_earpiece_enabled;
	return 0;
}

/*
*	codec_adcphonein_enabled == 1, the adc phone in is open. you can record the phonein from adc.
*	codec_adcphonein_enabled == 0,	the adc phone in is close.
*/
static int codec_set_dacphoneout(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	int ret = 0;
	codec_dacphoneout_enabled = ucontrol->value.integer.value[0];
	if (codec_dacphoneout_enabled) {
		/*EN DAC*/
		codec_wr_control(SUN7I_DAC_DPC, 0x1, DAC_EN, 0x1);
		/*EN DAC right channel and DAC left channel*/
		codec_wr_control(SUN7I_DAC_ACTL, 0x1, DACAEN_L, 0x1);
		codec_wr_control(SUN7I_DAC_ACTL, 0x1, DACAEN_R, 0x1);
		/*EN DAC right channel and DAC left channel TO MIXER*/
		codec_wr_control(SUN7I_DAC_ACTL, 0x1, LDAC_MIXER, 0x1);
		codec_wr_control(SUN7I_DAC_ACTL, 0x1, RDAC_MIXER, 0x1);
		/*EN MIXER*/
		codec_wr_control(SUN7I_DAC_ACTL, 0x1, MIXEN, 0x1);
	} else {
		/*disable DAC*/
		codec_wr_control(SUN7I_DAC_DPC, 0x1, DAC_EN, 0x0);
		/*disable DAC right channel and DAC left channel*/
		codec_wr_control(SUN7I_DAC_ACTL, 0x1, DACAEN_L, 0x0);
		codec_wr_control(SUN7I_DAC_ACTL, 0x1, DACAEN_R, 0x0);
		/*disable DAC right channel and DAC left channel TO MIXER*/
		codec_wr_control(SUN7I_DAC_ACTL, 0x1, LDAC_MIXER, 0x0);
		codec_wr_control(SUN7I_DAC_ACTL, 0x1, RDAC_MIXER, 0x0);
		/*disable MIXER*/
		codec_wr_control(SUN7I_DAC_ACTL, 0x1, MIXEN, 0x0);
	}

	return ret;
}

static int codec_get_dacphoneout(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = codec_dacphoneout_enabled;
	return 0;
}

/*
*	codec_phonemic_enabled == 1, open mic1.
*	codec_phonemic_enabled == 0, close mic1.
*/
static int codec_set_phonemic(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	codec_phonemic_enabled = ucontrol->value.integer.value[0];
	if (codec_phonemic_enabled) {
		/*close mic2 pa*/
		codec_wr_control(SUN7I_ADC_ACTL, 0x1, MIC2_EN, 0x0);
		/*enable mic1 pa*/
	 	codec_wr_control(SUN7I_ADC_ACTL, 0x1, MIC1_EN, 0x1);
		/*enable  VMIC bias*/
		codec_wr_control(SUN7I_ADC_ACTL, 0x1, VMIC_EN, 0x1);
		
		/*enable Right MIC1 Boost stage*/
		codec_wr_control(SUN7I_MIC_CRT, 0x1, RIGRT_PHONEOUT, 0x1);
		/*enable Left MIC1 Boost stage*/
		codec_wr_control(SUN7I_MIC_CRT, 0x1, LEFT_PHONEOUT, 0x1);
		
		/*set the headset mic flag false*/
		codec_headsetmic_enabled = 0;
	} else {
		/*disable mic pa*/
		codec_wr_control(SUN7I_ADC_ACTL, 0x1, MIC1_EN, 0x0);
		/*disable Master microphone bias*/
		codec_wr_control(SUN7I_ADC_ACTL, 0x1, VMIC_EN, 0x0);
		
		/*disable Right MIC1 Boost stage*/
		codec_wr_control(SUN7I_MIC_CRT, 0x1, RIGRT_PHONEOUT, 0x0);
		/*disable Left MIC1 Boost stage*/
		codec_wr_control(SUN7I_MIC_CRT, 0x1, LEFT_PHONEOUT, 0x0);
	}

	return 0;
}

static int codec_get_phonemic(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = codec_phonemic_enabled;
	return 0;
}

/*
*	codec_phoneout_enabled == 1, the phone out is open. receiver can hear the voice which you say.
*	codec_phoneout_enabled == 0,	the phone out is close.
*/
static int codec_set_phoneout(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	codec_phoneout_enabled = ucontrol->value.integer.value[0];
	if (codec_phoneout_enabled) {
		codec_wr_control(SUN7I_MIC_CRT, 0x1, PHONEOUT_EN, 0x1);
	} else {
		codec_wr_control(SUN7I_MIC_CRT, 0x1, PHONEOUT_EN, 0x0);
	}

	return 0;
}

static int codec_get_phoneout(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = codec_phoneout_enabled;
	return 0;
}

static int codec_set_headsetmic(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	codec_headsetmic_enabled = ucontrol->value.integer.value[0];
	if (codec_headsetmic_enabled) {
		/*close mic1 pa*/
		codec_wr_control(SUN7I_ADC_ACTL, 0x1, MIC1_EN, 0x0);
		/*enable mic2 pa*/
	 	codec_wr_control(SUN7I_ADC_ACTL, 0x1, MIC2_EN, 0x1);
		/*enable  VMIC bias*/
		codec_wr_control(SUN7I_ADC_ACTL, 0x1, VMIC_EN, 0x1);
		
		/*enable Right MIC2 Boost stage*/
		codec_wr_control(SUN7I_MIC_CRT, 0x1, RIGRT_PHONEOUT, 0x1);
		/*enable Left MIC2 Boost stage*/
		codec_wr_control(SUN7I_MIC_CRT, 0x1, LEFT_PHONEOUT, 0x1);
		
		/*set the headset mic flag false*/
		codec_phonemic_enabled = 0;
	} else {
		/*disable mic pa*/
		codec_wr_control(SUN7I_ADC_ACTL, 0x1, MIC2_EN, 0x0);
		/*disable Master microphone bias*/
		codec_wr_control(SUN7I_ADC_ACTL, 0x1, VMIC_EN, 0x0);
		
		/*disable Right MIC2 Boost stage*/
		codec_wr_control(SUN7I_MIC_CRT, 0x1, RIGRT_PHONEOUT, 0x0);
		/*disable Left MIC2 Boost stage*/
		codec_wr_control(SUN7I_MIC_CRT, 0x1, LEFT_PHONEOUT, 0x0);
	}

	return 0;
}

static int codec_get_headsetmic(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = codec_phonemic_enabled;
	return 0;
}

/*
*	codec_phone_capture_enabled == 1,select line-in+mic1
*	codec_phone_capture_enabled == 0,close line-in
*/
static int codec_set_phone_capture(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	int ret = 0;
	codec_phone_capture_enabled = ucontrol->value.integer.value[0];

	if (codec_phone_capture_enabled) {
		/*line-in mode*/
		codec_wr_control(SUN7I_ADC_ACTL, 0x1, line_in_mode, 0x0);
		/*the source of adc select line-in*/
		codec_wr_control(SUN7I_ADC_ACTL, 0x1, ADC_SELECT, 0x7);
		/*adc left and right en*/
		codec_wr_control(SUN7I_ADC_ACTL, 0x1, ADC_LF_EN, 0x1);
		codec_wr_control(SUN7I_ADC_ACTL, 0x1, ADC_RI_EN, 0x1);
		/*adc  en*/
		codec_wr_control(SUN7I_ADC_FIFOC, 0x1, ADC_DIG_EN, 0x1);
		
	} else {
		/*adc left and right disable*/
		codec_wr_control(SUN7I_ADC_ACTL, 0x1, ADC_LF_EN, 0x0);
		codec_wr_control(SUN7I_ADC_ACTL, 0x1, ADC_RI_EN, 0x0);
		/*disable adc*/
		codec_wr_control(SUN7I_ADC_FIFOC, 0x1, ADC_DIG_EN, 0x0);
	}

	return ret;
}

static int codec_get_phone_capture(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = codec_phone_capture_enabled;
	return 0;
}
/*
* 	.info = snd_codec_info_volsw, .get = snd_codec_get_volsw,\.put = snd_codec_put_volsw, 
*/
static const struct snd_kcontrol_new codec_snd_controls[] = {
	/*SUN7I_DAC_ACTL = 0x10,PAVOL*/
	CODEC_SINGLE("Master Playback Volume", SUN7I_DAC_ACTL,0,0x3f,0),
	CODEC_SINGLE("MIC output volume",SUN7I_DAC_ACTL,20,7,0),
	/*	FM Input to output mixer Gain Control
	* 	From -4.5db to 6db,1.5db/step,default is 0db
	*	-4.5db:0x0,-3.0db:0x1,-1.5db:0x2,0db:0x3
	*	1.5db:0x4,3.0db:0x5,4.5db:0x6,6db:0x7
	*/
	CODEC_SINGLE("Fm output Volume",SUN7I_DAC_ACTL,23,7,0),
	/*	Line-in gain stage to output mixer Gain Control
	*	0:-1.5db,1:0db
	*/
	CODEC_SINGLE("Line output Volume",SUN7I_DAC_ACTL,26,1,0),
	CODEC_SINGLE("LINEIN APM Volume", SUN7I_MIC_CRT,13,0x7,0),
	/*ADC Input Gain Control, capture volume
	* 000:-4.5db,001:-3db,010:-1.5db,011:0db,100:1.5db,101:3db,110:4.5db,111:6db
	*/
	CODEC_SINGLE("Capture Volume",SUN7I_ADC_ACTL,20,7,0),
	/*
	*	MIC2 pre-amplifier Gain Control
	*	00:0db,01:35db,10:38db,11:41db
	*/
	CODEC_SINGLE("Mic2 gain Volume",SUN7I_MIC_CRT,26,7,0),
	/*
	*	MIC1 pre-amplifier Gain Control
	*	00:0db,01:35db,10:38db,11:41db
	*/
	CODEC_SINGLE("Mic1 gain Volume",SUN7I_MIC_CRT,29,3,0),
	/*
	*	extern PA
	*/
	SOC_SINGLE_BOOL_EXT("Audio speaker out", 0, codec_get_speakerout, codec_set_speakerout),
	/*Mobile phone down simulation channel interface*/	
	SOC_SINGLE_BOOL_EXT("Audio earpiece out", 0, codec_get_earpieceout, codec_set_earpieceout),		  
	/*Mobile phone uplink analog channel interface */	
	SOC_SINGLE_BOOL_EXT("Audio dac phoneout", 0, codec_get_dacphoneout, codec_set_dacphoneout),    		
	SOC_SINGLE_BOOL_EXT("Audio phone phonemic", 0, codec_get_phonemic, codec_set_phonemic),//mic1
	SOC_SINGLE_BOOL_EXT("Audio phone out", 0, codec_get_phoneout, codec_set_phoneout),	
	SOC_SINGLE_BOOL_EXT("Audio phone headsetmic", 0, codec_get_headsetmic, codec_set_headsetmic),
	/*for phone capture IO*/
	SOC_SINGLE_BOOL_EXT("Audio phone capture", 0, codec_get_phone_capture, codec_set_phone_capture), 
};

int __init snd_chip_codec_mixer_new(struct snd_card *card)
{
  	static struct snd_device_ops ops = {
  		.dev_free	=	codec_dev_free,
  	};
  	unsigned char *clnt = "codec";
	int idx, err;

	for (idx = 0; idx < ARRAY_SIZE(codec_snd_controls); idx++) {
		if ((err = snd_ctl_add(card, snd_ctl_new1(&codec_snd_controls[idx],clnt))) < 0) {
			return err;
		}
	}
	
	if ((err = snd_device_new(card, SNDRV_DEV_CODEC, clnt, &ops)) < 0) {
		return err;
	}
	
	strcpy(card->mixername, "codec Mixer");
	       
	return 0;
}

static void sun7i_pcm_enqueue(struct snd_pcm_substream *substream)
{	
	int play_ret = 0, capture_ret = 0;
	struct sun7i_playback_runtime_data *play_prtd = NULL;
	struct sun7i_capture_runtime_data *capture_prtd = NULL;
	dma_addr_t play_pos 	= 0, capture_pos = 0;
	unsigned long play_len 	= 0, capture_len = 0;
	unsigned int play_limit = 0, capture_limit = 0;

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {  
		play_prtd 	= substream->runtime->private_data;
		play_pos 	= play_prtd->dma_pos;
		play_len 	= play_prtd->dma_period;
		play_limit 	= play_prtd->dma_limit; 
		while (play_prtd->dma_loaded < play_limit) {
			if ((play_pos + play_len) > play_prtd->dma_end) {
				play_len  = play_prtd->dma_end - play_pos;			
			}
			play_ret = sw_dma_enqueue(play_prtd->dma_hdl, play_pos, play_prtd->params->dma_addr, play_len);
			if (play_ret == 0) {
				play_prtd->dma_loaded++;
				play_pos += play_prtd->dma_period;
				if (play_pos >= play_prtd->dma_end) {
					play_pos = play_prtd->dma_start;
				}
			} else {
				break;
			}
		}
		play_prtd->dma_pos = play_pos;	
	} else {
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
				if (capture_pos >= capture_prtd->dma_end) {
					capture_pos = capture_prtd->dma_start;
				}
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
	capture_prtd = substream->runtime->private_data;

	if (substream) {				
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
	play_prtd = substream->runtime->private_data;
	if (substream) {				
		snd_pcm_period_elapsed(substream);
	}	

	spin_lock(&play_prtd->lock);
	{
		play_prtd->dma_loaded--;
		sun7i_pcm_enqueue(substream);
	}
	spin_unlock(&play_prtd->lock);
}

static snd_pcm_uframes_t snd_sun7i_codec_pointer(struct snd_pcm_substream *substream)
{
	unsigned long play_res = 0, capture_res = 0;
	struct sun7i_playback_runtime_data *play_prtd = NULL;
	struct sun7i_capture_runtime_data *capture_prtd = NULL;
    if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
    	play_prtd = substream->runtime->private_data;
   		spin_lock(&play_prtd->lock);
		sw_dma_getposition(play_prtd->dma_hdl, (dma_addr_t*)&play_dmasrc, (dma_addr_t*)&play_dmadst);
		play_res = play_dmasrc + play_prtd->dma_period - play_prtd->dma_start;
		spin_unlock(&play_prtd->lock);
		if (play_res >= snd_pcm_lib_buffer_bytes(substream)) {
			if (play_res == snd_pcm_lib_buffer_bytes(substream))
				play_res = 0;
		}
		return bytes_to_frames(substream->runtime, play_res);
    } else {
    	capture_prtd = substream->runtime->private_data;
    	spin_lock(&capture_prtd->lock);
		sw_dma_getposition(capture_prtd->dma_hdl, (dma_addr_t*)&capture_dmasrc, (dma_addr_t*)&capture_dmadst);
    	capture_res = capture_dmadst + capture_prtd->dma_period - capture_prtd->dma_start;
    	spin_unlock(&capture_prtd->lock);
    	if (capture_res >= snd_pcm_lib_buffer_bytes(substream)) {
			if (capture_res == snd_pcm_lib_buffer_bytes(substream))
				capture_res = 0;
		}
		return bytes_to_frames(substream->runtime, capture_res);
    }	
}

static int sun7i_codec_pcm_hw_params(struct snd_pcm_substream *substream, struct snd_pcm_hw_params *params)
{	
   
    struct snd_pcm_runtime *play_runtime = NULL, *capture_runtime = NULL;
    struct sun7i_playback_runtime_data *play_prtd 	= NULL;
    struct sun7i_capture_runtime_data *capture_prtd = NULL;
    unsigned long play_totbytes = 0, capture_totbytes = 0;

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
	  	play_runtime 	= substream->runtime;
		play_prtd 		= play_runtime->private_data;
		play_totbytes 	= params_buffer_bytes(params);
		snd_pcm_lib_malloc_pages(substream, params_buffer_bytes(params));
		if (play_prtd->params == NULL) {
			play_prtd->params = &sun7i_codec_pcm_stereo_play;			
			/*
			 * requeset audio dma handle(we don't care about the channel!)
			 */
			play_prtd->dma_hdl = sw_dma_request(play_prtd->params->name, CHAN_NORAML);
			//sw_dma_dump_chan(play_prtd->dma_hdl);
			if (NULL == play_prtd->dma_hdl) {
				printk(KERN_ERR "failed to request spdif dma handle\n");
				return -EINVAL;
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
			play_prtd->dma_loaded 	= 0;
			play_prtd->dma_limit 	= play_runtime->hw.periods_min;
			play_prtd->dma_period 	= params_period_bytes(params);
			play_prtd->dma_start 	= play_runtime->dma_addr;
			play_dmasrc 		= play_prtd->dma_start;
			play_prtd->dma_pos 	= play_prtd->dma_start;
			play_prtd->dma_end 	= play_prtd->dma_start + play_totbytes;
			spin_unlock_irq(&play_prtd->lock);
		}
	} else if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
		capture_runtime 	= substream->runtime;
		capture_prtd 		= capture_runtime->private_data;
		capture_totbytes 	= params_buffer_bytes(params);
		snd_pcm_lib_malloc_pages(substream, params_buffer_bytes(params));
		if (capture_prtd->params == NULL) {
			capture_prtd->params = &sun7i_codec_pcm_stereo_capture;
			/*
			 * requeset audio dma handle(we don't care about the channel!)
			 */
			capture_prtd->dma_hdl = sw_dma_request(capture_prtd->params->name, CHAN_NORAML);
			if (NULL == capture_prtd->dma_hdl) {
				printk(KERN_ERR "failed to request spdif dma handle\n");
				return -EINVAL;
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
						
			capture_dmadst = capture_prtd->dma_start;
			capture_prtd->dma_pos = capture_prtd->dma_start;
			capture_prtd->dma_end = capture_prtd->dma_start + capture_totbytes;
			spin_unlock_irq(&capture_prtd->lock);
		}
	} else {
		return -EINVAL;
	}
	return 0;	
}

static int snd_sun7i_codec_hw_free(struct snd_pcm_substream *substream)
{	
	struct sun7i_playback_runtime_data *play_prtd 	= NULL;
	struct sun7i_capture_runtime_data *capture_prtd = NULL;	
   	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {    	  
 		play_prtd = substream->runtime->private_data;
		snd_pcm_set_runtime_buffer(substream, NULL);
		if (play_prtd->params) {
			/*
			 * stop play dma transfer
			 */
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
			/*
			 * Clear out the DMA and any allocated buffers.
			 */
			snd_pcm_lib_free_pages(substream);
		}
   	} else {
		capture_prtd = substream->runtime->private_data;
   		
		snd_pcm_set_runtime_buffer(substream, NULL);
		if (capture_prtd->params) {
			/*
			 * stop play dma transfer
			 */
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
			/*
			 * Clear out the DMA and any allocated buffers.
			 */
			snd_pcm_lib_free_pages(substream);
		}
   	}
	return 0;
}

static int snd_sun7i_codec_prepare(struct	snd_pcm_substream	*substream)
{	
	dma_config_t codec_play_dma_conf;
	dma_config_t codec_capture_dma_conf;
	int play_ret = 0, capture_ret = 0;
	unsigned int reg_val;
	struct sun7i_playback_runtime_data *play_prtd = NULL;
	struct sun7i_capture_runtime_data *capture_prtd = NULL;

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		switch (substream->runtime->rate) {
			case 44100:							
				clk_set_rate(codec_pll2clk, 22579200);
				clk_set_rate(codec_moduleclk, 22579200);
				reg_val = readl(baseaddr + SUN7I_DAC_FIFOC);
				reg_val &=~(7<<29); 
				reg_val |=(0<<29);
				writel(reg_val, baseaddr + SUN7I_DAC_FIFOC);
				break;
			case 22050:
				clk_set_rate(codec_pll2clk, 22579200);
				clk_set_rate(codec_moduleclk, 22579200);
				reg_val = readl(baseaddr + SUN7I_DAC_FIFOC);
				reg_val &=~(7<<29); 
				reg_val |=(2<<29);
				writel(reg_val, baseaddr + SUN7I_DAC_FIFOC);
				break;
			case 11025:
				clk_set_rate(codec_pll2clk, 22579200);
				clk_set_rate(codec_moduleclk, 22579200);
				reg_val = readl(baseaddr + SUN7I_DAC_FIFOC);
				reg_val &=~(7<<29); 
				reg_val |=(4<<29);
				writel(reg_val, baseaddr + SUN7I_DAC_FIFOC);
				break;
			case 48000:
				clk_set_rate(codec_pll2clk, 24576000);
				clk_set_rate(codec_moduleclk, 24576000);
				reg_val = readl(baseaddr + SUN7I_DAC_FIFOC);
				reg_val &=~(7<<29); 
				reg_val |=(0<<29);
				writel(reg_val, baseaddr + SUN7I_DAC_FIFOC);
				break;
			case 96000:
				clk_set_rate(codec_pll2clk, 24576000);
				clk_set_rate(codec_moduleclk, 24576000);
				reg_val = readl(baseaddr + SUN7I_DAC_FIFOC);
				reg_val &=~(7<<29); 
				reg_val |=(7<<29);
				writel(reg_val, baseaddr + SUN7I_DAC_FIFOC);
				break;
			case 192000:
				clk_set_rate(codec_pll2clk, 24576000);
				clk_set_rate(codec_moduleclk, 24576000);
				reg_val = readl(baseaddr + SUN7I_DAC_FIFOC);
				reg_val &=~(7<<29); 
				reg_val |=(6<<29);
				writel(reg_val, baseaddr + SUN7I_DAC_FIFOC);
				break;
			case 32000:
				clk_set_rate(codec_pll2clk, 24576000);
				clk_set_rate(codec_moduleclk, 24576000);
				reg_val = readl(baseaddr + SUN7I_DAC_FIFOC);
				reg_val &=~(7<<29); 
				reg_val |=(1<<29);
				writel(reg_val, baseaddr + SUN7I_DAC_FIFOC);
				break;
			case 24000:
				clk_set_rate(codec_pll2clk, 24576000);
				clk_set_rate(codec_moduleclk, 24576000);
				reg_val = readl(baseaddr + SUN7I_DAC_FIFOC);
				reg_val &=~(7<<29); 
				reg_val |=(2<<29);
				writel(reg_val, baseaddr + SUN7I_DAC_FIFOC);
				break;
			case 16000:
				clk_set_rate(codec_pll2clk, 24576000);
				clk_set_rate(codec_moduleclk, 24576000);
				reg_val = readl(baseaddr + SUN7I_DAC_FIFOC);
				reg_val &=~(7<<29); 
				reg_val |=(3<<29);
				writel(reg_val, baseaddr + SUN7I_DAC_FIFOC);
				break;
			case 12000:
				clk_set_rate(codec_pll2clk, 24576000);
				clk_set_rate(codec_moduleclk, 24576000);
				reg_val = readl(baseaddr + SUN7I_DAC_FIFOC);
				reg_val &=~(7<<29); 
				reg_val |=(4<<29);
				writel(reg_val, baseaddr + SUN7I_DAC_FIFOC);
				break;
			case 8000:
				clk_set_rate(codec_pll2clk, 24576000);
				clk_set_rate(codec_moduleclk, 24576000);
				reg_val = readl(baseaddr + SUN7I_DAC_FIFOC);
				reg_val &=~(7<<29); 
				reg_val |=(5<<29);
				writel(reg_val, baseaddr + SUN7I_DAC_FIFOC);
				break;
			default:
				clk_set_rate(codec_pll2clk, 24576000);
				clk_set_rate(codec_moduleclk, 24576000);
				reg_val = readl(baseaddr + SUN7I_DAC_FIFOC);
				reg_val &=~(7<<29); 
				reg_val |=(0<<29);
				writel(reg_val, baseaddr + SUN7I_DAC_FIFOC);		
				break;
		}
		switch (substream->runtime->channels) {
			case 1:
				reg_val = readl(baseaddr + SUN7I_DAC_FIFOC);
				reg_val |=(1<<6);
				writel(reg_val, baseaddr + SUN7I_DAC_FIFOC);			
				break;
			case 2:
				reg_val = readl(baseaddr + SUN7I_DAC_FIFOC);
				reg_val &=~(1<<6);
				writel(reg_val, baseaddr + SUN7I_DAC_FIFOC);
				break;
			default:
				reg_val = readl(baseaddr + SUN7I_DAC_FIFOC);
				reg_val &=~(1<<6);
				writel(reg_val, baseaddr + SUN7I_DAC_FIFOC);
				break;
		}
	} else {
		switch (substream->runtime->rate) {
			case 44100:
				clk_set_rate(codec_pll2clk, 22579200);
				clk_set_rate(codec_moduleclk, 22579200);		
				reg_val = readl(baseaddr + SUN7I_ADC_FIFOC);
				reg_val &=~(7<<29); 
				reg_val |=(0<<29);
				writel(reg_val, baseaddr + SUN7I_ADC_FIFOC);
				break;
			case 22050:
				clk_set_rate(codec_pll2clk, 22579200);
				clk_set_rate(codec_moduleclk, 22579200);
				reg_val = readl(baseaddr + SUN7I_ADC_FIFOC);
				reg_val &=~(7<<29); 
				reg_val |=(2<<29);
				writel(reg_val, baseaddr + SUN7I_ADC_FIFOC);
				break;
			case 11025:
				clk_set_rate(codec_pll2clk, 22579200);
				clk_set_rate(codec_moduleclk, 22579200);				
				reg_val = readl(baseaddr + SUN7I_ADC_FIFOC);
				reg_val &=~(7<<29); 
				reg_val |=(4<<29);
				writel(reg_val, baseaddr + SUN7I_ADC_FIFOC);
				break;
			case 48000:				
				clk_set_rate(codec_pll2clk, 24576000);
				clk_set_rate(codec_moduleclk, 24576000);
				reg_val = readl(baseaddr + SUN7I_ADC_FIFOC);
				reg_val &=~(7<<29); 
				reg_val |=(0<<29);
				writel(reg_val, baseaddr + SUN7I_ADC_FIFOC);
				break;
			case 32000:
				clk_set_rate(codec_pll2clk, 24576000);
				clk_set_rate(codec_moduleclk, 24576000);
				reg_val = readl(baseaddr + SUN7I_ADC_FIFOC);
				reg_val &=~(7<<29); 
				reg_val |=(1<<29);
				writel(reg_val, baseaddr + SUN7I_ADC_FIFOC);
				break;
			case 24000:
				clk_set_rate(codec_pll2clk, 24576000);
				clk_set_rate(codec_moduleclk, 24576000);
				reg_val = readl(baseaddr + SUN7I_ADC_FIFOC);
				reg_val &=~(7<<29); 
				reg_val |=(2<<29);
				writel(reg_val, baseaddr + SUN7I_ADC_FIFOC);
				break;
			case 16000:
				clk_set_rate(codec_pll2clk, 24576000);
				clk_set_rate(codec_moduleclk, 24576000);
				reg_val = readl(baseaddr + SUN7I_ADC_FIFOC);
				reg_val &=~(7<<29); 
				reg_val |=(3<<29);
				writel(reg_val, baseaddr + SUN7I_ADC_FIFOC);
				break;
			case 12000:
				clk_set_rate(codec_pll2clk, 24576000);
				clk_set_rate(codec_moduleclk, 24576000);
				reg_val = readl(baseaddr + SUN7I_ADC_FIFOC);
				reg_val &=~(7<<29); 
				reg_val |=(4<<29);
				writel(reg_val, baseaddr + SUN7I_ADC_FIFOC);
				break;
			case 8000:
				clk_set_rate(codec_pll2clk, 24576000);
				clk_set_rate(codec_moduleclk, 24576000);
				reg_val = readl(baseaddr + SUN7I_ADC_FIFOC);
				reg_val &=~(7<<29); 
				reg_val |=(5<<29);
				writel(reg_val, baseaddr + SUN7I_ADC_FIFOC);
				break;
			default:
				clk_set_rate(codec_pll2clk, 24576000);
				clk_set_rate(codec_moduleclk, 24576000);	
				reg_val = readl(baseaddr + SUN7I_ADC_FIFOC);
				reg_val &=~(7<<29); 
				reg_val |=(0<<29);
				writel(reg_val, baseaddr + SUN7I_ADC_FIFOC);		
				break;
		}
		switch (substream->runtime->channels) {
			case 1:
				reg_val = readl(baseaddr + SUN7I_ADC_FIFOC);
				reg_val |=(1<<7);
				writel(reg_val, baseaddr + SUN7I_ADC_FIFOC);			
			break;
			case 2:
				reg_val = readl(baseaddr + SUN7I_ADC_FIFOC);
				reg_val &=~(1<<7);
				writel(reg_val, baseaddr + SUN7I_ADC_FIFOC);
			break;
			default:
				reg_val = readl(baseaddr + SUN7I_ADC_FIFOC);
				reg_val &=~(1<<7);
				writel(reg_val, baseaddr + SUN7I_ADC_FIFOC);
			break;
		}        	
	}
   if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
   	 	play_prtd = substream->runtime->private_data;
   	 	/* return if this is a bufferless transfer e.g.
	  	* codec <--> BT codec or GSM modem -- lg FIXME */       
   	 	if (!play_prtd->params)
			return 0;

		codec_play_open(substream);
   	 	//open the dac channel register
		memset(&codec_play_dma_conf, 0, sizeof(codec_play_dma_conf));
		codec_play_dma_conf.xfer_type.src_data_width 	= DATA_WIDTH_16BIT;
		codec_play_dma_conf.xfer_type.src_bst_len 		= DATA_BRST_4;
		codec_play_dma_conf.xfer_type.dst_data_width 	= DATA_WIDTH_16BIT;
		codec_play_dma_conf.xfer_type.dst_bst_len 		= DATA_BRST_4;
		codec_play_dma_conf.address_type.src_addr_mode 	= NDMA_ADDR_INCREMENT;
		codec_play_dma_conf.address_type.dst_addr_mode 	= NDMA_ADDR_NOCHANGE;
		codec_play_dma_conf.src_drq_type 				= N_SRC_SDRAM;
		codec_play_dma_conf.dst_drq_type 				= N_DST_AUDIO_CODEC_DA;
		codec_play_dma_conf.bconti_mode 				= false;
		codec_play_dma_conf.irq_spt 					= CHAN_IRQ_FD;
		if (0 != sw_dma_config(play_prtd->dma_hdl, &codec_play_dma_conf)) {
			return -EINVAL;
		}
		play_prtd->dma_loaded = 0;
		/* enqueue dma buffers */
		sun7i_pcm_enqueue(substream);
		return play_ret;
	} else {
		capture_prtd = substream->runtime->private_data;
   	 	/* return if this is a bufferless transfer e.g.
	  	 * codec <--> BT codec or GSM modem -- lg FIXME */       
   	 	if (!capture_prtd->params)
			return 0;
	   	//open the adc channel register
	   	codec_capture_open();
	   	//set the dma	   	
	   	memset(&codec_capture_dma_conf, 0, sizeof(codec_capture_dma_conf));
		codec_capture_dma_conf.xfer_type.src_data_width 	= DATA_WIDTH_16BIT;
		codec_capture_dma_conf.xfer_type.src_bst_len 		= DATA_BRST_4;
		codec_capture_dma_conf.xfer_type.dst_data_width 	= DATA_WIDTH_16BIT;
		codec_capture_dma_conf.xfer_type.dst_bst_len 		= DATA_BRST_4;
		codec_capture_dma_conf.address_type.src_addr_mode 	= NDMA_ADDR_NOCHANGE;
		codec_capture_dma_conf.address_type.dst_addr_mode 	= NDMA_ADDR_INCREMENT;
		codec_capture_dma_conf.src_drq_type 				= N_SRC_AUDIO_CODEC_AD;
		codec_capture_dma_conf.dst_drq_type 				= N_DST_SDRAM;
		codec_capture_dma_conf.bconti_mode 					= false;
		codec_capture_dma_conf.irq_spt 						= CHAN_IRQ_FD;

		if (0 != sw_dma_config(capture_prtd->dma_hdl, &codec_capture_dma_conf)) {
			return -EINVAL;
		}
		capture_prtd->dma_loaded = 0;
		/* enqueue dma buffers */
		sun7i_pcm_enqueue(substream);
		return capture_ret;
	}
}

static int snd_sun7i_codec_trigger(struct snd_pcm_substream *substream, int cmd)
{	
	int play_ret = 0, capture_ret = 0;
	struct sun7i_playback_runtime_data *play_prtd 	= NULL;
	struct sun7i_capture_runtime_data *capture_prtd = NULL;

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		play_prtd = substream->runtime->private_data;
		spin_lock(&play_prtd->lock);
		switch (cmd) {
			case SNDRV_PCM_TRIGGER_START:
			case SNDRV_PCM_TRIGGER_RESUME:
			case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
				play_prtd->state |= ST_RUNNING;		
				codec_play_start();				
				/*
				* start dma transfer
				*/
				if (0 != sw_dma_ctl(play_prtd->dma_hdl, DMA_OP_START, NULL)) {
					printk("%s err, dma start err\n", __FUNCTION__);
					return -EINVAL;
				}
				if (substream->runtime->rate >=192000) {
				} else if (substream->runtime->rate > 22050) {	
					mdelay(2);
				} else {
					mdelay(7);
				}
				//pa unmute
				codec_wr_control(SUN7I_DAC_ACTL, 0x1, PA_MUTE, 0x1);
				printk("codec:start 1\n");
				break;
			case SNDRV_PCM_TRIGGER_SUSPEND:				
				codec_play_stop();				
				break;
			case SNDRV_PCM_TRIGGER_STOP:			 				
				play_prtd->state &= ~ST_RUNNING;
				codec_play_stop();
				/*
				* stop  dma transfer
				*/
				if (0 != sw_dma_ctl(play_prtd->dma_hdl, DMA_OP_STOP, NULL)) {
					printk("%s err, dma stop err\n", __FUNCTION__);
					return -EINVAL;
				}
				break;
			case SNDRV_PCM_TRIGGER_PAUSE_PUSH:							
				play_prtd->state &= ~ST_RUNNING;
				/*
				* stop  dma transfer
				*/
				if (0 != sw_dma_ctl(play_prtd->dma_hdl, DMA_OP_STOP, NULL)) {
					printk("%s err, dma stop err\n", __FUNCTION__);
					return -EINVAL;
				}
				break;
			default:
				printk("error:%s,%d\n", __func__, __LINE__);
				play_ret = -EINVAL;
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
			capture_prtd->state |= ST_RUNNING;		 
			codec_capture_start();
			mdelay(1);
			codec_wr_control(SUN7I_ADC_FIFOC, 0x1, ADC_FIFO_FLUSH, 0x1);
			/*
			* start dma transfer
			*/
			if (0 != sw_dma_ctl(capture_prtd->dma_hdl, DMA_OP_START, NULL)) {
				printk("%s err, dma start err\n", __FUNCTION__);
				return -EINVAL;
			}
			printk("codec:start 2\n");
			break;
		case SNDRV_PCM_TRIGGER_SUSPEND:
			codec_capture_stop();		
			break;
		case SNDRV_PCM_TRIGGER_STOP:		 
			capture_prtd->state &= ~ST_RUNNING;
			codec_capture_stop();
			/*
			* stop  dma transfer
			*/
			if (0 != sw_dma_ctl(capture_prtd->dma_hdl, DMA_OP_STOP, NULL)) {
				printk("%s err, dma stop err\n", __FUNCTION__);
				return -EINVAL;
			}
			break;
		case SNDRV_PCM_TRIGGER_PAUSE_PUSH:		
			capture_prtd->state &= ~ST_RUNNING;
			/*
			* stop  dma transfer
			*/
			if (0 != sw_dma_ctl(capture_prtd->dma_hdl, DMA_OP_STOP, NULL)) {
				printk("%s err, dma stop err\n", __FUNCTION__);
				return -EINVAL;
			}
			break;	
		default:
			printk("error:%s,%d\n", __func__, __LINE__);
			capture_ret = -EINVAL;
			break;
		}
		spin_unlock(&capture_prtd->lock);
	}
	return 0;
}

static int snd_sun7icard_capture_open(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	int err;
	struct sun7i_capture_runtime_data *capture_prtd;

	capture_prtd = kzalloc(sizeof(struct sun7i_capture_runtime_data), GFP_KERNEL);
	if (capture_prtd == NULL)
		return -ENOMEM;

	spin_lock_init(&capture_prtd->lock);
	runtime->private_data = capture_prtd;
	runtime->hw = sun7i_pcm_capture_hardware;

	/* ensure that buffer size is a multiple of period size */
	if ((err = snd_pcm_hw_constraint_integer(runtime, SNDRV_PCM_HW_PARAM_PERIODS)) < 0)
		return err;
	if ((err = snd_pcm_hw_constraint_list(runtime, 0, SNDRV_PCM_HW_PARAM_RATE, &hw_constraints_rates)) < 0)
		return err;
        
	return 0;
}

static int snd_sun7icard_capture_close(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;

	kfree(runtime->private_data);
	return 0;
}

static int snd_sun7icard_playback_open(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	int err;
	struct sun7i_playback_runtime_data *play_prtd;

	play_prtd = kzalloc(sizeof(struct sun7i_playback_runtime_data), GFP_KERNEL);
	if (play_prtd == NULL)
		return -ENOMEM;

	spin_lock_init(&play_prtd->lock);
	runtime->private_data = play_prtd;
	runtime->hw = sun7i_pcm_playback_hardware;
	
	/* ensure that buffer size is a multiple of period size */
	if ((err = snd_pcm_hw_constraint_integer(runtime, SNDRV_PCM_HW_PARAM_PERIODS)) < 0)
		return err;
	if ((err = snd_pcm_hw_constraint_list(runtime, 0, SNDRV_PCM_HW_PARAM_RATE, &hw_constraints_rates)) < 0)
		return err;
    
	return 0;
}

static int snd_sun7icard_playback_close(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	kfree(runtime->private_data);

	return 0;
}

static struct snd_pcm_ops sun7i_pcm_playback_ops = {
	.open			= snd_sun7icard_playback_open,
	.close			= snd_sun7icard_playback_close,
	.ioctl			= snd_pcm_lib_ioctl,
	.hw_params	    = sun7i_codec_pcm_hw_params,
	.hw_free	    = snd_sun7i_codec_hw_free,
	.prepare		= snd_sun7i_codec_prepare,
	.trigger		= snd_sun7i_codec_trigger,
	.pointer		= snd_sun7i_codec_pointer,
};

static struct snd_pcm_ops sun7i_pcm_capture_ops = {
	.open			= snd_sun7icard_capture_open,
	.close			= snd_sun7icard_capture_close,
	.ioctl			= snd_pcm_lib_ioctl,
	.hw_params	    = sun7i_codec_pcm_hw_params,
	.hw_free	    = snd_sun7i_codec_hw_free,
	.prepare		= snd_sun7i_codec_prepare,
	.trigger		= snd_sun7i_codec_trigger,
	.pointer		= snd_sun7i_codec_pointer,
};

static int __init snd_card_sun7i_codec_pcm(struct sun7i_codec *sun7i_codec, int device)
{
	struct snd_pcm *pcm;
	int err;

	if ((err = snd_pcm_new(sun7i_codec->card, "M1 PCM", device, 1, 1, &pcm)) < 0){	
		printk("error,the func is: %s,the line is:%d\n", __func__, __LINE__);
		return err;
	}

	/*
	 * this sets up our initial buffers and sets the dma_type to isa.
	 * isa works but I'm not sure why (or if) it's the right choice
	 * this may be too large, trying it for now
	 */
	snd_pcm_lib_preallocate_pages_for_all(pcm, SNDRV_DMA_TYPE_DEV, 
					      snd_dma_isa_data(),
					      32*1024, 32*1024);

	snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_PLAYBACK, &sun7i_pcm_playback_ops);
	if (capture_used) {
		snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_CAPTURE, &sun7i_pcm_capture_ops);
	}
	pcm->private_data = sun7i_codec;
	pcm->info_flags = 0;
	strcpy(pcm->name, "sun7i PCM");

	return 0;
}

void snd_sun7i_codec_free(struct snd_card *card)
{
  
}

static void codec_resume_events(struct work_struct *work)
{
	//pa mute
	codec_wr_control(SUN7I_DAC_ACTL, 0x1, PA_MUTE, 0x0);
	msleep(20);
	//enable PA
	codec_wr_control(SUN7I_ADC_ACTL, 0x1, PA_ENABLE, 0x1);
	msleep(550);
    msleep(50);
	//pa mute
	codec_wr_control(SUN7I_DAC_ACTL, 0x1, PA_MUTE, 0x1);
	codec_wr_control(SUN7I_ADC_ACTL, 0x1, 8, 0x0);
	
	printk("====codec_resume_events turn on===\n");
}

static int __init sun7i_codec_probe(struct platform_device *pdev)
{
	int err;
	int ret;
	struct snd_card *card;
	struct sun7i_codec *chip;
	struct codec_board_info  *db;    
    printk("enter sun7i Audio codec!!!\n"); 
	/* register the soundcard */
	ret = snd_card_create(SNDRV_DEFAULT_IDX1, SNDRV_DEFAULT_STR1, THIS_MODULE, sizeof(struct sun7i_codec),
			      &card);
	if (ret != 0) {
		return -ENOMEM;
	}
	chip = card->private_data;
	card->private_free = snd_sun7i_codec_free;
	chip->card = card;
	chip->samplerate = AUDIO_RATE_DEFAULT;

	if ((err = snd_chip_codec_mixer_new(card)))
		goto nodev;
	if ((err = snd_card_sun7i_codec_pcm(chip, 0)) < 0)
	    goto nodev;

	strcpy(card->driver, "sun7i-CODEC");
	strcpy(card->shortname, "audiocodec");
	sprintf(card->longname, "sun7i-CODEC  Audio Codec");

	snd_card_set_dev(card, &pdev->dev);

	if ((err = snd_card_register(card)) == 0) {
		printk( KERN_INFO "sun7i audio support initialized\n" );
		platform_set_drvdata(pdev, card);
	}else{
      return err;
	}

	db = kzalloc(sizeof(*db), GFP_KERNEL);
	if (!db)
		return -ENOMEM;
  	/* codec_apbclk */
	codec_apbclk = clk_get(NULL,CLK_APB_ADDA);
	if (!codec_apbclk || IS_ERR(codec_apbclk)) {
		printk("try to get CLK_APB_ADDA failed!\n");
	}
	
	if (-1 == clk_enable(codec_apbclk)) {
		printk("codec_apbclk failed; \n");
	}
	/* codec_pll2clk */
	codec_pll2clk = clk_get(NULL,CLK_SYS_PLL2 );
	if (!codec_pll2clk || IS_ERR(codec_pll2clk)) {
		printk("try to get CLK_SYS_PLL2 failed!\n");
	}
	clk_enable(codec_pll2clk);	 
	/* codec_moduleclk */
	codec_moduleclk = clk_get(NULL,CLK_MOD_ADDA);
	if (!codec_moduleclk || IS_ERR(codec_moduleclk)) {
		printk("try to get CLK_MOD_ADDA failed!\n");
	}

	if (clk_set_parent(codec_moduleclk, codec_pll2clk)) {
		printk("try to set parent of codec_moduleclk to codec_pll2clk failed!\n");		
	}
	if (clk_set_rate(codec_moduleclk, 24576000)) {
		printk("set codec_moduleclk clock freq 24576000 failed!\n");
	}
	if (-1 == clk_enable(codec_moduleclk)) {
		printk("open codec_moduleclk failed; \n");
	}
	db->codec_base_res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	db->dev = &pdev->dev;
	
	if (db->codec_base_res == NULL) {
		ret = -ENOENT;
		printk("codec insufficient resources\n");
		goto out;
	}
	 /* codec address remap */
	 db->codec_base_req = request_mem_region(db->codec_base_res->start, 0x40,
					   pdev->name);
	 if (db->codec_base_req == NULL) {
		 ret = -EIO;
		 printk("cannot claim codec address reg area\n");
		 goto out;
	 }
	 baseaddr = ioremap(db->codec_base_res->start, 0x40);	 	 

	 if (baseaddr == NULL) {
		 ret = -EINVAL;
		 dev_err(db->dev,"failed to ioremap codec address reg\n");
		 goto out;
	 }

	kfree(db);
	codec_init();

	/*get the default pa val(close)*/
	type = script_get_item("audio_para", "audio_pa_ctrl", &item);
	if (SCIRPT_ITEM_VALUE_TYPE_PIO != type) {
		printk("script_get_item return type err\n");
		return -EFAULT;
	}
	/*request gpio*/
	req_status = gpio_request(item.gpio.gpio, NULL);
	if (0 != req_status) {
		printk("request gpio failed!\n");
	}
	/*config gpio info of audio_pa_ctrl, the default pa config is close(check pa sys_config1.fex).*/
	if (0 != sw_gpio_setall_range(&item.gpio, 1)) {
		printk("sw_gpio_setall_range failed\n");
	}

	resume_work_queue = create_singlethread_workqueue("codec_resume");
	if (resume_work_queue == NULL) {
		printk("[su4i-codec] try to create workqueue for codec failed!\n");
		ret = -ENOMEM;
		goto err_resume_work_queue;
	}

	printk("sun7i Audio codec successfully loaded..\n");
	return 0;
	err_resume_work_queue:
	out:
	 dev_err(db->dev, "not found (%d).\n", ret);
	
	nodev:
	snd_card_free(card);
	return err;
}

static int snd_sun7i_codec_suspend(struct platform_device *pdev,pm_message_t state)
{
	printk("[audio codec]:suspend start!\n");
	codec_wr_control(SUN7I_ADC_ACTL, 0x1, PA_ENABLE, 0x0);
	mdelay(100);
	//pa mute
	codec_wr_control(SUN7I_DAC_ACTL, 0x1, PA_MUTE, 0x0);
	mdelay(500);
    //disable dac analog
	codec_wr_control(SUN7I_DAC_ACTL, 0x1, 	DACAEN_L, 0x0);
	codec_wr_control(SUN7I_DAC_ACTL, 0x1, 	DACAEN_R, 0x0);
	
	//disable dac to pa
	codec_wr_control(SUN7I_DAC_ACTL, 0x1, 	DACPAS, 0x0);	
	codec_wr_control(SUN7I_DAC_DPC ,  0x1, DAC_EN, 0x0); 
	
	//disable mic
	codec_wr_control(SUN7I_ADC_ACTL, 0x1, MIC1_EN, 0x0);

	//disable VMIC
	codec_wr_control(SUN7I_ADC_ACTL, 0x1, VMIC_EN, 0x0);
	clk_disable(codec_moduleclk);
	printk("[audio codec]:suspend end!\n");
	return 0;	
}

static int snd_sun7i_codec_resume(struct platform_device *pdev)
{	
	if (-1 == clk_enable(codec_moduleclk)) {
		printk("open codec_moduleclk failed; \n");
	}
	/*process for normal standby*/
	if (NORMAL_STANDBY == standby_type) {
	/*process for super standby*/
	} else if(SUPER_STANDBY == standby_type) {
		codec_wr_control(SUN7I_DAC_ACTL, 0x6, VOLUME, 0x3b);
		codec_wr_control(SUN7I_DAC_FIFOC, 0x3, DRA_LEVEL,0x3);
		codec_wr_control(SUN7I_DAC_FIFOC ,  0x1,28, 0x1);
	}
	queue_work(resume_work_queue, &codec_resume_work);
	printk("[audio codec]:resume end\n");
	return 0;	
}

static int __devexit sun7i_codec_remove(struct platform_device *devptr)
{
	clk_disable(codec_moduleclk);
	clk_put(codec_pll2clk);
	clk_put(codec_apbclk);

	snd_card_free(platform_get_drvdata(devptr));
	platform_set_drvdata(devptr, NULL);
	return 0;
}

static void sun7i_codec_shutdown(struct platform_device *devptr)
{
	codec_wr_control(SUN7I_ADC_ACTL, 0x1, PA_ENABLE, 0x0);
	mdelay(100);
	//pa mute
	codec_wr_control(SUN7I_DAC_ACTL, 0x1, PA_MUTE, 0x0);
	mdelay(500);
    //disable dac analog
	codec_wr_control(SUN7I_DAC_ACTL, 0x1, 	DACAEN_L, 0x0);
	codec_wr_control(SUN7I_DAC_ACTL, 0x1, 	DACAEN_R, 0x0);

	//disable dac to pa
	codec_wr_control(SUN7I_DAC_ACTL, 0x1, 	DACPAS, 0x0);
	codec_wr_control(SUN7I_DAC_DPC ,  0x1, DAC_EN, 0x0);
	 
	clk_disable(codec_moduleclk);
}

static struct resource sun7i_codec_resource[] = {
	[0] = {
    	.start = CODEC_BASSADDRESS,
        .end   = CODEC_BASSADDRESS + 0x40,
		.flags = IORESOURCE_MEM,      
	},
};

/*data relating*/
static struct platform_device sun7i_device_codec = {
	.name = "sun7i-codec",
	.id = -1,
	.num_resources = ARRAY_SIZE(sun7i_codec_resource),
	.resource = sun7i_codec_resource,     	   
};

/*method relating*/
static struct platform_driver sun7i_codec_driver = {
	.probe		= sun7i_codec_probe,
	.remove		= sun7i_codec_remove,
	.shutdown   = sun7i_codec_shutdown,
#ifdef CONFIG_PM
	.suspend	= snd_sun7i_codec_suspend,
	.resume		= snd_sun7i_codec_resume,
#endif
	.driver		= {
		.name	= "sun7i-codec",
	},
};

static int audio_used = 1;
static int __init sun7i_codec_init(void)
{
	int err = 0;
	static script_item_u   val;
	script_item_value_type_e  type;

	type = script_get_item("audio_para", "audio_used", &val);
	if(SCIRPT_ITEM_VALUE_TYPE_INT != type){
		printk("type err!");
	}
	pr_info("value is %d\n", val.val);
    audio_used=val.val;
   if (audio_used) {
		if((platform_device_register(&sun7i_device_codec))<0)
			return err;

		if ((err = platform_driver_register(&sun7i_codec_driver)) < 0)
			return err;
	}
	return 0;
}

static void __exit sun7i_codec_exit(void)
{
	platform_driver_unregister(&sun7i_codec_driver);
}

module_init(sun7i_codec_init);
module_exit(sun7i_codec_exit);

MODULE_DESCRIPTION("sun7i CODEC ALSA codec driver");
MODULE_AUTHOR("software");
MODULE_LICENSE("GPL");
