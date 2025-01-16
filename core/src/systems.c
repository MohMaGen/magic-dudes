#include <core/core.h>
#include <stdlib.h>


typedef struct {
	systems_t systems;
	core_t   *core;
	void     *data;
} pack_data_t;


void *
pack_function(pack_data_t *pack_data)
{
	for (size_t i = 0; i < pack_data->systems.len; ++i) {
		system_f system = pack_data->systems.dat[i];

		if (!system(pack_data->core, pack_data->data))
			return (void*)"Failed to run system";
	}

	free(pack_data);

	return NULL;
}


bool
pack_run(pack_t *pack, core_t *core, void *data)
{
	pack_data_t *pack_data = malloc(sizeof(pack_data_t));
	*pack_data = (pack_data_t) {
		.systems = pack->systems,
		.core    = core,
		.data    = data,
	};

	return thread_create(&pack->thread, (thread_f)pack_function,
					    pack_data);
}

bool
pack_join(pack_t pack)
{
	char *res;

	if (!thread_join(pack.thread, (void*)&res)) return false;

	if (res != NULL) {
		log(LOG_ERR, "Failed to run pack: `%s'", res);
		return false;
	}

	return true;
}

bool
packs_run(packs_t packs, core_t *core, void *data)
{
	for (size_t i = 0; i < packs.len; ++i) {
		if (!pack_run(packs.dat + i, core, data)) {
			log(LOG_ERR, "Failed to run pack!");
		}
	}

	return true;
}

bool
packs_join(packs_t packs)
{
	for (size_t i = 0; i < packs.len; ++i) {
		if (!pack_join(packs.dat[i])) {
			log(LOG_ERR, "Failed to join pack!");
		}
	}

	return true;
}
