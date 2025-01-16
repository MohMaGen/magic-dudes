#define NOB_IMPLEMENTATION
#include <nob.h>
#include <common.h>
#include <module.h>

bool
module_init(Module *mod, Mem_Funcs temp, char *module_dir) 
{
	mod->name = temp.strdup("raylib");
	mod->c_inc_dir  = temp.sprintf("%s/inc", module_dir);

	mod->lang = mod_lang_Custom;
	mod->type = mod_type_Lib;

	char *src_dir = temp.sprintf("%s/lib/src", module_dir);

	mod->glob_libs = (Strings) { 0 };
	nob_da_append(&mod->glob_libs, temp.strdup("m"));

	mod->files.custom.build_cmd = (Nob_Cmd) { 0 };

	nob_cmd_append(&mod->files.custom.build_cmd,
		       temp.sprintf("%s/build.sh", module_dir));

	mod->files.custom.res = temp.sprintf("%s/libraylib.a", src_dir);

	mod->files.custom.deps = (Nob_File_Paths) { 0 };
	nob_da_append(&mod->files.custom.deps,
		      temp.sprintf("%s/lib/src/raylib.h", module_dir));

	Nob_Cmd cp_header = { 0 };
	nob_cmd_append(&cp_header, "cp",
		       temp.sprintf("%s/lib/src/raylib.h",    module_dir),
		       temp.sprintf("%s/inc/raylib/raylib.h", module_dir));

	if (!nob_cmd_run_sync_and_reset(&cp_header)) {
		nob_log(NOB_ERROR, "Failed to cp header!");
		return false;
	}

	return true;
}
