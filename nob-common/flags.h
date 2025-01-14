#ifndef __MAGIC_DUDES_NOB_COMMON_FLAGS__
#define __MAGIC_DUDES_NOB_COMMON_FLAGS__

#include "./common.h"

typedef struct {
	bool  help;
	bool  debug;
	bool  rebuild;
	char *config;
} Flags;

bool
set_bool_flag(char *name, bool is_long, Flags *flags)
{
	if (strcmp(name, "help") == 0 && is_long) {
		flags->help = true;	
		return true;
	}

	if (strcmp(name, "debug") == 0 && is_long) {
		flags->debug = true;	
		return true;
	}

	if (strcmp(name, "rebuild") == 0 && is_long) {
		flags->rebuild = true;
		return true;
	}

	return false;
}

bool
set_str_flag(char *name, Nob_String_View sv,  char *next, bool is_long,
	     Flags *flags)
{
	char *value = NULL;
	if (next != NULL)  value = next;
	if (sv.count != 0 && *sv.data == '=') value = (char *)(sv.data + 1);

	if (strcmp(name, "c") == 0 && !is_long) {
		if (value == NULL) {
			nob_log(NOB_ERROR, "Expect value of flag `config'");
			return false;
		}

		flags->config = value;	

		return true;
	}

	return false;
}

bool
read_flags(int argc, char **argv, Flags *flags)
{
	bool is_long = false;

	for (int i = 0; i < argc; ++i) {	
		Nob_String_View flag_sv = nob_sv_from_cstr(argv[i]);

		if (strstartswith(argv[i], "--")) {
			shift_sv(&flag_sv, 2);
			is_long = true;
		} else if (*argv[i] == '-') {
			shift_sv(&flag_sv, 1);
			is_long = false;
		} else continue;


		Nob_String_View name_sv = conf_read_bash_name(&flag_sv);

		if (name_sv.count == 0) {
			nob_log(NOB_ERROR, "invalid flag: `%s'", argv[i]);
			return false;
		}

		char *name = str_temp_of_sv(name_sv);

		if (set_bool_flag(name, is_long, flags)) continue;

		char *next = i + 1 < argc ? argv[i+1] : NULL;
		if (set_str_flag(name, flag_sv, next, is_long, flags))
			continue;
			 
		nob_log(NOB_ERROR, "Wrong flag: `%s'", argv[i]);
		return false;
	}

	return true;
}

#endif
