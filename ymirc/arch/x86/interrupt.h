#pragma once

#include "isr.h"

/** Interrupt handler function signature. */
typedef void (*Handler)(Context *ctx);

/** Subscriber callback function signature. */
typedef void (*SubscriberCallback)(void *self, Context *ctx);

/** Initialize the IDT. */
void itr_init();

/** Called from the ISR stub. Dispatches the interrupt to the appropriate
 * handler. */
void itr_dispatch(Context *ctx);

/** Register interrupt handler. */
void register_handler(uint8_t vector, Handler handler);

/** Subscribe to interrupts. Subscribers are called when an interrupt is
 * triggered before the interrupt handler. */
void subscribe2interrupt(void *ctx, SubscriberCallback callback);
