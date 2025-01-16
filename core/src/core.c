#include <core/core.h>
#include <raylib/raylib.h>
#include <string.h>
#include <stdlib.h>


void
__da_push(da_t *self, void *elem, size_t elem_size)
{
	if (self->len >= self->cap) {
		self->cap = self->cap ? self->cap * 2 : 0x10;
		self->dat = realloc(self->dat, self->cap * elem_size);
	}

	byte_t *offset = (byte_t*)self->dat + (self->len++) * elem_size;
	memcpy(offset, elem, elem_size);
}

void
__da_append(da_t *self, void **elems, size_t len, size_t elem_size)
{
	
	if (self->len + len > self->cap) {
		self->cap = self->cap ? self->cap * 2 : 0x10;
		self->cap += len;
		self->dat = realloc(self->dat, self->cap * elem_size);
	}

	byte_t *offset = (byte_t*)self->dat + (self->len) * elem_size;

	memcpy(offset, elems, elem_size * len);
	self->len += len;
}

void
da_free(da_t *self) {
	free(self);	
}

void
da_reset(da_t *self) {
	self->len = 0;
}

void
custom_log(int msgType, const char *text, va_list args)
{
	char buf[0x100] = { 0 };
	vsprintf(buf, text, args);

	switch (msgType)
	{
	case LOG_INFO:    log(LOG_INF, "%s", buf); break;
	case LOG_ERROR:   log(LOG_ERR, "%s", buf); break;
	case LOG_WARNING: log(LOG_WAR, "%s", buf); break;
	case LOG_DEBUG:   log(LOG_TXT, "%s", buf); break;
	default: break;
	}
}

core_t
core_init(const char *name) 
{
	log(LOG_INF, "Init core with name <%s>", name);

	SetTraceLogCallback(custom_log);

	InitWindow(0x300, 0x300, name);	
	SetWindowState(FLAG_WINDOW_RESIZABLE);
	SetTargetFPS(0x40);

	return (core_t) {
		.name = name,
		.should_close = false,
	};
}

bool
core_should_close(core_t core)
{
	return core.should_close || WindowShouldClose();
}

/*
	Close raylib.
 */
void
core_close(core_t *close)
{
	log(LOG_INF, "Close core");
	CloseWindow();	
}


