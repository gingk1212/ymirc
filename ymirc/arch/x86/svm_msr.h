#pragma once

#include "svm_vcpu.h"

/** Handle #VMEXIT caused by RDMSR or WRMSR instruction.
 * Note that this function does not increment the RIP. */
void handle_svm_msr_exit(SvmVcpu* vcpu);
