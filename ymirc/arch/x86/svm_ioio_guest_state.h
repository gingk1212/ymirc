#pragma once

#include <stdint.h>

typedef enum {
  SVM_PIC_INIT_PHASE_UNINITIALIZED,
  SVM_PIC_INIT_PHASE_PHASE1,
  SVM_PIC_INIT_PHASE_PHASE2,
  SVM_PIC_INIT_PHASE_PHASE3,
  SVM_PIC_INIT_PHASE_INITED,
} SvmPicInitPhase;

/** Guest IOIO state VMM needs to preserve. */
typedef struct {
  /** Serial port register values. */
  uint8_t ier;  // Interrupt Enable Register.
  uint8_t mcr;  // Modem Control Register.

  /** 8259 Programmable Interrupt Controller. */
  uint8_t primary_mask;           // Mask of the primary PIC.
  uint8_t secondary_mask;         // Mask of the secondary PIC.
  SvmPicInitPhase primary_phase;  // Initialization phase of the primary PIC.
  SvmPicInitPhase
      secondary_phase;     // Initialization phase of the secondary PIC.
  uint8_t primary_base;    // Vector offset of the primary PIC.
  uint8_t secondary_base;  // Vector offset of the secondary PIC.
} SvmIoioGuestState;

static inline SvmIoioGuestState svm_ioio_guest_state_new() {
  return (SvmIoioGuestState){
      .primary_mask = 0xFF,
      .secondary_mask = 0xFF,
      .primary_phase = SVM_PIC_INIT_PHASE_UNINITIALIZED,
      .secondary_phase = SVM_PIC_INIT_PHASE_UNINITIALIZED,
  };
}
