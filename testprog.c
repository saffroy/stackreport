#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <alloca.h>
#include <sys/prctl.h>
#include <assert.h>

struct thread_arg {
        int num;
        int stack_usage;
};

static void consume_stack(int usage) {
        char *p = alloca(usage);
        memset(p, 0xff, usage);
}

static void *thread(void *p) {
        struct thread_arg *arg = p;
        char name[16+1];

        snprintf(name, sizeof(name), "thread-%d", arg->num);
        prctl(PR_SET_NAME, name);

        consume_stack(arg->stack_usage);

        free(p);
        return NULL;
}

int main(int argc, char **argv) {
        assert(argc > 1);

        int main_stack_usage = atoi(argv[1]);

        pthread_t pt[argc-1];
        int i;
        for (i = 2; i < argc; i++) {
                struct thread_arg *arg = malloc(sizeof(*arg));

                arg->num = i-1;
                arg->stack_usage = atoi(argv[i]);
                assert(arg->stack_usage > 0);

                pthread_create(&pt[i-1], NULL, thread, arg);
        }

        consume_stack(main_stack_usage);

        for (i = 2; i < argc; i++) {
                pthread_join(pt[i-1], NULL);
        }

        return 0;
}
