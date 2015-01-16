/*
 * dtd.h
 *
 *  Created on: Jul 5, 2010
 *
 *  Synopsys Inc.
 *  SG DWC PT02 
 */

#ifndef DTD_H_
#define DTD_H_

#include "../util/types.h"

/**
 * @file
 * For detailed handling of this structure, refer to documentation of the functions
 */
typedef struct
{
    /** VIC code */
    u8 mCode;
    u16 mPixelRepetitionInput;
    u16 mPixelClock;
    u8 mInterlaced;
    u16 mHActive;
    u16 mHBlanking;
    u16 mHBorder;
    u16 mHImageSize;
    u16 mHSyncOffset;
    u16 mHSyncPulseWidth;
    u8 mHSyncPolarity;
    u16 mVActive;
    u16 mVBlanking;
    u16 mVBorder;
    u16 mVImageSize;
    u16 mVSyncOffset;
    u16 mVSyncPulseWidth;
    u8 mVSyncPolarity;
} dtd_t;
/**
 * Parses the Detailed Timing Descriptor.
 * Encapsulating the parsing process
 * @param dtd pointer to dtd_t strucutute for the information to be save in
 * @param data a pointer to the 18-byte structure to be parsed.
 * @return TRUE if success
 */
int dtd_Parse(dtd_t *dtd, u8 data[18]);

/**
 * @param dtd pointer to dtd_t strucutute for the information to be save in
 * @param code the CEA 861-D video code.
 * @param refreshRate the specified vertical refresh rate.
 * @return TRUE if success
 */
int dtd_Fill(dtd_t *dtd, u8 code, u32 refreshRate);
/**
 * @param dtd pointer to dtd_t strucutute where the information is held
 * @return the CEA861-D code if the DTD is listed in the spec
 * @return < 0 when the code has an undefined DTD
 */
u8 dtd_GetCode(const dtd_t *dtd);
/**
 * @param dtd pointer to dtd_t strucutute where the information is held
 * @return the input pixel reptition
 */
u16 dtd_GetPixelRepetitionInput(const dtd_t *dtd);
/**
 * @param dtd pointer to dtd_t strucutute where the information is held
 * @return the pixel clock rate of the DTD
 */
u16 dtd_GetPixelClock(const dtd_t *dtd);
/**
 * @param dtd pointer to dtd_t strucutute where the information is held
 * @return 1 if the DTD is of an interlaced viedo format
 */
u8 dtd_GetInterlaced(const dtd_t *dtd);
/**
 * @param dtd pointer to dtd_t strucutute where the information is held
 * @return the horizontal active pixels (addressable video)
 */
u16 dtd_GetHActive(const dtd_t *dtd);
/**
 * @param dtd pointer to dtd_t strucutute where the information is held
 * @return the horizontal blanking pixels
 */
u16 dtd_GetHBlanking(const dtd_t *dtd);
/**
 * @param dtd pointer to dtd_t strucutute where the information is held
 * @return the horizontal border in pixels
 */
u16 dtd_GetHBorder(const dtd_t *dtd);
/**
 * @param dtd pointer to dtd_t strucutute where the information is held
 * @return the horizontal image size in mm
 */
u16 dtd_GetHImageSize(const dtd_t *dtd);
/**
 * @param dtd pointer to dtd_t strucutute where the information is held
 * @return the horizontal sync offset (front porch) from blanking start to start of sync in pixels
 */
u16 dtd_GetHSyncOffset(const dtd_t *dtd);
/**
 * @param dtd pointer to dtd_t strucutute where the information is held
 * @return the horizontal sync polarity
 */
u8 dtd_GetHSyncPolarity(const dtd_t *dtd);
/**
 * @param dtd pointer to dtd_t strucutute where the information is held
 * @return the horizontal sync pulse width in pixels
 */
u16 dtd_GetHSyncPulseWidth(const dtd_t *dtd);
/**
 * @param dtd pointer to dtd_t strucutute where the information is held
 * @return the vertical active pixels (addressable video)
 */
u16 dtd_GetVActive(const dtd_t *dtd);
/**
 * @param dtd pointer to dtd_t strucutute where the information is held
 * @return the vertical border in pixels
 */
u16 dtd_GetVBlanking(const dtd_t *dtd);
/**
 * @param dtd pointer to dtd_t strucutute where the information is held
 * @return the horizontal image size
 */
u16 dtd_GetVBorder(const dtd_t *dtd);
/**
 * @param dtd pointer to dtd_t strucutute where the information is held
 * @return the vertical image size in mm
 */
u16 dtd_GetVImageSize(const dtd_t *dtd);
/**
 * @param dtd pointer to dtd_t strucutute where the information is held
 * @return the vertical sync offset (front porch) from blanking start to start
 *  of sync in lines
 */
u16 dtd_GetVSyncOffset(const dtd_t *dtd);
/**
 * @param dtd pointer to dtd_t strucutute where the information is held
 * @return the vertical sync polarity
 */
u8 dtd_GetVSyncPolarity(const dtd_t *dtd);
/**
 * @param dtd pointer to dtd_t strucutute where the information is held
 * @return the vertical sync pulse width in line
 */
u16 dtd_GetVSyncPulseWidth(const dtd_t *dtd);
/**
 * @param dtd1 pointer to dtd_t structure to be compared
 * @param dtd2 pointer to dtd_t structure to be compared to 
 * @return TRUE if the two DTDs are identical 
 * @note: although DTDs may have different refresh rates, and hence 
 * pixel clocks, they can still be identical
 */
int dtd_IsEqual(const dtd_t *dtd1, const dtd_t *dtd2);
/**
 * Set the desired pixel repetition
 * @param dtd pointer to dtd_t strucutute where the information is held
 * @param value of pixel repetitions
 * @return TRUE if successful
 * @note for CEA video modes, the value has to fall within the
 * defined range, otherwise the method will fail.
 */
int dtd_SetPixelRepetitionInput(dtd_t *dtd, u16 value);

#endif /* DTD_H_ */
