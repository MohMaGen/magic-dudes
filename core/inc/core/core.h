#ifndef __MAGIC_DUDES_CORE__
#define __MAGIC_DUDES_CORE__

#include <stdbool.h>
#include <stdio.h>

typedef unsigned char byte_t;


/*
 * ============================================================================
 *                                    LOG
 * ============================================================================
 */

typedef enum { LOG_ERR = 0, LOG_WAR, LOG_INF, LOG_TXT } log_level_e;

#ifndef LOG_LEVEL 
	#define LOG_LEVEL LOG_TXT
#endif


#ifndef log_fn

	#ifndef SIMPLE_LOG_FN
		#define SIMPLE_LOG_FN printf
	#endif


	#define log_fn(lvl, fmt, ...) do { switch (lvl) {                     \
		case LOG_ERR:                                                 \
			SIMPLE_LOG_FN("\x1B[31m[ERR]: " fmt "\x1B[0m\n",      \
				      ## __VA_ARGS__);                        \
			break;                                                \
		case LOG_WAR:                                                 \
			SIMPLE_LOG_FN("\x1B[33m[WAR]: " fmt "\x1B[0m\n",      \
				      ## __VA_ARGS__);                        \
			break;                                                \
		case LOG_INF:                                                 \
			SIMPLE_LOG_FN("\x1B[36m[INF]: " fmt "\x1B[0m\n",      \
				      ## __VA_ARGS__);                        \
			break;                                                \
		case LOG_TXT:                                                 \
			SIMPLE_LOG_FN("\x1B[37m[TXT]: " fmt "\x1B[0m\n",      \
				      ## __VA_ARGS__);                        \
			break;                                                \
	} } while (0)                                                         \

#endif

#define log(lvl, fmt, ...) do {                                               \
		if (lvl <= LOG_LEVEL) log_fn(lvl, fmt, ##__VA_ARGS__);        \
	} while (0)                                                           \



/*
 * ============================================================================
 *                                DYNAMIC ARRAYS
 * ============================================================================
 */

typedef struct { void *dat; size_t len, cap; } da_t;

void __da_push(da_t *self, void *elem, size_t elem_size);
#define da_push(da, elem) __da_push((void*)da, elem, sizeof(*elem))

void da_free(da_t *self);

void da_reset(da_t *self);

void __da_append(da_t *self, void **elems, size_t len, size_t elem_size);
#define da_append(da, elems, len) __da_append(da, elems, len, sizeof(**elems))


/*
 * ============================================================================
 *	                            THREADS
 * ============================================================================
 */
#ifdef WIN32
#error "Threads on windows not implemented yet! Maybe try to use not toy OS"
#else
#include <pthread.h>
typedef pthread_t       thread_t;
typedef pthread_mutex_t mutex_t;
#endif

typedef  void* (*thread_f)(void*);

typedef struct { mutex_t mutex; char payload[]; } mutex_c;

bool thread_create(thread_t *thread, thread_f fn, void *data);
bool thread_join(thread_t thread, void **result);

/*
 *	Wrapper for `pthread_mutex_lock' for mutex_c like objects.
 */
bool mutex_lock(mutex_c *);

/*
 *	Wrapper for `pthread_mutex_trylock' for mutex_c like objects.
 */
bool mutex_trylock(mutex_c *);

/*
 *	Wrapper for `pthread_mutex_unlock' for mutex_c like objects.
 */
bool mutex_unlock(mutex_c *);


/*
 *
 *	Block mutex_c like object while running smths with this object.
 *
 *	This macro will wait before will able to lock.
 *	acts like `pthread_mutex_lock'.
 *
 *	sruct { pthread_mutex_t mutex; int value; } var;
 *
 *	USAGE:
 *	core_lock_while(var, { printf("%d\n", var.value); ++value; });
 */
#define mutex_lock_while(m, todo) if (mutex_lock((mutex_c*)m)) do {           \
		do todo while (0); mutex_unlock((mutex_c*)m);                 \
	} while (0)                                                           \

/*
 *	Same as `mutex_lock_while', but acts like `pthread_mutex_trylock'.
 */
#define mutex_trylock_while(m, todo) if (mutex_trylock((mutex_c*)m)) do {     \
		do todo while(0); mutex_unlock((mutex_c*)m);                  \
	} while (0)                                                           \


/*
 * ============================================================================	
 *                                RAYLIB STUF
 *                                    AND
 *                               SOME CORE STUF
 * ============================================================================
 */

typedef struct {
	mutex_t mutex;
	const char *name;	
	bool should_close;
} core_t;

/*
	Init raylib.
 */
core_t core_init(const char *name);

/*
	Check if game should be closed.
 */
bool core_should_close(core_t core);

/*
	Close raylib.
 */
void core_close(core_t *core);


/*
 * ============================================================================
 *                               SYSTEMS AND PACKS
 * ============================================================================
 */


typedef bool (*system_f)(core_t *, void *);

typedef struct { system_f *dat; size_t len, cap; } systems_t;

typedef struct { systems_t systems; thread_t  thread; } pack_t;

typedef struct { pack_t *dat; size_t len, cap; } packs_t;

bool packs_run(packs_t packs, core_t *core, void *data);
bool packs_join(packs_t packs);

bool pack_run(pack_t *pack, core_t *core, void *data);

bool pack_join(pack_t pack);


#endif
