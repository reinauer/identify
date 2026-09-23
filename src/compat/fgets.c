/* 1.3 FD-file support. LGPL-3.0-or-later. */
#include "dos_support.h"

/* Only used on regular FD files. Read a block and seek back the unused tail;
 * all state belongs to the caller's file handle, so calls can be interleaved.
 */
char *CompatFGets(__reg("d1") BPTR file, __reg("d2") char *buffer,
                  __reg("d3") LONG size)
{
    LONG got, used;
    if (DOSBase->dl_lib.lib_Version >= 36) return FGets(file, buffer, size);
    if (size < 2) { set_error(ERROR_BAD_NUMBER); return 0; }
    set_error(0);
    got = Read(file, buffer, size - 1);
    if (got <= 0) return 0;
    for (used = 0; used < got; ++used)
        if (buffer[used] == '\n') { ++used; break; }
    if (used < got && Seek(file, used - got, OFFSET_CURRENT) == -1) return 0;
    buffer[used] = 0;
    set_error(0);
    return buffer;
}
