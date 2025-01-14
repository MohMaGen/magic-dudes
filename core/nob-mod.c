#define NOB_IMPLEMENTATION
#include <nob.h>
#include <common.h>
#include <module.h>

bool
module_init(Module *mod, Mem_Funcs temp, char *module_dir) 
{
	mod->name = temp.strdup("core");

	char *src_dir  = temp.sprintf("%s/src", module_dir);
	mod->c_inc_dir = temp.sprintf("%s/inc", module_dir);

	mod->lang  = mod_lang_C;
	mod->type  = mod_type_Lib;

	nob_da_append(&mod->deps, temp.strdup("raylib"));
	

	Nob_File_Paths srcs = { 0 };
	nob_read_entire_dir(src_dir, &srcs);
	for (size_t i = 0; i < srcs.count; ++i) {
		char *path = temp.sprintf("%s/%s", src_dir, srcs.items[i]);
		if (strendswith(path, ".c")) {
			nob_da_append(&mod->files.c.srcs, path);
		}

		if (strendswith(path, ".h")) {
			nob_da_append(&mod->files.c.incs, path);
		}	
	}
	nob_da_free(srcs);

	char *inc_dir = temp.sprintf("%s/inc/core", module_dir);

	Nob_File_Paths incs = { 0 };
	nob_read_entire_dir(inc_dir, &incs);
	for (size_t i = 0; i < incs.count; ++i) {
		char *path = temp.sprintf("%s/%s", src_dir, srcs.items[i]);

		if (strendswith(path, ".h")) {
			nob_da_append(&mod->files.c.incs, path);
		}	
	}

	return true;
}
