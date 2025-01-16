#define NOB_IMPLEMENTATION
#include <nob.h>
#include <common.h>
#include <module.h>

bool
module_init(Module *mod, Mem_Funcs temp, char *module_dir) 
{
	mod->name = temp.strdup("hello-probe");
	mod->lang  = mod_lang_C;
	mod->type  = mod_type_Exe;

	nob_da_append(&mod->deps, temp.strdup("core"));
	nob_da_append(&mod->deps, temp.strdup("raylib"));
	
	char *main = temp.sprintf("%s/main.c", module_dir);
	nob_da_append(&mod->files.c.srcs, main);

	return true;
}
