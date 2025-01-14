#ifndef __MAGIC_DUDES_NOB_COMMON_CONF__
#define __MAGIC_DUDES_NOB_COMMON_CONF__

#include "./common.h"


struct conf {
	/*
		Feilds are read from nob.conf file
	 */
        char   *build_dir;
        char   *c_compiler;
	Strings c_debug_flags;
        Strings c_warnings_flags;

        char   *c3_compiler;
        Strings c3_warnings_flags;
	Strings nob_dl_flags;

	char   *archive;
	Strings archive_flags;


	/*
		Feilds are setted from execution flags
	 */
	bool debug; // -d or --debug
	bool rebuild;
};



Nob_String_View
conf_read_name(Nob_String_View *line)
{
        size_t curr = 0;
        const char  *data = line->data;

        while (curr < line->count && 
                (isalnum(line->data[curr]) ||
                 line->data[curr] == '-')) curr++;

        *line = sv_shift(*line, curr);

        return (Nob_String_View) { .count = curr, .data = data };
}


bool
conf_expect_char(Nob_String_View *line, char symbol)
{
        if (line->count == 0 && *line->data != symbol) return false;
        
        *line = sv_shift(*line, 1);

        return true;
}

bool
conf_expect_one_of(Nob_String_View *line, const char *seq)
{
        if (line->count == 0) return false;

        for (const char *curr = seq; *curr != 0; ++curr) {
                if (*line->data == *curr) {
                        *line = sv_shift(*line, 1);
                        return true;
                }
        }
        
        *line = sv_shift(*line, 1);

        return false;
}



Nob_String_View
conf_read_bash_name(Nob_String_View *line)
{
                
        size_t curr = 0;
        const char  *data = line->data;

        while (curr < line->count &&
                (isalnum(line->data[curr]) ||
                 line->data[curr] == '-'   ||
                 line->data[curr] == '_')) curr++;


        *line = sv_shift(*line, curr);

        return (Nob_String_View) { .count = curr, .data = data };
}


Nob_String_View
conf_read_string_literal(Nob_String_View *line)
{
        Nob_String_Builder  sb = { 0 };
        Nob_String_View result = { 0 };

        if (line->count == 0 || line->data == NULL)
                return (Nob_String_View) { 0 };

        char *end_seq = ",: \n\0";
        if (*line->data == '"') {
                end_seq = "\"";
                *line = sv_shift(*line, 1);
        }

	Nob_String_View tmp = *line;
	if (conf_expect_one_of(&tmp, end_seq))
		return (Nob_String_View) { 0 };

        while (line->count > 0 && !one_of(*line->data, end_seq)) {
                nob_sb_append_buf(&sb, line->data, 1);
                *line = sv_shift(*line, 1);

        } 

	/*if (sb.items[sb.count-1] != '"') {
		line->data--;
		line->count++;
	}*/

defer:
        if (sb.items != NULL) {        
                nob_sb_append_null(&sb);

                result = nob_sb_to_sv(sb);
                result.data = nob_temp_strdup(result.data);

                nob_sb_free(sb);
        }

        return result;
}

Nob_String_View
conf_read_path(Nob_String_View *line)
{
        Nob_String_Builder  sb = { 0 };
        Nob_String_View    result = { 0 };

        if (line->count == 0 || line->data == NULL)
                return (Nob_String_View) { 0 };

        char *end_seq = ", :\n\0";
        if (*line->data == '"') {
                end_seq = "\"";
                *line = sv_shift(*line, 1);
        }

        do {                
                if (line->count == 0)
                        nob_return_defer(((Nob_String_View) { 0 }));


                if (*line->data == '$') {
                        *line = sv_shift(*line, 1);

                        Nob_String_View env_sv = conf_read_bash_name(line);


                        char *env = str_temp_of_sv(env_sv);
                        char *val = getenv(env);

                        if (val == NULL) continue;

                        nob_sb_append_buf(&sb, val, strlen(val));
                } else {
                        nob_sb_append_buf(&sb, line->data, 1);
                }

        } while (!conf_expect_one_of(line, end_seq));

defer:
        if (sb.items != NULL) {        
                nob_sb_append_null(&sb);

                result = nob_sb_to_sv(sb);
                result.data = nob_temp_strdup(result.data);

                nob_sb_free(sb);
        }

        return result;
}


Strings
conf_read_strings(Nob_String_View *line)
{
        Strings         result = { 0 };
        Nob_String_View string = { 0 };

        do {
                if (line->count == 0) break;

                *line = nob_sv_trim_left(*line);
                string = conf_read_string_literal(line);

		if (string.count == 0) continue;

                nob_da_append(&result, str_temp_of_sv(string));
                
        } while (conf_expect_one_of(line, ","));

        return result;
}


bool
read_conf(const char *conf_path, struct conf *build_conf)
{
        bool result = true;

        char line[0x100] = { 0 };


        FILE *conf = fopen(conf_path, "r");

        while (fgets(line, NOB_ARRAY_LEN(line), conf) != NULL) {
                Nob_String_View line_view = nob_sv_from_cstr(line);
                if (line_view.count == 0) continue;

                line_view = nob_sv_trim_left(line_view);
                if (line_view.count == 0) continue;

                if (line_view.data[line_view.count-1] == '\n') {
                        line_view.count--;
                }



                if (line_view.data[0] == '#') continue;

                Nob_String_View name = conf_read_name(&line_view);

                if (name.count == 0) {
                        nob_log(NOB_ERROR, "Expect prop name, but get `%.*s'",
                                (int)line_view.count, line_view.data);
                        nob_return_defer(false);
                }

                line_view = nob_sv_trim_left(line_view);

                if(!conf_expect_char(&line_view, '=')) {
                        nob_log(NOB_ERROR,
                                "Expect '=' symbol after property name, "
                                "but get: `%s'",
                                line_view);

                        nob_return_defer(false);
                }

                if (nob_sv_eq(name, nob_sv_from_cstr("build-dir"))) {
                        Nob_String_View path = conf_read_path(&line_view);
                        path = nob_sv_trim_right(path);
                                
                        build_conf->build_dir = str_temp_of_sv(path);
                }

                if (nob_sv_eq(name, nob_sv_from_cstr("c-compiler"))) {
                        Nob_String_View path = conf_read_path(&line_view);
                        path = nob_sv_trim_right(path);
                                
                        build_conf->c_compiler = str_temp_of_sv(path);
                }

                if (nob_sv_eq(name, nob_sv_from_cstr("c-warnings"))) {
                        Strings strings = conf_read_strings(&line_view);

                        build_conf->c_warnings_flags = strings;
                }

                if (nob_sv_eq(name, nob_sv_from_cstr("c3-compiler"))) {
                        Nob_String_View path = conf_read_path(&line_view);
                        path = nob_sv_trim_right(path);

                        build_conf->c3_compiler = str_temp_of_sv(path);
                }

                if (nob_sv_eq(name, nob_sv_from_cstr("c3-warnings-flags"))) {
                        Strings strings = conf_read_strings(&line_view);

                        build_conf->c3_warnings_flags = strings;
                }

                if (nob_sv_eq(name, nob_sv_from_cstr("nob-dl-flags"))) {
                        Strings strings = conf_read_strings(&line_view);

                        build_conf->nob_dl_flags = strings;
                }

		if (nob_sv_eq(name, nob_sv_from_cstr("c-debug-flags"))) {
                        Strings strings = conf_read_strings(&line_view);

                        build_conf->c_debug_flags = strings;		
		}

                if (nob_sv_eq(name, nob_sv_from_cstr("archive"))) {
                        Nob_String_View path = conf_read_path(&line_view);
                        path = nob_sv_trim_right(path);

                        if (path.count == 0) {
                                nob_log(NOB_ERROR,
                                        "Expect path as `config-dir' value,",
                                        "but get: `%s'",
                                        line_view);

                                nob_return_defer(false);
                        }
                                
                        build_conf->archive = str_temp_of_sv(path);
                }

		if (nob_sv_eq(name, nob_sv_from_cstr("archive-flags"))) {
                        Strings strings = conf_read_strings(&line_view);

                        build_conf->archive_flags = strings;		
		}
        }

defer:
        fclose(conf);
        return result;
}

#define d_bool(b) (b ? "true" : "false")

void
log_conf(struct conf build_conf)
{	
        nob_log(NOB_INFO, "build config:");

        nob_log(NOB_INFO, "    | nob-dl-flags:      %s", 
                          display_strings(build_conf.nob_dl_flags));

        nob_log(NOB_INFO, "    | build-dir:         %s",
			  build_conf.build_dir);
        nob_log(NOB_INFO, "    | c-compiler:        %s",
			  build_conf.c_compiler);
        nob_log(NOB_INFO, "    | c-warnings-flags:  %s", 
                          display_strings(build_conf.c_warnings_flags));
        nob_log(NOB_INFO, "    | c-debug-flags:     %s", 
                          display_strings(build_conf.c_debug_flags));

        nob_log(NOB_INFO, "    | c3-compiler:       %s",
			  build_conf.c3_compiler);
        nob_log(NOB_INFO, "    | c3-warnings-flags: %s", 
                          display_strings(build_conf.c3_warnings_flags));

        nob_log(NOB_INFO, "    | archive:           %s",
			  build_conf.archive);
        nob_log(NOB_INFO, "    | archive-flags:     %s", 
                          display_strings(build_conf.archive_flags));

	nob_log(NOB_INFO, "flag options:");
        nob_log(NOB_INFO, "    | rebuild:           %s", 
			  d_bool(build_conf.rebuild));
        nob_log(NOB_INFO, "    | debug:             %s", 
			  d_bool(build_conf.debug));
}

#endif
