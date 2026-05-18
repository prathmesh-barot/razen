#pragma once
#include <cstdint>

extern "C" {

void __razen_print_str(const char* s);
void __razen_print_str_newline(const char* s);
void __razen_print_char(char c);
void __razen_print_bool(bool b);
void __razen_print_int(int32_t v);
void __razen_print_uint(uint32_t v);
void __razen_print_int64(int64_t v);
void __razen_print_uint64(uint64_t v);
void __razen_print_float(float v);
void __razen_print_double(double v);
void __razen_print_ptr(const void* p);
void __razen_print_newline();

}
