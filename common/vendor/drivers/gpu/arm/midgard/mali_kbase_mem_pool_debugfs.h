/*
 *
 * (C) COPYRIGHT 2014-2015 ARM Limited. All rights reserved.
 *
 * This program is free software and is provided to you under the terms of the
 * GNU General Public License version 2 as published by the Free Software
 * Foundation, and any use by you of this program is subject to the terms
 * of such GNU licence.
 *
 * A copy of the licence is included with the program, and can also be obtained
 * from Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 *
 */

#ifndef KBASE_MEM_POOL_DEBUGFS_H
#define KBASE_MEM_POOL_DEBUGFS_H

#include <mali_kbase.h>

/**
 * kbase_mem_pool_debugfs_init - add debugfs knobs for @pool
 * @parent: Parent debugfs dentry
 * @pool:   Memory pool to control
 *
 * Adds two debugfs files under @parent:
 * - mem_pool_size: get/set the current size of @pool
 * - mem_pool_max_size: get/set the max size of @pool
 */
void kbase_mem_pool_debugfs_init(struct dentry *parent, struct kbase_mem_pool *pool);

#endif /* _KBASE_MEM_POOL_DEBUGFS_H */
