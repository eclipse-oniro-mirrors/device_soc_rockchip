/*
 * Copyright (C) 2013, 2016-2017 ARM Limited. All rights reserved.
 *
 * This program is free software and is provided to you under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation, and any use by you of this program is subject to the terms of such GNU
 * licence.
 *
 * A copy of the licence is included with the program, and can also be obtained from Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "mali_spinlock_reentrant.h"

#include "mali_osk.h"
#include "mali_kernel_common.h"

struct mali_spinlock_reentrant *mali_spinlock_reentrant_init(_mali_osk_lock_order_t lock_order)
{
    struct mali_spinlock_reentrant *spinlock;

    spinlock = mali_osk_calloc(1, sizeof(struct mali_spinlock_reentrant));
    if (spinlock == NULL) {
        return NULL;
    }

    spinlock->lock = mali_osk_spinlock_irq_init(_MALI_OSK_LOCKFLAG_ORDERED, lock_order);
    if (spinlock->lock == NULL) {
        mali_spinlock_reentrant_term(spinlock);
        return NULL;
    }

    return spinlock;
}

void mali_spinlock_reentrant_term(struct mali_spinlock_reentrant *spinlock)
{
    MALI_DEBUG_ASSERT_POINTER(spinlock);
    MALI_DEBUG_ASSERT(spinlock->counter == 0 && spinlock->owner == 0);

    if (spinlock->lock != NULL) {
        _mali_osk_spinlock_irq_term(spinlock->lock);
    }

    _mali_osk_free(spinlock);
}

void mali_spinlock_reentrant_wait(struct mali_spinlock_reentrant *spinlock, u32 tid)
{
    MALI_DEBUG_ASSERT_POINTER(spinlock);
    MALI_DEBUG_ASSERT_POINTER(spinlock->lock);
    MALI_DEBUG_ASSERT(tid != 0);

    MALI_DEBUG_PRINT(MALI_KERNEL_LEVEL_DATA, ("%s ^\n", __FUNCTION__));

    if (tid != spinlock->owner) {
        mali_osk_spinlock_irq_lock(spinlock->lock);
        MALI_DEBUG_ASSERT(spinlock->owner == 0 && spinlock->counter == 0);
        spinlock->owner = tid;
    }

    MALI_DEBUG_PRINT(MALI_KERNEL_LEVEL_DATA, ("%s v\n", __FUNCTION__));

    ++spinlock->counter;
}

void mali_spinlock_reentrant_signal(struct mali_spinlock_reentrant *spinlock, u32 tid)
{
    MALI_DEBUG_ASSERT_POINTER(spinlock);
    MALI_DEBUG_ASSERT_POINTER(spinlock->lock);
    MALI_DEBUG_ASSERT(tid != 0 && tid == spinlock->owner);

    --spinlock->counter;
    if (spinlock->counter == 0) {
        spinlock->owner = 0;
        MALI_DEBUG_PRINT(MALI_KERNEL_LEVEL_DATA, ("%s release last\n", __FUNCTION__));
        mali_osk_spinlock_irq_unlock(spinlock->lock);
    }
}
