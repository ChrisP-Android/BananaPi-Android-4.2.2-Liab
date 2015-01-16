/******************************************************************************
 *
 *  Copyright (C) 2009-2012 Realtek Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/

/******************************************************************************
 *
 *  Filename:      hardware.c
 *
 *  Description:   Contains controller-specific functions, like
 *                      firmware patch download
 *                      low power mode operations
 *
 ******************************************************************************/

#define LOG_TAG "bt_hwcfg"

#include <utils/Log.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <ctype.h>
#include <cutils/properties.h>
#include <stdlib.h>
#include "bt_hci_bdroid.h"
#include "bt_vendor_rtk.h"
#include "userial.h"
#include "userial_vendor.h"
#include "upio.h"

/******************************************************************************
**  Constants & Macros
******************************************************************************/

#ifndef BTHW_DBG
#define BTHW_DBG FALSE
#endif

#if (BTHW_DBG == TRUE)
#define BTHWDBG(param, ...) {ALOGD(param, ## __VA_ARGS__);}
#else
#define BTHWDBG(param, ...) {}
#endif


#define HCI_UART_H4	0
#define HCI_UART_3WIRE	2


#define FIRMWARE_DIRECTORY "/system/etc/firmware/rtlbt/"
#define BT_CONFIG_DIRECTORY "/system/etc/firmware/rtlbt/"
#define PATCH_DATA_FIELD_MAX_SIZE     252


struct patch_struct {
    int		nTxIndex; 	// current sending pkt number
    int 	nTotal; 	// total pkt number
    int		nRxIndex; 	// ack index from board
    int		nNeedRetry;	// if no response from board 
};
static struct patch_struct rtk_patch;


#define RTK_VENDOR_CONFIG_MAGIC 0x8723ab55
struct rtk_bt_vendor_config_entry{
	uint16_t offset;
	uint8_t entry_len;
	uint8_t entry_data[0];
} __attribute__ ((packed));


struct rtk_bt_vendor_config{
	uint32_t signature;
	uint16_t data_len;
	struct rtk_bt_vendor_config_entry entry[0];
} __attribute__ ((packed));

int gHwFlowControlEnable = 1;
int gFinalSpeed = 0;

#define FW_PATCHFILE_EXTENSION      ".hcd"
#define FW_PATCHFILE_EXTENSION_LEN  4
#define FW_PATCHFILE_PATH_MAXLEN    248 /* Local_Name length of return of
                                           HCI_Read_Local_Name */

#define HCI_CMD_MAX_LEN             258

#define HCI_RESET                               0x0C03

#define HCI_VSC_UPDATE_BAUDRATE                 0xFC17
#define HCI_VSC_DOWNLOAD_FW_PATCH                0xFC20

#define HCI_VSC_H5_INIT                0xFCEE


#define HCI_EVT_CMD_CMPL_STATUS_RET_BYTE        5
#define HCI_EVT_CMD_CMPL_LOCAL_NAME_STRING      6
#define HCI_EVT_CMD_CMPL_LOCAL_BDADDR_ARRAY     6
#define HCI_EVT_CMD_CMPL_OPCODE                 3
#define LPM_CMD_PARAM_SIZE                      12
#define UPDATE_BAUDRATE_CMD_PARAM_SIZE          6
#define HCI_CMD_PREAMBLE_SIZE                   3
#define HCD_REC_PAYLOAD_LEN_BYTE                2
#define BD_ADDR_LEN                             6
#define LOCAL_NAME_BUFFER_LEN                   32
#define LOCAL_BDADDR_PATH_BUFFER_LEN            256


#define H5_SYNC_REQ_SIZE 2
#define H5_SYNC_RESP_SIZE 2
#define H5_CONF_REQ_SIZE 3
#define H5_CONF_RESP_SIZE 2

#define STREAM_TO_UINT16(u16, p) {u16 = ((uint16_t)(*(p)) + (((uint16_t)(*((p) + 1))) << 8)); (p) += 2;}
#define UINT16_TO_STREAM(p, u16) {*(p)++ = (uint8_t)(u16); *(p)++ = (uint8_t)((u16) >> 8);}
#define UINT32_TO_STREAM(p, u32) {*(p)++ = (uint8_t)(u32); *(p)++ = (uint8_t)((u32) >> 8); *(p)++ = (uint8_t)((u32) >> 16); *(p)++ = (uint8_t)((u32) >> 24);}

/******************************************************************************
**  Local type definitions
******************************************************************************/

/* Hardware Configuration State */
enum {
    HW_CFG_H5_INIT = 1,
    HW_CFG_START,    
    HW_CFG_SET_UART_BAUD_HOST,//change FW baudrate
    HW_CFG_SET_UART_BAUD_CONTROLLER,//change Host baudrate
    HW_CFG_DL_FW_PATCH
};

/* h/w config control block */
typedef struct
{
    uint8_t state;                          /* Hardware configuration state */
    int     fw_fd;                          /* FW patch file fd */
    uint8_t f_set_baud_2;                   /* Baud rate switch state */
    char    local_chip_name[LOCAL_NAME_BUFFER_LEN];
} bt_hw_cfg_cb_t;

/* low power mode parameters */
typedef struct
{
    uint8_t sleep_mode;                     /* 0(disable),1(UART),9(H5) */
    uint8_t host_stack_idle_threshold;      /* Unit scale 300ms/25ms */
    uint8_t host_controller_idle_threshold; /* Unit scale 300ms/25ms */
    uint8_t bt_wake_polarity;               /* 0=Active Low, 1= Active High */
    uint8_t host_wake_polarity;             /* 0=Active Low, 1= Active High */
    uint8_t allow_host_sleep_during_sco;
    uint8_t combine_sleep_mode_and_lpm;
    uint8_t enable_uart_txd_tri_state;      /* UART_TXD Tri-State */
    uint8_t sleep_guard_time;               /* sleep guard time in 12.5ms */
    uint8_t wakeup_guard_time;              /* wakeup guard time in 12.5ms */
    uint8_t txd_config;                     /* TXD is high in sleep state */
    uint8_t pulsed_host_wake;               /* pulsed host wake if mode = 1 */
} bt_lpm_param_t;


/******************************************************************************
**  Externs
******************************************************************************/

void hw_config_cback(void *p_evt_buf);
extern uint8_t vnd_local_bd_addr[BD_ADDR_LEN];


/******************************************************************************
**  Static variables
******************************************************************************/

static char fw_patchfile_path[256] = FW_PATCHFILE_LOCATION;
static char fw_patchfile_name[128] = { 0 };


static bt_hw_cfg_cb_t hw_cfg_cb;

static bt_lpm_param_t lpm_param =
{
    LPM_SLEEP_MODE,
    LPM_IDLE_THRESHOLD,
    LPM_HC_IDLE_THRESHOLD,
    LPM_BT_WAKE_POLARITY,
    LPM_HOST_WAKE_POLARITY,
    LPM_ALLOW_HOST_SLEEP_DURING_SCO,
    LPM_COMBINE_SLEEP_MODE_AND_LPM,
    LPM_ENABLE_UART_TXD_TRI_STATE,
    0,  /* not applicable */
    0,  /* not applicable */
    0,  /* not applicable */
    LPM_PULSED_HOST_WAKE
};

/*******************************************************************************
**
** Function        ms_delay
**
** Description     sleep unconditionally for timeout milliseconds
**
** Returns         None
**
*******************************************************************************/
void ms_delay (uint32_t timeout)
{
    struct timespec delay;
    int err;

    if (timeout == 0)
        return;

    delay.tv_sec = timeout / 1000;
    delay.tv_nsec = 1000 * 1000 * (timeout%1000);

    /* [u]sleep can't be used because it uses SIGALRM */
    do {
        err = nanosleep(&delay, &delay);
    } while (err < 0 && errno ==EINTR);
}

/*******************************************************************************
**
** Function        line_speed_to_userial_baud
**
** Description     helper function converts line speed number into USERIAL baud
**                 rate symbol
**
** Returns         unit8_t (USERIAL baud symbol)
**
*******************************************************************************/
uint8_t line_speed_to_userial_baud(uint32_t line_speed)
{
    uint8_t baud;

    if (line_speed == 4000000)
        baud = USERIAL_BAUD_4M;
    else if (line_speed == 3000000)
        baud = USERIAL_BAUD_3M;
    else if (line_speed == 2000000)
        baud = USERIAL_BAUD_2M;
    else if (line_speed == 1500000)
        baud = USERIAL_BAUD_1_5M;
    else if (line_speed == 1000000)
        baud = USERIAL_BAUD_1M;
    else if (line_speed == 921600)
        baud = USERIAL_BAUD_921600;
    else if (line_speed == 460800)
        baud = USERIAL_BAUD_460800;
    else if (line_speed == 230400)
        baud = USERIAL_BAUD_230400;
    else if (line_speed == 115200)
        baud = USERIAL_BAUD_115200;
    else if (line_speed == 57600)
        baud = USERIAL_BAUD_57600;
    else if (line_speed == 19200)
        baud = USERIAL_BAUD_19200;
    else if (line_speed == 9600)
        baud = USERIAL_BAUD_9600;
    else if (line_speed == 1200)
        baud = USERIAL_BAUD_1200;
    else if (line_speed == 600)
        baud = USERIAL_BAUD_600;
    else
    {
        ALOGE( "userial vendor: unsupported baud speed %d", line_speed);
        baud = USERIAL_BAUD_115200;
    }

    return baud;
}

typedef struct _baudrate_ex
{
	int rtk_speed;
	int uart_speed;
}baudrate_ex;

baudrate_ex baudrates[] = 
{

	{0x00006004, 921600},
	{0x05F75004, 921600},//RTL8723BS
	{0x00004003, 1500000},
	{0x04928002, 1500000},//RTL8723BS	
	{0x00005002, 2000000},//same as RTL8723AS
	{0x00008001, 3000000},
	{0x04928001, 3000000},//RTL8723BS	
       {0x00007001, 3500000},
       {0x052A6001, 3500000},//RTL8723BS
	{0x00005001, 4000000},//same as RTL8723AS
	{0x0000701d, 115200},	
     	{0x0252C002, 115200}	//RTL8723BS
};

/**
* Change realtek Bluetooth speed to uart speed. It is matching in the struct baudrates:
*
* @code
* baudrate_ex baudrates[] = 
* {
*  	{0x7001, 3500000},
*	{0x6004, 921600},
*	{0x4003, 1500000},
*	{0x5001, 4000000},
*	{0x5002, 2000000},
*	{0x8001, 3000000},
*	{0x701d, 115200}
* };
* @endcode
*
* If there is no match in baudrates, uart speed will be set as #115200.
*
* @param rtk_speed realtek Bluetooth speed
* @param uart_speed uart speed
*
*/
static void rtk_speed_to_uart_speed(uint32_t rtk_speed, uint32_t* uart_speed)
{
	*uart_speed = 115200;

	uint8_t i;
	for (i=0; i< sizeof(baudrates)/sizeof(baudrate_ex); i++)
	{
		if (baudrates[i].rtk_speed == rtk_speed){
			*uart_speed = baudrates[i].uart_speed;
			return;
		}
	}
	return;
}

/**
* Change uart speed to realtek Bluetooth speed. It is matching in the struct baudrates:
*
* @code
* baudrate_ex baudrates[] = 
* {
*  	{0x7001, 3500000},
*	{0x6004, 921600},
*	{0x4003, 1500000},
*	{0x5001, 4000000},
*	{0x5002, 2000000},
*	{0x8001, 3000000},
*	{0x701d, 115200}
* };
* @endcode
*
* If there is no match in baudrates, realtek Bluetooth speed will be set as #0x701D.
*
* @param uart_speed uart speed
* @param rtk_speed realtek Bluetooth speed
*
*/
static inline void uart_speed_to_rtk_speed(int uart_speed, int* rtk_speed)
{
	*rtk_speed = 0x701D;

	unsigned int i;
	for (i=0; i< sizeof(baudrates)/sizeof(baudrate_ex); i++)
	{
	  if (baudrates[i].uart_speed == uart_speed){
		  *rtk_speed = baudrates[i].rtk_speed;
	  	  return;
	  }
	}
	return;
}


/*******************************************************************************
**
** Function         hw_config_set_bdaddr
**
** Description      Program controller's Bluetooth Device Address
**
** Returns          TRUE, if valid address is sent
**                  FALSE, otherwise
**
*******************************************************************************/
static uint8_t hw_config_set_controller_baudrate(HC_BT_HDR *p_buf, uint16_t baudrate)
{
    uint8_t retval = FALSE;
    uint8_t *p = (uint8_t *) (p_buf + 1);

    UINT16_TO_STREAM(p, HCI_VSC_UPDATE_BAUDRATE);
    *p++ = 4; /* parameter length */
    UINT32_TO_STREAM(p, baudrate);    

    
    p_buf->len = HCI_CMD_PREAMBLE_SIZE + 4;
    hw_cfg_cb.state = HW_CFG_SET_UART_BAUD_HOST;

    retval = bt_vendor_cbacks->xmit_cb(HCI_VSC_UPDATE_BAUDRATE, p_buf, \
                                 hw_config_cback);

    return (retval);
}

static const char *get_firmware_name()
{
	static char firmware_file_name[PATH_MAX] = {0};
	sprintf(firmware_file_name, FIRMWARE_DIRECTORY"rtlbt_fw");
	return firmware_file_name;
}

uint32_t rtk_parse_config_file(unsigned char* config_buf, size_t filelen, char bt_addr[6])
{
	struct rtk_bt_vendor_config* config = (struct rtk_bt_vendor_config*) config_buf;
	uint16_t config_len = config->data_len, temp = 0;
	struct rtk_bt_vendor_config_entry* entry = config->entry;
	unsigned int i = 0;
       uint32_t baudrate = 0;


	
	bt_addr[0] = 0; //reset bd addr byte 0 to zero
	
	if (config->signature != RTK_VENDOR_CONFIG_MAGIC)
	{
		ALOGE("config signature magic number(%x) is not set to RTK_VENDOR_CONFIG_MAGIC", config->signature);
		return 0;
	}
	
	if (config_len != filelen - sizeof(struct rtk_bt_vendor_config))
	{
		ALOGE("config len(%x) is not right(%x)", config_len, filelen-sizeof(struct rtk_bt_vendor_config));
		return 0;
	}

	for (i=0; i<config_len;)
	{

		switch(entry->offset)
		{
			case 0x3c:
			{
				int j=0;
				for (j=0; j<entry->entry_len; j++)
					entry->entry_data[j] = bt_addr[entry->entry_len - 1 - j];
				break;
			}
			case 0xc:
			{
				baudrate = *(uint32_t*)entry->entry_data;
				if (entry->entry_len >= 12) //0ffset 0x18 - 0xc
				{
						gHwFlowControlEnable = (entry->entry_data[12] & 0x4) ? 1:0; //0x18 byte bit2
				}
				ALOGI("config baud rate to :%08x, hwflowcontrol:%x, %x", baudrate, entry->entry_data[12], gHwFlowControlEnable);
				break;
			}
			default:
				ALOGI("config offset(%x),length(%x)", entry->offset, entry->entry_len);
				break;
		}
		temp = entry->entry_len + sizeof(struct rtk_bt_vendor_config_entry);
		i += temp;
		entry = (struct rtk_bt_vendor_config_entry*)((uint8_t*)entry + temp);
	}

	
	return baudrate;	
}	


uint32_t rtk_get_bt_config(unsigned char** config_buf, uint32_t* config_baud_rate)
{
	char bt_config_file_name[PATH_MAX] = {0};
	uint8_t* bt_addr_temp = NULL;
	char bt_addr[6]={0x00, 0xe0, 0x4c, 0x88, 0x88, 0x88};
	struct stat st;
	size_t filelen;
	int fd;
	FILE* file = NULL;
    
	sprintf(bt_config_file_name, BT_CONFIG_DIRECTORY"rtlbt_config");

	if (stat(bt_config_file_name, &st) < 0)
	{
		ALOGE("can't access bt config file:%s, errno:%d\n", bt_config_file_name, errno);
		return -1;
	}

	filelen = st.st_size;
	

	if ((fd = open(bt_config_file_name, O_RDONLY)) < 0) {
		ALOGE("Can't open bt config file");
		return -1;
	}

	if ((*config_buf = malloc(filelen)) == NULL)
	{
		ALOGE("malloc buffer for config file fail(%x)\n", filelen);
		return -1;
	}

	//
	//we may need to parse this config file. 
	//for easy debug, only get need data.
	
	if (read(fd, *config_buf, filelen) < (ssize_t)filelen) {
		ALOGE("Can't load bt config file");
		free(*config_buf);	
		close(fd);
		return -1;
	}

	*config_baud_rate = rtk_parse_config_file(*config_buf, filelen, bt_addr);
	ALOGI("Get config baud rate from config file:%x", *config_baud_rate);

	close(fd);
	return filelen;
}

int rtk_get_bt_firmware(uint8_t** fw_buf, size_t addi_len)
{
	const char *filename = NULL;
	struct stat st;
	int fd = -1;
	size_t fwsize = 0;
       size_t buf_size = 0;

	filename = get_firmware_name();
	
	if (stat(filename, &st) < 0) {
		ALOGE("Can't access firmware, errno:%d", errno);
		return -1;
	}

	fwsize = st.st_size;
	buf_size = fwsize + addi_len; 

	if ((fd = open(filename, O_RDONLY)) < 0) {
		ALOGE("Can't open firmware, errno:%d", errno);
		return -1;
	}

	if (!(*fw_buf = malloc(buf_size))) {
		ALOGE("Can't alloc memory for fw&config, errno:%d", errno);
		return -1;
	}

	if (read(fd, *fw_buf, fwsize) < (ssize_t) fwsize) {
		free(*fw_buf);
		*fw_buf = NULL;
		return -1;
	}
	ALOGI("Load FW OK");
	return buf_size;
}


static int hci_download_patch_h4(HC_BT_HDR *p_buf, int index, uint8_t *data, int len)
{
    uint8_t retval = FALSE;
    uint8_t *p = (uint8_t *) (p_buf + 1); 
    
    UINT16_TO_STREAM(p, HCI_VSC_DOWNLOAD_FW_PATCH);
    *p++ = 1 + len;  /* parameter length */
    *p++ = index;
    memcpy(p, data, len);

    
    p_buf->len = HCI_CMD_PREAMBLE_SIZE + 1+len;
    
    hw_cfg_cb.state = HW_CFG_DL_FW_PATCH;

    retval = bt_vendor_cbacks->xmit_cb(HCI_VSC_DOWNLOAD_FW_PATCH, p_buf, \
                                 hw_config_cback);
    return retval;
}

static void rtk_download_fw_config(HC_BT_HDR *p_buf, uint8_t* buf, size_t filesize, int is_sent_changerate, int proto)
{

	uint8_t iCurIndex = 0;
	uint8_t iCurLen = 0;
	uint8_t iEndIndex = 0;
	uint8_t iLastPacketLen = 0;
	uint8_t iAdditionPkt = 0;
	uint8_t iTotalIndex = 0;

	unsigned char *bufpatch = NULL;

	iEndIndex = (uint8_t)((filesize-1)/PATCH_DATA_FIELD_MAX_SIZE);	
	iLastPacketLen = (filesize)%PATCH_DATA_FIELD_MAX_SIZE;

	if (is_sent_changerate)
		iAdditionPkt = (iEndIndex+2)%8?(8-(iEndIndex+2)%8):0;
	else
		iAdditionPkt = (iEndIndex+1)%8?(8-(iEndIndex+1)%8):0;

	iTotalIndex = iAdditionPkt + iEndIndex;
	rtk_patch.nTotal = iTotalIndex;	//init TotalIndex

	ALOGI("iEndIndex:%d  iLastPacketLen:%d iAdditionpkt:%d\n", iEndIndex, iLastPacketLen, iAdditionPkt);
	
	if (iLastPacketLen == 0) 
		iLastPacketLen = PATCH_DATA_FIELD_MAX_SIZE;
		
	bufpatch = buf;

	int i;
	for (i=0; i<=iTotalIndex; i++) {
		if (iCurIndex < iEndIndex) {
			iCurIndex = iCurIndex&0x7F;
			iCurLen = PATCH_DATA_FIELD_MAX_SIZE;
		}
		else if (iCurIndex == iEndIndex) {	//send last data packet			
			if (iCurIndex == iTotalIndex)
				iCurIndex = iCurIndex | 0x80;
			else
			iCurIndex = iCurIndex&0x7F;
			iCurLen = iLastPacketLen;
		}
		else if (iCurIndex < iTotalIndex) {
			iCurIndex = iCurIndex&0x7F;
			bufpatch = NULL;
			iCurLen = 0;
			//printf("addtional packet index:%d  iCurIndex:%d\n", i, iCurIndex);
		}		
		else {				//send end packet
			bufpatch = NULL;
			iCurLen = 0;
			iCurIndex = iCurIndex|0x80;
			//printf("end packet index:%d iCurIndex:%d\n", i, iCurIndex);			
		}	
		
		if (iCurIndex & 0x80)
			ALOGI("Send FW last command");

		if (proto == HCI_UART_H4) {
			iCurIndex = hci_download_patch_h4(p_buf, iCurIndex, bufpatch, iCurLen);
			if ((iCurIndex != i) && (i != rtk_patch.nTotal))
			{ 
			   // check index but ignore last pkt
			   ALOGE("index mismatch i:%d iCurIndex:%d, patch fail\n", i, iCurIndex);
			   return;
			}
		}
		else if(proto == HCI_UART_3WIRE) 
			//hci_download_patch(fd, iCurIndex, bufpatch, iCurLen);
			ALOGI("iHCI_UART_3WIRE");

		if (iCurIndex < iEndIndex) {
			bufpatch += PATCH_DATA_FIELD_MAX_SIZE;			
		}
		iCurIndex ++;	
	}

	//set last ack packet down
	if (proto == HCI_UART_3WIRE)
	{
		//rtk_send_pure_ack_down(fd);
	}
}




/*******************************************************************************
**
** Function         hw_config_cback
**
** Description      Callback function for controller configuration
**
** Returns          None
**
*******************************************************************************/
void hw_config_cback(void *p_mem)
{
    HC_BT_HDR *p_evt_buf = (HC_BT_HDR *) p_mem;
    char        *p_name, *p_tmp;
    uint8_t     *p, status;
    uint16_t    opcode;
    HC_BT_HDR  *p_buf=NULL;
    uint8_t     is_proceeding = FALSE;
    int         i;

    static int buf_len = 0;
    static uint8_t* buf = NULL;    
    static uint32_t baudrate = 0;   

    static uint8_t iCurIndex = 0;
    static uint8_t iCurLen = 0;
    static uint8_t iEndIndex = 0;
    static uint8_t iLastPacketLen = 0;
    static uint8_t iAdditionPkt = 0;
    static uint8_t iTotalIndex = 0;
    static unsigned char *bufpatch = NULL;

    static uint8_t iIndexRx = 0;
    
    uint8_t     *ph5_ctrl_pkt = NULL;
    uint16_t     h5_ctrl_pkt = 0;

                
#if (USE_CONTROLLER_BDADDR == TRUE)
    const uint8_t null_bdaddr[BD_ADDR_LEN] = {0,0,0,0,0,0};
#endif

    status = *((uint8_t *)(p_evt_buf + 1) + HCI_EVT_CMD_CMPL_STATUS_RET_BYTE);
    p = (uint8_t *)(p_evt_buf + 1) + HCI_EVT_CMD_CMPL_OPCODE;
    STREAM_TO_UINT16(opcode,p);


    /* Ask a new buffer big enough to hold any HCI commands sent in here */
    if ((status == 0) && bt_vendor_cbacks)
        p_buf = (HC_BT_HDR *) bt_vendor_cbacks->alloc(BT_HC_HDR_SIZE + \
                                                       HCI_CMD_MAX_LEN);

    if (p_buf != NULL)
    {
        p_buf->event = MSG_STACK_TO_HC_HCI_CMD;
        p_buf->offset = 0;
        p_buf->len = 0;
        p_buf->layer_specific = 0;

        p = (uint8_t *) (p_buf + 1);

        ALOGI("hw_cfg_cb.state = %i", hw_cfg_cb.state);
        switch (hw_cfg_cb.state)
        {
            case HW_CFG_START:
            {

                uint8_t*config_file_buf = NULL;    
                int config_len = -1;                
                   
                ALOGI("bt vendor lib:HW_CFG_START");
                //reset all static variable here
                buf_len = 0;
                buf = NULL;    
                baudrate = 0;   

                iCurIndex = 0;
                iCurLen = 0;
                iEndIndex = 0;
                iLastPacketLen = 0;
                iAdditionPkt = 0;
                iTotalIndex = 0;
                bufpatch = NULL;

                iIndexRx = 0;

                //download patch 
    
                //get efuse config file and patch code file          
                
                config_len = rtk_get_bt_config(&config_file_buf, &baudrate);
                if (config_len < 0)
                {
                    ALOGE("Get Config file error, just use efuse settings");
                    config_len = 0;
                }

                buf_len = rtk_get_bt_firmware(&buf, config_len);
                if (buf_len < 0)
                {
                    ALOGE("Get BT firmare error, continue without bt firmware");
                }
                else
                {
                    if (config_len)
                    {
                        memcpy(&buf[buf_len - config_len], config_file_buf, config_len);
                    }
                }

                if (config_file_buf)
                free(config_file_buf);

                ALOGI("Fw:%s exists, config file:%s exists", (buf_len > 0) ? "":"not", (config_len>0)?"":"not");

                if(buf_len > 0){
                    iEndIndex = (uint8_t)((buf_len-1)/PATCH_DATA_FIELD_MAX_SIZE);	
                    iLastPacketLen = (buf_len)%PATCH_DATA_FIELD_MAX_SIZE;
               
                    if (baudrate)
                        iAdditionPkt = (iEndIndex+2)%8?(8-(iEndIndex+2)%8):0;
                    else
                        iAdditionPkt = (iEndIndex+1)%8?(8-(iEndIndex+1)%8):0;

                    iTotalIndex = iAdditionPkt + iEndIndex;
                    rtk_patch.nTotal = iTotalIndex;	//init TotalIndex

                    ALOGI("iEndIndex:%d  iLastPacketLen:%d iAdditionpkt:%d\n", iEndIndex, iLastPacketLen, iAdditionPkt);

                    if (iLastPacketLen == 0) 
                        iLastPacketLen = PATCH_DATA_FIELD_MAX_SIZE;

                    bufpatch = buf;
                }
                else {
                    
                    bt_vendor_cbacks->dealloc(p_buf);
                    bt_vendor_cbacks->fwcfg_cb(BT_VND_OP_RESULT_SUCCESS);

                    hw_cfg_cb.state = 0;
                    is_proceeding = TRUE;      
                    break;
                }
                
                if ((buf_len>0) && (config_len == 0))
                {
                    goto DOWNLOAD_FW;
                }
                
            }
            /* fall through intentionally */

            case HW_CFG_SET_UART_BAUD_CONTROLLER:

                
                ALOGI("bt vendor lib: set CONTROLLER UART baud %x", baudrate);

                
                is_proceeding = hw_config_set_controller_baudrate(p_buf, baudrate);
                
    
                break;

            case HW_CFG_SET_UART_BAUD_HOST:
            {
                uint32_t HostBaudRate = 0;
 
                /* update baud rate of host's UART port */
                rtk_speed_to_uart_speed(baudrate, &HostBaudRate);
                ALOGI("bt vendor lib: set HOST UART baud %i", HostBaudRate);
                userial_vendor_set_baud( \
                    line_speed_to_userial_baud(HostBaudRate) \
                );
                ms_delay(100);
            }
             //fall through    
DOWNLOAD_FW:
            case HW_CFG_DL_FW_PATCH:        

                status = *((uint8_t *)(p_evt_buf + 1) + HCI_EVT_CMD_CMPL_STATUS_RET_BYTE);
                ALOGI("bt vendor lib: HW_CFG_DL_FW_PATCH status:%i, opcode:%x", status, opcode); 

                //recv command complete event for patch code download command
                if(opcode == HCI_VSC_DOWNLOAD_FW_PATCH){
                     iIndexRx = *((uint8_t *)(p_evt_buf + 1) + HCI_EVT_CMD_CMPL_STATUS_RET_BYTE + 1);
                     ALOGI("bt vendor lib: HW_CFG_DL_FW_PATCH status:%i, iIndexRx:%i", status, iIndexRx); 
                    //update buf of patch and index.
                    if (iCurIndex < iEndIndex) {
                        bufpatch += PATCH_DATA_FIELD_MAX_SIZE;			
                    }
                    iCurIndex ++;
                }
                 
                 if( (opcode ==HCI_VSC_DOWNLOAD_FW_PATCH)&&( iIndexRx&0x80 || iIndexRx == iTotalIndex) ){   
                    
                    ALOGI("vendor lib fwcfg completed");
                    if(buf) {
                        free(buf);
                        buf = NULL;
                    }
                    bt_vendor_cbacks->dealloc(p_buf);
                    bt_vendor_cbacks->fwcfg_cb(BT_VND_OP_RESULT_SUCCESS);

                    hw_cfg_cb.state = 0;
                    
                    if(gHwFlowControlEnable)
                    {
                        userial_vendor_set_hw_fctrl(1);
                    }
                    else
                    {
                        userial_vendor_set_hw_fctrl(0);
                    }
                    

                    if (hw_cfg_cb.fw_fd != -1)
                    {
                        close(hw_cfg_cb.fw_fd);
                        hw_cfg_cb.fw_fd = -1;
                    }
                    is_proceeding = TRUE;      
                    
                    break;
                 }
                 
                    if (iCurIndex < iEndIndex) {
                        	iCurIndex = iCurIndex&0x7F;
                        	iCurLen = PATCH_DATA_FIELD_MAX_SIZE;
                    }
                    else if (iCurIndex == iEndIndex) {	//send last data packet			
                    	if (iCurIndex == iTotalIndex)
                    		iCurIndex = iCurIndex | 0x80;
                    	else
                        	iCurIndex = iCurIndex&0x7F;
                    	iCurLen = iLastPacketLen;
                    }
                    else if (iCurIndex < iTotalIndex) {
                        	iCurIndex = iCurIndex&0x7F;
                        	bufpatch = NULL;
                        	iCurLen = 0;
                        	//printf("addtional packet index:%d  iCurIndex:%d\n", i, iCurIndex);
                    }		
                    else {				//send end packet
                    	bufpatch = NULL;
                    	iCurLen = 0;
                    	iCurIndex = iCurIndex|0x80;
                    	//printf("end packet index:%d iCurIndex:%d\n", i, iCurIndex);			
                    }	

                    if (iCurIndex & 0x80)
                    	ALOGI("Send FW last command");

                    ALOGI("iCurIndex = %i, iCurLen = %i", iCurIndex, iCurLen);    
                    
                    is_proceeding = hci_download_patch_h4(p_buf, iCurIndex, bufpatch, iCurLen);

                   
                break;
                
                default:
                    break;
        } // switch(hw_cfg_cb.state)
    } // if (p_buf != NULL)

    /* Free the RX event buffer */
    if (bt_vendor_cbacks)
        bt_vendor_cbacks->dealloc(p_evt_buf);

    if (is_proceeding == FALSE)
    {
        ALOGE("vendor lib fwcfg aborted!!!");
        if (bt_vendor_cbacks)
        {
            if (p_buf != NULL)
                bt_vendor_cbacks->dealloc(p_buf);

            bt_vendor_cbacks->fwcfg_cb(BT_VND_OP_RESULT_FAIL);
        }

        if (hw_cfg_cb.fw_fd != -1)
        {
            close(hw_cfg_cb.fw_fd);
            hw_cfg_cb.fw_fd = -1;
        }

        hw_cfg_cb.state = 0;
    }
}

/******************************************************************************
**   LPM Static Functions
******************************************************************************/

/*******************************************************************************
**
** Function         hw_lpm_ctrl_cback
**
** Description      Callback function for lpm enable/disable rquest
**
** Returns          None
**
*******************************************************************************/
void hw_lpm_ctrl_cback(void *p_mem)
{
    HC_BT_HDR *p_evt_buf = (HC_BT_HDR *) p_mem;
    bt_vendor_op_result_t status = BT_VND_OP_RESULT_FAIL;

    if (*((uint8_t *)(p_evt_buf + 1) + HCI_EVT_CMD_CMPL_STATUS_RET_BYTE) == 0)
    {
        status = BT_VND_OP_RESULT_SUCCESS;
    }

    if (bt_vendor_cbacks)
    {
        bt_vendor_cbacks->lpm_cb(status);
        bt_vendor_cbacks->dealloc(p_evt_buf);
    }
}




/*****************************************************************************
**   Hardware Configuration Interface Functions
*****************************************************************************/


/*******************************************************************************
**
** Function        hw_config_start
**
** Description     Kick off controller initialization process
**
** Returns         None
**
*******************************************************************************/
void hw_config_start(void)
{
    HC_BT_HDR  *p_buf = NULL;
    uint8_t     *p;

    hw_cfg_cb.state = 0;
    hw_cfg_cb.fw_fd = -1;
    hw_cfg_cb.f_set_baud_2 = FALSE;


    /* Start from sending H5 SYNC */

    if (bt_vendor_cbacks)
    {
        p_buf = (HC_BT_HDR *) bt_vendor_cbacks->alloc(BT_HC_HDR_SIZE + \
                                                       2);
    }

    if (p_buf)
    {
        p_buf->event = MSG_STACK_TO_HC_HCI_CMD;
        p_buf->offset = 0;
        p_buf->layer_specific = 0;
        p_buf->len = 2;

        p = (uint8_t *) (p_buf + 1);
        UINT16_TO_STREAM(p, HCI_VSC_H5_INIT);

        hw_cfg_cb.state = HW_CFG_START;
 	 ALOGI("hw_config_start");
        bt_vendor_cbacks->xmit_cb(HCI_VSC_H5_INIT, p_buf, hw_config_cback);
    }
    else
    {
        if (bt_vendor_cbacks)
        {
            ALOGE("vendor lib fw conf aborted [no buffer]");
            bt_vendor_cbacks->fwcfg_cb(BT_VND_OP_RESULT_FAIL);
        }
    }
}


