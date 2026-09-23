/* Global environment-file support. LGPL-3.0-or-later. */
#include "dos_support.h"

/* 1.3 has ENV: files but no DOS local-variable list. InstallIfy explicitly
 * requests GLOBAL on that OS. This is private support, not a DOS patch.
 */
LONG CompatSetVar(__reg("d1") const char *name, __reg("d2") const char *buffer,
                   __reg("d3") LONG size, __reg("d4") ULONG flags)
{
    ULONG bytes, i;
    char *path;
    BPTR file;
    LONG result = 0, error = 0;
    if (DOSBase->dl_lib.lib_Version >= 36)
        return SetVar((STRPTR)name, (STRPTR)buffer, size, flags);
    if (!name || !*name || size < -1 || !(flags & GVF_GLOBAL_ONLY) ||
        (flags & ~(GVF_GLOBAL_ONLY | GVF_BINARY_VAR))) {
        set_error(ERROR_BAD_NUMBER);
        return 0;
    }
    bytes = length(name) + 5;
    path = alloc_mem(bytes, MEMF_PUBLIC, EXECBASE);
    if (!path) { set_error(ERROR_NO_FREE_STORE); return 0; }
    path[0] = 'E'; path[1] = 'N'; path[2] = 'V'; path[3] = ':';
    for (i = 0; i < bytes - 4; ++i) path[i + 4] = name[i];
    if (!buffer) {
        result = DeleteFile(path);
        if (!result) error = IoErr();
    } else {
        if (size == -1) size = length(buffer);
        file = Open(path, MODE_NEWFILE);
        if (file) {
            result = Write(file, (APTR)buffer, size) == size;
            if (!result) {
                error = IoErr();
                if (!error) error = ERROR_DISK_FULL;
            }
            /* Old DOS Close has no documented success return value. */
            Close(file);
        } else error = IoErr();
    }
    free_mem(path, bytes, EXECBASE);
    set_error(error);
    return result ? DOSTRUE : DOSFALSE;
}
