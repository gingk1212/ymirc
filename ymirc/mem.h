#pragma once

#include <stdint.h>

typedef uintptr_t Phys;
typedef uintptr_t Virt;

Phys virt2phys(uintptr_t addr);
Virt phys2virt(uintptr_t addr);
