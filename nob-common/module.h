#ifndef __MAGIC_DUDES_NOB_COMMON_MODULE__
#define __MAGIC_DUDES_NOB_COMMON_MODULE__

#include "./conf.h"
#include <dlfcn.h>


typedef struct  {
        /*
                Fields set by module itself.
         */
        char *name;

        enum { mod_lang_C = 0,   mod_lang_C3, mod_lang_Custom } lang;

        enum { mod_type_Exe = 0, mod_type_Lib } type;

        Strings deps;

        union {
                struct { Nob_File_Paths srcs, incs; } c;
                struct { Nob_File_Paths srcs; } c3;
                struct { Nob_Cmd build_cmd; char *res; } custom; 
        } files;        

        char *c_inc_dir; // dir with c headers.

        /*
                Fields set by compilation process.
         */
        Nob_File_Paths objs;

        char *build_dir;
        
        enum {
                mod_state_Inited = 0,
                mod_state_Builded,
                mod_state_Linked,
                mod_state_Failed,
        } state;
} Module;


typedef struct {
	char * (*strdup)  (const char *);
	char * (*sprintf) (const char *, ...);
	void * (*alloc)   (size_t);
} Mem_Funcs;

typedef void* (*module_temp_alloc_fn)      (size_t);
typedef bool  (*module_init_fn) (Module *, Mem_Funcs, const char *);



char *
module_name(const char *path)
{
        Nob_String_Builder sb = { 0 };
        
        for (const char *curr = path; *curr != '\0'; ++curr) {
                if (*curr == '/') {
                        nob_sb_append_buf(&sb, ".", 1);
                } else {
                        nob_sb_append_buf(&sb, curr, 1);
                }
        }

        nob_sb_append_null(&sb);

        char *ret = nob_temp_strdup(sb.items);
        nob_sb_free(sb);

        return ret;
}


bool
load_module(struct conf build_conf, const char *path, const char *dir,
            Module *mod, Nob_Cmd *cmd)
{
        *mod = (Module) { 0 };
        
        char *modules_dir = nob_temp_sprintf("%s/%s",
                                        build_conf.build_dir,
                                        "modules/");

        nob_mkdir_if_not_exists(modules_dir);


        nob_log(NOB_INFO, "load nob module from `%s'", path);        

        char *bin = nob_temp_sprintf("%s/%s.so", modules_dir,
                                                 module_name(path));

        int ret = nob_needs_rebuild1(bin, path);
        if (ret < 0) {
                nob_log(NOB_ERROR, "failed to check dependencies of "
                                   "nob module at `%s'", path);
                mod->state = mod_state_Failed;
                return false;
        }
        if (build_conf.rebuild || ret > 0) {
		nob_cmd_append(cmd, build_conf.c_compiler, "-o", bin);
		nob_cmd_append(cmd, "-I", "nob-common/");
		nob_cmd_append(cmd, "-I", "deps/nob/");
		if (build_conf.debug)
		nob_da_append_many(cmd, build_conf.c_debug_flags.items,
					build_conf.c_debug_flags.count);

		nob_da_append_many(cmd, build_conf.nob_dl_flags.items,
					build_conf.nob_dl_flags.count);
		nob_da_append(cmd,  path);

		nob_log(NOB_INFO, "build nob module");

		if (!nob_cmd_run_sync_and_reset(cmd)) {
			nob_log(NOB_ERROR, "failed to build nob module at `%s'",
				           path);
			mod->state = mod_state_Failed;
			return false;
		}
        }


	nob_log(NOB_INFO, "open module dll at path `%s'", bin);

        void *mod_lib = dlopen(bin, RTLD_LAZY);

        if (!mod_lib) {
                nob_log(NOB_ERROR, "failed to load module");
                nob_log(NOB_ERROR, "dlerror: ", dlerror());
                mod->state = mod_state_Failed;
                return false;
        }

	nob_log(NOB_INFO, "load `module_init' function from module `%s'", bin);

        module_init_fn init;
        init = dlsym(mod_lib, "module_init");

	if (init == NULL) {
		nob_log(NOB_ERROR, "Failed to load `module_init' function from "
				   "module at `%s', created from `%s",
				   bin, path);
		mod->state = mod_state_Failed;
		dlclose(mod_lib);
		return false;
	}

	Mem_Funcs funcs = {
		nob_temp_strdup,
		nob_temp_sprintf,
		nob_temp_alloc,
	};

        if (!init(mod, funcs, dir)) {
                nob_log(NOB_ERROR, "failed to init module");
                mod->state = mod_state_Failed;
		dlclose(mod_lib);
                return false;
        }

	dlclose(mod_lib);

	nob_log(NOB_INFO, "module `%s' inited.", mod->name);
        mod->state = mod_state_Inited;

        return true;
}

bool
load_module_dir(struct conf build_conf, const char *dir, Module *mod,
                 Nob_Cmd *cmd)
{
        char *path = nob_temp_sprintf("%s/%s", dir, "nob-mod.c");
        mod->build_dir = nob_temp_sprintf("%s/%s", build_conf.build_dir,
                                          mod->name);
        
        if (access(path, F_OK) != 0) {
                nob_log(NOB_ERROR, "Expect nob config file at path `%s'",
                                   path);
                return false;
        }
        
        return load_module(build_conf, path, dir, mod, cmd);
}

typedef struct {
        Module *items;
        size_t count;
        size_t capacity;
} Modules;

typedef struct {
        Module *items;
        size_t len, max_size;
} Modules_Box; // pre allocated array with constant capacity (max_size)

#define box_push(box, item) do {                                              \
                if ((box)->len >= (box)->max_size) {                          \
                        nob_log(NOB_ERROR, "Box out of the space!!!!!!!! D:");\
                } else {                                                      \
                        (box)->items[(box)->len++] = item;                    \
                }                                                             \
        } while (false)                                                       \




bool
get_module_idx(Modules modules, const char *name, size_t *idx)
{
        for (size_t i = 0; i < modules.count; ++i) {
                if (strcmp(modules.items[i].name, name) == 0) {
                        if (idx != NULL) *idx = i;
                        return true;
                }
        }

        return false;        
}

bool
cmd_append_inc_dirs(Modules modules, Module *mod, Nob_Cmd *cmd)
{
        Module *dep;
        size_t idx;

        for (size_t i = 0; i < mod->deps.count; ++i) {
                if (!get_module_idx(modules, mod->deps.items[i], &idx)) {
                        nob_log(NOB_ERROR, "Failed!");
                        return false;
                }

                dep = modules.items + idx;

                if (dep->c_inc_dir == NULL) {
                        nob_log(NOB_ERROR, "Inc dir of module `%s' was not set"
                                           " , but expected while beeing "
                                           "building module `%s'.",
                                           dep->name, mod->name);
                        return false;
                }
                nob_cmd_append(cmd, "-I", dep->c_inc_dir);
        }

        return true;
}

bool
build_c_module(struct conf build_conf, Modules modules, Module *mod,
               Nob_Cmd *cmd)
{
        bool result = true;

        Nob_File_Paths deps = { 0 };
        Nob_Procs     procs = { 0 };

        mod->objs = (Nob_File_Paths) { 0 };


        for (size_t i = 0; i < mod->files.c.srcs.count; ++i) {
                deps = (Nob_File_Paths) { 0 };

                char *obj = nob_temp_sprintf("%s/%s.o", mod->build_dir,
                                             mod->files.c.srcs.items[i]);
                nob_da_append(&mod->objs, obj);

                nob_da_append(&deps,  mod->files.c.srcs.items[i]);
                nob_da_append_many(&deps, mod->files.c.incs.items,
                                          mod->files.c.incs.count);

                int ret = nob_needs_rebuild(obj, deps.items, deps.count);
                if (ret < 0) { // failed.

                        nob_log(NOB_ERROR, "Failed to check deps of `%s'"
                                           " in module `%s'",
                                           mod->files.c.srcs.items[i],
                                           mod->name);

                        nob_log(NOB_ERROR, "Deps: %s",
                                           display_strings(*(Strings*)&deps));

                        mod->state = mod_state_Failed;
                        nob_return_defer(false); 
                }

                if (ret == 0 && !build_conf.rebuild) { // no need to rebuild.
                        nob_log(NOB_INFO, "No need to rebuild module `%s'",
                                          mod->name);
                        nob_return_defer(true);  
                }

                nob_cmd_append(cmd, build_conf.c_compiler, "-o", obj,
                               "-c", mod->files.c.srcs.items[i]);

                nob_da_append_many(cmd, build_conf.c_warnings_flags.items,
                                        build_conf.c_warnings_flags.count);

                if (build_conf.debug)
                nob_da_append_many(cmd, build_conf.c_debug_flags.items,
                                        build_conf.c_debug_flags.count);


                nob_da_append(&procs, nob_cmd_run_async_and_reset(cmd));

                if (deps.items != NULL) nob_da_free(deps);
                deps = (Nob_File_Paths) { 0 };
        }

        if (!nob_procs_wait_and_reset(&procs)) {
                nob_log(NOB_ERROR, "Failed to build objects of module `%s'",
                                   mod->name);
                mod->state = mod_state_Failed;

                nob_return_defer(false);
        }

        mod->state = mod_state_Builded;
defer:
        if (deps.items  != NULL) nob_da_free(deps);
        if (procs.items != NULL) nob_da_free(procs);

        return result;
}

bool
build_module(struct conf build_conf, Modules modules, Module *mod,
             Nob_Cmd *cmd)
{
        mod->build_dir = nob_temp_sprintf("%s/%s", build_conf.build_dir,
                                                  mod->name);

        switch (mod->lang) {
        case mod_lang_C:
                return build_c_module(build_conf, modules, mod, cmd);
        case mod_lang_C3:
                nob_log(NOB_ERROR, "C3 language isnot implemented yet");
                return false;
        case mod_lang_Custom:
		if (!nob_cmd_run_sync(mod->files.custom.build_cmd)) {
			nob_log(NOB_ERROR, "Failed to run build cmd for mod %s",
					   mod->name);
			mod->state = mod_state_Failed;
			return false; 
		}
                return true;
        }
}
bool
link_module(struct conf build_conf, Modules modules, size_t idx,
            Nob_Cmd *cmd, Modules_Box inherit_modules);


bool
link_deps(struct conf build_conf, Modules modules, Module *mod, Nob_Cmd *cmd,
          Modules_Box inherit_modules)
{
        Module *dep;

        size_t dep_idx;        

        for (size_t i = 0; i < mod->deps.count; ++i) {
                if (!get_module_idx(modules, mod->deps.items[i], &dep_idx)) {
                        nob_log(NOB_ERROR, "Invalid dependency name `%s' "
                                            "of module `%s'!!!!! D:",
                                            mod->deps.items[i], mod->name);
                        return false;
                }

                dep = modules.items + dep_idx;


                if (dep->type == mod_type_Exe) {
                        nob_log(NOB_ERROR, "Executable cannot be dependency!! "
                                           "The mod is `%s', the dep is `%s",
                                           mod->name, dep->name);
                        return false;
                }

                if (dep->state != mod_state_Linked) {
                        
                        bool ret = link_module(build_conf, modules, dep_idx,
                                               cmd, inherit_modules);

                        if (!ret || dep->state != mod_state_Linked) {
                                nob_log(NOB_ERROR,
                                        "Failed to link depenency!!!! D:",
                                        "The mod is `%s', the dep is `%s'",
                                           mod->name, dep->state);
                                return false;
                        }
                }
        }

        return true;
}


void
add_deps_c_module(struct conf build_conf, Modules modules, Module *mod,
                  Nob_Cmd *cmd)
{
        Module *dep;

        size_t dep_idx;        

        for (size_t i = 0; i < mod->deps.count; ++i) {
                get_module_idx(modules, mod->deps.items[i], &dep_idx);
                dep = modules.items + dep_idx;

                if (dep->type == mod_type_Lib) {
                        add_deps_c_module(build_conf, modules, dep, cmd);
                        
                }

                nob_cmd_append(cmd, nob_temp_sprintf("-L%s", dep->build_dir));
                nob_cmd_append(cmd, nob_temp_sprintf("-l:lib%s.a", dep->name));
        }
}

bool
link_c_module(struct conf build_conf, Modules modules, Module *mod,
              Nob_Cmd *cmd)
{

        switch (mod->type) {
        case mod_type_Exe: {

                nob_cmd_append(cmd, build_conf.c_compiler);

                nob_da_append_many(cmd, build_conf.c_warnings_flags.items,
                                        build_conf.c_warnings_flags.count);

                if (build_conf.debug)
                nob_da_append_many(cmd, build_conf.c_debug_flags.items,
                                        build_conf.c_debug_flags.count);

                add_deps_c_module(build_conf, modules, mod, cmd);

                nob_da_append_many(cmd, mod->objs.items, mod->objs.count);

		char *output;
		output = nob_temp_sprintf("%s/%s", mod->build_dir, mod->name);
                nob_cmd_append(cmd, "-o", output);

        } break;
        case mod_type_Lib: {
                nob_cmd_append(cmd, build_conf.archive);
                nob_da_append_many(cmd, build_conf.archive_flags.items,
                                        build_conf.archive_flags.count);

                nob_cmd_append(cmd, nob_temp_sprintf("%s/lib%s.a",
                                                mod->build_dir, mod->name));

                nob_da_append_many(cmd, mod->objs.items, mod->objs.count);
        } break;
        }

        if (!nob_cmd_run_sync_and_reset(cmd)) {
                nob_log(NOB_ERROR, "Failed to link module `%s'!!!! D:",
                                   mod->name);
        }


        return true;
}


bool
link_module(struct conf build_conf, Modules modules, size_t idx,
            Nob_Cmd *cmd, Modules_Box inherit_modules)
{                
	Module *mod = modules.items + idx;

        if (get_module_idx(*(Modules*)&inherit_modules, mod->name, NULL)) {
                nob_log(NOB_ERROR, "Looks like dependencies is looped!!!! D:");
                return false;
        }

        if (modules.count <= idx) {
                nob_log(NOB_ERROR, "Module index out of range!!");
                return false;
        }
        box_push(&inherit_modules, *mod); // set this modules as inherited for
                                          // its dependecies.

        if (!link_deps(build_conf, modules, mod, cmd, inherit_modules)) {
                // error logging already done :)
                return false;
        }

        switch (modules.items[idx].lang) {
        case mod_lang_C:
                return link_c_module(build_conf, modules, mod, cmd);
        case mod_lang_C3:
                nob_log(NOB_ERROR, "C3 language isnot implemented yet");
                return false;
        case mod_lang_Custom:	
		nob_cmd_append(cmd, "cp", mod->files.custom.res,
					  mod->build_dir);
                return true;
        }
}

bool
link_module_by_name(struct conf build_conf, Modules modules,
		    const char *module_name, Nob_Cmd *cmd,
		    Modules_Box inherit_modules)
{
        size_t idx;
        if (!get_module_idx(modules, module_name, &idx)) return false;


        return link_module(build_conf, modules, idx, cmd, inherit_modules);
}


#endif
