/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 Fuzhou Rockchip Electronics Co., Ltd
 */
#ifndef __ROCKCHIP_CPUFREQ_H
#define __ROCKCHIP_CPUFREQ_H

#define ROCKCHIP_SOC_INFO_ZERO 0
#define ROCKCHIP_SOC_INFO_ONE 1
#define ROCKCHIP_SOC_INFO_TWO 2
#define ROCKCHIP_SOC_INFO_THREE 3
#define ROCKCHIP_SOC_INFO_FOUR 4

#if IS_ENABLED(CONFIG_ARM_ROCKCHIP_CPUFREQ)
int rockchip_cpufreq_adjust_power_scale(struct device *dev);
int rockchip_cpufreq_opp_set_rate(struct device *dev, unsigned long target_freq);
#else
static inline int rockchip_cpufreq_adjust_power_scale(struct device *dev)
{
    return -EOPNOTSUPP;
}

static inline int rockchip_cpufreq_opp_set_rate(struct device *dev, unsigned long target_freq)
{
    return -EOPNOTSUPP;
}
#endif /* CONFIG_ARM_ROCKCHIP_CPUFREQ */

#endif
