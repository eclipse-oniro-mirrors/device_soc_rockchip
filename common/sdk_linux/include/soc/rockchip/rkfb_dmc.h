/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Rockchip devfb driver will probe earlier than devfreq, so it needs to register
 * dmc_notify after than rk3399 dmc driver.
 */
#ifndef SOC_ROCKCHIP_RKFB_DMC_H
#define SOC_ROCKCHIP_RKFB_DMC_H

#if defined(CONFIG_LCDC_RK322X)
int vop_register_dmc(void);
#else
static inline int vop_register_dmc(void)
{
    return 0;
};
#endif
#endif /* SOC_ROCKCHIP_RKFB_DMC_H */
