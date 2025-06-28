/*
 * CSR and PMP Access Test
 * Author: Behnam Farnaghinejad <behnam.farnaghinejad@polito.it>
 *
 * This bare-metal test reads and writes key RISC-V CSRs including:
 * - misa (to detect XLEN and U-mode support)
 * - mstatus (to extract MPP/MPV fields)
 * - pmpcfg0 and pmpaddr0 (to check PMP configuration and writeability)
 */

#include <stdint.h>
#include <stdio.h>

#define CSR_READ(csr) ({ uint32_t val; asm volatile("csrr %0, " #csr : "=r"(val)); val; })
#define CSR_WRITE(csr, val) asm volatile("csrw " #csr ", %0" :: "r"(val))

#define MASK(x, high, low) (((x) >> (low)) & ((1 << ((high)-(low)+1)) - 1))

int main(void)
{
    printf("\n--- CSR Access & U-mode Capability Test ---\n");

    // 1. Read misa and decode
    uint32_t misa = CSR_READ(misa);
    char xlen = (misa >> 30) == 1 ? 32 : ((misa >> 30) == 2 ? 64 : '?');
    int has_U = (misa >> ('U' - 'A')) & 1;

    printf("misa     : 0x%08x\n", misa);
    printf("  XLEN   : RV%d\n", xlen);
    printf("  U-mode : %s\n", has_U ? "Supported" : "Not supported");

    // 2. Read mstatus and extract MPP, MPV bits
    uint32_t mstatus = CSR_READ(mstatus);
    uint32_t mpp = MASK(mstatus, 12, 11);
    uint32_t mpv = (mstatus >> 17) & 1;  // if implemented

    printf("mstatus  : 0x%08x\n", mstatus);
    printf("  MPP    : 0x%x\n", mpp);
    printf("  MPV    : %u (if present)\n", mpv);

    // 3. Try reading/writing pmpcfg0 and pmpaddr0
    uint32_t old_cfg = CSR_READ(pmpcfg0);
    uint32_t old_addr = CSR_READ(pmpaddr0);

    printf("pmpcfg0  : 0x%02x\n", old_cfg);
    printf("pmpaddr0 : 0x%08x\n", old_addr);

    // Write and read back test values
    CSR_WRITE(pmpcfg0, 0x1F);      // A=NAPOT, R/W/X
    CSR_WRITE(pmpaddr0, 0x1FFFFF); // arbitrary

    uint32_t new_cfg = CSR_READ(pmpcfg0);
    uint32_t new_addr = CSR_READ(pmpaddr0);

    printf("pmpcfg0* : 0x%02x\n", new_cfg);
    printf("pmpaddr0*: 0x%08x\n", new_addr);
    
    if ((new_cfg == 0x1F) && (new_addr == 0x1FFFFF))
        printf("PMP CSRs : Read/Write OK\n");
    else
        printf("PMP CSRs : Mismatch after write\n");

    return 0;
}
