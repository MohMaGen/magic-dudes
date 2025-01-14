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
"        run        -- build and run executable targets.\n"
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
		return -1;
	}

	// game modules
	for (size_t i = 0; i < NOB_ARRAY_LEN(module_names); ++i) {
		const char *name = module_names[i];

		nob_log(NOB_INFO, "load module `%s'", name);
		if (!load_module_dir(build_config, name, &mod, &cmd)) {
			return -1;
		}

		nob_da_append(modules, mod);

	}
}


int
build_cmd(char *target, Flags flags)
{
	int result = 0;

        struct conf build_config = { 0 };
	if (!get_config(&build_config, flags)) return -1;

	Modules modules = { 0 };

	init_modules(build_config, &modules);

	nob_log(NOB_INFO, "Build modules");

	Module *mod = NULL;
	Nob_Cmd cmd = { 0 };

	for (size_t i = 0; i < modules.count; ++i) {
		mod = modules.items + i;
		cmd = (Nob_Cmd) { 0 };

		if (mod->state != mod_state_Inited) continue;

		nob_log(NOB_INFO, "Build module `%s'", mod->name);

		if (!build_module(build_config, modules, mod, &cmd)) {
			nob_log(NOB_ERROR, "Failed to build module `%s'",
					   mod->name);
			nob_return_defer(-1);
		}

	}
defer:
	if (modules.items != NULL) nob_da_free(modules);
	if (cmd.items != NULL)     nob_da_free(cmd);

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
		char *target = NULL;
		if (argc >= 3) target = argv[2];
		
		return build_cmd(target, flags);
	}


        return 0;
}
