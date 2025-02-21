/*
 *
 * Copyright 2015 Rockchip Electronics Co., LTD.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __MPP_BUF_SLOT_H__
#define __MPP_BUF_SLOT_H__

#include "mpp_frame.h"

typedef void* MppBufSlots;

/*
 * buffer slot index range is 0~254, 0xff (255) will indicated the invalid index
 */
#define SLOT_IDX_BUTT                   (0xff)

#ifdef __cplusplus
extern "C" {
#endif

MPP_RET mpp_buf_slot_init(MppBufSlots *slots);
MPP_RET mpp_buf_slot_deinit(MppBufSlots slots);
MPP_RET mpp_buf_slot_setup(MppBufSlots slots, RK_S32 count);
RK_U32  mpp_buf_slot_is_changed(MppBufSlots slots);
MPP_RET mpp_buf_slot_ready(MppBufSlots slots);
size_t  mpp_buf_slot_get_size(MppBufSlots slots);
RK_S32  mpp_buf_slot_get_count(MppBufSlots slots);
MPP_RET mpp_buf_slot_get_unused(MppBufSlots slots, RK_S32 *index);

typedef enum SlotUsageType_e {
    SLOT_CODEC_READY,       // bit flag             for buffer is prepared by codec
    SLOT_CODEC_USE,         // bit flag             for buffer is used as reference by codec
    SLOT_HAL_INPUT,         // counter              for buffer is used as hardware input
    SLOT_HAL_OUTPUT,        // counter + bit flag   for buffer is used as hardware output
    SLOT_QUEUE_USE,         // bit flag             for buffer is hold in different queues
    SLOT_USAGE_BUTT,
} SlotUsageType;

MPP_RET mpp_buf_slot_set_flag(MppBufSlots slots, RK_S32 index, SlotUsageType type);
MPP_RET mpp_buf_slot_clr_flag(MppBufSlots slots, RK_S32 index, SlotUsageType type);

typedef enum SlotQueueType_e {
    QUEUE_OUTPUT,           // queue for mpp output to user
    QUEUE_DISPLAY,          // queue for decoder output display
    QUEUE_DEINTERLACE,      // queue for deinterlace process
    QUEUE_COLOR_CONVERT,    // queue for color convertion process
    QUEUE_BUTT,
} SlotQueueType;

MPP_RET mpp_buf_slot_enqueue(MppBufSlots slots, RK_S32  index, SlotQueueType type);
MPP_RET mpp_buf_slot_dequeue(MppBufSlots slots, RK_S32 *index, SlotQueueType type);

typedef enum SlotPropType_e {
    SLOT_EOS,
    SLOT_FRAME,
    SLOT_BUFFER,
    SLOT_FRAME_PTR,
    SLOT_PROP_BUTT,
} SlotPropType;

MPP_RET mpp_buf_slot_set_prop(MppBufSlots slots, RK_S32 index, SlotPropType type, void *val);
MPP_RET mpp_buf_slot_get_prop(MppBufSlots slots, RK_S32 index, SlotPropType type, void *val);

typedef enum SlotsPropType_e {
    SLOTS_EOS,
    SLOTS_NUMERATOR,            // numerator of buffer size scale ratio
    SLOTS_DENOMINATOR,          // denominator of buffer size scale ratio
    SLOTS_HOR_ALIGN,            // input must be buf_align function pointer
    SLOTS_VER_ALIGN,            // input must be buf_align function pointer
    SLOTS_LEN_ALIGN,
    SLOTS_COUNT,
    SLOTS_SIZE,
    SLOTS_FRAME_INFO,
    SLOTS_PROP_BUTT,
} SlotsPropType;

typedef RK_U32 (*AlignFunc)(RK_U32 val);

RK_U32 mpp_slots_is_empty(MppBufSlots slots, SlotQueueType type);
RK_S32  mpp_slots_get_used_count(MppBufSlots slots);
RK_S32  mpp_slots_get_unused_count(MppBufSlots slots);
MPP_RET mpp_slots_set_prop(MppBufSlots slots, SlotsPropType type, void *val);
MPP_RET mpp_slots_get_prop(MppBufSlots slots, SlotsPropType type, void *val);
MPP_RET mpp_buf_slot_reset(MppBufSlots slots, RK_S32 index); // rest slot status when info_change no ok

// special one for generate default frame to slot at index and return pointer
MPP_RET mpp_buf_slot_default_info(MppBufSlots slots, RK_S32 index, void *val);

#ifdef __cplusplus
}
#endif

#endif /* __MPP_BUF_SLOT_H__ */
