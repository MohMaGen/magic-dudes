#ifndef __MAGIC_DUDES_CORE__
#define __MAGIC_DUDES_CORE__


/*
	Core
 */

/*
DESIGN IDEA
typedef struct {
	struct {
		points_t  hp;
		vector2_t pos;
	} main_hero;
} state_t;

enum { MAIN_MENU, CREATE_CHARACTER, GAME, STATES_COUNT } state_e; 

state_config_t states[STATES_COUNT] = {
	[MAIN_MENU] = (state_config_t) {
		.init  = init_main_menu,
		.close = close_main_menu,
	},
	[CREATE_CHARACTER] = (state_config_t) {
		.init  = init_create_character,	
		.close = close_create_character,
	},
	[GAME] = (state_config_t) {
		.init  = init_game,	
		.close = close_game,
	},
};

int main(void)
{
	core_t core = { 0 };
	core = core_init();

	core_init_state(states, STATES_COUNT);

	core_add_module();

	while (!core_should_close())	
	{	
		core_update_systems({ .cores = 4 });
		core_draw_systems();

	}

	core_close();

	return 0;
}
*/

typedef struct {
} core_t;


#endif

