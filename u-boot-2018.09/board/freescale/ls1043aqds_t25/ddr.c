// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2015 Freescale Semiconductor, Inc.
 */

#include <common.h>
#include <fsl_ddr_sdram.h>
#include <fsl_ddr_dimm_params.h>
#ifdef CONFIG_FSL_DEEP_SLEEP
#include <fsl_sleep.h>
#endif
#include <asm/arch/clock.h>
#include "ddr.h"

#define HAS_T25_DDR_4GB_RAM        /* Use 4GB DDR memory */

#ifdef HAS_T25_DDR_4GB_RAM
/* 4GB DDR memory */
#define USE_K4A8G085WC_BCTD_DDR    /* DDR model number: K4A8G085WC-BCTD (1G x 8) */
#ifdef USE_K4A8G085WC_BCTD_DDR
static char ddr_chip_info[] = "K4A8G085WC-BCTD";
#else
static char ddr_chip_info[] = "Unknown DDR";
#endif
#else
/* 2GB DDR memory */
#define FIX_RX_ERROR_COUNT_ISSUE
#define USE_SUBSTITUTE_NANYA_DDR
#ifdef USE_SUBSTITUTE_NANYA_DDR
static char ddr_chip_info[] = "SUBSTITUTE NANYA DDR";
#else
static char ddr_chip_info[] = "NT5AD512M8E3-JR";        /* Original DDR */
#endif
#endif /* End of #ifdef HAS_T25_DDR_4GB_RAM */

DECLARE_GLOBAL_DATA_PTR;

void fsl_ddr_board_options(memctl_options_t *popts,
			   dimm_params_t *pdimm,
			   unsigned int ctrl_num)
{
	const struct board_specific_parameters *pbsp, *pbsp_highest = NULL;
	ulong ddr_freq;

	if (ctrl_num > 3) {
		printf("Not supported controller number %d\n", ctrl_num);
		return;
	}
	if (!pdimm->n_ranks)
		return;

	pbsp = udimms[0];

	/* Get clk_adjust, wrlvl_start, wrlvl_ctl, according to the board ddr
	 * freqency and n_banks specified in board_specific_parameters table.
	 */
	ddr_freq = get_ddr_freq(0) / 1000000;
	while (pbsp->datarate_mhz_high) {
		if (pbsp->n_ranks == pdimm->n_ranks) {
			if (ddr_freq <= pbsp->datarate_mhz_high) {
				popts->clk_adjust = pbsp->clk_adjust;
				popts->wrlvl_start = pbsp->wrlvl_start;
				popts->wrlvl_ctl_2 = pbsp->wrlvl_ctl_2;
				popts->wrlvl_ctl_3 = pbsp->wrlvl_ctl_3;
				popts->cpo_override = pbsp->cpo_override;
				popts->write_data_delay =
					pbsp->write_data_delay;
				goto found;
			}
			pbsp_highest = pbsp;
		}
		pbsp++;
	}

	if (pbsp_highest) {
		printf("Error: board specific timing not found for %lu MT/s\n",
		       ddr_freq);
		printf("Trying to use the highest speed (%u) parameters\n",
		       pbsp_highest->datarate_mhz_high);
		popts->clk_adjust = pbsp_highest->clk_adjust;
		popts->wrlvl_start = pbsp_highest->wrlvl_start;
		popts->wrlvl_ctl_2 = pbsp->wrlvl_ctl_2;
		popts->wrlvl_ctl_3 = pbsp->wrlvl_ctl_3;
	} else {
		panic("DIMM is not supported by this board");
	}
found:
	debug("Found timing match: n_ranks %d, data rate %d, rank_gb %d\n",
	      pbsp->n_ranks, pbsp->datarate_mhz_high, pbsp->rank_gb);

	/* force DDR bus width to 32 bits */
	popts->data_bus_width = 1;
	popts->otf_burst_chop_en = 0;
	popts->burst_length = DDR_BL8;
	popts->bstopre = 0;		/* enable auto precharge */

	/*
	 * Factors to consider for half-strength driver enable:
	 *	- number of DIMMs installed
	 */
	popts->half_strength_driver_enable = 1;
	/*
	 * Write leveling override
	 */
	popts->wrlvl_override = 1;
	popts->wrlvl_sample = 0xf;

	/*
	 * Rtt and Rtt_WR override
	 */
	popts->rtt_override = 0;

	/* Enable ZQ calibration */
	popts->zq_en = 1;

#ifdef CONFIG_SYS_FSL_DDR4
	popts->ddr_cdr1 = DDR_CDR1_DHC_EN | DDR_CDR1_ODT(DDR_CDR_ODT_80ohm);
	popts->ddr_cdr2 = DDR_CDR2_ODT(DDR_CDR_ODT_80ohm) |
			  DDR_CDR2_VREF_OVRD(70);	/* Vref = 70% */

	/* optimize cpo for erratum A-009942 */
	popts->cpo_sample = 0x59;
#else
	popts->cswl_override = DDR_CSWL_CS0;

	/* DHC_EN =1, ODT = 75 Ohm */
	popts->ddr_cdr1 = DDR_CDR1_DHC_EN | DDR_CDR1_ODT(DDR_CDR_ODT_75ohm);
	popts->ddr_cdr2 = DDR_CDR2_ODT(DDR_CDR_ODT_75ohm);
#endif
	popts->cpo_sample = 0x41;
}

/* DDR model number: NT5AD1024M8A3-HR (1024Mb x 8) */
#ifdef CONFIG_SYS_DDR_RAW_TIMING
dimm_params_t ddr_raw_timing = {
	.n_ranks = 1,
#ifdef USE_K4A8G085WC_BCTD_DDR
	.rank_density = 0x100000000,    /* 4GB */
	.capacity = 0x100000000,        /* 4GB */
#else
	.rank_density = 0x80000000,     /* 2GB */
	.capacity = 0x80000000,         /* 2GB */
#endif
	.primary_sdram_width = 32,
	.ec_sdram_width = 0,
	.registered_dimm = 0,
	.mirrored_dimm = 0,
#ifdef USE_K4A8G085WC_BCTD_DDR
	.n_row_addr = 16,					// P.1
#else
	.n_row_addr = 15,                                       // P.1
#endif
	.n_col_addr = 10,				        // P.1
	.bank_addr_bits = 2,					// P.1
	.bank_group_bits = 2,					// P.1
	.edc_config = 0,
	.burst_lengths_bitmask = 0x0c,

#ifdef USE_SUBSTITUTE_NANYA_DDR
	.tckmin_x_ps = 1250,					// P.243
	.tckmax_ps = 1375,					// P.243
#else
	.tckmin_x_ps = 1250,                                    // P.242 P.273 P.249
	.tckmax_ps = 1500,
#endif	
	.caslat_x = 0x000DFA00,
#ifdef USE_K4A8G085WC_BCTD_DDR
	.taa_ps = 12500,					// P.249 P.246 taa = tCK * CL 
	.trcd_ps = 12500,					// P.249 11 nCK		
	.trp_ps = 12500,					// P.249 11 nCK		
#else
	.taa_ps = 13750,                                        // P.249 P.246 taa = tCK * CL
	.trcd_ps = 13750,                                       // P.249 11 nCK
	.trp_ps = 13750,
#endif
#ifdef USE_SUBSTITUTE_NANYA_DDR
	.tras_ps = 35000,					// P.243	
	.trc_ps = 35000,					// P.243
#else
	.tras_ps = 35000,                                       // P.249 28 nCK
	#ifdef USE_K4A8G085WC_BCTD_DDR
	.trc_ps = 47500,
	#else
	.trc_ps = 48750,
	#endif
#endif
#ifdef USE_K4A8G085WC_BCTD_DDR
	.trfc1_ps = 350000,					// P.278 350 ns
#else
	.trfc1_ps = 260000,                                     // P.278 260 ns
#endif
#ifdef USE_SUBSTITUTE_NANYA_DDR
	.trfc2_ps = 260000,					// P.285
	.trfc4_ps = 260000,					// P.285
	.tfaw_ps = 25000,					// P.280
	.trrds_ps = 6250,					// P.280
	.trrdl_ps = 6250,					// P.280
	#ifdef FIX_RX_ERROR_COUNT_ISSUE
	.tccdl_ps = 6250,					// P.280 from Senao's 0001-uboot-Modify-ddr-config-for-tCCD_L.patch
	#else
	.tccdl_ps = 62500,					// P.280
	#endif
#else
	#ifdef USE_K4A8G085WC_BCTD_DDR
	.trfc2_ps = 350000,                                     // P.278 350 ns
	.trfc4_ps = 350000,                                     // P.278 350 ns
	#else
	.trfc2_ps = 160000,                                     // P.278 160 ns
	.trfc4_ps = 110000,                                     // P.278 110 ns
	#endif
	.tfaw_ps = 21000,                                       // P.274 20 nCK
	.trrds_ps = 5000,                                       // P.274 4 nCK
	.trrdl_ps = 6000,                                       // P.274 4 nCK
	.tccdl_ps = 6250,
#endif
	.refresh_rate_ps = 3900000,				// P.1 Tc > 85 C
	.dq_mapping[0] = 0x15,			
	.dq_mapping[1] = 0x36,			
	.dq_mapping[2] = 0x15,			
	.dq_mapping[3] = 0x22,			
	.dq_mapping[4] = 0x16,			
	.dq_mapping[5] = 0x36,			
	.dq_mapping[6] = 0x15,			
	.dq_mapping[7] = 0x36,			
	.dq_mapping[8] = 0x0,
	.dq_mapping[9] = 0x0,
	.dq_mapping[10] = 0x0,
	.dq_mapping[11] = 0x0,
	.dq_mapping[12] = 0x0,
	.dq_mapping[13] = 0x0,
	.dq_mapping[14] = 0x0,
	.dq_mapping[15] = 0x0,
	.dq_mapping[16] = 0x0,
	.dq_mapping[17] = 0x0,
	.dq_mapping_ors = 0,
};

int fsl_ddr_get_dimm_params(dimm_params_t *pdimm,
		unsigned int controller_number,
		unsigned int dimm_number)
{
	const char dimm_model[] = "Fixed DDR4 on board";

	if (((controller_number == 0) && (dimm_number == 0)) ||
		((controller_number == 1) && (dimm_number == 0))) {
			memcpy(pdimm, &ddr_raw_timing, sizeof(dimm_params_t));
			memset(pdimm->mpart, 0, sizeof(pdimm->mpart));
			memcpy(pdimm->mpart, dimm_model, sizeof(dimm_model) - 1);
	}

	return 0;
}
#endif

#ifdef CONFIG_TFABOOT
int fsl_initdram(void)
{
	gd->ram_size = tfa_get_dram_size();
	if (!gd->ram_size)
		gd->ram_size = fsl_ddr_sdram_size();

	return 0;
}
#else
int fsl_initdram(void)
{
	phys_size_t dram_size;

#if defined(CONFIG_SPL) && !defined(CONFIG_SPL_BUILD)
	gd->ram_size = fsl_ddr_sdram_size();

	return 0;
#else
#ifndef CONFIG_SYS_DDR_RAW_TIMING
	puts("Initializing DDR....using SPD\n");
#else
	printf("Initializing DDR...(%s)(ddr_raw_timing: capacity = 0x%lx, tckmax_ps = %d, tccdl_ps = %d)\n", ddr_chip_info, ddr_raw_timing.capacity, ddr_raw_timing.tckmax_ps, ddr_raw_timing.tccdl_ps);
#endif

	dram_size = fsl_ddr_sdram();
#endif
	erratum_a008850_post();

#ifdef CONFIG_FSL_DEEP_SLEEP
	fsl_dp_ddr_restore();
#endif

	gd->ram_size = dram_size;

	return 0;
}
#endif
