#pragma once

#ifdef CONFIG_GDBSTUB

/** Initialize GDB stub. */
void gdbstub_init(void);

/* This function will generate a breakpoint exception.  It is used at the
   beginning of a program to sync up with a debugger and can be used
   otherwise as a quick means to stop program execution and "break" into
   the debugger.  */
void gdbstub_breakpoint(void);

#endif /* CONFIG_GDBSTUB */
