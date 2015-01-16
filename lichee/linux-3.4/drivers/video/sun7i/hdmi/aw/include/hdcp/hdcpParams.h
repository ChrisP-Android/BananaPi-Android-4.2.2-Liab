/*
 * hdcpParams.h
 *
 *  Created on: Jul 20, 2010
 *
 *  Synopsys Inc.
 *  SG DWC PT02 
 */

#ifndef HDCPPARAMS_H_
#define HDCPPARAMS_H_

typedef struct
{
    int mEnable11Feature;
    int mRiCheck;
    int mI2cFastMode;
    int mEnhancedLinkVerification;
} hdcpParams_t;

void hdcpParams_Reset(hdcpParams_t *params);

int hdcpParams_GetEnable11Feature(hdcpParams_t *params);

int hdcpParams_GetRiCheck(hdcpParams_t *params);

int hdcpParams_GetI2cFastMode(hdcpParams_t *params);

int hdcpParams_GetEnhancedLinkVerification(hdcpParams_t *params);

void hdcpParams_SetEnable11Feature(hdcpParams_t *params, int value);

void hdcpParams_SetEnhancedLinkVerification(hdcpParams_t *params, int value);

void hdcpParams_SetI2cFastMode(hdcpParams_t *params, int value);

void hdcpParams_SetRiCheck(hdcpParams_t *params, int value);

#endif /* HDCPPARAMS_H_ */
