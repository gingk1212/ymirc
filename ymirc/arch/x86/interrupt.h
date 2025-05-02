#pragma once

#include "isr.h"

void itr_init();
void itr_dispatch(Context *ctx);
