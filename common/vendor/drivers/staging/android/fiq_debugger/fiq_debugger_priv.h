/*
 * Copyright (C) 2014 Google, Inc.
 * Author: Colin Cross <ccross@android.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef _FIQ_DEBUGGER_PRIV_H_
#define _FIQ_DEBUGGER_PRIV_H_

#define DEBUGER_INDEX_TW 2
#define DEBUGER_INDEX_TH 3
#define DEBUGER_INDEX_FO 4
#define DEBUGER_INDEX_FI 5
#define DEBUGER_INDEX_SI 6
#define DEBUGER_INDEX_SE 7
#define DEBUGER_INDEX_EI 8
#define DEBUGER_INDEX_NI 9
#define DEBUGER_INDEX_TE 10
#define DEBUGER_INDEX_EIE 11
#define DEBUGER_INDEX_TWE 12
#define DEBUGER_INDEX_THI 13
#define DEBUGER_INDEX_FOU 14
#define DEBUGER_INDEX_FIF 15
#define DEBUGER_INDEX_SIX 16
#define DEBUGER_INDEX_SEV 17
#define DEBUGER_INDEX_EIG 18
#define DEBUGER_INDEX_NIN 19
#define DEBUGER_INDEX_TWEN 20
#define DEBUGER_INDEX_TWEO 21
#define DEBUGER_INDEX_TWET 22
#define DEBUGER_INDEX_TWETH 23
#define DEBUGER_INDEX_TWEF 24
#define DEBUGER_INDEX_TWEFI 25
#define DEBUGER_INDEX_TWES 26
#define DEBUGER_INDEX_TWESE 27
#define DEBUGER_INDEX_TWEE 28
#define DEBUGER_INDEX_TWENI 29
#define DEBUGER_INDEX_THIR 30
#define DEBUGER_INDEX_THIO 31

#ifndef CONFIG_THREAD_INFO_IN_TASK
#define THREAD_INFO(sp) ((struct thread_info *)((unsigned long)(sp) & ~(THREAD_SIZE - 1)))
#endif

struct fiq_debugger_output {
    void (*printf)(struct fiq_debugger_output *output, const char *fmt, ...);
};

struct pt_regs;

void fiq_debugger_dump_pc(struct fiq_debugger_output *output, const struct pt_regs *regs);
void fiq_debugger_dump_regs(struct fiq_debugger_output *output, const struct pt_regs *regs);
void fiq_debugger_dump_allregs(struct fiq_debugger_output *output, const struct pt_regs *regs);
void fiq_debugger_dump_stacktrace(struct fiq_debugger_output *output, const struct pt_regs *regs, unsigned int depth,
                                  void *ssp);

#endif
