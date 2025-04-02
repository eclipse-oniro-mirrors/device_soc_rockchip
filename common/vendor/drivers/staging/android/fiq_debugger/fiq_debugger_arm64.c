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

#include <linux/ptrace.h>
#include <asm/stacktrace.h>

#include "fiq_debugger_priv.h"

static char *mode_name(const struct pt_regs *regs)
{
    if (compat_user_mode(regs)) {
        return "USR";
    } else {
        switch (processor_mode(regs)) {
            case PSR_MODE_EL0t:
                return "EL0t";
            case PSR_MODE_EL1t:
                return "EL1t";
            case PSR_MODE_EL1h:
                return "EL1h";
            case PSR_MODE_EL2t:
                return "EL2t";
            case PSR_MODE_EL2h:
                return "EL2h";
            default:
                return "???";
        }
    }
}

void fiq_debugger_dump_pc(struct fiq_debugger_output *output, const struct pt_regs *regs)
{
    output->printf(output, " pc %016lx cpsr %08lx mode %s\n", regs->pc, regs->pstate, mode_name(regs));
}

void fiq_debugger_dump_regs_aarch32(struct fiq_debugger_output *output, const struct pt_regs *regs)
{
    output->printf(output, " r0 %08x  r1 %08x  r2 %08x  r3 %08x\n", regs->compat_usr(0), regs->compat_usr(1),
                   regs->compat_usr(DEBUGER_INDEX_TW), regs->compat_usr(DEBUGER_INDEX_TH));
    output->printf(output, " r4 %08x  r5 %08x  r6 %08x  r7 %08x\n", regs->compat_usr(DEBUGER_INDEX_FO),
                   regs->compat_usr(DEBUGER_INDEX_FI), regs->compat_usr(DEBUGER_INDEX_SI),
                   regs->compat_usr(DEBUGER_INDEX_SE));
    output->printf(output, " r8 %08x  r9 %08x r10 %08x r11 %08x\n", regs->compat_usr(DEBUGER_INDEX_EI),
                   regs->compat_usr(DEBUGER_INDEX_NI), regs->compat_usr(DEBUGER_INDEX_TE),
                   regs->compat_usr(DEBUGER_INDEX_EIE));
    output->printf(output, " ip %08x  sp %08x  lr %08x  pc %08x\n", regs->compat_usr(DEBUGER_INDEX_TWE),
                   regs->compat_sp, regs->compat_lr, regs->pc);
    output->printf(output, " cpsr %08x (%s)\n", regs->pstate, mode_name(regs));
}

void fiq_debugger_dump_regs_aarch64(struct fiq_debugger_output *output, const struct pt_regs *regs)
{
    output->printf(output, "  x0 %016lx   x1 %016lx\n", regs->regs[0], regs->regs[1]);
    output->printf(output, "  x2 %016lx   x3 %016lx\n", regs->regs[DEBUGER_INDEX_TW], regs->regs[DEBUGER_INDEX_TH]);
    output->printf(output, "  x4 %016lx   x5 %016lx\n", regs->regs[DEBUGER_INDEX_FO], regs->regs[DEBUGER_INDEX_FI]);
    output->printf(output, "  x6 %016lx   x7 %016lx\n", regs->regs[DEBUGER_INDEX_SI], regs->regs[DEBUGER_INDEX_SE]);
    output->printf(output, "  x8 %016lx   x9 %016lx\n", regs->regs[DEBUGER_INDEX_EI], regs->regs[DEBUGER_INDEX_NI]);
    output->printf(output, " x10 %016lx  x11 %016lx\n", regs->regs[DEBUGER_INDEX_TE], regs->regs[DEBUGER_INDEX_EIE]);
    output->printf(output, " x12 %016lx  x13 %016lx\n", regs->regs[DEBUGER_INDEX_TWE], regs->regs[DEBUGER_INDEX_THI]);
    output->printf(output, " x14 %016lx  x15 %016lx\n", regs->regs[DEBUGER_INDEX_FOU], regs->regs[DEBUGER_INDEX_FIF]);
    output->printf(output, " x16 %016lx  x17 %016lx\n", regs->regs[DEBUGER_INDEX_SIX], regs->regs[DEBUGER_INDEX_SEV]);
    output->printf(output, " x18 %016lx  x19 %016lx\n", regs->regs[DEBUGER_INDEX_EIG], regs->regs[DEBUGER_INDEX_NIN]);
    output->printf(output, " x20 %016lx  x21 %016lx\n", regs->regs[DEBUGER_INDEX_TWEN], regs->regs[DEBUGER_INDEX_TWEO]);
    output->printf(output, " x22 %016lx  x23 %016lx\n", regs->regs[DEBUGER_INDEX_TWET],
                   regs->regs[DEBUGER_INDEX_TWETH]);
    output->printf(output, " x24 %016lx  x25 %016lx\n", regs->regs[DEBUGER_INDEX_TWEF],
                   regs->regs[DEBUGER_INDEX_TWEFI]);
    output->printf(output, " x26 %016lx  x27 %016lx\n", regs->regs[DEBUGER_INDEX_TWES],
                   regs->regs[DEBUGER_INDEX_TWESE]);
    output->printf(output, " x28 %016lx  x29 %016lx\n", regs->regs[DEBUGER_INDEX_TWEE],
                   regs->regs[DEBUGER_INDEX_TWENI]);
    output->printf(output, " x30 %016lx   sp %016lx\n", regs->regs[DEBUGER_INDEX_THIR], regs->sp);
    output->printf(output, "  pc %016lx cpsr %08x (%s)\n", regs->pc, regs->pstate, mode_name(regs));
}

void fiq_debugger_dump_regs(struct fiq_debugger_output *output, const struct pt_regs *regs)
{
    if (compat_user_mode(regs)) {
        fiq_debugger_dump_regs_aarch32(output, regs);
    } else {
        fiq_debugger_dump_regs_aarch64(output, regs);
    }
}

#define READ_SPECIAL_REG(x)                                                                                            \
    ( {                                                                                                                \
        u64 val;                                                                                                       \
        asm volatile("mrs %0, " #x : "=r"(val));                                                                       \
        val;                                                                                                           \
    })

void fiq_debugger_dump_allregs(struct fiq_debugger_output *output, const struct pt_regs *regs)
{
    u32 pstate = READ_SPECIAL_REG(CurrentEl);
    bool in_el2 = (pstate & PSR_MODE_MASK) >= PSR_MODE_EL2t;

    fiq_debugger_dump_regs(output, regs);

    output->printf(output, " sp_el0   %016lx\n", READ_SPECIAL_REG(sp_el0));

    if (in_el2) {
        output->printf(output, " sp_el1   %016lx\n", READ_SPECIAL_REG(sp_el1));
    }

    output->printf(output, " elr_el1  %016lx\n", READ_SPECIAL_REG(elr_el1));

    output->printf(output, " spsr_el1 %08lx\n", READ_SPECIAL_REG(spsr_el1));

    if (in_el2) {
        output->printf(output, " spsr_irq %08lx\n", READ_SPECIAL_REG(spsr_irq));
        output->printf(output, " spsr_abt %08lx\n", READ_SPECIAL_REG(spsr_abt));
        output->printf(output, " spsr_und %08lx\n", READ_SPECIAL_REG(spsr_und));
        output->printf(output, " spsr_fiq %08lx\n", READ_SPECIAL_REG(spsr_fiq));
        output->printf(output, " spsr_el2 %08lx\n", READ_SPECIAL_REG(elr_el2));
        output->printf(output, " spsr_el2 %08lx\n", READ_SPECIAL_REG(spsr_el2));
    }
}

#ifndef CONFIG_FIQ_DEBUGGER_MODULE
struct stacktrace_state {
    struct fiq_debugger_output *output;
    unsigned int depth;
};

static bool report_trace(void *data, unsigned long pc)
{
    struct stacktrace_state *sts = data;

    if (sts->depth) {
        sts->output->printf(sts->output, "[<%016lx>] %pS:\n", pc, pc);
        sts->depth--;
        return true;
    }
    sts->output->printf(sts->output, "  ...\n");

    return sts->depth != 0;
}

void fiq_debugger_dump_stacktrace(struct fiq_debugger_output *output, const struct pt_regs *regs, unsigned int depth,
                                  void *ssp)
{
    struct stacktrace_state sts;

    sts.depth = depth;
    sts.output = output;

    if (!current) {
        output->printf(output, "current NULL\n");
    } else {
        output->printf(output, "pid: %d  comm: %s\n", current->pid, current->comm);
    }
    fiq_debugger_dump_regs(output, regs);

    if (!user_mode(regs)) {
        struct stackframe frame;
        frame.fp = regs->regs[0x1D];
        frame.pc = regs->pc;
        output->printf(output, "\n");
        walk_stackframe(current, &frame, report_trace, &sts);
    }
}
#endif
