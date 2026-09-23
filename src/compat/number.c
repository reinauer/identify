/* Signed decimal conversion, no OS calls. LGPL-3.0-or-later. */
#include "args.h"

int compat_strtol(const char *text, int32_t *value)
{
    const char *start = text;
    uint32_t n = 0;
    int negative = 0, digits = 0;
    *value = 0;
    while (*text == ' ' || *text == '\t') ++text;
    if (*text == '-' || *text == '+') negative = *text++ == '-';
    while (*text >= '0' && *text <= '9') {
        unsigned int digit = *text++ - '0';
        if (n > 214748364UL || (n == 214748364UL && digit > (negative ? 8U : 7U)))
            return -1;
        n = n * 10 + digit;
        ++digits;
    }
    if (!digits) return -1;
    *value = (int32_t)(negative ? 0UL - n : n);
    return (int)(text - start);
}
