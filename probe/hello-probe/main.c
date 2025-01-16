#include <raylib/raylib.h>
#include <core/core.h>

enum pack_e {
	UPDATE1 = 0,	
	UPDATE2,	
	UPDATE3,	
	UPDATE4,	
	LOG,
	PACKS_LEN
};

typedef struct  {
	mutex_t mutex;
	int x, y, w, h;
	int value;
} state_t;

bool
update_hero(core_t *core, state_t *state)
{
	if (IsKeyDown(KEY_D)) state->x++;
	if (IsKeyDown(KEY_A)) state->x--;
	if (IsKeyDown(KEY_W)) state->y--;
	if (IsKeyDown(KEY_S)) state->y++;

	return true;
}

bool
update_quit(core_t *core, state_t *state)
{
	if (IsKeyPressed(KEY_Q)) mutex_lock_while((mutex_c*)core, 
		core->should_close = true;
	);

	return true;
}

bool
update_btn(core_t *core, state_t *state)
{
	if (IsKeyDown(KEY_B)) mutex_lock_while((mutex_c*)state,
		state->value++;
	);

	return true;
}


bool
log_system(core_t *core, state_t *state)
{
	log(LOG_TXT, "rect: { .x=%d, .y=%d, .w=%d, .h=%d }",
		     state->x, state->y, state->w, state->h);

	log(LOG_TXT, "btn: %d", state->value);

	return true;
}


bool
draw(state_t *state)
{
	DrawRectangle(state->x, state->y, state->w, state->h,
		      GetColor(0xbb4020ff));

	return true;
}


int
main(void)
{
	packs_t packs = { 0 };
	packs.dat = (pack_t[PACKS_LEN]) { 0 };
	packs.len = PACKS_LEN;
	packs.cap = PACKS_LEN;

	core_t core = core_init("!Hello, Aboba?");
	
	system_f sys;
	sys = (system_f)update_hero;
	da_push(&packs.dat[UPDATE1].systems, &sys);
	sys = (system_f)update_quit;
	da_push(&packs.dat[UPDATE1].systems, &sys);
	sys = (system_f)update_btn;
	da_push(&packs.dat[UPDATE1].systems, &sys);

	sys = (system_f)update_btn;
	da_push(&packs.dat[UPDATE2].systems, &sys);

	sys = (system_f)update_btn;
	da_push(&packs.dat[UPDATE3].systems, &sys);

	sys = (system_f)update_btn;
	da_push(&packs.dat[UPDATE4].systems, &sys);

	sys = (system_f)log_system;
	da_push(&packs.dat[LOG].systems,    &sys);

	state_t state = { .x = 0x30, .y = 0x30, .w = 0x20, .h = 0x20 };

	while (!core_should_close(core)) {

		log(LOG_INF, "Run packs!");
		packs_run(packs, &core, &state);

		packs_join(packs);
		log(LOG_INF, "Packs joined!");

		BeginDrawing();
		ClearBackground(GetColor(0x202020ff));

		draw(&state);

		EndDrawing();
	}



	core_close(&core);	
}
