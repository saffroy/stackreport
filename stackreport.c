#include <dlfcn.h>

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include <sys/types.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <assert.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <pthread.h>
#include <ucontext.h>
#include <string.h>
#include <syscall.h>
#include <sys/prctl.h>

#define err_noabort(fmt,arg...)                                         \
	do {								\
		fprintf(stderr, "error(%s:%d:%s): " fmt "\n",		\
			__FILE__, __LINE__, __FUNCTION__, ##arg);	\
	} while(0)

#define err(fmt,arg...)                         \
	do {					\
		err_noabort(fmt, ##arg);	\
		abort();			\
	} while(0)

#if 1
#define debug(fmt,arg...) /* nothing */
#else
#define debug(fmt,arg...)                                               \
	do {								\
		fprintf(stderr, "debug(%s:%d:%s): " fmt "\n",		\
			__FILE__, __LINE__, __FUNCTION__, ##arg);	\
	} while(0)
#endif

#define ENV_NAME "SR_OUTPUT_FILE"
#define DUMMY ((void*)0xdeadbeef)

static int (*old_pthread_create)(pthread_t *thread, const pthread_attr_t *attr,
                                 void *(*start_routine) (void *), void *arg);

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static unsigned long page_size;

static void *
aligned(const void *p)
{
	return (void*)((unsigned long)p & ~(page_size - 1));
}

static int
incore(const char *p)
{
	unsigned char v;
	int ret;

	ret = mincore(aligned(p), 1, &v);
	if (ret < 0 && errno == ENOMEM)
		/* unmapped memory */
		return 0;
	assert(ret == 0);
	return v & 1;
}

static int
cmp(const void *key, const void *p)
{
	assert(key == DUMMY);
	int a = incore(p);
	int b = incore(p + page_size);
	debug("p %p a %d b %d \n", p, a, b);
	if (a != b)
		return 0;
	else if (a)
		return -1;
	else
		return 1;
}

static pid_t 
gettid(void)
{
        return syscall(SYS_gettid);
}

static void
do_report(FILE *out)
{
        /* basic thread info */

        char name[16+1] = {};
        prctl(PR_GET_NAME, name);

        fprintf(out, "tid %lu \"%s\"\n", (long)gettid(), name);

        /* stack info */

	pthread_attr_t attr;
	int ret;
	size_t stack_size, guard_size;
	void *stack_addr;

	ret = pthread_getattr_np(pthread_self(), &attr);
	assert(ret == 0);
	ret = pthread_attr_getstack(&attr, &stack_addr, &stack_size);
	assert(ret == 0);
	ret = pthread_attr_getguardsize(&attr, &guard_size);
	assert(ret == 0);
	fprintf(out, "Guard size          = %lu bytes\n", guard_size);
	fprintf(out, "Stack address       = %p\n", stack_addr);
	fprintf(out, "Stack size          = %lu\n", stack_size);
	assert(stack_addr && stack_size);

        /* probe for used stack pages */

	assert(!incore(stack_addr));
	assert(incore(stack_addr + stack_size - page_size));

	void *last_used = bsearch(DUMMY,
				  (void*)stack_addr,
				  stack_size/page_size - 1,
				  page_size, cmp);
	assert(last_used);
	unsigned long used = stack_addr + stack_size - last_used;
	fprintf(out, "base = %p lim = %p used = %lu\n",
	       stack_addr, stack_addr + stack_size, used);

        fputs("\n", out);
}

static void
thread_reporter(void *unused)
{
        char *outfile = getenv(ENV_NAME);
        FILE *out;

        pthread_mutex_lock(&mutex);

        if (!outfile) {
                do_report(stderr);
        } else {
                out = fopen(outfile, "a");
                if (out) {
                        do_report(out);
                        fclose(out);
                }
        }

        pthread_mutex_unlock(&mutex);
}

struct wrapper_arg {
        void *(*start_routine) (void *);
        void *arg;
};

static void *
start_routine_wrapper(void *arg)
{
        struct wrapper_arg *wrapper_arg = arg;
        struct wrapper_arg wa = *wrapper_arg;
        void *rc;

        free(arg);

        pthread_cleanup_push(thread_reporter, NULL);
        rc = wa.start_routine(wa.arg);
        pthread_cleanup_pop(1);

        return rc;
}

int
pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                   void *(*start_routine) (void *), void *arg)
{
        struct wrapper_arg *wrapper_arg;

        wrapper_arg = malloc(sizeof(*wrapper_arg));
        wrapper_arg->start_routine = start_routine;
        wrapper_arg->arg = arg;

        return old_pthread_create(thread, attr, start_routine_wrapper, wrapper_arg);
}

static void * get_sym(const char *symbol) {
	void *sym;
	char *err;

	sym = dlsym(RTLD_NEXT, symbol);
	err = dlerror();
	if(err) 
		err("dlsym(%s) error: %s", symbol, err);
	return sym;
}

void __strackreport_init(void) __attribute__((constructor));
void __strackreport_init(void)
{
	page_size = sysconf(_SC_PAGESIZE);
	old_pthread_create = get_sym("pthread_create");

        char *outfile = getenv(ENV_NAME);

        if (outfile) {
                FILE *out = fopen(outfile, "w");
                if (out)
                        fclose(out);
        }
}
