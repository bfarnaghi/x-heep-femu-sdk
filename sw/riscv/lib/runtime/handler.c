// Copyright lowRISC contributors.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

#include "handler.h"

#include "csr.h"
#include "stdasm.h"

/**
 * Return value of mtval
 */
static uint32_t get_mtval(void) {
  uint32_t mtval;
  CSR_READ(CSR_REG_MTVAL, &mtval);
  return mtval;
}

/**
 * Default Error Handling
 * @param msg error message supplied by caller
 * TODO - this will be soon by a real print formatting
 */
static void print_exc_msg(const char *msg) {
  printf("%s", msg);
  printf("MTVAL value is 0x%x\n", get_mtval());
  while (1) {
  };
}


// Below functions are default weak exception handlers meant to be overriden
__attribute__((weak, aligned(4))) void handler_exception(void) {
  uint32_t syscall_id;
  uint32_t   a1;
  uintptr_t  a0;
  asm volatile("mv %0, a0" : "=r"(a0));
  asm volatile("mv %0, a1" : "=r"(a1));
  asm volatile("mv %0, a7" : "=r"(syscall_id));
    
//   uintptr_t  sp;
//   asm volatile("mv %0, sp" : "=r"(sp));
//     printf("%lx\r\n",sp);
    
    
  uint32_t mcause;
  exc_id_t exc_cause;

  CSR_READ(CSR_REG_MCAUSE, &mcause);
  exc_cause = (exc_id_t)(mcause & kIdMax);
    
  uintptr_t mepc;
  asm volatile("csrr %0, mepc" : "=r"(mepc));
 
  switch (exc_cause) {
    case kInstMisa:
      handler_instr_acc_fault();
      break;
    case kInstAccFault:
      handler_instr_acc_fault();
      break;
    case kInstIllegalFault:
      handler_instr_ill_fault();
      break;
    case kBkpt:
      handler_bkpt();
      break;
    case kLoadAccFault:
      handler_lsu_fault();
      break;
    case kStrAccFault:
      handler_lsu_fault();
      break;
    case kECall:
      handler_ecall();
      break;
    case uECall:

      /* ------------ make room and save ra + gp + tp + s0–s11 ------------- */
    asm volatile(
        "addi   sp,  sp,  -60\n"   /* 15×4 = 60 bytes                     */
        "sw     ra,  56(sp)\n"
        "sw     gp,  52(sp)\n"
        "sw     tp,  48(sp)\n"
        "sw     s0,  44(sp)\n"
        "sw     s1,  40(sp)\n"
        "sw     s2,  36(sp)\n"
        "sw     s3,  32(sp)\n"
        "sw     s4,  28(sp)\n"
        "sw     s5,  24(sp)\n"
        "sw     s6,  20(sp)\n"
        "sw     s7,  16(sp)\n"
        "sw     s8,  12(sp)\n"
        "sw     s9,   8(sp)\n"
        "sw     s10,  4(sp)\n"
        "sw     s11,  0(sp)\n"
        ::: "memory");
 
      //---------------------------
      handler_user_ecall(syscall_id,a0,a1);
      //---------------------------
 
      uintptr_t mepc;
      asm volatile("csrr %0, mepc" : "=r"(mepc));
      uint16_t instr = *(uint16_t *)mepc;  // read instruction at trap address
      mepc += (instr & 0x3) == 0x3 ? 4 : 2;
      asm volatile("csrw mepc, %0" :: "r"(mepc));
          /* ----------------------- restore the frame ------------------------- */
    asm volatile(
        "lw     s11,  0(sp)\n"
        "lw     s10,  4(sp)\n"
        "lw     s9,   8(sp)\n"
        "lw     s8,  12(sp)\n"
        "lw     s7,  16(sp)\n"
        "lw     s6,  20(sp)\n"
        "lw     s5,  24(sp)\n"
        "lw     s4,  28(sp)\n"
        "lw     s3,  32(sp)\n"
        "lw     s2,  36(sp)\n"
        "lw     s1,  40(sp)\n"
        "lw     s0,  44(sp)\n"
        "lw     tp,  48(sp)\n"
        "lw     gp,  52(sp)\n"
        "lw     ra,  56(sp)\n"
        "addi   sp,  sp,  60\n"
        ::: "memory");

      asm volatile("mret");
      break;
      //---------------------------
    default:
      while (1) {
      };
  }
}

__attribute__((weak)) void handler_irq_software(void) {
  printf("Software IRQ triggered!\n");
  while (1) {
  }
}

__attribute__((weak)) void handler_irq_timer(void) {
  printf("Timer IRQ triggered!\n");
  while (1) {
  }
}

__attribute__((weak)) void handler_irq_external(void) {
  printf("External IRQ triggered!\n");
  while (1) {
  }
}

__attribute__((weak)) void handler_irq_fast_timer_1(void) {
  printf("Fast timer 1 IRQ triggered!\n");
  while (1) {
  }
}

__attribute__((weak)) void handler_irq_fast_timer_2(void) {
  printf("Fast timer 2 IRQ triggered!\n");
  while (1) {
  }
}

__attribute__((weak)) void handler_irq_fast_timer_3(void) {
  printf("Fast timer 3 IRQ triggered!\n");
  while (1) {
  }
}

__attribute__((weak)) void handler_irq_fast_dma(void) {
  printf("Fast dma IRQ triggered!\n");
  while (1) {
  }
}

__attribute__((weak)) void handler_irq_fast_spi(void) {
  printf("Fast spi IRQ triggered!\n");
  while (1) {
  }
}

__attribute__((weak)) void handler_irq_fast_spi_flash(void) {
  printf("Fast spi flash IRQ triggered!\n");
  while (1) {
  }
}

__attribute__((weak)) void handler_irq_fast_gpio_0(void) {
  printf("Fast gpio 0 IRQ triggered!\n");
  while (1) {
  }
}

__attribute__((weak)) void handler_irq_fast_gpio_1(void) {
  printf("Fast gpio 1 IRQ triggered!\n");
  while (1) {
  }
}

__attribute__((weak)) void handler_irq_fast_gpio_2(void) {
  printf("Fast gpio 2 IRQ triggered!\n");
  while (1) {
  }
}

__attribute__((weak)) void handler_irq_fast_gpio_3(void) {
  printf("Fast gpio 3 IRQ triggered!\n");
  while (1) {
  }
}

__attribute__((weak)) void handler_irq_fast_gpio_4(void) {
  printf("Fast gpio 4 IRQ triggered!\n");
  while (1) {
  }
}

__attribute__((weak)) void handler_irq_fast_gpio_5(void) {
  printf("Fast gpio 5 IRQ triggered!\n");
  while (1) {
  }
}

__attribute__((weak)) void handler_irq_fast_gpio_6(void) {
  printf("Fast gpio 6 IRQ triggered!\n");
  while (1) {
  }
}

__attribute__((weak)) void handler_irq_fast_gpio_7(void) {
  printf("Fast gpio 7 IRQ triggered!\n");
  while (1) {
  }
}

__attribute__((weak)) void handler_instr_acc_fault(void) {
  const char fault_msg[] =
      "Instruction access fault, mtval shows fault address\n";
  print_exc_msg(fault_msg);
}

__attribute__((weak)) void handler_instr_ill_fault(void) {
  const char fault_msg[] =
      "Illegal Instruction fault, mtval shows instruction content\n";
  print_exc_msg(fault_msg);
}

__attribute__((weak)) void handler_bkpt(void) {
  const char exc_msg[] =
      "Breakpoint triggerd, mtval shows the breakpoint address\n";
  print_exc_msg(exc_msg);
}

__attribute__((weak)) void handler_lsu_fault(void) {
  const char exc_msg[] = "Load/Store fault, mtval shows the fault address\n";
  print_exc_msg(exc_msg);
}

__attribute__((weak)) void handler_ecall(void) {
  printf("Environment call encountered\n");
  while (1) {
  }
}

#include "tee_syscall.h"
#include "uart.h"
#include "lenet5_test.h"
#include <string.h>
#include "scpi/scpi.h"

extern volatile uart_t uart;
extern volatile scpi_t scpi_context;

__attribute__((weak)) void handler_user_ecall(uint32_t syscall_id,uintptr_t  ptr,uint32_t len) {

  switch (syscall_id) {
          
    case TEE_EC_INFER:
        const uint32_t ram2_lo = 0x00078000;
        const uint32_t ram2_hi = 0x00080000;
        if ((ptr < ram2_lo) || (ptr + len > ram2_hi)) {
            printf("[M] Bad user buffer (0x%08lx, len=%u)\r\n", (long)ptr, len);
            return;                    /* or place error code in a0 */
        }
        printf("Len = %u\r\n",len);
        const char *input = (const char *)ptr;
        if (len > 0) {
            //printf("Got command\r\n");
            SCPI_Input(&scpi_context, input, len);
            SCPI_Input(&scpi_context, "\r\n", 2);
        }
        SCPI_Flush(&scpi_context);
        break;
    
    case TEE_EC_UART_PUTCHAR: 
        uart_putchar(&uart, (uint8_t)ptr);
      break;
        
    case TEE_EC_UART_GETCHAR: 
      uint8_t c;
      uart_getchar(&uart, &c);
      asm volatile("mv a0, %0" :: "r"(c));  // return value in a0
      break;
    
    case TEE_EC_RDCOUNTERS: 
        /* arguments from user */
        uint32_t *user_buf = (uint32_t *)ptr;    /* a0 */
        /* a1 is ‘len’ – ignored here or check ==4 */

        /* snapshot performance counters */
        uint32_t c_lo, c_hi, i_lo, i_hi;
        asm volatile (
            "csrr %0, mcycle   \n"
            "csrr %1, mcycleh  \n"
            "csrr %2, minstret \n"
            "csrr %3, minstreth"
            : "=r"(c_lo), "=r"(c_hi), "=r"(i_lo), "=r"(i_hi));

         printf("%lx\n\r",c_lo);
         printf("%lx\n\r",c_hi);
         printf("%lx\n\r",i_lo);
         printf("%lx\n\r",i_hi);

        /* optionally return 0 in a0 */
        asm volatile ("" :: "r"(0) : "a0");
    break;
              
    default:
        printf("[M] Unknown syscall ID: %u\n", syscall_id);
        while (1);
        break;
  }
}


