/*
 * Copyright (c) 2022 HiHope Open Source Organization .
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

#ifndef HDI_MPP_MPI_H
#define HDI_MPP_MPI_H

#include "rk_mpi.h"

// mpp base api
typedef MPP_RET (*hdiMppCreate)(MppCtx *, MppApi **);
typedef MPP_RET (*hdiMppInit)(MppCtx, MppCtxType, MppCodingType);
typedef MPP_RET (*hdiMppStart)(MppCtx);
typedef MPP_RET (*hdiMppStop)(MppCtx);
typedef MPP_RET (*hdiMppDestroy)(MppCtx);
typedef MPP_RET (*hdiMppCheckSupportFormat)(MppCtxType, MppCodingType);
typedef void (*hdiMppShowSupportFormat)(void);
typedef void (*hdiMppShowColorFormat)(void);
// mpp packet api
typedef MPP_RET (*hdiMppPacketNew)(MppPacket *);
typedef MPP_RET (*hdiMppPacketInit)(MppPacket *, void *, size_t);
typedef MPP_RET (*hdiMppPacketInitWithBuffer)(MppPacket *, MppBuffer);
typedef MPP_RET (*hdiMppPacketCopyInit)(MppPacket *, const MppPacket);
typedef MPP_RET (*hdiMppPacketDeinit)(MppPacket*);
typedef RK_U32  (*hdiMppPacketGetEos)(MppPacket);
typedef MPP_RET (*hdiMppPacketSetEos)(MppPacket);
typedef RK_S64  (*hdiMppPacketGetPts)(const MppPacket);
typedef void    (*hdiMppPacketSetPts)(MppPacket, RK_S64);
typedef void    (*hdiMppPacketSetData)(MppPacket packet, void *data);
typedef void    (*hdiMppPacketSetSize)(MppPacket, size_t);
typedef void*   (*hdiMppPacketGetPos)(const MppPacket);
typedef void    (*hdiMppPacketSetPos)(MppPacket packet, void *data);
typedef void    (*hdiMppPacketSetLength)(MppPacket, size_t);
typedef size_t  (*hdiMppPacketGetLength)(const MppPacket);
typedef RK_U32  (*hdiMppPacketIsPartition)(const MppPacket);
typedef RK_U32  (*hdiMppPacketIsEoi)(const MppPacket);
typedef MPP_RET (*hdiMppMetaSetPacket)(MppMeta, MppMetaKey, MppPacket);
// mpp frame api
typedef MPP_RET (*hdiMppFrameInit)(MppFrame*);
typedef MPP_RET (*hdiMppFrameDeinit)(MppFrame*);
typedef MppFrame (*hdiMppFrameGetNext)(MppFrame);
typedef RK_U32 (*hdiMppFrameGetInfoChange)(MppFrame);
typedef RK_U32 (*hdiMppFrameGetWidth)(MppFrame);
typedef RK_U32 (*hdiMppFrameGetHeight)(MppFrame);
typedef RK_U32 (*hdiMppFrameGetHorStride)(MppFrame);
typedef RK_U32 (*hdiMppFrameGetVerStride)(MppFrame);
typedef RK_U32 (*hdiMppFrameGetBufferSize)(MppFrame);
typedef MppFrameFormat (*hdiMppFrameGetFormat)(MppFrame);
typedef RK_U32 (*hdiMppFrameGetErrinfo)(const MppFrame);
typedef RK_U32 (*hdiMppFrameGetDiscard)(const MppFrame);
typedef MppBuffer (*hdiMppFrameGetBuffer)(const MppFrame);
typedef void (*hdiMppFrameSetBuffer)(MppFrame, MppBuffer);
typedef RK_U32 (*hdiMppFrameGetEos)(const MppFrame);
typedef void (*hdiMppFrameSetEos)(const MppFrame, RK_U32);
typedef void (*hdiMppFrameSetFormat)(MppFrame, MppFrameFormat);
typedef void (*hdiMppFrameSetWidth)(MppFrame, RK_U32);
typedef void (*hdiMppFrameSetHeight)(MppFrame, RK_U32);
typedef void (*hdiMppFrameSetHorStride)(MppFrame, RK_U32);
typedef void (*hdiMppFrameSetVerStride)(MppFrame, RK_U32);
typedef MppMeta (*hdiMppFrameGetMeta)(const MppFrame);
// mpp dec config api
typedef MPP_RET (*hdiMppDecCfgDeinit)(MppDecCfg *);
typedef MPP_RET (*hdiMppDecCfgInit)(MppDecCfg *);
typedef MPP_RET (*hdiMppDecCfgSetU32)(MppDecCfg, const char *, RK_U32);
// mpp enc config api
typedef MPP_RET (*hdiMppEncCfgInit)(MppEncCfg *);
typedef MPP_RET (*hdiMppEncCfgDeinit)(MppEncCfg);
typedef MPP_RET (*hdiMppEncCfgSetS32)(MppEncCfg, const char *, RK_S32);
typedef MPP_RET (*hdiMppEncCfgSetU32)(MppEncCfg, const char *, RK_U32);
typedef MPP_RET (*hdiMppEncRefCfgInit)(MppEncRefCfg *);
typedef MPP_RET (*hdiMppEncRefCfgDeinit)(MppEncRefCfg *);
typedef MPP_RET (*hdiMppEncGenRefCfg)(MppEncRefCfg, RK_U32);
typedef MPP_RET (*hdiMppEncGenSmartGopRefCfg)(MppEncRefCfg, RK_U32, RK_S32);
// mpp buffer group api
typedef MPP_RET (*hdiMppBufferGroupGet)(MppBufferGroup *, MppBufferType,
    MppBufferMode, const char *, const char *);
typedef MPP_RET (*hdiMppBufferGroupPut)(MppBufferGroup);
typedef MPP_RET (*hdiMppBufferGroupClear)(MppBufferGroup);
typedef MPP_RET (*hdiMppBufferGroupLimitConfig)(MppBufferGroup, size_t, RK_S32);
typedef MPP_RET (*hdiMppBufferGetFdWithCaller)(MppBufferGroup *, const char *);
typedef MPP_RET (*hdiMppBufferGetWithTag)(MppBufferGroup, MppBuffer *, size_t, const char *, const char *);
typedef void* (*hdiMppBufferGetPtrWithCaller)(MppBuffer, const char *);
typedef size_t (*hdiMppBufferGroupUsage)(MppBufferGroup);
typedef MPP_RET (*hdiMppBufferPutWithCaller)(MppBuffer, const char *);
// mpp task api
typedef MPP_RET (*hdiMppTaskMetaGetPacket)(MppBufferGroup);
// mpp env api
typedef RK_S32 (*hdiMppEnvGetU32)(const char *, RK_U32 *, RK_U32);

typedef struct {
    // mpp base api
    hdiMppCreate HdiMppCreate;
    hdiMppInit HdiMppInit;
    hdiMppStart HdiMppStart;
    hdiMppStop HdiMppStop;
    hdiMppDestroy HdiMppDestroy;
    hdiMppCheckSupportFormat HdiMppCheckSupportFormat;
    hdiMppShowSupportFormat HdiMppShowSupportFormat;
    hdiMppShowColorFormat HdiMppShowColorFormat;
    // mpp packet api
    hdiMppPacketNew HdiMppPacketNew;
    hdiMppPacketInit HdiMppPacketInit;
    hdiMppPacketInitWithBuffer HdiMppPacketInitWithBuffer;
    hdiMppPacketCopyInit HdiMppPacketCopyInit;
    hdiMppPacketDeinit HdiMppPacketDeinit;
    hdiMppPacketGetEos HdiMppPacketGetEos;
    hdiMppPacketSetEos HdiMppPacketSetEos;
    hdiMppPacketGetPts HdiMppPacketGetPts;
    hdiMppPacketSetPts HdiMppPacketSetPts;
    hdiMppPacketSetData HdiMppPacketSetData;
    hdiMppPacketSetSize HdiMppPacketSetSize;
    hdiMppPacketGetPos HdiMppPacketGetPos;
    hdiMppPacketSetPos HdiMppPacketSetPos;
    hdiMppPacketSetLength HdiMppPacketSetLength;
    hdiMppPacketGetLength HdiMppPacketGetLength;
    hdiMppPacketIsPartition HdiMppPacketIsPartition;
    hdiMppPacketIsEoi HdiMppPacketIsEoi;
    hdiMppMetaSetPacket HdiMppMetaSetPacket;
    // mpp frame api
    hdiMppFrameInit HdiMppFrameInit;
    hdiMppFrameDeinit HdiMppFrameDeinit;
    hdiMppFrameGetNext HdiMppFrameGetNext;
    hdiMppFrameGetInfoChange HdiMppFrameGetInfoChange;
    hdiMppFrameGetWidth HdiMppFrameGetWidth;
    hdiMppFrameGetHeight HdiMppFrameGetHeight;
    hdiMppFrameGetHorStride HdiMppFrameGetHorStride;
    hdiMppFrameGetVerStride HdiMppFrameGetVerStride;
    hdiMppFrameGetBufferSize HdiMppFrameGetBufferSize;
    hdiMppFrameGetFormat HdiMppFrameGetFormat;
    hdiMppFrameGetErrinfo HdiMppFrameGetErrinfo;
    hdiMppFrameGetDiscard HdiMppFrameGetDiscard;
    hdiMppFrameGetBuffer HdiMppFrameGetBuffer;
    hdiMppFrameSetBuffer HdiMppFrameSetBuffer;
    hdiMppFrameGetEos HdiMppFrameGetEos;
    hdiMppFrameSetEos HdiMppFrameSetEos;
    hdiMppFrameSetFormat HdiMppFrameSetFormat;
    hdiMppFrameSetWidth HdiMppFrameSetWidth;
    hdiMppFrameSetHeight HdiMppFrameSetHeight;
    hdiMppFrameSetHorStride HdiMppFrameSetHorStride;
    hdiMppFrameSetVerStride HdiMppFrameSetVerStride;
    hdiMppFrameGetMeta HdiMppFrameGetMeta;
    // mpp dec config api
    hdiMppDecCfgDeinit HdiMppDecCfgDeinit;
    hdiMppDecCfgInit HdiMppDecCfgInit;
    hdiMppDecCfgSetU32 HdiMppDecCfgSetU32;
    // mpp enc config api
    hdiMppEncCfgInit HdiMppEncCfgInit;
    hdiMppEncCfgDeinit HdiMppEncCfgDeinit;
    hdiMppEncCfgSetS32 HdiMppEncCfgSetS32;
    hdiMppEncCfgSetU32 HdiMppEncCfgSetU32;
    hdiMppEncRefCfgInit HdiMppEncRefCfgInit;
    hdiMppEncRefCfgDeinit HdiMppEncRefCfgDeinit;
    hdiMppEncGenRefCfg HdiMppEncGenRefCfg;
    hdiMppEncGenSmartGopRefCfg HdiMppEncGenSmartGopRefCfg;
    // mpp buffer group api
    hdiMppBufferGroupGet HdiMppBufferGroupGet;
    hdiMppBufferGroupPut HdiMppBufferGroupPut;
    hdiMppBufferGroupClear HdiMppBufferGroupClear;
    hdiMppBufferGroupLimitConfig HdiMppBufferGroupLimitConfig;
    hdiMppBufferGetFdWithCaller HdiMppBufferGetFdWithCaller;
    hdiMppBufferGetWithTag HdiMppBufferGetWithTag;
    hdiMppBufferGetPtrWithCaller HdiMppBufferGetPtrWithCaller;
    hdiMppBufferGroupUsage HdiMppBufferGroupUsage;
    hdiMppBufferPutWithCaller HdiMppBufferPutWithCaller;
    // mpp task api
    hdiMppTaskMetaGetPacket HdiMppTaskMetaGetPacket;
    // mpp env api
    hdiMppEnvGetU32 HdiMppEnvGetU32;
}RKMppApi;

int32_t GetMppApi(RKMppApi **mppApi);
void ReleaseMppApi(RKMppApi *mppApi);

#endif // HDI_MPP_MPI_H