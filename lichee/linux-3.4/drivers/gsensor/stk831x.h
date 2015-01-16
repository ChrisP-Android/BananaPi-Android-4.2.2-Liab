/*
 * Definitions for Sensortek stk8312 accelerometer
 */
#ifndef _STK831X_H_
#define _STK831X_H_

#include <linux/ioctl.h>
#define STK831X_I2C_NAME		"stk831x"
#define STKDIR				0x3D
/******************************** stk8312 reg *****************************************************/
#define ACC_STK8312_NAME		"stk8312"
#define STK_LSB_1G_8312		        21
#define STK_ZG_COUNT_8312	        1
#define STK_TUNE_XYOFFSET_8312          3
#define STK_TUNE_ZOFFSET_8312           6
#define STK_TUNE_NOISE_8312             5
/* registers for stk8312 registers */

#define	STK8312_XOUT	                0x00	/* x-axis acceleration*/
#define	STK8312_YOUT	                0x01	/* y-axis acceleration*/
#define	STK8312_ZOUT	                0x02	/* z-axis acceleration*/
#define	STK8312_TILT	 	        0x03	/* Tilt Status */
#define	STK8312_SRST	                0x04	/* Sampling Rate Status */
#define	STK8312_SPCNT	                0x05	/* Sleep Count */
#define	STK8312_INTSU	                0x06	/* Interrupt setup*/
#define	STK8312_MODE	                0x07
#define	STK8312_SR		        0x08	/* Sample rate */
#define	STK8312_PDET	                0x09	/* Tap Detection */
#define	STK8312_DEVID	                0x0B	/* Device ID */
#define	STK8312_OFSX	                0x0C	/* X-Axis offset */
#define	STK8312_OFSY	                0x0D	/* Y-Axis offset */
#define	STK8312_OFSZ	                0x0E	/* Z-Axis offset */
#define	STK8312_PLAT	                0x0F	/* Tap Latency */
#define	STK8312_PWIN	                0x10	/* Tap Window */	
#define	STK8312_FTH		        0x11	/* Free-Fall Threshold */
#define	STK8312_FTM	                0x12	/* Free-Fall Time */
#define	STK8312_STH	                0x13	/* Shake Threshold */
#define	STK8312_CTRL	                0x14	/* Control Register */
#define	STK8312_RESET	                0x20	/*software reset*/
/************************   stk8312 end ********************************************************/

/************************   stk8313 reg ********************************************************/
#define ACC_STK8313_NAME	        "stk8313"
#define STK_LSB_1G_8313			256
#define STK_ZG_COUNT_8313	        4
#define STK_TUNE_XYOFFSET_8313          35
#define STK_TUNE_ZOFFSET_8313           75
#define STK_TUNE_NOISE_8313             20
/* register for stk8313 registers */

#define	STK8313_XOUT	                0x00
#define	STK8313_YOUT	                0x02
#define	STK8313_ZOUT	                0x04
#define	STK8313_TILT	                0x06	/* Tilt Status */
#define	STK8313_SRST	                0x07	/* Sampling Rate Status */
#define	STK8313_SPCNT	                0x08	/* Sleep Count */
#define	STK8313_INTSU	                0x09	/* Interrupt setup*/
#define	STK8313_MODE	                0x0A
#define	STK8313_SR	                0x0B	/* Sample rate */
#define	STK8313_PDET	                0x0C	/* Tap Detection */
#define	STK8313_DEVID	                0x0E	/* Device ID */
#define	STK8313_OFSX	                0x0F	/* X-Axis offset */
#define	STK8313_OFSY	                0x10	/* Y-Axis offset */
#define	STK8313_OFSZ	                0x11	/* Z-Axis offset */
#define	STK8313_PLAT	                0x12	/* Tap Latency */
#define	STK8313_PWIN	                0x13	/* Tap Window */	
#define	STK8313_FTH	                0x14	/* Fre	e-Fall Threshold */
#define	STK8313_FTM	                0x15	/* Free-Fall Time */
#define	STK8313_STH	                0x16	/* Shake Threshold */
#define	STK8313_ISTMP	                0x17	/* Interrupt Setup */
#define STK8313_INTMAP	                0x18	/*Interrupt Map*/
#define	STK8313_RESET	                0x20	/*software reset*/

/**************************stk8313 end ********************************************************************/

struct stk_device_info{
        int     stk_lsb_1g;
        int     stk_zg_cout;
        int     stk_tune_xyoffset;
        int     stk_tune_zoffset;
        int     stk_tune_noise;
        char    stk831x_xout;
        char    stk831x_yout;
        char    stk831x_zout;
        char    stk831x_tilt;
        char    stk831x_srst;
        char    stk831x_spcnt;
        char    stk831x_intsu;
        char    stk831x_mode;
        char    stk831x_sr;
        char    stk831x_pdet;
        char    stk831x_devid;
        char    stk831x_ofsx;
        char    stk831x_ofsy;
        char    stk831x_ofsz;
        char    stk831x_plat;
        char    stk831x_pwin;
        char    stk831x_fth;
        char    stk831x_ftm;
        char    stk831x_sth;
        char    stk831x_reset;       
        
        /*8312*/
        char    stk831x_ctrl;

        /*8313*/
        char    stk831x_istmp;
        char    stk831x_intmap;
};


/* IOCTLs*/
#define STK_IOCTL_WRITE		        _IOW(STKDIR, 0x01, char[8])
#define STK_IOCTL_READ		        _IOWR(STKDIR, 0x02, char[8])
#define STK_IOCTL_SET_ENABLE	        _IOW(STKDIR, 0x03, char)
#define STK_IOCTL_GET_ENABLE	        _IOR(STKDIR, 0x04, char)
#define STK_IOCTL_SET_DELAY	        _IOW(STKDIR, 0x05, char)
#define STK_IOCTL_GET_DELAY	        _IOR(STKDIR, 0x06, char)
#define STK_IOCTL_SET_OFFSET	        _IOW(STKDIR, 0x07, char[3])
#define STK_IOCTL_GET_OFFSET	        _IOR(STKDIR, 0x08, char[3])
#define STK_IOCTL_GET_ACCELERATION	_IOR(STKDIR, 0x09, int[3])
#define STK_IOCTL_SET_RANGE	        _IOW(STKDIR, 0x10, char)
#define STK_IOCTL_GET_RANGE	        _IOR(STKDIR, 0x11, char)
#define STK_IOCTL_SET_CALI	        _IOW(STKDIR, 0x12, char)


#endif