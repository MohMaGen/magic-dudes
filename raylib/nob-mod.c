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

	char *src_dir = temp.sprintf("%s/raylib/src", module_dir);

	nob_cmd_append(&mod->files.custom.build_cmd, "make", "-C");
	nob_cmd_append(&mod->files.custom.build_cmd, src_dir);

	mod->files.custom.res = temp.sprintf("%s/libraylib.a", src_dir);

	return true;
}
