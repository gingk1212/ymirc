#include "svm_vmmc.h"

#include "log.h"

/**
 * ASCII art from https://patorjk.com/software/taag/
 * Font: Varsity
 */
static const char *logo =
    " ____  ____  ____    ____  _____  _______      ______  \n"
    "|_  _||_  _||_   \\  /   _||_   _||_   __ \\   .' ___  | \n"
    "  \\ \\  / /    |   \\/   |    | |    | |__) | / .'   \\_| \n"
    "   \\ \\/ /     | |\\  /| |    | |    |  __ /  | |        \n"
    "   _|  |_    _| |_\\/_| |_  _| |_  _| |  \\ \\_\\ `.___.'\\ \n"
    "  |______|  |_____||_____||_____||____| |___|`.____ .' \n";

typedef enum {
  VMMCALL_NR_HELLO = 0,
} VmmcallNr;

static void vmmc_hello() {
  LOG_INFO("GREETINGS FROM VMM...\n%s\n", logo);
  LOG_INFO("This OS is hypervisored by YmirC.\n");
}

void handle_svm_vmmcall_exit(SvmVcpu *vcpu) {
  uint64_t rax = vcpu->vmcb->rax;

  switch (rax) {
    case VMMCALL_NR_HELLO:
      vmmc_hello();
      break;
    default:
      LOG_ERROR("Unhandled VMCALL: nr=%d\n", rax);
  }
}
