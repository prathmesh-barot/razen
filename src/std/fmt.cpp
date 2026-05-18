#include "fmt.h"
#include <cstdio>

extern "C" {

void __razen_print_str(const char* s) {
    fputs(s, stdout);
}

void __razen_print_str_newline(const char* s) {
    puts(s);
}

void __razen_print_char(char c) {
    putchar(c);
}

void __razen_print_bool(bool b) {
    fputs(b ? "true" : "false", stdout);
}

void __razen_print_int(int32_t v) {
    printf("%d", v);
}

void __razen_print_uint(uint32_t v) {
    printf("%u", v);
}

void __razen_print_int64(int64_t v) {
    printf("%ld", v);
}

void __razen_print_uint64(uint64_t v) {
    printf("%lu", v);
}

void __razen_print_float(float v) {
    printf("%g", (double)v);
}

void __razen_print_double(double v) {
    printf("%g", v);
}

void __razen_print_ptr(const void* p) {
    printf("%p", p);
}

void __razen_print_newline() {
    putchar('\n');
}

}
