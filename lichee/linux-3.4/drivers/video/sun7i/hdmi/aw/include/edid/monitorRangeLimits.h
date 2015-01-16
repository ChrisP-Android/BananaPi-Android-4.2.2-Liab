/*
 * monitorRangeLimits.h
 *
 *  Created on: Jul 22, 2010
 * 
 * Synopsys Inc.
 * SG DWC PT02
 */

#ifndef MONITORRANGELIMITS_H_
#define MONITORRANGELIMITS_H_

#include "../util/types.h"
/**
 * @file
 * Second Monitor Descriptor
 * Parse and hold Monitor Range Limits information read from EDID
 */

typedef struct
{
    u8 mMinVerticalRate;
    u8 mMaxVerticalRate;
    u8 mMinHorizontalRate;
    u8 mMaxHorizontalRate;
    u8 mMaxPixelClock;
    int mValid;
} monitorRangeLimits_t;

void monitorRangeLimits_Reset(monitorRangeLimits_t * mrl);

int monitorRangeLimits_Parse(monitorRangeLimits_t * mrl, u8 * data);

/**
 * @return the maximum parameter for horizontal frequencies
 */
u8 monitorRangeLimits_GetMaxHorizontalRate(monitorRangeLimits_t * mrl);
/**
 * @return the maximum parameter for pixel clock rate
 */
u8 monitorRangeLimits_GetMaxPixelClock(monitorRangeLimits_t * mrl);
/**
 * @return the maximum parameter for vertical frequencies
 */
u8 monitorRangeLimits_GetMaxVerticalRate(monitorRangeLimits_t * mrl);
/**
 * @return the minimum parameter for horizontal frequencies
 */
u8 monitorRangeLimits_GetMinHorizontalRate(monitorRangeLimits_t * mrl);
/**
 * @return the minimum parameter for vertical frequencies
 */
u8 monitorRangeLimits_GetMinVerticalRate(monitorRangeLimits_t * mrl);

#endif /* MONITORRANGELIMITS_H_ */
