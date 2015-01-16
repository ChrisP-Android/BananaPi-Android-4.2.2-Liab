#ifndef _MAG_CALIBRATE_H
#define  _MAG_CALIBRATE_H

#define  LOG_PRINT_RAW_DATA				(0x01)
#define  LOG_PRINT_CALIBRATED_DATA		(0x01 << 1)
#define  LOG_PRINT_FITTERROR_DATA       (0x01 << 2)
#define  LOG_PRINT_ALL_DATA				(LOG_PRINT_RAW_DATA | LOG_PRINT_CALIBRATED_DATA |LOG_PRINT_FITTERROR_DATA)

extern void   getcfg();
extern int    setAccuracyThreshold(float threshold);
extern int    setLogPrintSetting(int printLogTag);
extern int    getcalib();
extern void   write_calib();
extern int    returnCalibratedData(float *pfBfx, float *pfBfy, float *pfBfz,float *pLPPsi, float *pLPThe,float *pLPPhi);
extern void   HouseKeep(int magX, int magY, int magZ, int accX, int accY, int accZ);
extern void   ResetFunc();
extern void   setuTperBit(float upb);
#endif
