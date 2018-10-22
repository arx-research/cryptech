/* 
 * novena-eim.c
 * ------------
 * This module contains the userland magic to set up and use the EIM bus.
 *
 * 
 * Author: Pavel Shatov
 * Copyright (c) 2014-2015, NORDUnet A/S All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * - Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 *
 * - Neither the name of the NORDUnet nor the names of its contributors may
 *   be used to endorse or promote products derived from this software
 *   without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

//------------------------------------------------------------------------------
// Headers
//------------------------------------------------------------------------------
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/mman.h>

#include "novena-eim.h"


//------------------------------------------------------------------------------
// Defines
//------------------------------------------------------------------------------
#define MEMORY_DEVICE   "/dev/mem"

#define IOMUXC_MUX_MODE_ALT0                    0       // 000

#define IOMUXC_PAD_CTL_SRE_FAST                 1       // 1
#define IOMUXC_PAD_CTL_DSE_33_OHM               7       // 111
#define IOMUXC_PAD_CTL_SPEED_MEDIUM_10          2       // 10
#define IOMUXC_PAD_CTL_ODE_DISABLED             0       // 0
#define IOMUXC_PAD_CTL_PKE_DISABLED             0       // 0
#define IOMUXC_PAD_CTL_PUE_PULL                 1       // 1
#define IOMUXC_PAD_CTL_PUS_100K_OHM_PU          2       // 10
#define IOMUXC_PAD_CTL_HYS_DISABLED             0       // 0

#define CCM_CGR_OFF                             0       // 00
#define CCM_CGR_ON_EXCEPT_STOP                  3       // 11


//------------------------------------------------------------------------------
// CPU Registers
//------------------------------------------------------------------------------
enum IMX6DQ_REGISTER_OFFSET
{
        IOMUXC_SW_MUX_CTL_PAD_EIM_CS0_B         = 0x020E00F8,
        IOMUXC_SW_MUX_CTL_PAD_EIM_OE_B          = 0x020E0100,
        IOMUXC_SW_MUX_CTL_PAD_EIM_RW            = 0x020E0104,
        IOMUXC_SW_MUX_CTL_PAD_EIM_LBA_B         = 0x020E0108,
        IOMUXC_SW_MUX_CTL_PAD_EIM_AD00          = 0x020E0114,
        IOMUXC_SW_MUX_CTL_PAD_EIM_AD01          = 0x020E0118,
        IOMUXC_SW_MUX_CTL_PAD_EIM_AD02          = 0x020E011C,
        IOMUXC_SW_MUX_CTL_PAD_EIM_AD03          = 0x020E0120,
        IOMUXC_SW_MUX_CTL_PAD_EIM_AD04          = 0x020E0124,
        IOMUXC_SW_MUX_CTL_PAD_EIM_AD05          = 0x020E0128,
        IOMUXC_SW_MUX_CTL_PAD_EIM_AD06          = 0x020E012C,
        IOMUXC_SW_MUX_CTL_PAD_EIM_AD07          = 0x020E0130,
        IOMUXC_SW_MUX_CTL_PAD_EIM_AD08          = 0x020E0134,
        IOMUXC_SW_MUX_CTL_PAD_EIM_AD09          = 0x020E0138,
        IOMUXC_SW_MUX_CTL_PAD_EIM_AD10          = 0x020E013C,
        IOMUXC_SW_MUX_CTL_PAD_EIM_AD11          = 0x020E0140,
        IOMUXC_SW_MUX_CTL_PAD_EIM_AD12          = 0x020E0144,
        IOMUXC_SW_MUX_CTL_PAD_EIM_AD13          = 0x020E0148,
        IOMUXC_SW_MUX_CTL_PAD_EIM_AD14          = 0x020E014C,
        IOMUXC_SW_MUX_CTL_PAD_EIM_AD15          = 0x020E0150,
        IOMUXC_SW_MUX_CTL_PAD_EIM_WAIT_B        = 0x020E0154,
        IOMUXC_SW_MUX_CTL_PAD_EIM_BCLK          = 0x020E0158,

        IOMUXC_SW_PAD_CTL_PAD_EIM_CS0_B         = 0x020E040C,
        IOMUXC_SW_PAD_CTL_PAD_EIM_OE_B          = 0x020E0414,
        IOMUXC_SW_PAD_CTL_PAD_EIM_RW            = 0x020E0418,
        IOMUXC_SW_PAD_CTL_PAD_EIM_LBA_B         = 0x020E041C,
        IOMUXC_SW_PAD_CTL_PAD_EIM_AD00          = 0x020E0428,
        IOMUXC_SW_PAD_CTL_PAD_EIM_AD01          = 0x020E042C,
        IOMUXC_SW_PAD_CTL_PAD_EIM_AD02          = 0x020E0430,
        IOMUXC_SW_PAD_CTL_PAD_EIM_AD03          = 0x020E0434,
        IOMUXC_SW_PAD_CTL_PAD_EIM_AD04          = 0x020E0438,
        IOMUXC_SW_PAD_CTL_PAD_EIM_AD05          = 0x020E043C,
        IOMUXC_SW_PAD_CTL_PAD_EIM_AD06          = 0x020E0440,
        IOMUXC_SW_PAD_CTL_PAD_EIM_AD07          = 0x020E0444,
        IOMUXC_SW_PAD_CTL_PAD_EIM_AD08          = 0x020E0448,
        IOMUXC_SW_PAD_CTL_PAD_EIM_AD09          = 0x020E044C,
        IOMUXC_SW_PAD_CTL_PAD_EIM_AD10          = 0x020E0450,
        IOMUXC_SW_PAD_CTL_PAD_EIM_AD11          = 0x020E0454,
        IOMUXC_SW_PAD_CTL_PAD_EIM_AD12          = 0x020E0458,
        IOMUXC_SW_PAD_CTL_PAD_EIM_AD13          = 0x020E045C,
        IOMUXC_SW_PAD_CTL_PAD_EIM_AD14          = 0x020E0460,
        IOMUXC_SW_PAD_CTL_PAD_EIM_AD15          = 0x020E0464,
        IOMUXC_SW_PAD_CTL_PAD_EIM_WAIT_B        = 0x020E0468,
        IOMUXC_SW_PAD_CTL_PAD_EIM_BCLK          = 0x020E046C,
        
        CCM_CCGR6                               = 0x020C4080,
        
        EIM_CS0GCR1                             = 0x021B8000,
        EIM_CS0GCR2                             = 0x021B8004,
        EIM_CS0RCR1                             = 0x021B8008,
        EIM_CS0RCR2                             = 0x021B800C,
        EIM_CS0WCR1                             = 0x021B8010,
        EIM_CS0WCR2                             = 0x021B8014,

        EIM_WCR                                 = 0x021B8090,
        EIM_WIAR                                = 0x021B8094,
        EIM_EAR                                 = 0x021B8098,
};


//------------------------------------------------------------------------------
// Structures
//------------------------------------------------------------------------------
struct IOMUXC_SW_MUX_CTL_PAD_EIM
{
        unsigned int    mux_mode                :  3;
        unsigned int    reserved_3              :  1;
        unsigned int    sion                    :  1;
        unsigned int    reserved_31_5           : 27;
};

struct IOMUXC_SW_PAD_CTL_PAD_EIM
{
        unsigned int    sre                     : 1;
        unsigned int    reserved_2_1            : 2;
        unsigned int    dse                     : 3;
        unsigned int    speed                   : 2;
        unsigned int    reserved_10_8           : 3;
        unsigned int    ode                     : 1;
        unsigned int    pke                     : 1;
        unsigned int    pue                     : 1;
        unsigned int    pus                     : 2;
        unsigned int    hys                     : 1;
        unsigned int    reserved_31_17          : 15;
};

struct CCM_CCGR6
{
        unsigned int    cg0_usboh3              : 2;
        unsigned int    cg1_usdhc1              : 2;
        unsigned int    cg2_usdhc2              : 2;
        unsigned int    cg3_usdhc3              : 2;
        
        unsigned int    cg3_usdhc4              : 2;
        unsigned int    cg5_eim_slow            : 2;
        unsigned int    cg6_vdoaxiclk           : 2;
        unsigned int    cg7_vpu                 : 2;
        
        unsigned int    cg8_reserved            : 2;
        unsigned int    cg9_reserved            : 2;
        unsigned int    cg10_reserved           : 2;
        unsigned int    cg11_reserved           : 2;
        
        unsigned int    cg12_reserved           : 2;
        unsigned int    cg13_reserved           : 2;
        unsigned int    cg14_reserved           : 2;
        unsigned int    cg15_reserved           : 2;
};

struct EIM_CS_GCR1
{
        unsigned int    csen                    : 1;
        unsigned int    swr                     : 1;
        unsigned int    srd                     : 1;
        unsigned int    mum                     : 1;
        unsigned int    wfl                     : 1;
        unsigned int    rfl                     : 1;
        unsigned int    cre                     : 1;
        unsigned int    crep                    : 1;
        unsigned int    bl                      : 3;
        unsigned int    wc                      : 1;
        unsigned int    bcd                     : 2;
        unsigned int    bcs                     : 2;
        unsigned int    dsz                     : 3;
        unsigned int    sp                      : 1;
        unsigned int    csrec                   : 3;
        unsigned int    aus                     : 1;
        unsigned int    gbc                     : 3;
        unsigned int    wp                      : 1;
        unsigned int    psz                     : 4;
};

struct EIM_CS_GCR2
{
        unsigned int    adh                     :  2;
        unsigned int    reserved_3_2            :  2;
        unsigned int    daps                    :  4;
        unsigned int    dae                     :  1;
        unsigned int    dap                     :  1;
        unsigned int    reserved_11_10          :  2;
        unsigned int    mux16_byp_grant         :  1;
        unsigned int    reserved_31_13          : 19;
};

struct EIM_CS_RCR1
{
        unsigned int    rcsn                    : 3;
        unsigned int    reserved_3              : 1;
        unsigned int    rcsa                    : 3;
        unsigned int    reserved_7              : 1;
        unsigned int    oen                     : 3;
        unsigned int    reserved_11             : 1;
        unsigned int    oea                     : 3;
        unsigned int    reserved_15             : 1;
        unsigned int    radvn                   : 3;
        unsigned int    ral                     : 1;
        unsigned int    radva                   : 3;
        unsigned int    reserved_23             : 1;
        unsigned int    rwsc                    : 6;
        unsigned int    reserved_31_30          : 2;
};

struct EIM_CS_RCR2
{
        unsigned int    rben                    :  3;
        unsigned int    rbe                     :  1;
        unsigned int    rbea                    :  3;
        unsigned int    reserved_7              :  1;
        unsigned int    rl                      :  2;
        unsigned int    reserved_11_10          :  2;
        unsigned int    pat                     :  3;
        unsigned int    apr                     :  1;
        unsigned int    reserved_31_16          : 16;
};

struct EIM_CS_WCR1
{
        unsigned int    wcsn                    : 3;
        unsigned int    wcsa                    : 3;
        unsigned int    wen                     : 3;
        unsigned int    wea                     : 3;
        unsigned int    wben                    : 3;
        unsigned int    wbea                    : 3;
        unsigned int    wadvn                   : 3;
        unsigned int    wadva                   : 3;
        unsigned int    wwsc                    : 6;
        unsigned int    wbed                    : 1;
        unsigned int    wal                     : 1;
};

struct EIM_CS_WCR2
{
        unsigned int    wbcdd                   :  1;
        unsigned int    reserved_31_1           : 31;
};

struct EIM_WCR
{
        unsigned int    bcm                     :  1;
        unsigned int    gbcd                    :  2;
        unsigned int    reserved_3              :  1;
        unsigned int    inten                   :  1;
        unsigned int    intpol                  :  1;
        unsigned int    reserved_7_6            :  2;
        unsigned int    wdog_en                 :  1;
        unsigned int    wdog_limit              :  2;
        unsigned int    reserved_31_11          : 21;
};

struct EIM_WIAR
{
        unsigned int    ips_req                 :  1;
        unsigned int    ips_ack                 :  1;
        unsigned int    irq                     :  1;
        unsigned int    errst                   :  1;
        unsigned int    aclk_en                 :  1;
        unsigned int    reserved_31_5           : 27;
};

struct EIM_EAR
{
        unsigned int    error_addr              : 32;
};


//------------------------------------------------------------------------------
// Variables
//------------------------------------------------------------------------------
static long     mem_page_size   = 0;
static int      mem_dev_fd      = -1;
static void *   mem_map_ptr     = MAP_FAILED;
static off_t    mem_base_addr   = 0;


//------------------------------------------------------------------------------
// Prototypes
//------------------------------------------------------------------------------
static void     _eim_setup_iomuxc       (void);
static void     _eim_setup_ccm          (void);
static void     _eim_setup_eim          (void);
static void     _eim_cleanup            (void);
static off_t    _eim_calc_offset        (off_t);
static void     _eim_remap_mem          (off_t);


//------------------------------------------------------------------------------
// Set up EIM bus. Returns 0 on success, -1 on failure.
//------------------------------------------------------------------------------
int eim_setup(void)
{
    // register cleanup function
    if (atexit(_eim_cleanup) != 0) {
        fprintf(stderr, "ERROR: atexit() failed.\n");
        return -1;
    }

    // determine memory page size to use in mmap()
    mem_page_size = sysconf(_SC_PAGESIZE);
    if (mem_page_size < 1) {
        fprintf(stderr, "ERROR: sysconf(_SC_PAGESIZE) == %ld\n", mem_page_size);
        return -1;
    }

    // try to open memory device
    mem_dev_fd = open(MEMORY_DEVICE, O_RDWR | O_SYNC);
    if (mem_dev_fd == -1) {
        fprintf(stderr, "ERROR: open(%s) failed.\n", MEMORY_DEVICE);
        return -1;
    }

    // configure IOMUXC
    _eim_setup_iomuxc();

    // configure Clock Controller Module
    _eim_setup_ccm();

    /* We need to properly configure EIM mode and all the corresponding parameters.
     * That's a lot of code, let's do it now.
     */
    _eim_setup_eim();

    // done
    return 0;
}


//------------------------------------------------------------------------------
// Shut down EIM bus. This is called automatically on exit().
//------------------------------------------------------------------------------
static void _eim_cleanup(void)
{
    // unmap memory if needed
    if (mem_map_ptr != MAP_FAILED)
        if (munmap(mem_map_ptr, mem_page_size) != 0)
            fprintf(stderr, "WARNING: munmap() failed.\n");

    // close memory device if needed
    if (mem_dev_fd != -1)
        if (close(mem_dev_fd) != 0)
            fprintf(stderr, "WARNING: close() failed.\n");
}


//------------------------------------------------------------------------------
// Several blocks in the CPU have common pins. We use the I/O MUX Controller
// to configure what block will actually use I/O pins. We wait for the EIM
// module to be able to communicate with the on-board FPGA.
//------------------------------------------------------------------------------
static void _eim_setup_iomuxc(void)
{
    // create structures
    struct IOMUXC_SW_MUX_CTL_PAD_EIM    reg_mux;                        // mux control register
    struct IOMUXC_SW_PAD_CTL_PAD_EIM    reg_pad;                        // pad control register

    // setup mux control register
    reg_mux.mux_mode            = IOMUXC_MUX_MODE_ALT0;                 // ALT0 mode must be used for EIM
    reg_mux.sion                = 0;                                    // forced input not needed
    reg_mux.reserved_3          = 0;                                    // must be 0
    reg_mux.reserved_31_5       = 0;                                    // must be 0

    // setup pad control register
    reg_pad.sre                 = IOMUXC_PAD_CTL_SRE_FAST;              // fast slew rate
    reg_pad.dse                 = IOMUXC_PAD_CTL_DSE_33_OHM;            // highest drive strength
    reg_pad.speed               = IOMUXC_PAD_CTL_SPEED_MEDIUM_10;       // medium speed
    reg_pad.ode                 = IOMUXC_PAD_CTL_ODE_DISABLED;          // open drain not needed
    reg_pad.pke                 = IOMUXC_PAD_CTL_PKE_DISABLED;          // neither pull nor keeper are needed
    reg_pad.pue                 = IOMUXC_PAD_CTL_PUE_PULL;              // doesn't matter actually, because PKE is disabled
    reg_pad.pus                 = IOMUXC_PAD_CTL_PUS_100K_OHM_PU;       // doesn't matter actually, because PKE is disabled
    reg_pad.hys                 = IOMUXC_PAD_CTL_HYS_DISABLED;          // use CMOS, not Schmitt trigger input
    reg_pad.reserved_2_1        = 0;                                    // must be 0
    reg_pad.reserved_10_8       = 0;                                    // must be 0
    reg_pad.reserved_31_17      = 0;                                    // must be 0

    // all the pins must be configured to use the same ALT0 mode
    eim_write_32(IOMUXC_SW_MUX_CTL_PAD_EIM_CS0_B,       (uint32_t *)&reg_mux);
    eim_write_32(IOMUXC_SW_MUX_CTL_PAD_EIM_OE_B,        (uint32_t *)&reg_mux);
    eim_write_32(IOMUXC_SW_MUX_CTL_PAD_EIM_RW,          (uint32_t *)&reg_mux);
    eim_write_32(IOMUXC_SW_MUX_CTL_PAD_EIM_LBA_B,       (uint32_t *)&reg_mux);
    eim_write_32(IOMUXC_SW_MUX_CTL_PAD_EIM_AD00,        (uint32_t *)&reg_mux);
    eim_write_32(IOMUXC_SW_MUX_CTL_PAD_EIM_AD01,        (uint32_t *)&reg_mux);
    eim_write_32(IOMUXC_SW_MUX_CTL_PAD_EIM_AD02,        (uint32_t *)&reg_mux);
    eim_write_32(IOMUXC_SW_MUX_CTL_PAD_EIM_AD03,        (uint32_t *)&reg_mux);
    eim_write_32(IOMUXC_SW_MUX_CTL_PAD_EIM_AD04,        (uint32_t *)&reg_mux);
    eim_write_32(IOMUXC_SW_MUX_CTL_PAD_EIM_AD05,        (uint32_t *)&reg_mux);
    eim_write_32(IOMUXC_SW_MUX_CTL_PAD_EIM_AD06,        (uint32_t *)&reg_mux);
    eim_write_32(IOMUXC_SW_MUX_CTL_PAD_EIM_AD07,        (uint32_t *)&reg_mux);
    eim_write_32(IOMUXC_SW_MUX_CTL_PAD_EIM_AD08,        (uint32_t *)&reg_mux);
    eim_write_32(IOMUXC_SW_MUX_CTL_PAD_EIM_AD09,        (uint32_t *)&reg_mux);
    eim_write_32(IOMUXC_SW_MUX_CTL_PAD_EIM_AD10,        (uint32_t *)&reg_mux);
    eim_write_32(IOMUXC_SW_MUX_CTL_PAD_EIM_AD11,        (uint32_t *)&reg_mux);
    eim_write_32(IOMUXC_SW_MUX_CTL_PAD_EIM_AD12,        (uint32_t *)&reg_mux);
    eim_write_32(IOMUXC_SW_MUX_CTL_PAD_EIM_AD13,        (uint32_t *)&reg_mux);
    eim_write_32(IOMUXC_SW_MUX_CTL_PAD_EIM_AD14,        (uint32_t *)&reg_mux);
    eim_write_32(IOMUXC_SW_MUX_CTL_PAD_EIM_AD15,        (uint32_t *)&reg_mux);
    eim_write_32(IOMUXC_SW_MUX_CTL_PAD_EIM_WAIT_B,      (uint32_t *)&reg_mux);
    eim_write_32(IOMUXC_SW_MUX_CTL_PAD_EIM_BCLK,        (uint32_t *)&reg_mux);

    // we need to configure all the I/O pads too
    eim_write_32(IOMUXC_SW_PAD_CTL_PAD_EIM_CS0_B,       (uint32_t *)&reg_pad);
    eim_write_32(IOMUXC_SW_PAD_CTL_PAD_EIM_OE_B,        (uint32_t *)&reg_pad);
    eim_write_32(IOMUXC_SW_PAD_CTL_PAD_EIM_RW,          (uint32_t *)&reg_pad);
    eim_write_32(IOMUXC_SW_PAD_CTL_PAD_EIM_LBA_B,       (uint32_t *)&reg_pad);
    eim_write_32(IOMUXC_SW_PAD_CTL_PAD_EIM_AD00,        (uint32_t *)&reg_pad);
    eim_write_32(IOMUXC_SW_PAD_CTL_PAD_EIM_AD01,        (uint32_t *)&reg_pad);
    eim_write_32(IOMUXC_SW_PAD_CTL_PAD_EIM_AD02,        (uint32_t *)&reg_pad);
    eim_write_32(IOMUXC_SW_PAD_CTL_PAD_EIM_AD03,        (uint32_t *)&reg_pad);
    eim_write_32(IOMUXC_SW_PAD_CTL_PAD_EIM_AD04,        (uint32_t *)&reg_pad);
    eim_write_32(IOMUXC_SW_PAD_CTL_PAD_EIM_AD05,        (uint32_t *)&reg_pad);
    eim_write_32(IOMUXC_SW_PAD_CTL_PAD_EIM_AD06,        (uint32_t *)&reg_pad);
    eim_write_32(IOMUXC_SW_PAD_CTL_PAD_EIM_AD07,        (uint32_t *)&reg_pad);
    eim_write_32(IOMUXC_SW_PAD_CTL_PAD_EIM_AD08,        (uint32_t *)&reg_pad);
    eim_write_32(IOMUXC_SW_PAD_CTL_PAD_EIM_AD09,        (uint32_t *)&reg_pad);
    eim_write_32(IOMUXC_SW_PAD_CTL_PAD_EIM_AD10,        (uint32_t *)&reg_pad);
    eim_write_32(IOMUXC_SW_PAD_CTL_PAD_EIM_AD11,        (uint32_t *)&reg_pad);
    eim_write_32(IOMUXC_SW_PAD_CTL_PAD_EIM_AD12,        (uint32_t *)&reg_pad);
    eim_write_32(IOMUXC_SW_PAD_CTL_PAD_EIM_AD13,        (uint32_t *)&reg_pad);
    eim_write_32(IOMUXC_SW_PAD_CTL_PAD_EIM_AD14,        (uint32_t *)&reg_pad);
    eim_write_32(IOMUXC_SW_PAD_CTL_PAD_EIM_AD15,        (uint32_t *)&reg_pad);
    eim_write_32(IOMUXC_SW_PAD_CTL_PAD_EIM_WAIT_B,      (uint32_t *)&reg_pad);
    eim_write_32(IOMUXC_SW_PAD_CTL_PAD_EIM_BCLK,        (uint32_t *)&reg_pad);
}


//------------------------------------------------------------------------------
// Configure Clock Controller Module to enable clocking of EIM block.
//------------------------------------------------------------------------------
static void _eim_setup_ccm(void)
{
    // create structure
    struct CCM_CCGR6 ccm_ccgr6;

    // read register
    eim_read_32(CCM_CCGR6, (uint32_t *)&ccm_ccgr6);

    // modify register
    ccm_ccgr6.cg0_usboh3                = CCM_CGR_ON_EXCEPT_STOP;
    ccm_ccgr6.cg1_usdhc1                = CCM_CGR_OFF;
    ccm_ccgr6.cg2_usdhc2                = CCM_CGR_ON_EXCEPT_STOP;
    ccm_ccgr6.cg3_usdhc3                = CCM_CGR_ON_EXCEPT_STOP;

    ccm_ccgr6.cg3_usdhc4                = CCM_CGR_OFF;
    ccm_ccgr6.cg5_eim_slow              = CCM_CGR_ON_EXCEPT_STOP;
    ccm_ccgr6.cg6_vdoaxiclk             = CCM_CGR_OFF;
    ccm_ccgr6.cg7_vpu                   = CCM_CGR_OFF;

    ccm_ccgr6.cg8_reserved              = 0;
    ccm_ccgr6.cg9_reserved              = 0;
    ccm_ccgr6.cg10_reserved             = 0;
    ccm_ccgr6.cg11_reserved             = 0;
    ccm_ccgr6.cg12_reserved             = 0;
    ccm_ccgr6.cg13_reserved             = 0;
    ccm_ccgr6.cg14_reserved             = 0;
    ccm_ccgr6.cg15_reserved             = 0;

    // write register
    eim_write_32(CCM_CCGR6, (uint32_t *)&ccm_ccgr6);
}


//------------------------------------------------------------------------------
// Configure EIM mode and all the corresponding parameters. That's a lot of code.
//------------------------------------------------------------------------------
static void _eim_setup_eim(void)
{
    // create structures
    struct EIM_CS_GCR1  gcr1;
    struct EIM_CS_GCR2  gcr2;
    struct EIM_CS_RCR1  rcr1;
    struct EIM_CS_RCR2  rcr2;
    struct EIM_CS_WCR1  wcr1;
    struct EIM_CS_WCR2  wcr2;

    struct EIM_WCR              wcr;
    struct EIM_WIAR             wiar;
    struct EIM_EAR              ear;

    // read all the registers
    eim_read_32(EIM_CS0GCR1, (uint32_t *)&gcr1);
    eim_read_32(EIM_CS0GCR2, (uint32_t *)&gcr2);
    eim_read_32(EIM_CS0RCR1, (uint32_t *)&rcr1);
    eim_read_32(EIM_CS0RCR2, (uint32_t *)&rcr2);
    eim_read_32(EIM_CS0WCR1, (uint32_t *)&wcr1);
    eim_read_32(EIM_CS0WCR2, (uint32_t *)&wcr2);

    eim_read_32(EIM_WCR,        (uint32_t *)&wcr);
    eim_read_32(EIM_WIAR,       (uint32_t *)&wiar);
    eim_read_32(EIM_EAR,        (uint32_t *)&ear);

    // manipulate registers as needed
    gcr1.csen           = 1;    // chip select is enabled
    gcr1.swr            = 1;    // write is sync
    gcr1.srd            = 1;    // read is sync
    gcr1.mum            = 1;    // address and data are multiplexed
    gcr1.wfl            = 0;    // write latency is not fixed
    gcr1.rfl            = 0;    // read latency is not fixed
    gcr1.cre            = 0;    // CRE signal not needed
    //gcr1.crep         = x;    // don't care, CRE not used
    gcr1.bl             = 4;    // burst length
    gcr1.wc             = 0;    // write is not continuous
    gcr1.bcd            = 3;    // BCLK divisor is 3+1=4
    gcr1.bcs            = 1;    // delay from ~CS to BCLK is 1 cycle
    gcr1.dsz            = 1;    // 16 bits per databeat at DATA[15:0]
    gcr1.sp             = 0;    // supervisor protection is disabled
    gcr1.csrec          = 1;    // ~CS recovery is 1 cycle
    gcr1.aus            = 1;    // address is not shifted
    gcr1.gbc            = 1;    // ~CS gap is 1 cycle
    gcr1.wp             = 0;    // write protection is not enabled
    //gcr1.psz          = x;    // don't care, page mode is not used

    gcr2.adh            = 0;    // address hold duration is 1 cycle
    //gcr2.daps         = x;    // don't care, DTACK is not used
    gcr2.dae            = 0;    // DTACK is not used
    //gcr2.dap          = x;    // don't care, DTACK is not used
    gcr2.mux16_byp_grant= 1;    // enable grant mechanism
    gcr2.reserved_3_2   = 0;    // must be 0
    gcr2.reserved_11_10 = 0;    // must be 0
    gcr2.reserved_31_13 = 0;    // must be 0

    //rcr1.rcsn         = x;    // don't care in sync mode
    rcr1.rcsa           = 0;    // no delay for ~CS needed
    //rcr1.oen          = x;    // don't care in sync mode
    rcr1.oea            = 0;    // no delay for ~OE needed
    rcr1.radvn          = 0;    // no delay for ~LBA needed
    rcr1.ral            = 0;    // clear ~LBA when needed
    rcr1.radva          = 0;    // no delay for ~LBA needed
    rcr1.rwsc           = 1;    // one wait state
    rcr1.reserved_3     = 0;    // must be 0
    rcr1.reserved_7     = 0;    // must be 0
    rcr1.reserved_11    = 0;    // must be 0
    rcr1.reserved_15    = 0;    // must be 0
    rcr1.reserved_23    = 0;    // must be 0
    rcr1.reserved_31_30 = 0;    // must be 0

    //rcr2.rben         = x;    // don't care in sync mode
    rcr2.rbe            = 0;    // BE is disabled
    //rcr2.rbea         = x;    // don't care when BE is not used
    rcr2.rl             = 0;    // read latency is 0
    //rcr2.pat          = x;    // don't care when page read is not used
    rcr2.apr            = 0;    // page read mode is not used
    rcr2.reserved_7     = 0;    // must be 0
    rcr2.reserved_11_10 = 0;    // must be 0
    rcr2.reserved_31_16 = 0;    // must be 0

    //wcr1.wcsn         = x;    // don't care in sync mode
    wcr1.wcsa           = 0;    // no delay for ~CS needed
    //wcr1.wen          = x;    // don't care in sync mode
    wcr1.wea            = 0;    // no delay for ~WR_N needed
    //wcr1.wben         = x;    // don't care in sync mode
    //wcr1.wbea         = x;    // don't care in sync mode
    wcr1.wadvn          = 0;    // no delay for ~LBA needed
    wcr1.wadva          = 0;    // no delay for ~LBA needed
    wcr1.wwsc           = 1;    // no wait state in needed
    wcr1.wbed           = 1;    // BE is disabled
    wcr1.wal            = 0;    // clear ~LBA when needed

    wcr2.wbcdd          = 0;    // write clock division is not needed
    wcr2.reserved_31_1  = 0;    // must be 0

    wcr.bcm             = 0;    // clock is only active during access
    //wcr.gbcd          = x;    // don't care when BCM=0
    wcr.inten           = 0;    // interrupt is not used
    //wcr.intpol        = x;    // don't care when interrupt is not used
    wcr.wdog_en         = 1;    // watchdog is enabled
    wcr.wdog_limit      = 00;   // timeout is 128 BCLK cycles
    wcr.reserved_3      = 0;    // must be 0
    wcr.reserved_7_6    = 0;    // must be 0
    wcr.reserved_31_11  = 0;    // must be 0

    wiar.ips_req        = 0;    // IPS not needed
    wiar.ips_ack        = 0;    // IPS not needed
    //wiar.irq          = x;    // don't touch
    //wiar.errst        = x;    // don't touch
    wiar.aclk_en        = 1;    // clock is enabled
    wiar.reserved_31_5  = 0;    // must be 0

    //ear.error_addr    = x;    // read-only

    // write modified registers
    eim_write_32(EIM_CS0GCR1,   (uint32_t *)&gcr1);
    eim_write_32(EIM_CS0GCR2,   (uint32_t *)&gcr2);
    eim_write_32(EIM_CS0RCR1,   (uint32_t *)&rcr1);
    eim_write_32(EIM_CS0RCR2,   (uint32_t *)&rcr2);
    eim_write_32(EIM_CS0WCR1,   (uint32_t *)&wcr1);
    eim_write_32(EIM_CS0WCR2,   (uint32_t *)&wcr2);
    eim_write_32(EIM_WCR,               (uint32_t *)&wcr);
    eim_write_32(EIM_WIAR,      (uint32_t *)&wiar);
/*  eim_write_32(EIM_EAR,       (uint32_t *)&ear);*/
}


//------------------------------------------------------------------------------
// Write a 32-bit word to EIM.
// If EIM is not set up correctly, this will abort with a bus error.
//------------------------------------------------------------------------------
void eim_write_32(off_t offset, uint32_t *pvalue)
{
    // calculate memory offset
    uint32_t *ptr = (uint32_t *)_eim_calc_offset(offset);

    // write data to memory
    memcpy(ptr, pvalue, sizeof(uint32_t));
}

//------------------------------------------------------------------------------
// Read a 32-bit word from EIM.
// If EIM is not set up correctly, this will abort with a bus error.
//------------------------------------------------------------------------------
void eim_read_32(off_t offset, uint32_t *pvalue)
{
    // calculate memory offset
    uint32_t *ptr = (uint32_t *)_eim_calc_offset(offset);

    // read data from memory
    memcpy(pvalue, ptr, sizeof(uint32_t));
}


//------------------------------------------------------------------------------
// Calculate an offset into the currently-mapped EIM page.
//------------------------------------------------------------------------------
static off_t _eim_calc_offset(off_t offset)
{
    // make sure that memory is mapped
    if (mem_map_ptr == MAP_FAILED)
        _eim_remap_mem(offset);

    // calculate starting and ending addresses of currently mapped page
    off_t offset_low    = mem_base_addr;
    off_t offset_high   = mem_base_addr + (mem_page_size - 1);

    // check that offset is in currently mapped page, remap new page otherwise
    if ((offset < offset_low) || (offset > offset_high))
        _eim_remap_mem(offset);

    // calculate pointer
    return (off_t)mem_map_ptr + (offset - mem_base_addr);
}


//------------------------------------------------------------------------------
// Map in a new EIM page.
//------------------------------------------------------------------------------
static void _eim_remap_mem(off_t offset)
{
    // unmap old memory page if needed
    if (mem_map_ptr != MAP_FAILED) {
        if (munmap(mem_map_ptr, mem_page_size) != 0) {
            fprintf(stderr, "ERROR: munmap() failed.\n");
            exit(EXIT_FAILURE);
        }
    }

    // calculate starting address of new page
    while (offset % mem_page_size)
        offset--;

    // try to map new memory page
    mem_map_ptr = mmap(NULL, mem_page_size, PROT_READ | PROT_WRITE, MAP_SHARED,
                       mem_dev_fd, offset);
    if (mem_map_ptr == MAP_FAILED) {
        fprintf(stderr, "ERROR: mmap() failed.\n");
        exit(EXIT_FAILURE);
    }

    // save last mapped page address
    mem_base_addr = offset;
}


//------------------------------------------------------------------------------
// End-of-File
//------------------------------------------------------------------------------
