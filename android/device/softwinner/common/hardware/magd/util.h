/*
* Copyright (c) 2008 The Android Open Source Project
* Copyright (c) 2012 Freescale Semiconductor, Inc. All rights reserved.
*
* This software program is proprietary program of Freescale Semiconductor
* licensed to authorized Licensee under Software License Agreement (SLA)
* executed between the Licensee and Freescale Semiconductor.
* 
* Use of the software by unauthorized third party, or use of the software 
* beyond the scope of the SLA is strictly prohibited.
* 
* software distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#ifndef _SENSOR_UTIL_H_
#define _SENSOR_UTIL_H_

#ifdef DEBUG
#define DBG(fmt,arg...)  LOGD(fmt,arg...)
#else
#define DBG(fmt,arg...)
#endif

#define TRUE "true"
#define FALSE "false"
#define GSENSOR_NAME "gsensor_name"
#define GSENSOR_DIRECTX "gsensor_direct_x"
#define GSENSOR_DIRECTY "gsensor_direct_y"
#define GSENSOR_DIRECTZ "gsensor_direct_z"
#define GSENSOR_XY "gsensor_xy_revert"
#define GSENSOR_CONFIG_PATH    "/system/usr/gsensor.cfg"
#define LINE_LENGTH  (128)

#define SENSOR_ACCEL   		1
#define SENSOR_MAG     		0
#define SENSORS_NUM    		2

#define ACCEL_INPUT_NAME 	"FreescaleAccelerometer"
#define MAG_INPUT_NAME   	"FreescaleMagnetometer"
#define ECOMPASS_INPUT_NAME 	"eCompass"

#define ABS_STATUS ABS_WHEEL
//#define DEBUG_SENSOR
 
#define INPUT_DEV_READ_ONLY  0
#define INPUT_DEV_WRITED	 1
int  init_sensors(const char *acc_name, const char *mag_name);
int  read_sensor_data(short *pAccX, short *pAccY, short *pAccZ,short *pMagX, short *pMagY, short *pMagZ);
void inject_calibrated_data(int *iBfx, int *iBfy, int *iBfz,int *pLPPsi, int *pLPThe,int *pLPPhi,int status);
void deinit_sensors();

#endif
