/* DOS compatibility, using native V36 calls when available. LGPL-3.0-or-later.
 * No C runtime or automatic library opening: usable from identify.library.
 */
#include "dos_support.h"
#include "args.h"

extern void CompatCountChar(void), CompatPutChar(void);
char *CompatArgPtr;
ULONG CompatArgLen;
LONG CompatOutputError;

static LONG write_output(const char *s, ULONG size)
{
    LONG n = Write(Output(), (APTR)s, size);
    if (n != size) { CompatOutputError = 1; return -1; }
    return n;
}

LONG CompatPutStr(__reg("d1") const char *text)
{
    if (DOSBase->dl_lib.lib_Version >= 36) {
        LONG result = PutStr((STRPTR)text);
        if (result < 0) CompatOutputError = 1;
        return result;
    }
    return write_output(text, length(text)) < 0 ? -1 : 0;
}

LONG CompatVPrintf(__reg("d1") const char *format, __reg("d2") APTR args)
{
    ULONG size = 0;
    char *buffer;
    LONG result;
    if (DOSBase->dl_lib.lib_Version >= 36) {
        result = VPrintf((STRPTR)format, args);
        if (result < 0) CompatOutputError = 1;
        return result;
    }
    raw_format(format, args, CompatCountChar, &size, EXECBASE);
    buffer = alloc_mem(size, MEMF_PUBLIC, EXECBASE);
    if (!buffer) {
        set_error(ERROR_NO_FREE_STORE);
        CompatOutputError = 1;
        return -1;
    }
    raw_format(format, args, CompatPutChar, buffer, EXECBASE);
    result = write_output(buffer, size - 1); /* RawDoFmt emits a final NUL. */
    free_mem(buffer, size, EXECBASE);
    return result;
}

struct arguments {
    ULONG size;
    int32_t numbers[COMPAT_MAX_ARGS];
    char line[1];
};

APTR CompatReadArgs(__reg("d1") const char *template,
                     __reg("d2") uintptr_t *result, __reg("d3") APTR unused)
{
    struct arguments *args;
    ULONG i, size;
    if (DOSBase->dl_lib.lib_Version >= 36)
        return ReadArgs((STRPTR)template, (LONG *)result, unused);
    if (!CompatArgPtr || CompatArgLen > 65535) {
        set_error(ERROR_LINE_TOO_LONG);
        return 0;
    }
    size = sizeof(*args) + CompatArgLen;
    args = alloc_mem(size, MEMF_PUBLIC, EXECBASE);
    if (!args) { set_error(ERROR_NO_FREE_STORE); return 0; }
    args->size = size;
    for (i = 0; i < CompatArgLen; ++i) args->line[i] = CompatArgPtr[i];
    args->line[i] = 0;
    if (compat_parse_args(template, args->line, result, args->numbers)) {
        free_mem(args, size, EXECBASE);
        set_error(ERROR_BAD_TEMPLATE);
        return 0;
    }
    return args;
}

void CompatFreeArgs(__reg("d1") struct arguments *args)
{
    if (DOSBase->dl_lib.lib_Version >= 36) {
        FreeArgs((struct RDArgs *)args);
        return;
    }
    if (args) free_mem(args, args->size, EXECBASE);
}
