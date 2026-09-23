/* DOS number conversion with a 1.3 fallback. LGPL-3.0-or-later. */
#include "dos_support.h"
#include "args.h"

LONG CompatStrToLong(__reg("d1") const char *text, __reg("d2") int32_t *value)
{
    if (DOSBase->dl_lib.lib_Version >= 36)
        return StrToLong((STRPTR)text, (LONG *)value);
    return compat_strtol(text, value);
}
