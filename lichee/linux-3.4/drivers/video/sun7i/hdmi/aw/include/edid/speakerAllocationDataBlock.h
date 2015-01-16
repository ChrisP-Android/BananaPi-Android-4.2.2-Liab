/*
 * speakerAllocationDataBlock.h
 *
 *  Created on: Jul 22, 2010
  * 
 * Synopsys Inc.
 * SG DWC PT02
 */

#ifndef SPEAKERALLOCATIONDATABLOCK_H_
#define SPEAKERALLOCATIONDATABLOCK_H_

#include "../util/types.h"

/** 
 * @file
 * SpeakerAllocation Data Block.
 * Holds and parse the Speaker Allocation datablock information.
 * For detailed handling of this structure, refer to documentation of the functions
 */

typedef struct
{
    u8 mByte1;
    int mValid;
} speakerAllocationDataBlock_t;

void speakerAllocationDataBlock_Reset(speakerAllocationDataBlock_t * sadb);

int speakerAllocationDataBlock_Parse(speakerAllocationDataBlock_t * sadb,u8 * data);

int speakerAllocationDataBlock_SupportsFlFr(speakerAllocationDataBlock_t * sadb);

int speakerAllocationDataBlock_SupportsLfe(speakerAllocationDataBlock_t * sadb);

int speakerAllocationDataBlock_SupportsFc(speakerAllocationDataBlock_t * sadb);

int speakerAllocationDataBlock_SupportsRlRr(speakerAllocationDataBlock_t * sadb);

int speakerAllocationDataBlock_SupportsRc(speakerAllocationDataBlock_t * sadb);

int speakerAllocationDataBlock_SupportsFlcFrc(speakerAllocationDataBlock_t * sadb);

int speakerAllocationDataBlock_SupportsRlcRrc(speakerAllocationDataBlock_t * sadb);
/**
 * @return the Channel Allocation code used in the Audio Infoframe to ease the translation process
 */
u8 speakerAllocationDataBlock_GetChannelAllocationCode(speakerAllocationDataBlock_t * sadb);
/**
 * @return the whole byte of Speaker Allocation DataBlock where speaker allocation is indicated. User wishing to access and interpret it must know specifically how to parse it.
 */
u8 speakerAllocationDataBlock_GetSpeakerAllocationByte(speakerAllocationDataBlock_t * sadb);

#endif /* SPEAKERALLOCATIONDATABLOCK_H_ */
