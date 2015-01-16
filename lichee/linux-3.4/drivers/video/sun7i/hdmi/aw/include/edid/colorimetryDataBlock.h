/*
 * colorimetryDataBlock.h
 *
 *  Created on: Jul 22, 2010
 * 
 * Synopsys Inc.
 * SG DWC PT02
 */

#ifndef COLORIMETRYDATABLOCK_H_
#define COLORIMETRYDATABLOCK_H_

#include "../util/types.h"

/**
 * @file
 * Colorimetry Data Block class.
 * Holds and parses the Colorimetry datablock information.
 */

typedef struct
{
    u8 mByte3;
    u8 mByte4;
    int mValid;

} colorimetryDataBlock_t;

void colorimetryDataBlock_Reset(colorimetryDataBlock_t * cdb);

int colorimetryDataBlock_Parse(colorimetryDataBlock_t * cdb, u8 * data);

int colorimetryDataBlock_SupportsXvYcc709(colorimetryDataBlock_t * cdb);

int colorimetryDataBlock_SupportsXvYcc601(colorimetryDataBlock_t * cdb);

int colorimetryDataBlock_SupportsMetadata0(colorimetryDataBlock_t * cdb);

int colorimetryDataBlock_SupportsMetadata1(colorimetryDataBlock_t * cdb);

int colorimetryDataBlock_SupportsMetadata2(colorimetryDataBlock_t * cdb);

#endif /* COLORIMETRYDATABLOCK_H_ */
