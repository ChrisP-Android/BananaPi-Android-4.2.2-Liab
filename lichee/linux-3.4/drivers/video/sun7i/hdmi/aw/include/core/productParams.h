/*
 * productParams.h
 *
 *  Created on: Jul 6, 2010
 * 
 * Synopsys Inc.
 * SG DWC PT02
 */

#ifndef PRODUCTPARAMS_H_
#define PRODUCTPARAMS_H_

#include "../util/types.h"

/** For detailed handling of this structure, refer to documentation of the functions */
typedef struct
{
    /* Vendor Name of eight 7-bit ASCII characters */
    u8 mVendorName[8];

    u8 mVendorNameLength;

    /* Product name or description, consists of sixteen 7-bit ASCII characters */
    u8 mProductName[16];

    u8 mProductNameLength;

    /* Code that classifies the source device (CEA Table 15) */
    u8 mSourceType;

    /* oui 24 bit IEEE Registration Identifier */
    u32 mOUI;

    u8 mVendorPayload[24];

    u8 mVendorPayloadLength;

} productParams_t;

void productParams_Reset(productParams_t *params);

u8 productParams_SetProductName(productParams_t *params, const u8 * data,
        u8 length);

u8 * productParams_GetProductName(productParams_t *params);

u8 productParams_GetProductNameLength(productParams_t *params);

u8 productParams_SetVendorName(productParams_t *params, const u8 * data,
        u8 length);

u8 * productParams_GetVendorName(productParams_t *params);

u8 productParams_GetVendorNameLength(productParams_t *params);

u8 productParams_SetSourceType(productParams_t *params, u8 value);

u8 productParams_GetSourceType(productParams_t *params);

u8 productParams_SetOUI(productParams_t *params, u32 value);

u32 productParams_GetOUI(productParams_t *params);

u8 productParams_SetVendorPayload(productParams_t *params, const u8 * data,
        u8 length);

u8 * productParams_GetVendorPayload(productParams_t *params);

u8 productParams_GetVendorPayloadLength(productParams_t *params);

u8 productParams_IsSourceProductValid(productParams_t *params);

u8 productParams_IsVendorSpecificValid(productParams_t *params);

#endif /* PRODUCTPARAMS_H_ */
