#ifndef __MAGIC_DUDES_NOB_COMMON_COMMON__
#define __MAGIC_DUDES_NOB_COMMON_COMMON__

bool
strstartswith(const char *str, const char *pref)
{
        size_t str_len = strlen(str), pref_len = strlen(pref);

        if (pref_len > str_len) return false;

        return memcmp(str, pref, pref_len) == 0;
}

bool
strendswith(const char *str, const char *sufx)
{
        size_t str_len = strlen(str), sufx_len = strlen(sufx);

        if (sufx_len > str_len) return false;

        return memcmp(str + str_len - sufx_len, sufx, sufx_len) == 0;
}


typedef struct {
        char **items;
        size_t count;
        size_t capacity;
} Strings;

char *
display_strings(Strings strings)
{
        Nob_String_Builder sb = { 0 };        

	if (strings.count == 0 || strings.items == NULL) return "(null)";

        for (size_t i = 0; i < strings.count; ++i) {
                if (i != 0) nob_sb_append_buf(&sb, ", ", 2);

                nob_sb_append_buf(&sb, strings.items[i],
				       strlen(strings.items[i]));

        }
        nob_sb_append_null(&sb);

        char *ret =  nob_temp_strdup(sb.items);
        nob_sb_free(sb);

        return ret;
}


Nob_String_View
sv_shift(Nob_String_View view, size_t shiftwidth)
{
        if (shiftwidth >= view.count) return (Nob_String_View) { 0 };

        return (Nob_String_View) {
                .count = view.count - shiftwidth,
                .data  = view.data + shiftwidth,
        };
}

void
shift_sv(Nob_String_View *view, size_t shiftwidth)
{
	*view = sv_shift(*view, shiftwidth);
}

bool
one_of(char symbol, const char *seq)
{
        for (const char *curr = seq; *curr != 0; ++curr) {
                if (symbol == *curr) {
                        return true;
                }
        }

        return false;
}


bool
sv_starts_with(Nob_String_View str, Nob_String_View pref)
{
        if (pref.count > str.count) return false;

        return memcmp(str.data, pref.data, pref.count) == 0;
}

char *
str_temp_of_sv(Nob_String_View view)
{
        char *str = nob_temp_alloc((view.count + 1) * sizeof (char));
        memcpy(str, view.data, view.count);
        str[view.count] = '\0';

        return str;
}


#endif
