/* Private DOS support; no startup or auto-open code. LGPL-3.0-or-later. */
#ifndef IDENTIFY_DOS_SUPPORT_H
#define IDENTIFY_DOS_SUPPORT_H
#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/var.h>
#include <dos/rdargs.h>
#include <clib/dos_protos.h>
#include <pragmas/dos_pragmas.h>

extern struct DosLibrary *DOSBase;

static APTR alloc_mem(__reg("d0") ULONG size, __reg("d1") ULONG flags,
                      __reg("a6") APTR base) = "\tjsr\t-198(a6)";
static void free_mem(__reg("a1") APTR mem, __reg("d0") ULONG size,
                     __reg("a6") APTR base) = "\tjsr\t-210(a6)";
static APTR find_task(__reg("a1") APTR name, __reg("a6") APTR base) =
    "\tjsr\t-294(a6)";
static void raw_format(__reg("a0") const char *format, __reg("a1") APTR args,
                       __reg("a2") void (*putch)(void), __reg("a3") APTR data,
                       __reg("a6") APTR base) = "\tjsr\t-522(a6)";

#define EXECBASE (*(APTR *)4)

static void set_error(LONG error)
{
    struct Process *process = find_task(0, EXECBASE);
    process->pr_Result2 = error;
}

static ULONG length(const char *s)
{
    ULONG n = 0;
    while (s[n]) ++n;
    return n;
}


#endif
