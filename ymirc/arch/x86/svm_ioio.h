#pragma once

#include "svm_vcpu.h"

/** Handle #VMEXIT caused by IOIO instructions.
 * Note that this function does not increment the RIP. */
void handle_svm_ioio_exit(SvmVcpu *vcpu);
