/*
 * videoParams.h
 *
 * Synopsys Inc.
 * SG DWC PT02
 */
 /**
 * @file
 * Define video parameters structure and functions to manipulate the
 * information held by the structure.
 * Video encoding and colorimetry are also defined here
 */

#ifndef VIDEOPARAMS_H_
#define VIDEOPARAMS_H_

#include "../util/types.h"
#include "../edid/dtd.h"

typedef enum
{
    RGB = 0,
    YCC444,
    YCC422

} encoding_t;

typedef enum
{

    ITU601 = 1,
    ITU709,
    EXTENDED_COLORIMETRY

} colorimetry_t;

typedef struct
{
    u8 mHdmi;
    encoding_t mEncodingOut;
    encoding_t mEncodingIn;
    u8 mColorResolution;
    u8 mPixelRepetitionFactor;
    dtd_t mDtd;
    u8 mRgbQuantizationRange;
    u8 mPixelPackingDefaultPhase;
    u8 mColorimetry;
    u8 mScanInfo;
    u8 mActiveFormatAspectRatio;
    u8 mNonUniformScaling;
    u8 mExtColorimetry;
    u8 mItContent;
    u16 mEndTopBar;
    u16 mStartBottomBar;
    u16 mEndLeftBar;
    u16 mStartRightBar;
    u16 mCscFilter;
    u16 mCscA[4];
    u16 mCscC[4];
    u16 mCscB[4];
    u16 mCscScale;
    u8 mHdmiVideoFormat;
    u8 m3dStructure;
    u8 m3dExtData;
    u8 mHdmiVic;

} videoParams_t;

/**
 * This method should be called before setting any parameters
 * to put the state of the strucutre to default
 * @param params pointer to the video parameters structure
 */
void videoParams_Reset(videoParams_t *params);
/**
 * @param params pointer to the video parameters structure
 * @return active format aspect ratio code in Table 9 in CEA-D/E
 */
u8 videoParams_GetActiveFormatAspectRatio(videoParams_t *params);
/**
 * @param params pointer to the video parameters structure
 * @return colorimetry code in Table 9 in CEA-D/E
 */
u8 videoParams_GetColorimetry(videoParams_t *params);
/**
 * @param params pointer to the video parameters structure
 * @return color resolution/depth (8/10/12/16-bits per pixel)
 */
u8 videoParams_GetColorResolution(videoParams_t *params);
/**
 * @param params pointer to the video parameters structure
 * @return the colour space convertion filter
 */
u8 videoParams_GetCscFilter(videoParams_t *params);
/**
 * @param params pointer to the video parameters structure
 * @return pointer to the DTD structure that describes the video output stream
 */
dtd_t* videoParams_GetDtd(videoParams_t *params);
/**
 * @param params pointer to the video parameters structure
 * @return the input video encoding type
 */
encoding_t videoParams_GetEncodingIn(videoParams_t *params);
/**
 * @param params pointer to the video parameters structure
 * @return the output video encoding type
 */
encoding_t videoParams_GetEncodingOut(videoParams_t *params);
/**
 * @param params pointer to the video parameters structure
 *  @return the pixel number of end of left bar
 */
u16 videoParams_GetEndLeftBar(videoParams_t *params);
/**
 * @param params pointer to the video parameters structure
 *  @return line number of end of top bar
 */
u16 videoParams_GetEndTopBar(videoParams_t *params);
/**
 * @param params pointer to the video parameters structure
 *  @return the extended colorimetry code in Table 11 in CEA-D/E
 */
u8 videoParams_GetExtColorimetry(videoParams_t *params);
/**
 * @param params pointer to the video parameters structure
 * @return TRUE if output stream is HDMI
 */
u8 videoParams_GetHdmi(videoParams_t *params);
/**
 * @param params pointer to the video parameters structure
 * @return TRUE if stream is IT content, refer to Table 11 in CEA-D/E
 */
u8 videoParams_GetItContent(videoParams_t *params);
/**
 * @param params pointer to the video parameters structure
 *  @return non uniform scaling code, refer to Table 11 in CEA-D/E
 */
u8 videoParams_GetNonUniformScaling(videoParams_t *params);
/**
 * @param params pointer to the video parameters structure
 *  @return default phase (0 or 1)
 */
u8 videoParams_GetPixelPackingDefaultPhase(videoParams_t *params);
/**
 * @param params pointer to the video parameters structure
 * @return desired pixel repetition factor
 */
u8 videoParams_GetPixelRepetitionFactor(videoParams_t *params);
/**
 * @param params pointer to the video parameters structure
 * @return the quantisation range of the video stream,
 * refer to Table 11 in CEA-D/E
 */
u8 videoParams_GetRgbQuantizationRange(videoParams_t *params);
/**
 * @param params pointer to the video parameters structure
 * @return the scan info code, refer to Table 8 in CEA-D/E
 */
u8 videoParams_GetScanInfo(videoParams_t *params);
/**
 * @param params pointer to the video parameters structure
 * @return the line number of start of bottom bar
 */
u16 videoParams_GetStartBottomBar(videoParams_t *params);
/**
 * @param params pointer to the video parameters structure
 * @return the pixel number of start of right bar
 */
u16 videoParams_GetStartRightBar(videoParams_t *params);
/**
 * @param params pointer to the video parameters structure
 * @return custom csc scale
 */
u16 videoParams_GetCscScale(videoParams_t *params);
/**
 * @param params pointer to the video parameters structure
 * @param value active format aspect ratio code in Table 9 in CEA-D/E
 */
void videoParams_SetActiveFormatAspectRatio(videoParams_t *params, u8 value);
/**
 * @param params pointer to the video parameters structure
 * @param value colorimetry code in Table 9 in CEA-D/E
 */
void videoParams_SetColorimetry(videoParams_t *params, u8 value);
/**
 * @param params pointer to the video parameters structure
 * @param value color resolution/depth (8/10/12/16-bits per pixel)
 */
void videoParams_SetColorResolution(videoParams_t *params, u8 value);
/**
 * @param params pointer to the video parameters structure
 * @param value colour space convertion chroma filter
 * check Color Space Converter configuration in the
 * HDMI Transmitter Controller Databook
 */
void videoParams_SetCscFilter(videoParams_t *params, u8 value);
/**
 * @param params pointer to the video parameters structure
 * @param dtd pointer to dtd_t structure describing the video output stream
 */
void videoParams_SetDtd(videoParams_t *params, dtd_t *dtd);
/**
 * @param params pointer to the video parameters structure
 * @param value of thie input video one of enumurators: RGB/YCC444/YCC422 of type encoding_t
 */
void videoParams_SetEncodingIn(videoParams_t *params, encoding_t value);
/**
 * @param params pointer to the video parameters structure
 * @param value one of enumurators: RGB/YCC444/YCC422 of type encoding_t
 */
void videoParams_SetEncodingOut(videoParams_t *params, encoding_t value);
/**
 * @param params pointer to the video parameters structure
 * @param value 
 */
void videoParams_SetEndLeftBar(videoParams_t *params, u16 value);
/**
 * @param params pointer to the video parameters structure
 * @param value 
 */
void videoParams_SetEndTopBar(videoParams_t *params, u16 value);
/**
 * @param params pointer to the video parameters structure
 * @param value extended colorimetry code in Table 11 in CEA-D/E
 */
void videoParams_SetExtColorimetry(videoParams_t *params, u8 value);
/**
 * @param params pointer to the video parameters structure
 * @param value HDMI TRUE, DVI FALSE
 */
void videoParams_SetHdmi(videoParams_t *params, u8 value);
/**
 * @param params pointer to the video parameters structure
 * @param value 
 */
void videoParams_SetItContent(videoParams_t *params, u8 value);
/**
 * @param params pointer to the video parameters structure
 * @param value 
 */
void videoParams_SetNonUniformScaling(videoParams_t *params, u8 value);
/**
 * @param params pointer to the video parameters structure
 * @param value 
 */
void videoParams_SetPixelPackingDefaultPhase(videoParams_t *params, u8 value);
/**
 * @param params pointer to the video parameters structure
 * @param value 
 */
void videoParams_SetPixelRepetitionFactor(videoParams_t *params, u8 value);
/**
 * @param params pointer to the video parameters structure
 * @param value 
 */
void videoParams_SetRgbQuantizationRange(videoParams_t *params, u8 value);
/**
 * @param params pointer to the video parameters structure
 * @param value 
 */
void videoParams_SetScanInfo(videoParams_t *params, u8 value);
/**
 * @param params pointer to the video parameters structure
 * @param value 
 */
void videoParams_SetStartBottomBar(videoParams_t *params, u16 value);
/**
 * @param params pointer to the video parameters structure
 * @param value 
 */
void videoParams_SetStartRightBar(videoParams_t *params, u16 value);
/**
 * @param params pointer to the video parameters structure
 * @return the custom csc coefficients A
 */
u16 * videoParams_GetCscA(videoParams_t *params);

void videoParams_SetCscA(videoParams_t *params, u16 value[4]);
/**
 * @param params pointer to the video parameters structure
 * @return the custom csc coefficients B
 */
u16 * videoParams_GetCscB(videoParams_t *params);

void videoParams_SetCscB(videoParams_t *params, u16 value[4]);
/**
 * @param params pointer to the video parameters structure
 * @return the custom csc coefficients C
 */
u16 * videoParams_GetCscC(videoParams_t *params);

void videoParams_SetCscC(videoParams_t *params, u16 value[4]);

void videoParams_SetCscScale(videoParams_t *params, u16 value);
/**
 * @param params pointer to the video parameters structure
 * @return Video PixelClock in [0.01 MHz]
 */
u16 videoParams_GetPixelClock(videoParams_t *params);
/**
 * @param params pointer to the video parameters structure
 * @return TMDS Clock in [0.01 MHz]
 */
u16 videoParams_GetTmdsClock(videoParams_t *params);
/** 
 * @param params pointer to the video parameters structure
 * @return Ration clock x 100 (should be multiplied by x 0.01 afterwards)
 */
unsigned videoParams_GetRatioClock(videoParams_t *params);
/**
 * @param params pointer to the video parameters structure
 * @return TRUE if csc is needed
 */
int videoParams_IsColorSpaceConversion(videoParams_t *params);
/**
 * @param params pointer to the video parameters structure
 * @return TRUE if color space decimation is needed
 */
int videoParams_IsColorSpaceDecimation(videoParams_t *params);
/**
 * @param params pointer to the video parameters structure
 * @return TRUE if if video is interpolated
 */
int videoParams_IsColorSpaceInterpolation(videoParams_t *params);
/**
 * @param params pointer to the video parameters structure
 * @return TRUE if if video has pixel repetition
 */
int videoParams_IsPixelRepetition(videoParams_t *params);
/**
 * @param params pointer to the video parameters structure
 *  @return non uniform scaling code, refer to Table 11 in CEA-D/E
 */
u8 videoParams_GetHdmiVideoFormat(videoParams_t *params);

void videoParams_SetHdmiVideoFormat(videoParams_t *params, u8 value);
/**
 * @param params pointer to the video parameters structure
 * @return non uniform scaling code, refer to Table 11 in CEA-D/E
 */
u8 videoParams_Get3dStructure(videoParams_t *params);

void videoParams_Set3dStructure(videoParams_t *params, u8 value);
/**
 * @param params pointer to the video parameters structure
 * @return non uniform scaling code, refer to Table 11 in CEA-D/E
 */
u8 videoParams_Get3dExtData(videoParams_t *params);

void videoParams_Set3dExtData(videoParams_t *params, u8 value);
/**
 * @param params pointer to the video parameters structure
 * @return HDMI Video Format Identification Code (CEA-D/E and HDMI 1.4 Table 8-14)
 */
u8 videoParams_GetHdmiVic(videoParams_t *params);

void videoParams_SetHdmiVic(videoParams_t *params, u8 value);

void videoParams_UpdateCscCoefficients(videoParams_t *params);

#endif /* VIDEOPARAMS_H_ */
