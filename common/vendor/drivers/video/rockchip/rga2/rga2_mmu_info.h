/* SPDX-License-Identifier: GPL-2.0 */
#ifndef H_RGA2_MMU_INFO_H
#define H_RGA2_MMU_INFO_H

#include "rga2.h"
#include "RGA2_API.h"

#ifndef MIN
#define MIN(X, Y) ((X) < (Y) ? (X) : (Y))
#endif

#ifndef MAX
#define MAX(X, Y) ((X) > (Y) ? (X) : (Y))
#endif

extern struct rga2_drvdata_t *rga2_drvdata;

enum {
    MMU_MAP_CLEAN = 1 << 0,
    MMU_MAP_INVALID = 1 << 1,
    MMU_MAP_MASK = 0x03,
    MMU_UNMAP_CLEAN = 1 << 2,
    MMU_UNMAP_INVALID = 1 << 3,
    MMU_UNMAP_MASK = 0x0c,
};

int rga2_set_mmu_info(struct rga2_reg *reg, struct rga2_req *req);
void rga2_dma_flush_range(void *pstart, void *pend);
dma_addr_t rga2_dma_flush_page(struct page *page, int map);

int rga2_get_dma_info(struct rga2_reg *reg, struct rga2_req *req);
void rga2_put_dma_info(struct rga2_reg *reg);

extern struct rga2_service_info rga2_service;
extern struct rga2_mmu_buf_t rga2_mmu_buf;
extern struct rga2_drvdata_t *rga2_drvdata;

#endif
