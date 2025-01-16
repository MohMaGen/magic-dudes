#include <core/core.h>

#ifdef WIN32
#error "Threads on windows not implemented yet! Maybe try to use not toy OS"
#else
#include <pthread.h>

bool
thread_create(thread_t *thread, thread_f fn, void *data)
{
	return pthread_create(thread, NULL, fn, data) == 0;
}

bool
thread_join(thread_t thread, void **result)
{
	return pthread_join(thread, result) == 0;
}
bool
mutex_lock(mutex_c *m) {
	return pthread_mutex_lock(&m->mutex) == 0;
}

bool
mutex_trylock(mutex_c *m) {
	return pthread_mutex_trylock(&m->mutex) == 0;
}

bool
mutex_unlock(mutex_c *m) {
	return pthread_mutex_unlock(&m->mutex) == 0;
}

#endif
