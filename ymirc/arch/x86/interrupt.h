#pragma once

#include "isr.h"

/** Interrupt handler function signature. */
typedef void (*Handler)(Context *ctx);

void itr_init();
void itr_dispatch(Context *ctx);
void register_handler(uint8_t vector, Handler handler);
