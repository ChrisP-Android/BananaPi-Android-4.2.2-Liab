/*
 * shortVideoDesc.h
 * 
 *  Synopsys Inc.
 *  SG DWC PT02
 */

#ifndef SHORTVIDEODESC_H_
#define SHORTVIDEODESC_H_

#include "../util/types.h"

/**
 * @file
 * Short Video Descriptor.
 * Parse and hold Short Video Descriptors found in Video Data Block in EDID.
 */
/** For detailed handling of this structure, refer to documentation of the functions */
typedef struct
{
    int mNative;
    unsigned mCode;
} shortVideoDesc_t;

void shortVideoDesc_Reset(shortVideoDesc_t *svd);

int shortVideoDesc_Parse(shortVideoDesc_t *svd, u8 data);
/**
 * @return the video code (VIC) defined in CEA-861-(D or later versions)
 */
unsigned shortVideoDesc_GetCode(shortVideoDesc_t *svd);

/**
 * @return TRUE if video mode is native to sink
 */
int shortVideoDesc_GetNative(shortVideoDesc_t *svd);

#endif /* SHORTVIDEODESC_H_ */
