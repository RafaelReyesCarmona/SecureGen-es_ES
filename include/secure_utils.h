#pragma once
#include <stddef.h>
#include <stdint.h>

// Secure memory zeroing — prevents compiler dead-store elimination at -O3
static inline void secure_memzero(void* ptr, size_t len) {
    volatile uint8_t* p = static_cast<volatile uint8_t*>(ptr);
    while (len--) *p++ = 0;
}
