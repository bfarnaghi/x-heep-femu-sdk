#include "handler.h"

#include "csr.h"
#include "stdasm.h"

static uint32_t get_mtval(void) {
    uint32_t mtval;
    CSR_READ(CSR_REG_MTVAL, &mtval);
    return mtval;
}

static uint32_t get_mcause(void) {
    uint32_t mcause;
    CSR_READ(CSR_REG_MCAUSE, &mcause);
    return mcause;
}

static uint32_t get_mepc(void) {
    uint32_t mepc;
    CSR_READ(CSR_REG_MEPC, &mepc);
    return mepc;
}



void __attribute__((noreturn)) reboot(const char msg[]) {
    printf("\r\n%s", msg);
    printf("MTVAL value is 0x%08x\r\n", get_mtval());
    printf("Rebooting...\r\n");

    asm("li ra, 0x180");
    asm("jr ra");

    printf("What did just happen\r\n");
    while (1) {
    };
}

void handler_instr_acc_fault(void) {
    const char fault_msg[] =
      "Instruction access fault, mtval shows fault address\r\n";
    reboot(fault_msg);
}

void handler_instr_ill_fault(void) {
    const char fault_msg[] =
      "Illegal Instruction fault, mtval shows instruction content\r\n";
    printf("\r\nMEPC is 0x%08x\r\n", get_mepc());
    reboot(fault_msg);
}

void handler_bkpt(void) {
  const char exc_msg[] =
      "Breakpoint triggered, mtval shows the breakpoint address\r\n";
  reboot(exc_msg);
}

void handler_lsu_fault(void) {
  const char exc_msg[] = "Load/Store fault, mtval shows the fault address\r\n";
  reboot(exc_msg);
}