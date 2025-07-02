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
volatile int exit_scpi = 0;

/* --------  DATA THE USER IS ALLOWED TO READ  ------------------- */
__attribute__((section(".user_data")))
static int8_t  g_user_result[16];
__attribute__((section(".user_data")))
static uint8_t g_user_input[lenet_input_data_size];
/* linker symbols exported above */
extern uint8_t __user_start, __user_end;
extern uint8_t __user_stack_top;
/* -------------------------------------------------------------- */
void __attribute__((noinline)) SCPI_from_user(const char *buf, size_t len) {
  SCPI_Input(&scpi_context, buf, len);
  SCPI_Input(&scpi_context, "\r\n", 2);
  SCPI_Flush(&scpi_context);
}
/* ------- minimal ECALL-from-U trap handler (M-mode) --------- */
__attribute__((naked))
void m_trap_handler(void)
{
    __asm__ volatile (
        "csrr  t0, mcause\n"
        "li    t1, 11\n"              /* ECALL from U-mode */
        "bne   t0, t1, bad\n"
        "bnez  a7, bad\n"             /* only service ID 0 */

        /* a0 = ptr, a1 = len */
        "call  %[c]\n"                /* run SCPI_from_user   */

        /* return to user */
        "csrr  t0, mstatus\n"
        /* MPP = 00 (U-mode)*/
        "li    t1, ~(3<<11)\n"     // Load the mask into t1
        "and   t0, t0, t1\n"       // Perform the AND
        "csrw  mstatus, t0\n"
        "mret\n"

    "bad:\n"
        "j should_not_happen\n"
        :
        : [c]"i"(SCPI_from_user)
        : "memory"
    );
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
        uart_getchar(uart, &c);
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
        uart_putchar(uart, c);
        if (c == '\n') uart_putchar(uart, '\r');
        else if (c == '\r') uart_putchar(uart, '\n');
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
      register uintptr_t a0 __asm__("a0") = (uintptr_t)buf;
      register uintptr_t a1 __asm__("a1") = n;
      register uintptr_t a7 __asm__("a7") = 0;  // syscall ID 0
      __asm__ volatile ("ecall" : : "r"(a0), "r"(a1), "r"(a7));
    }
  }
}

__attribute__((section(".user_text")))
void __attribute__((noinline)) uart_scpi(scpi_t * context, uart_t * uart) {
  char buffer[2048];
  printf("Starting SCPI loop...\r\n");
    uintptr_t user_stack_top = (uintptr_t)&__user_stack_top;
  uintptr_t user_entry     = (uintptr_t)&user_uart_loop;
  uintptr_t user_buffer    = (uintptr_t)&g_user_input;
  uintptr_t user_buf_size  = sizeof(g_user_input);

  __asm__ volatile (
      "csrr  t0, mstatus\n"
      "li    t1, ~(3<<11)\n"     // clear MPP bits
      "and   t0, t0, t1\n"
      "li    t1, 0\n"            // U-mode = 00
      "or    t0, t0, t1\n"
      "csrw  mstatus, t0\n"
      "csrw  mepc, %0\n"
      "mv    a0, %1\n"
      "li    a1, %2\n"
      "mv    sp, %3\n"
      "mret\n"
      :
      : "r"(user_entry), "r"(user_buffer), "i"(lenet_input_data_size), "r"(user_stack_top)
      : "t0", "t1", "a0", "a1"
  );
}

mmio_region_t mmio_region_from_adr(uintptr_t address) {
  return (mmio_region_t){
      .base = (volatile void *)address,
  };
}
/* --- helper ------------------------------------------------------- */
static inline uintptr_t pmp_napot(uintptr_t base, size_t length_pow2)
{
    /*  length_pow2 = exact size, MUST be a power of two ≥ 8
     *  Returns the value to write to pmpaddr.                      */
    return (base >> 2) | ((length_pow2 >> 3) - 1);
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
  
    printf("user region start: %p\r\n", &__user_start);
    printf("user region end  : %p\r\n", &__user_end);
    
     /* ----------  PMP slot 0 : user sandbox  ---------------------- */
    uintptr_t user_start = (uintptr_t)&__user_start;
    uintptr_t user_end   = (uintptr_t)&__user_end;
    size_t    span       = user_end - user_start;
    
    // Next power of two
    size_t pow2 = 1u << (32 - __builtin_clz(span - 1));
    // Align base
    uintptr_t user_base = user_start & ~(pow2 - 1);

    // Encode for NAPOT
    uintptr_t napot = (user_base >> 2) | ((pow2 >> 3) - 1);

    printf("user region start: 0x%lx\r\n", user_start);
    printf("user region end  : 0x%lx\r\n", user_end);
    printf("user span = %lu bytes, NAPOT size = 0x%lx, aligned base = 0x%lx\r\n",
            span, (unsigned long)pow2, (unsigned long)user_base);

    __asm__ volatile ("csrw pmpcfg0,  %0" :: "r"(0x0F));   // RWX
    __asm__ volatile ("csrw pmpaddr0, %0" :: "r"(napot));
    
    uintptr_t base  = user_base;
    uintptr_t limit = user_base + pow2 - 1;
    printf("PMP region: 0x%08lx — 0x%08lx\r\n", base, limit);

    /* ----------  PMP slot 1 : UART registers (4 KiB page) --------- */
    uintptr_t uart_page  = ((uintptr_t)uart.base_addr.base) & ~0xFFFu;
    uintptr_t napot_uart = pmp_napot(uart_page, 0x1000);

    __asm__ volatile ("csrw pmpcfg1,  %0" :: "r"(0x03));   /* RW  , A=NAPOT */
    __asm__ volatile ("csrw pmpaddr1, %0" :: "r"(napot_uart));


  uart_scpi(&scpi_context, &uart);

  return 0;
}

void __attribute__((optimize("O0"))) should_not_happen() {
    printf("This should not happen\r\n");
}