/*
 * Copyright (C) 2012-2014, 2016-2017 ARM Limited. All rights reserved.
 *
 * This program is free software and is provided to you under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation, and any use by you of this program is subject to the terms of such GNU
 * licence.
 *
 * A copy of the licence is included with the program, and can also be obtained from Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "mali_broadcast.h"
#include "mali_kernel_common.h"
#include "mali_osk.h"

#define MALI_BROADCAST_REGISTER_SIZE 0x1000
#define MALI_BROADCAST_REG_BROADCAST_MASK 0x0
#define MALI_BROADCAST_REG_INTERRUPT_MASK 0x4

struct mali_bcast_unit {
    struct mali_hw_core hw_core;
    u32 current_mask;
};

struct mali_bcast_unit *mali_bcast_unit_create(const _mali_osk_resource_t *resource)
{
    struct mali_bcast_unit *bcast_unit = NULL;

    MALI_DEBUG_ASSERT_POINTER(resource);

    bcast_unit = mali_osk_malloc(sizeof(struct mali_bcast_unit));
    if (bcast_unit == NULL) {
        MALI_PRINT_ERROR(("Broadcast: Failed to allocate memory for Broadcast unit\n"));
        return NULL;
    }

    if (MALI_OSK_ERR_OK == mali_hw_core_create(&bcast_unit->hw_core, resource, MALI_BROADCAST_REGISTER_SIZE)) {
        bcast_unit->current_mask = 0;
        mali_bcast_reset(bcast_unit);

        return bcast_unit;
    } else {
        MALI_PRINT_ERROR(("Broadcast: Failed map broadcast unit\n"));
    }

    _mali_osk_free(bcast_unit);

    return NULL;
}

void mali_bcast_unit_delete(struct mali_bcast_unit *bcast_unit)
{
    MALI_DEBUG_ASSERT_POINTER(bcast_unit);
    mali_hw_core_delete(&bcast_unit->hw_core);
    _mali_osk_free(bcast_unit);
}

/* Call this function to add the @group's id into bcast mask
 * Note: redundant calling this function with same @group
 * doesn't make any difference as calling it once
 */
void mali_bcast_add_group(struct mali_bcast_unit *bcast_unit, struct mali_group *group)
{
    u32 bcast_id;
    u32 broadcast_mask;

    MALI_DEBUG_ASSERT_POINTER(bcast_unit);
    MALI_DEBUG_ASSERT_POINTER(group);

    bcast_id = mali_pp_core_get_bcast_id(mali_group_get_pp_core(group));

    broadcast_mask = bcast_unit->current_mask;

    broadcast_mask |= (bcast_id);                                    /* add PP core to broadcast */
    broadcast_mask |= (bcast_id << MALI_PP_JOB_FB_LOOKUP_LIST_SIZE); /* add MMU to broadcast */

    /* store mask so we can restore on reset */
    bcast_unit->current_mask = broadcast_mask;
}

/* Call this function to remove @group's id from bcast mask
 * Note: redundant calling this function with same @group
 * doesn't make any difference as calling it once
 */
void mali_bcast_remove_group(struct mali_bcast_unit *bcast_unit, struct mali_group *group)
{
    u32 bcast_id;
    u32 broadcast_mask;

    MALI_DEBUG_ASSERT_POINTER(bcast_unit);
    MALI_DEBUG_ASSERT_POINTER(group);

    bcast_id = mali_pp_core_get_bcast_id(mali_group_get_pp_core(group));

    broadcast_mask = bcast_unit->current_mask;

    broadcast_mask &= ~((bcast_id << MALI_PP_JOB_FB_LOOKUP_LIST_SIZE) | bcast_id);

    /* store mask so we can restore on reset */
    bcast_unit->current_mask = broadcast_mask;
}

void mali_bcast_reset(struct mali_bcast_unit *bcast_unit)
{
    MALI_DEBUG_ASSERT_POINTER(bcast_unit);

    /* set broadcast mask */
    mali_hw_core_register_write(&bcast_unit->hw_core, MALI_BROADCAST_REG_BROADCAST_MASK, bcast_unit->current_mask);

    /* set IRQ override mask */
    mali_hw_core_register_write(&bcast_unit->hw_core, MALI_BROADCAST_REG_INTERRUPT_MASK,
                                bcast_unit->current_mask & 0xFF);
}

void mali_bcast_disable(struct mali_bcast_unit *bcast_unit)
{
    MALI_DEBUG_ASSERT_POINTER(bcast_unit);

    /* set broadcast mask */
    mali_hw_core_register_write(&bcast_unit->hw_core, MALI_BROADCAST_REG_BROADCAST_MASK, 0x0);

    /* set IRQ override mask */
    mali_hw_core_register_write(&bcast_unit->hw_core, MALI_BROADCAST_REG_INTERRUPT_MASK, 0x0);
}
