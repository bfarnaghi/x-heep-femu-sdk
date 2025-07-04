/* tee_syscall.h ------------------------------------------------- */
#pragma once
#include <stdint.h>
#include <stddef.h>

/* --- numeric IDs seen in a7 --- */
#define TEE_EC_INFER          0
#define TEE_EC_UART_PUTCHAR   1
#define TEE_EC_UART_GETCHAR   2

// /* helper macro ---------------------------------------------------- */
// #define TEE_SYSCALL(id, arg0, arg1, out0)                    \
//   do {                                                       \
//       register uintptr_t a7 __asm__("a7") = (id);            \
//       register uintptr_t a0 __asm__("a0") = (arg0);          \
//       register uintptr_t a1 __asm__("a1") = (arg1);          \
//       __asm__ volatile (                                     \
//           "ecall"                                            \
//           : "+r"(a0)                                         \
//           : "r"(a7), "r"(a1)                                 \
//           : "a0", "a1", "a7", "memory");                     \
//       (out0) = a0;                                           \
//   } while (0)

// /* ---------------------------------------------------------------- */
// __attribute__((section(".user_text"), aligned(4), noinline))
// static inline void tee_infer(const void *buf, size_t len)
// {
//     uintptr_t dummy;
//     TEE_SYSCALL(TEE_EC_INFER, (uintptr_t)buf, len, dummy);
// }

// __attribute__((section(".user_text"), aligned(4), noinline))
// static inline void tee_uart_putchar(uint8_t c)
// {
//     uintptr_t dummy;
//     TEE_SYSCALL(TEE_EC_UART_PUTCHAR, c, 0, dummy);
// }

// __attribute__((section(".user_text"), aligned(4), noinline))
// static inline uint8_t tee_uart_getchar(void)
// {
//     uintptr_t ch;
//     TEE_SYSCALL(TEE_EC_UART_GETCHAR, 0, 0, ch);
//     return (uint8_t)ch;
// }

/* ------------- U-side API (lives in .user_text) ------------- */
__attribute__((section(".user_text"), aligned(4), noinline))
static inline void tee_infer(const void *buf, size_t len)
{
    register uint32_t syscall_id __asm__("a7") = TEE_EC_INFER;  
    register const char *buf_ptr __asm__("a0") = buf;
    register uint32_t len_val __asm__("a1") = len;

    __asm__ volatile (
        "ecall"
        :: "r" (syscall_id), "r" (buf_ptr), "r" (len_val)
        : "memory"
    );
}

__attribute__((section(".user_text"), aligned(4), noinline))
static inline void tee_uart_putchar(uint8_t c)
{
    register uint32_t syscall_id __asm__("a7") = TEE_EC_UART_PUTCHAR;  
    register uint32_t c_val   __asm__("a0") = c;
    register uint32_t dummy __asm__("a1") = 0;

    __asm__ volatile (
        "ecall"
        :: "r" (syscall_id), "r" (c_val), "r" (dummy)
        : "memory"
    );
}

__attribute__((section(".user_text"), aligned(4), noinline))
static inline uint8_t tee_uart_getchar(void)
{
    register uint32_t syscall_id __asm__("a7") = TEE_EC_UART_GETCHAR;  
    register uint32_t dummy __asm__("a0") = 0;
    register uint32_t dummy2 __asm__("a1") = 0;
    uint32_t ch;
    
    __asm__ volatile (
        "ecall"
        : "=r" (dummy)
        : "r" (syscall_id), "r" (dummy), "r" (dummy2)
        : "memory"
    );
    ch = dummy;
    return (uint8_t)ch;
}