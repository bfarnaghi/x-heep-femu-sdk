/* tee_syscall.h ------------------------------------------------- */
#pragma once
#include <stdint.h>
#include <stddef.h>

/* --- numeric IDs seen in a7 --- */
#define TEE_EC_INFER          0
#define TEE_EC_UART_PUTCHAR   1
#define TEE_EC_UART_GETCHAR   2

/* ------------- U-side API (lives in .user_text) ------------- */
__attribute__((section(".user_text")))
static inline void tee_infer(const void *buf, size_t len)
{
    register uint32_t syscall_id asm("a7") = TEE_EC_INFER;  
    register const char *buf_ptr asm("a0") = buf;
    register uint32_t len_val asm("a1") = len;

    asm volatile (
        "ecall"
        :: "r" (syscall_id), "r" (buf_ptr), "r" (len_val)
        : "memory"
    );
}

__attribute__((section(".user_text")))
static inline void tee_uart_putchar(uint8_t c)
{
    register uint32_t syscall_id asm("a7") = TEE_EC_UART_PUTCHAR;  
    register uint32_t c_val   asm("a0") = c;
    register uint32_t dummy asm("a1") = 0;

    asm volatile (
        "ecall"
        :: "r" (syscall_id), "r" (c_val), "r" (dummy)
        : "memory"
    );
}

__attribute__((section(".user_text")))
static inline uint8_t tee_uart_getchar(void)
{
    register uint32_t syscall_id asm("a7") = TEE_EC_UART_GETCHAR;  
    register uint32_t dummy asm("a0") = 0;
    register uint32_t dummy2 asm("a1") = 0;
    uint32_t ch;
    
    asm volatile (
        "ecall"
        : "=r" (dummy)
        : "r" (syscall_id), "r" (dummy), "r" (dummy2)
        : "memory"
    );
    ch = dummy;
    return (uint8_t)ch;
}