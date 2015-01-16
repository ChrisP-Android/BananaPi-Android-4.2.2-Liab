/*
 * shortAudioDesc.h
 *
 *  Created on: Jul 22, 2010
  * 
 * Synopsys Inc.
 * SG DWC PT02
 */

#ifndef SHORTAUDIODESC_H_
#define SHORTAUDIODESC_H_

#include "../util/types.h"
/**
 * @file
 * Short Audio Descriptor.
 * Found in Audio Data Block (shortAudioDesc_t *sad, CEA Data Block Tage Code 1).
 * Parse and hold information from EDID data structure
 */
/** For detailed handling of this structure, refer to documentation of the functions */
typedef struct
{
    u8 mFormat;
    u8 mMaxChannels;
    u8 mSampleRates;
    u8 mByte3;
} shortAudioDesc_t;

void shortAudioDesc_Reset(shortAudioDesc_t *sad);

/**
 * Parse Short Audio Descriptor
 */
int shortAudioDesc_Parse(shortAudioDesc_t *sad, u8 * data);

/**
 * This byte is type dependent and is " further defined in other format-specific documents"
 * For LPCM (shortAudioDesc_t *sad, code 0001)  it holds the sample size
 * for audio codes (shortAudioDesc_t *sad, 2- 8) it holds "Maximum bit rate divided by 8 kHz"
 * for other audio codes: [Default = 0, unless Defined by Audio Codec Vendor]
 * @return Byte 3 from the descriptor
 */
u8 shortAudioDesc_GetByte3(shortAudioDesc_t *sad);

/**
 * @return the audio format code (shortAudioDesc_t *sad, refer to Table 37 in CEA-861-D)
 */

u8 shortAudioDesc_GetFormatCode(shortAudioDesc_t *sad);

/**
 * @return the maximum number of channels  - 1
 */
u8 shortAudioDesc_GetMaxChannels(shortAudioDesc_t *sad);

/**
 *@return the sample rates byte, where bit 7 is always 0 and rates are sorted respectively starting with bit 6:
 * 192 kHz  176.4 kHz  96 kHz  88.2 kHz  48 kHz  44.1 kHz  32 kHz
 */
u8 shortAudioDesc_GetSampleRates(shortAudioDesc_t *sad);

int shortAudioDesc_Support32k(shortAudioDesc_t *sad);

int shortAudioDesc_Support44k1(shortAudioDesc_t *sad);

int shortAudioDesc_Support48k(shortAudioDesc_t *sad);

int shortAudioDesc_Support88k2(shortAudioDesc_t *sad);

int shortAudioDesc_Support96k(shortAudioDesc_t *sad);

int shortAudioDesc_Support176k4(shortAudioDesc_t *sad);

int shortAudioDesc_Support192k(shortAudioDesc_t *sad);

/**
 * @return the maximum bit rate divided by 8KHz for formats of codes 2 - 8
 */
u8 shortAudioDesc_GetMaxBitRate(shortAudioDesc_t *sad);

int shortAudioDesc_Support16bit(shortAudioDesc_t *sad);

int shortAudioDesc_Support20bit(shortAudioDesc_t *sad);

int shortAudioDesc_Support24bit(shortAudioDesc_t *sad);

#endif /* SHORTAUDIODESC_H_ */
