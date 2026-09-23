/* Internal CLI parser shared by the Kickstart 1.3 tools. LGPL-3.0-or-later. */
#ifndef IDENTIFY_ARGS_H
#define IDENTIFY_ARGS_H

#include <stdint.h>

#define COMPAT_MAX_ARGS 16

/* Modifies line in place. Result strings point into line; /N values into nums.
 * Supports the /A, /K, /N and /S template options used by Identify's tools.
 * Returns zero on success, -1 for invalid input or an unsupported template. */
int compat_parse_args(const char *template, char *line, uintptr_t *result,
                      int32_t *nums);
int compat_strtol(const char *text, int32_t *value);

#endif
