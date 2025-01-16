#define  NOB_IMPLEMENTATION
#include "./deps/nob/nob.h"

#include "./nob-common/common.h"
#include "./nob-common/module.h"
#include "./nob-common/flags.h"
#include "./nob-common/conf.h"

const char HELP_MSG[] =
"USAGE: nob command [command-options]\n"
"\n"
"COMMANDS:\n"
"        build      -- build targets.\n"
"        exec       -- build and run executable targets.\n"
"        config     -- show current configuration.\n"
"FLAGS:\n"
"        --debug    -- build/run with debug compiler flags.\n"
"        --rebuild  -- force to rebuild every object.\n"
"        --help     -- print this message.\n"
"        -c         -- custom path to config file. (defualt is noh.conf)\n";


/*
	To add module loaded by directory, just add there its name.
 */
const char module_names[][0x10] = {
	"core",
	"raylib",
};


bool
get_config(struct conf *build_config, Flags flags)
{
	if (!read_conf("nob-common/default-build.conf", build_config)) {
		nob_log(NOB_ERROR, "Failed to default config");
		return false;
	}

	char *config = "./nob.conf";
	if (flags.config != NULL) config = flags.config;

	if (access(config,  F_OK) != 0) {
		nob_log(NOB_INFO, "No config at `%s', use default.", config);	
	} else if (!read_conf(config, build_config)) {
		nob_log(NOB_ERROR, "Failed to read build config");
		return false;
	}

	if (flags.debug)   build_config->debug   = true;
	if (flags.rebuild) build_config->rebuild = true;

	return true;	
}


int
config_cmd(Flags flags)
{
        struct conf build_config = { 0 };
	if (!get_config(&build_config, flags)) {
		return -1;
	}

	log_conf(build_config);

	return 0;
}




bool
init_modules(struct conf build_config, Modules *modules)
{
	Module  mod = { 0 };
	Nob_Cmd cmd = { 0 };

	if (!nob_mkdir_if_not_exists(build_config.build_dir)) {
		nob_log(NOB_ERROR, "failed to creat build dir: %s",
				   build_config.build_dir);
		return false;
	}

	// game modules
	for (size_t i = 0; i < NOB_ARRAY_LEN(module_names); ++i) {
		const char *name = module_names[i];

		nob_log(NOB_INFO, "load module `%s'", name);
		if (!load_module_dir(build_config, name, &mod, &cmd)) {
			return false;
		}

		nob_da_append(modules, mod);
	}


	return true;
}

bool
build_modules(struct conf build_conf, Modules modules)
{
	bool result = true;

	nob_log(NOB_INFO, "Build modules");

	Module *mod = NULL;
	Nob_Cmd cmd = { 0 };

	for (size_t i = 0; i < modules.count; ++i) {
		mod = modules.items + i;
		cmd = (Nob_Cmd) { 0 };

		if (mod->state != mod_state_Inited) continue;

		nob_log(NOB_INFO, "Build module `%s'", mod->name);

		if (!build_module(build_conf, modules, mod, &cmd)) {
			nob_log(NOB_ERROR, "Failed to build module `%s'",
					   mod->name);
			nob_return_defer(false);
		}
	}

defer:
	if (cmd.items != NULL)     nob_da_free(cmd);
	return result;
}

bool
link_modules(struct conf build_conf, Modules modules)
{
	bool result = true;

	Nob_Cmd cmd = { 0 };
	Module *mod = NULL;
	Modules_Box inhmods = {
		.items    = malloc(sizeof(Module) * modules.count),
		.len      = 0,
		.max_size = modules.count,
	};

	nob_log(NOB_INFO, "Linking modules");

	for (size_t i = 0; i < modules.count; ++i) {
		mod = modules.items + i;

		if (mod->state != mod_state_Builded) continue;
		nob_log(NOB_INFO, "Linking module `%s'", mod->name);

		if (!link_module(build_conf, modules, i, &cmd, inhmods)) {
			nob_log(NOB_ERROR, "Failed to link module `%s'",
					   mod->name);
			nob_return_defer(false);
		}
	}

defer:
	if (inhmods.items != NULL) free(inhmods.items);
	if (cmd.items     != NULL) nob_da_free(cmd);

	return result;
}


bool
build_probes(char *target, struct conf build_conf, Modules *modules)
{
	bool result = true;
	nob_log(NOB_INFO, "Build probes");

	Nob_File_Paths probes = { 0 };

	nob_read_entire_dir("probe", &probes);

	Nob_Cmd cmd = { 0 };
	Module *mod = NULL;

	Modules_Box inhmods;

	for (size_t i = 0; i < probes.count; ++i) {
		if (strcmp(target, "all") != 0 &&
		    strcmp(target, probes.items[i])) continue;

		char *path = nob_temp_sprintf("probe/%s", probes.items[i]);
		if (strendswith(path, "/.") || strendswith(path, "/..")) 
			continue;

		if (nob_get_file_type(path) != NOB_FILE_DIRECTORY)
			continue;

		nob_log(NOB_INFO, "Init probe module at `%s'", path);

		Module md = { 0 };
		if (!load_module_dir(build_conf, path, &md, &cmd)) {
			nob_log(NOB_ERROR, "Failed to init probe module.");
			nob_return_defer(false);
		}
		nob_da_append(modules, md);
		size_t idx = modules->count - 1;
		mod = modules->items + modules->count - 1;

		nob_log(NOB_INFO, "Build probe module `%s'", mod->name);

		if (!build_module(build_conf, *modules, mod, &cmd)) {
			nob_log(NOB_ERROR, "Failed to build module `%s'",
					   mod->name);
			nob_return_defer(false);
		}

		nob_log(NOB_INFO, "Link probe module `%s'", mod->name);
		inhmods = (Modules_Box) {
			.items    = malloc(sizeof(Module) * modules->count),
			.len      = 0,
			.max_size = modules->count,
		};

		if (!link_module(build_conf, *modules, idx, &cmd, inhmods)) {
			nob_log(NOB_ERROR, "Failed to link module `%s'",
					   mod->name);
			free(inhmods.items);
			nob_return_defer(false);
		}
		free(inhmods.items);
			
	}
defer:
	if (cmd.items != NULL)     nob_da_free(cmd);
	return result;
}

int
build_cmd(char *target, Flags flags, struct conf *build_config)
{
	int result = 0;

	if (!get_config(build_config, flags)) return -1;

	Modules modules = { 0 };

	if (!init_modules(*build_config, &modules)) {
		nob_log(NOB_ERROR, "Failed to init modules!");
		nob_return_defer(-1);
	}

	if (!build_modules(*build_config, modules)) {
		nob_log(NOB_ERROR, "Failed to build modules!");
		nob_return_defer(-1);
	}

	if (!link_modules(*build_config, modules)) {
		nob_log(NOB_ERROR, "Failed to build modules!");
		nob_return_defer(-1);
	}

	if (!build_probes(target, *build_config, &modules)) {
		nob_log(NOB_ERROR, "Failed to build modules!");
		nob_return_defer(-1);
	}

defer:
	if (modules.items != NULL) nob_da_free(modules);

	return 0;
}

#define cmd(name) if (strcmp(command, name) == 0)

int
main(int argc, char **argv)
{
        NOB_GO_REBUILD_URSELF(argc, argv);

	Flags flags = { 0 };
	if (!read_flags(argc, argv, &flags)) {
		nob_log(NOB_ERROR, "Failed to read flags");
		puts(HELP_MSG);
		return -1;
	}

	if (argc < 2) {
		nob_log(NOB_ERROR, "Expect command. The usage of nob:");
		puts(HELP_MSG);
		return 1;
	}


	char *command = argv[1];

	if (flags.help) {
		puts(HELP_MSG);
		return 0;
	}

	cmd("config") {
		return config_cmd(flags);
	}

	cmd("build") {
		char *target = "all";
		if (argc >= 3) target = argv[2];

		struct conf build_config = { 0 };
		return build_cmd(target, flags, &build_config);
	}

	cmd("exec") {
		int ret;

		if (argc < 3) {
			nob_log(NOB_ERROR, "Expect target to run!");
			return -1;
		}

		
		char *target = argv[2];
		
		struct conf build_config = { 0 };
		ret = build_cmd(target, flags, &build_config);

		if (ret != 0) return ret;

		Nob_Cmd cmd = { 0 };
		nob_cmd_append(&cmd, nob_temp_sprintf("%s/%s/%s",
							build_config.build_dir,
							target, target));

		if (!nob_cmd_run_sync_and_reset(&cmd)) {
			nob_log(NOB_ERROR, "Failed to run `%s'", target);
			return -1;
		}
	}


        return 0;
}
