/* Kickstart 1.3 argument parsing. LGPL-3.0-or-later. No OS or C runtime calls. */
#include "args.h"

#define REQUIRED 1
#define KEYWORD  2
#define NUMBER   4
#define SWITCH   8

struct field {
    const char *name;
    unsigned int length;
    unsigned int flags;
    int present;
};

static int upper(int c)
{
    return c >= 'a' && c <= 'z' ? c - ('a' - 'A') : c;
}

/* Tokenize without a token array. '=' is a separator outside quotes. */
static int token(char **cursor, char **value, int *equals, int *quoted)
{
    char *p = *cursor, *out;
    int closed = 0;
    while (*p == ' ' || *p == '\t') ++p;
    *equals = *quoted = 0;
    if (!*p || *p == '\n' || *p == '\r') { *cursor = p; return 0; }
    if (*p == '=') return -1;
    if (*p == '"') { *quoted = 1; ++p; }
    *value = out = p;
    while (*p && *p != '\n' && *p != '\r') {
        char c = *p++;
        if (*quoted) {
            if (c == '"') {
                if (*p && *p != '=' && *p != ' ' && *p != '\t' &&
                    *p != '\n' && *p != '\r') return -1;
                closed = 1;
                break;
            }
            if (c == '*') {
                c = *p++;
                if (!c || c == '\n' || c == '\r') return -1;
                if (upper(c) == 'N') c = '\n';
                else if (upper(c) == 'E') c = 27;
            }
        } else if (c == '=' || c == ' ' || c == '\t') {
            if (c == '=') *equals = 1;
            break;
        }
        *out++ = c;
        if (*quoted && (!*p || *p == '\n' || *p == '\r')) return -1;
    }
    if (*quoted && !closed) return -1;
    while (*p == ' ' || *p == '\t') ++p;
    if (*p == '=' && !*equals) { *equals = 1; ++p; }
    *cursor = p;
    *out = 0;
    return 1;
}

static int matches(const struct field *f, const char *word)
{
    unsigned int pos = 0;
    while (pos < f->length) {
        unsigned int i = 0;
        while (pos + i < f->length && f->name[pos + i] != '=' && word[i] &&
               upper(f->name[pos + i]) == upper(word[i])) ++i;
        if (!word[i] && (pos + i == f->length || f->name[pos + i] == '='))
            return 1;
        while (pos < f->length && f->name[pos] != '=') ++pos;
        ++pos;
    }
    return 0;
}

int compat_parse_args(const char *template, char *line, uintptr_t *result,
                      int32_t *nums)
{
    struct field fields[COMPAT_MAX_ARGS];
    int count = 0, i, rc, equals, quoted;
    char *word;
    while (*template) {
        struct field *f;
        if (count == COMPAT_MAX_ARGS) return -1;
        f = &fields[count];
        result[count++] = 0;
        f->name = template;
        f->flags = 0;
        f->present = 0;
        while (*template && *template != ',' && *template != '/') ++template;
        f->length = (unsigned int)(template - f->name);
        if (!f->length) return -1;
        while (*template == '/') {
            ++template;
            switch (upper(*template++)) {
                case 'A': f->flags |= REQUIRED; break;
                case 'K': f->flags |= KEYWORD; break;
                case 'N': f->flags |= NUMBER; break;
                case 'S': f->flags |= SWITCH; break;
                default: return -1;
            }
        }
        if (*template && *template++ != ',') return -1;
    }
    while ((rc = token(&line, &word, &equals, &quoted)) > 0) {
        int selected = -1;
        if (!quoted) {
            if (word[0] == '?' && !word[1]) return -1;
            for (i = 0; i < count; ++i)
                if (matches(&fields[i], word)) { selected = i; break; }
        }
        if (selected >= 0) {
            if (fields[selected].present) return -1;
            if (fields[selected].flags & SWITCH) {
                if (equals) return -1;
                result[selected] = (uintptr_t)-1;
                fields[selected].present = 1;
                continue;
            }
            if (token(&line, &word, &equals, &quoted) != 1 || equals) return -1;
        } else {
            if (equals) return -1;
            for (i = 0; i < count; ++i)
                if (!fields[i].present && !(fields[i].flags & (KEYWORD | SWITCH))) {
                    selected = i;
                    break;
                }
            if (selected < 0) return -1;
        }
        if (fields[selected].flags & NUMBER) {
            int used = compat_strtol(word, &nums[selected]);
            if (used < 0 || word[used]) return -1;
            result[selected] = (uintptr_t)&nums[selected];
        } else result[selected] = (uintptr_t)word;
        fields[selected].present = 1;
    }
    if (rc < 0) return -1;
    for (i = 0; i < count; ++i)
        if ((fields[i].flags & REQUIRED) && !fields[i].present) return -1;
    return 0;
}
