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

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <poll.h>
#include "util.h"
#include "magcalibrate.h"
#include "getopt.h"

#define LOG_TAG "MagDaemon"
#include <cutils/log.h>

#define MAG_CONVT 	2.0f
#define ORI_CONV   	100.0f   //should rely on the CONVERT value in HAL
#define FSL_MAG_FILE "/data/misc/fsl_mag.txt"

static  float mThreshold = 100.0f;
static  int   PrintLogTag = 0;

static int magdreadcmdline(int argc ,char ** argv)
{
   
   int oc;					  
   char *b_opt_arg; 
   PrintLogTag = 0;
   while((oc = getopt(argc, argv, "a:rcfl")) != -1) {
	 switch(oc)
	 {
		case 'a':
			mThreshold = atof(optarg);
		    if(mThreshold <= 0.0f)
			   mThreshold = 100.0f;
			   ALOGD("a value is %s\n",optarg);
			   break;
			case 'r':
			   PrintLogTag |= LOG_PRINT_RAW_DATA;
			   break;
			case 'c':
			   PrintLogTag |= LOG_PRINT_CALIBRATED_DATA;				  
			   break;
			case 'f':
			   PrintLogTag |= LOG_PRINT_FITTERROR_DATA;		
			   break;
			case 'l':
			   PrintLogTag |= LOG_PRINT_ALL_DATA;
			   break;
			case '?':
			   PrintLogTag = 0;
			   mThreshold = 100.0f;
			   break;
		  }
	  }
   return 0;
}

int main(int argc ,char **argv)
{
  
	int rv, is_updated = 0;
	short rmagX, rmagY, rmagZ, raccX, raccY, raccZ;
	float fBfx, fBfy,fBfz;
	int iBfx, iBfy,iBfz;
	float LPPsi,LPThe,LPPhi;
	int iLPPsi,iLPThe,iLPPhi;
	int status = 0;
	if (init_sensors(ACCEL_INPUT_NAME, MAG_INPUT_NAME) < 0){
		 ALOGD("inital sensor error\n");
	     return -1;
	}
	ALOGD("magnetic sensor calibrate daemon start\n");
        magdreadcmdline(argc,argv);
	ResetFunc();
	getcalib();
	setAccuracyThreshold(mThreshold);
	setLogPrintSetting(PrintLogTag);
	ALOGD("magnetic calibrate daemon set threshold %f\n",mThreshold);
	while (1) {
	   rv = read_sensor_data(&raccX, &raccY, &raccZ, &rmagX, &rmagY, &rmagZ);
	   if(rv || (raccX == 0 && raccY == 0 && raccZ == 0))
	   {
		  ALOGD("read sensor data error ,cancel this data");
		  usleep(10000);
		  continue;
	   }
           HouseKeep(rmagX,rmagY,rmagZ,raccX,raccY,raccZ);
	   status = returnCalibratedData(&fBfx, &fBfy, &fBfz, &LPPsi, &LPThe,&LPPhi);
	   iBfx   =  static_cast<int>(fBfx * MAG_CONVT);
	   iBfy   =  static_cast<int>(fBfy * MAG_CONVT);
	   iBfz   =  static_cast<int>(fBfz * MAG_CONVT);
	   iLPPsi =  static_cast<int>(LPPsi * ORI_CONV);
	   iLPThe =  static_cast<int>(LPThe * ORI_CONV);
	   iLPPhi =  static_cast<int>(LPPhi * ORI_CONV);
	   inject_calibrated_data(&iBfx,&iBfy,&iBfz,&iLPPsi,&iLPThe,&iLPPhi,status);
	   is_updated++;
	   if(is_updated>=100)
	   {
		is_updated=0;
		write_calib();
	   }
	   usleep(5000);
	}
  	deinit_sensors();
	return 0;
}
