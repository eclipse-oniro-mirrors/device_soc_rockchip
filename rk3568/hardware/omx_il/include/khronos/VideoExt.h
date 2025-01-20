/*
 * Copyright (C) 2021 HiHope Open Source Organization .
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef VideoExt_h
#define VideoExt_h

#include <OMX_IVCommon.h>
#include <codec_omx_ext.h>
typedef enum OMX_RANGE {
    RangeUnspecified,
    RangeFull,
    RangeLimited,
    RangeOther = 0xff,
} OMX_RANGE;

typedef enum OMX_PRIMARIES {
    PrimariesUnspecified,
    PrimariesBT709_5,       // Rec.ITU-R BT.709-5 or equivalent
    PrimariesBT470_6M,      // Rec.ITU-R BT.470-6 System M or equivalent
    PrimariesBT601_6_625,   // Rec.ITU-R BT.601-6 625 or equivalent
    PrimariesBT601_6_525,   // Rec.ITU-R BT.601-6 525 or equivalent
    PrimariesGenericFilm,   // Generic Film
    PrimariesBT2020,        // Rec.ITU-R BT.2020 or equivalent
    PrimariesOther = 0xff,
} OMX_PRIMARIES;

typedef enum OMX_TRANSFER {
    TransferUnspecified,
    TransferLinear,         // Linear transfer characteristics
    TransferSRGB,           // sRGB or equivalent
    TransferSMPTE170M,      // SMPTE 170M or equivalent (e.g. BT.601/709/2020)
    TransferGamma22,        // Assumed display gamma 2.2
    TransferGamma28,        // Assumed display gamma 2.8
    TransferST2084,         // SMPTE ST 2084 for 10/12/14/16 bit systems
    TransferHLG,            // ARIB STD-B67 hybrid-log-gamma
    TransferSMPTE240M = 0x40, // SMPTE 240M
    TransferXvYCC,          // IEC 61966-2-4
    TransferBT1361,         // Rec.ITU-R BT.1361 extended gamut
    TransferST428,          // SMPTE ST 428-1
    TransferOther = 0xff,
} OMX_TRANSFER;

typedef enum OMX_MATRIXCOEFFS {
    MatrixUnspecified,
    MatrixBT709_5,          // Rec.ITU-R BT.709-5 or equivalent
    MatrixBT470_6M,         // KR=0.30, KB=0.11 or equivalent
    MatrixBT601_6,          // Rec.ITU-R BT.601-6 625 or equivalent
    MatrixSMPTE240M,        // SMPTE 240M or equivalent
    MatrixBT2020,           // Rec.ITU-R BT.2020 non-constant luminance
    MatrixBT2020Constant,   // Rec.ITU-R BT.2020 constant luminance
    MatrixOther = 0xff,
} OMX_MATRIXCOEFFS;

typedef struct OMX_COLORASPECTS {
    OMX_RANGE mRange;                // IN/OUT
    OMX_PRIMARIES mPrimaries;        // IN/OUT
    OMX_TRANSFER mTransfer;          // IN/OUT
    OMX_MATRIXCOEFFS mMatrixCoeffs;  // IN/OUT
} OMX_COLORASPECTS;

typedef struct ISO_COLORASPECTS {
    OMX_U32 mRange;                 // IN/OUT
    OMX_U32 mPrimaries;             // IN/OUT
    OMX_U32 mTransfer;              // IN/OUT
    OMX_U32 mMatrixCoeffs;          // IN/OUT
} ISO_COLORASPECTS;

typedef struct OMX_CONFIG_DESCRIBECOLORASPECTSPARAMS {
    OMX_U32  nSize;                // IN
    OMX_VERSIONTYPE nVersion;      // IN
    OMX_U32  nPortIndex;           // IN
    OMX_BOOL bRequestingDataSpace; // IN
    OMX_BOOL bDataSpaceChanged;    // IN
    OMX_U32  nPixelFormat;         // IN
    OMX_U32  nDataSpace;           // OUT
    OMX_COLORASPECTS sAspects;  // IN/OUT
} OMX_CONFIG_DESCRIBECOLORASPECTSPARAMS;

typedef enum OMX_VIDEO_CODINGTYPEEXT {
    OMX_VIDEO_CodingVP8EXT = 9,
    OMX_VIDEO_CodingVP9,
    OMX_VIDEO_AVCLevel52  = 0x10000,
}OMX_VIDEO_CODINGTYPEEXT;

typedef enum OMX_VIDEO_VP9PROFILETYPE {
    OMX_VIDEO_VP9Profile0       = 0x1,
    OMX_VIDEO_VP9Profile1       = 0x2,
    OMX_VIDEO_VP9Profile2       = 0x4,
    OMX_VIDEO_VP9Profile3       = 0x8,
    // HDR profiles also support passing HDR metadata
    OMX_VIDEO_VP9Profile2HDR    = 0x1000,
    OMX_VIDEO_VP9Profile3HDR    = 0x2000,
    OMX_VIDEO_VP9ProfileUnknown = 0x6EFFFFFF,
    OMX_VIDEO_VP9ProfileMax     = 0x7FFFFFFF
} OMX_VIDEO_VP9PROFILETYPE;

/** VP9 levels */
typedef enum OMX_VIDEO_VP9LEVELTYPE {
    OMX_VIDEO_VP9Level1       = 0x1,
    OMX_VIDEO_VP9Level11      = 0x2,
    OMX_VIDEO_VP9Level2       = 0x4,
    OMX_VIDEO_VP9Level21      = 0x8,
    OMX_VIDEO_VP9Level3       = 0x10,
    OMX_VIDEO_VP9Level31      = 0x20,
    OMX_VIDEO_VP9Level4       = 0x40,
    OMX_VIDEO_VP9Level41      = 0x80,
    OMX_VIDEO_VP9Level5       = 0x100,
    OMX_VIDEO_VP9Level51      = 0x200,
    OMX_VIDEO_VP9Level52      = 0x400,
    OMX_VIDEO_VP9Level6       = 0x800,
    OMX_VIDEO_VP9Level61      = 0x1000,
    OMX_VIDEO_VP9Level62      = 0x2000,
    OMX_VIDEO_VP9LevelUnknown = 0x6EFFFFFF,
    OMX_VIDEO_VP9LevelMax     = 0x7FFFFFFF
} OMX_VIDEO_VP9LEVELTYPE;

/** Structure to define if dependent slice segments should be used */
typedef struct OMX_VIDEO_SLICESEGMENTSTYPE {
    OMX_U32 nSize;
    OMX_VERSIONTYPE nVersion;
    OMX_U32 nPortIndex;
    OMX_BOOL bDepedentSegments;
    OMX_BOOL bEnableLoopFilterAcrossSlices;
} OMX_VIDEO_SLICESEGMENTSTYPE;

/** Structure to return timestamps of rendered output frames as well as EOS
 *  for tunneled components.
 */
typedef struct OMX_VIDEO_RENDEREVENTTYPE {
    OMX_S64 nMediaTimeUs;  // timestamp of rendered video frame
    OMX_S64 nSystemTimeNs; // system monotonic time at the time frame was rendered
    // Use INT64_MAX for nMediaTimeUs to signal that the EOS
    // has been reached. In this case, nSystemTimeNs MUST be
    // the system time when the last frame was rendered.
    // This MUST be done in addition to returning (and
    // following) the render information for the last frame.
} OMX_VIDEO_RENDEREVENTTYPE;

/** Dolby Vision Profile enum type */
typedef enum OMX_VIDEO_DOLBYVISIONPROFILETYPE {
    OMX_VIDEO_DolbyVisionProfileUnknown = 0x0,
    OMX_VIDEO_DolbyVisionProfileDvavPer = 0x1,
    OMX_VIDEO_DolbyVisionProfileDvavPen = 0x2,
    OMX_VIDEO_DolbyVisionProfileDvheDer = 0x4,
    OMX_VIDEO_DolbyVisionProfileDvheDen = 0x8,
    OMX_VIDEO_DolbyVisionProfileDvheDtr = 0x10,
    OMX_VIDEO_DolbyVisionProfileDvheStn = 0x20,
    OMX_VIDEO_DolbyVisionProfileDvheDth = 0x40,
    OMX_VIDEO_DolbyVisionProfileDvheDtb = 0x80,
    OMX_VIDEO_DolbyVisionProfileMax     = 0x7FFFFFFF
} OMX_VIDEO_DOLBYVISIONPROFILETYPE;

/** Dolby Vision Level enum type */
typedef enum OMX_VIDEO_DOLBYVISIONLEVELTYPE {
    OMX_VIDEO_DolbyVisionLevelUnknown = 0x0,
    OMX_VIDEO_DolbyVisionLevelHd24    = 0x1,
    OMX_VIDEO_DolbyVisionLevelHd30    = 0x2,
    OMX_VIDEO_DolbyVisionLevelFhd24   = 0x4,
    OMX_VIDEO_DolbyVisionLevelFhd30   = 0x8,
    OMX_VIDEO_DolbyVisionLevelFhd60   = 0x10,
    OMX_VIDEO_DolbyVisionLevelUhd24   = 0x20,
    OMX_VIDEO_DolbyVisionLevelUhd30   = 0x40,
    OMX_VIDEO_DolbyVisionLevelUhd48   = 0x80,
    OMX_VIDEO_DolbyVisionLevelUhd60   = 0x100,
    OMX_VIDEO_DolbyVisionLevelmax     = 0x7FFFFFFF
} OMX_VIDEO_DOLBYVISIONLEVELTYPE;

#endif