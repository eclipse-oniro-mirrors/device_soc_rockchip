/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */

/* Copyright (c) 2020 Fuzhou Rockchip Electronics Co., Ltd */

#ifndef H_ROCKCHIP_DEBUG_H
#define H_ROCKCHIP_DEBUG_H

struct fiq_debugger_output;

#if IS_ENABLED(CONFIG_FIQ_DEBUGGER)
int rockchip_debug_dump_pcsr(struct fiq_debugger_output *output);
#endif

#endif
