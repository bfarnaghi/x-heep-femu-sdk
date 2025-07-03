#include <stdio.h>
#include "models/lenet5_input.h"
#include "lenet5_test.h"
#include "scpi/scpi.h"
#include "uart.h"
#include "soc_ctrl.h"
#include "core_v_mini_mcu.h"
#include "mmio.h"

#define ECHO 1

#define SCPI_IDN1 "MANUFACTURE"
#define SCPI_IDN2 "INSTR2013"
#define SCPI_IDN3 NULL
#define SCPI_IDN4 "01-02"

volatile soc_ctrl_t soc_ctrl;
volatile uart_t uart;
volatile scpi_t scpi_context;
__attribute__((section(".user_data")))
volatile int exit_scpi = 0;

#include "tee_syscall.h"
extern void tee_infer(const void *buf, size_t len);
extern void tee_uart_putchar(uint8_t c);         
extern uint8_t tee_uart_getchar(void);            
extern void scpi_infer_dispatch(const uint8_t *buf, size_t len);  /* glue: SCPI+infer+send */

/* --------  DATA THE USER IS ALLOWED TO READ  ------------------- */
__attribute__((section(".user_data")))
int8_t  g_user_result[16];
__attribute__((section(".user_data")))
static uint8_t g_user_input[lenet_input_data_size];
/* linker symbols exported above */
extern uint8_t __user_start, __user_end;
extern uint8_t __user_stack_top;
extern uint8_t __user_text_start, __user_text_end;
extern uint8_t __user_data_start; 
/* -------------------------------------------------------------- */
void __attribute__((noinline)) SCPI_from_user(const char *buf, size_t len) {
  SCPI_Input(&scpi_context, buf, len);
  SCPI_Input(&scpi_context, "\r\n", 2);
  SCPI_Flush(&scpi_context);
}
/* -------------------------------------------------------------- */
scpi_result_t __attribute__((noinline)) InferExample(scpi_t * context) { 
  const char *out;
  size_t len;
  const int8_t *data = lenet_input_data;
    
  /* ── 1. enable cycle & instret counters for lower priv-levels (if ever needed) ── */
    __asm__ volatile (
        "li   t0, 0x5\n"          /* bit0 = CY, bit2 = IR                    */
        "csrs mcounteren, t0\n"   /* set bits, leave any others untouched    */
    );
  /* ── 2. snapshot “start” values ── */
    uint32_t start_c_lo, start_c_hi;
    uint32_t start_i_lo, start_i_hi;
    __asm__ volatile (
        "csrr %0, mcycle      \n"
        "csrr %1, mcycleh     \n"
        "csrr %2, minstret    \n"
        "csrr %3, minstreth   \n"
        : "=r"(start_c_lo), "=r"(start_c_hi),
          "=r"(start_i_lo), "=r"(start_i_hi)
    );
  /* ── 3. run the inference ── */  
  int a = infer((const char *) data, lenet_input_data_size, &out, &len);
  /* ── 4. snapshot “end” values ── */
    uint32_t end_c_lo, end_c_hi;
    uint32_t end_i_lo, end_i_hi;
    __asm__ volatile (
        "csrr %0, mcycle      \n"
        "csrr %1, mcycleh     \n"
        "csrr %2, minstret    \n"
        "csrr %3, minstreth   \n"
        : "=r"(end_c_lo), "=r"(end_c_hi),
          "=r"(end_i_lo), "=r"(end_i_hi)
    );
  /* ── 5. compute 64-bit deltas ── */
    uint64_t cycles = (((uint64_t)end_c_hi << 32) | end_c_lo) -
                      (((uint64_t)start_c_hi << 32) | start_c_lo);
    uint64_t insts  = (((uint64_t)end_i_hi << 32) | end_i_lo) -
                      (((uint64_t)start_i_hi << 32) | start_i_lo);
    printf("Cycles:       hi=0x%lx lo=0x%08lx\r\n",
       (unsigned long)(cycles >> 32), (unsigned long)(cycles & 0xFFFFFFFF));
    printf("Instructions: hi=0x%lx lo=0x%08lx\r\n",
       (unsigned long)(insts  >> 32), (unsigned long)(insts  & 0xFFFFFFFF));
  /* ── 6. original SCPI output ── */  
  if (a == 0) {
    SCPI_ResultArrayInt8(context, (const int8_t *) out, len, SCPI_FORMAT_ASCII);
  } else {
    SCPI_ResultText(context, "Error");
  }
  return SCPI_RES_OK;
}

scpi_result_t __attribute__((noinline)) InferData(scpi_t * context) {
  const char *out;
  size_t out_len;

  const int8_t tflite_input_data[lenet_input_data_size];
  const int8_t *scpi_out;
  size_t scpi_len;
  SCPI_ParamArbitraryBlock(context, &scpi_out, &scpi_len, true);
  printf("Read: %d bytes\r\n", scpi_len);
  memcpy(tflite_input_data, scpi_out, scpi_len);

  int a = infer((const char *) tflite_input_data, lenet_input_data_size, &out, &out_len);
  if (a == 0) {
    SCPI_ResultArrayInt8(context, (const int8_t *) out, out_len, SCPI_FORMAT_ASCII);
  } else {
    SCPI_ResultText(context, "Inference error");
  }
  return SCPI_RES_OK;
}

scpi_result_t __attribute__((noinline))  Exit(scpi_t * context) {
    exit_scpi = 1;
    uart_write(&uart, (const uint8_t *) "Exiting...\r\n", 12);
    return SCPI_RES_OK;
}

volatile scpi_command_t scpi_commands[] = {
	{ "NN:INFEr:EXAMple?", InferExample, 0},
  { "NN:INFEr:DATA?", InferData, 0},
  { "EXT", Exit, 0},
	SCPI_CMD_LIST_END
};

size_t __attribute__((noinline)) scrivi(scpi_t * context, const char * data, size_t len) {
    (void) context;
    size_t a = uart_write(&uart, (const uint8_t *) data, len);
    return a;
}

int __attribute__((noinline))  SCPI_Error(scpi_t * context, int_fast16_t err) {
    (void) context;
    uart_write(&uart, (const uint8_t *) "ERR!\r\n", 6);
    return 0;
}

scpi_result_t __attribute__((noinline)) SCPI_Control(scpi_t * context, scpi_ctrl_name_t ctrl, scpi_reg_val_t val) {
    return SCPI_RES_OK;
}
scpi_result_t __attribute__((noinline)) SCPI_Reset(scpi_t * context) {
    return SCPI_RES_OK;
}
scpi_result_t __attribute__((noinline))  SCPI_Flush(scpi_t * context) {
    return SCPI_RES_OK;
}

volatile scpi_interface_t scpi_interface = {
	.write = scrivi,
	.error = SCPI_Error,
	.control = NULL,
    .flush = NULL,
    .reset = NULL
};

#define SCPI_INPUT_BUFFER_LENGTH 2048
static char scpi_input_buffer[SCPI_INPUT_BUFFER_LENGTH];

#define SCPI_ERROR_QUEUE_SIZE 17
scpi_error_t scpi_error_queue_data[SCPI_ERROR_QUEUE_SIZE];

static int modifier = 0;

__attribute__((section(".user_text")))
size_t __attribute__((noinline)) uart_gets(uart_t *uart, char *buf, size_t len) {
    size_t i = 0;
    while (i < len - 1) {
        uint8_t c;
        c = tee_uart_getchar();
        if (c == '\\') {
            if (!modifier) modifier = 1;
            else {
                buf[i] = c;
                i++;
                modifier = 0;
            }
            continue;
        }
        #if ECHO
        tee_uart_putchar(c);
        if (c == '\n') tee_uart_putchar('\r');
        else if (c == '\r') tee_uart_putchar('\n');
        #endif
        if (c == '\n' || c == '\r') {
            if (modifier) {
                buf[i] = c;
                i++;
                modifier = 0;
            } else {
                buf[i] = '\0';
                break;
            }
        } else {
            buf[i] = c;
            i++;
            modifier = 0;
        }
    }
    return i;
}

__attribute__((section(".user_text")))
void __attribute__((noinline)) user_uart_loop(uint8_t *buf, size_t len) {
  while (!exit_scpi) {
    size_t n = uart_gets(&uart, (char *)buf, len);
    if (n > 0) {
      tee_infer(buf, n);
    }
  }
}

void switch_to_user_mode() {
    __asm__ volatile (
        "la t0, user_uart_loop     \n"  // Load user function address
        "csrw mepc, t0             \n"  // Set MEPC 

        "la sp, __user_stack_top   \n"  
        "li t2, 4096               \n"  
        "add sp, sp, t2            \n"  

        "csrr t1, mstatus          \n"
        "li t2, ~(3 << 11)         \n"  // Clear MPP (bits 12:11)
        "and t1, t1, t2            \n"
        "csrw mstatus, t1          \n"

        "mret                      \n"  
    );
}


void __attribute__((noinline)) uart_scpi(scpi_t * context, uart_t * uart) {
  char buffer[2048];
  printf("Starting SCPI loop...\r\n");
  printf("[Switch to User Mode]\n");
  switch_to_user_mode();
}

mmio_region_t mmio_region_from_adr(uintptr_t address) {
  return (mmio_region_t){
      .base = (volatile void *)address,
  };
}

// Helper to compute PMP NAPOT-encoded address
static inline uintptr_t napot_encode(uintptr_t base, size_t size_pow2) {
    return (base >> 2) | ((size_pow2 >> 3) - 1);
}

#define CSR_MSECCFG 0x747
void pmp_setup(void)
{
//     uint32_t val;
//     __asm__ volatile ("csrr %0, %1" : "=r"(val) : "i"(CSR_MSECCFG));
//     printf("mseccfg = 0x%08x\n", val);
    
//     __asm__ volatile ("csrw 0x747, 0x4");
    
//     __asm__ volatile ("csrr %0, %1" : "=r"(val) : "i"(CSR_MSECCFG));
//     printf("mseccfg = 0x%08x\n", val);
    
    /* ── 0. unlock all entries first ─────────────────────────────── */
//     __asm__ volatile("csrw pmpcfg0, 0");

//     /* ── 1. exec-only code window (.user_text)  ───────────────────── */
//     uintptr_t txt_lo  = (uintptr_t)&__user_text_start;
//     uintptr_t txt_hi  = (uintptr_t)&__user_text_end;     /* exclusive */
//     size_t    txt_len = txt_hi - txt_lo;

//     size_t pow2       = 1u << (32 - __builtin_clz(txt_len - 1));
//     uintptr_t txt_base = txt_lo & ~(pow2 - 1);
//     uintptr_t pmpaddr0 = napot_encode(txt_base, pow2);
//     printf("[PMP] Writing addr0=0x%08lx\n", pmpaddr0);
    
//     __asm__ volatile("csrw pmpaddr0, %0" :: "r"(pmpaddr0));

//     /* cfg0 : L=1, A=**NAPOT** (11), X=1, W=0, R=0  → 0x9C */
//     const uint8_t cfg0 = 0x1C;   /* 0001 1100 */

//     /* ── 2. RW no-exec data+stack window  (.user_data .. __user_end) ─ */
//     uintptr_t dat_lo  = (uintptr_t)&__user_data_start;
//     uintptr_t dat_hi  = (uintptr_t)&__user_end;
//     size_t    dat_len = dat_hi - dat_lo;

//     pow2         = 1u << (32 - __builtin_clz(dat_len - 1));
//     uintptr_t dat_base  = dat_lo & ~(pow2 - 1);
//     uintptr_t pmpaddr1  = napot_encode(dat_base, pow2);
    
//     printf("[PMP] Writing addr1=0x%08lx\n", pmpaddr1);
    
//     __asm__ volatile("csrw pmpaddr1, %0" :: "r"(pmpaddr1));

//     /* cfg1 : L=1, A=**NAPOT** (11), R=1, W=1, X=0  → 0x9B */
//     const uint8_t cfg1 = 0x1B;   /* 0001 1011 */

//     /* ── 4. commit all three bytes in one shot ────────────────────── */
//     uint32_t pmpcfg0 = cfg0 | (cfg1 << 8);
//     printf("[PMP] Writing cfg0=0x%08x\n", pmpcfg0);
//     __asm__ volatile("csrw pmpcfg0, %0" :: "r"(pmpcfg0));
//     __asm__  volatile ("fence.i");

//     /* ── Debug printout ───────────────────────────────────────────── */
//     printf("[PMP] txt  base=0x%08lx size=0x%lx  cfg0=0x%02x\n",
//            txt_base, pow2, cfg0);
//     printf("[PMP] data base=0x%08lx size=0x%lx  cfg1=0x%02x\n",
//            dat_base, pow2, cfg1);
    /* Allow RXW in [0x00078000 , 0x00080000) and lock it */
    __asm__ volatile(
        "csrw  pmpcfg0, zero\n\t"          /* disable entry 0 first      */
        "li    t0,   0x1e000\n\t"          /* 0x00078000 >> 2 → bottom   */
        "csrw  pmpaddr0, t0\n\t"
        "li    t0,   0x20000\n\t"          /* 0x00080000 >> 2 → top      */
        "csrw  pmpaddr1, t0\n\t"           /* entry 1 addr               */
        "li    t0,   0x8f\n\t"             /* TOR | R | W | X | L        */
        "slli t0, t0, 8\n\t"
        "csrs pmpcfg0, t0\n\t"             /* program **entry 1** cfg    */
        "fence.i\n\t"
    );
    
    uint32_t tmp;

    __asm__ volatile ("csrr %0, pmpcfg0" : "=r"(tmp));
    printf("[CSR] pmpcfg0  = 0x%08x\n", tmp);

    __asm__ volatile ("csrr %0, pmpaddr0" : "=r"(tmp));
    printf("[CSR] pmpaddr0 = 0x%08x\n", tmp);

    __asm__ volatile ("csrr %0, pmpaddr1" : "=r"(tmp));
    printf("[CSR] pmpaddr1 = 0x%08x\n", tmp);
}

int main() {
  printf("Initializing peripherals...\r\n");
  soc_ctrl.base_addr = mmio_region_from_adr((uintptr_t)SOC_CTRL_START_ADDRESS);
  printf("Initialized soc_ctrl\r\n");
  printf("soc_ctrl.base_addr: %p\r\n", soc_ctrl.base_addr);
    uart.base_addr   = mmio_region_from_adr((uintptr_t)UART_START_ADDRESS);
    uart.baudrate    = 115200;
    uart.clk_freq_hz = soc_ctrl_get_frequency(&soc_ctrl);
    if (uart_init(&uart) != kErrorOk) {
        return;
    }
  printf("Initialized UART\r\n");
  printf("uart.base_addr: %p\r\n", uart.base_addr);
  printf("uart.baudrate: %d\r\n", uart.baudrate);
  printf("uart.clk_freq_hz: %d\r\n", uart.clk_freq_hz);

    SCPI_Init(&scpi_context, 
              scpi_commands, 
              &scpi_interface, 
              scpi_units_def, 
              SCPI_IDN1, SCPI_IDN2, SCPI_IDN3, SCPI_IDN4, 
              scpi_input_buffer, SCPI_INPUT_BUFFER_LENGTH,
              scpi_error_queue_data, SCPI_ERROR_QUEUE_SIZE);

      printf("Initialized x.ruSCPI\r\n");
  // Print available SCPI commands
  printf("Available SCPI commands:\r\n");
  for (int i = 0; scpi_commands[i].pattern != NULL; i++) {
    printf("%s\r\n", scpi_commands[i].pattern);
  }
  init_tflite();
  printf("Initialized TFLite\r\n");
  
  printf("[PMP Setup]\n");
  pmp_setup();

  uart_scpi(&scpi_context, &uart);

  return 0;
}

void __attribute__((optimize("O0"))) should_not_happen() {
    printf("This should not happen\r\n");
}