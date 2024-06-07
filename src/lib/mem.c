#include "mem.h"

void mem_copy(void* dst, void* src, u64 size) {
    u8* c_src = src;
    u8* c_dst = dst;

    for (u64 i = 0; i < size; ++i) { c_dst[i] = c_src[i]; }
}

void* mem_set(void* dst, i32 value, u64 size) {
    u8* c_dst = dst;

    for (u64 i = 0; i < size; ++i) { c_dst[i] = (u8)value; }
    // while (size > 0) {
    //     *c_dst = (u8)value;
    //     c_dst++;
    //     size--;
    // }

    return dst;
}

void mem_swap(void* ptr_a, void* ptr_b, u64 size) {
    u8* a = ptr_a;
    u8* b = ptr_b;
    u8 temp;

    for (u64 i = 0; i < size; ++i) {
        temp = a[i];
        a[i] = b[i];
        b[i] = temp;
    }
}
