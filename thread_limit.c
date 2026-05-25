

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/resource.h>

#define MAX_THREADS 4096

/* Кожен поток просто спить — тримає ресурс відкритим */
static void *thread_func(void *arg) {
    (void)arg;
    sleep(60); /* блокуємось, щоб поток "жив" під час тесту */
    return NULL;
}

/* Повертає поточний м'який ліміт ulimit -u (RLIMIT_NPROC) */
static rlim_t get_nproc_limit(void) {
    struct rlimit rl;
    if (getrlimit(RLIMIT_NPROC, &rl) == 0)
        return rl.rlim_cur;
    return (rlim_t)-1;
}

int main(void) {
    pthread_t tids[MAX_THREADS];
    pthread_attr_t attr;
    int created = 0;

    /* Друкуємо поточний ліміт */
    rlim_t lim = get_nproc_limit();
    if (lim == (rlim_t)RLIM_INFINITY)
        printf("ulimit -u : необмежено\n");
    else
        printf("ulimit -u : %lu потоків/процесів\n", (unsigned long)lim);

    /* Мінімальний розмір стека для потоку — зменшуємо, щоб
     * вичерпати ліміт швидше, не витрачаючи пам'ять даремно */
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 65536); /* 64 КБ */

    printf("Починаємо створювати потоки...\n");

    for (int i = 0; i < MAX_THREADS; i++) {
        int rc = pthread_create(&tids[created], &attr, thread_func, NULL);

        if (rc != 0) {
            /* EAGAIN — ресурс вичерпано (досягнуто ulimit -u або nproc) */
            if (rc == EAGAIN) {
                printf("\n[!] pthread_create повернула EAGAIN після %d потоків.\n",
                       created);
                printf("    Причина: досягнуто ліміту ulimit -u або RLIMIT_NPROC.\n");
            } else {
                fprintf(stderr, "\n[!] pthread_create помилка: %s (після %d потоків)\n",
                        strerror(rc), created);
            }
            break;
        }

        created++;

        /* Прогрес кожні 50 потоків */
        if (created % 50 == 0)
            printf("  Створено потоків: %d\n", created);
    }

    printf("\n=== Результат ===\n");
    printf("Успішно створено потоків : %d\n", created);
    if (lim != (rlim_t)RLIM_INFINITY)
        printf("Ліміт ulimit -u          : %lu\n", (unsigned long)lim);

    /* Відміняємо всі потоки та чекаємо їх завершення */
    printf("Завершення потоків...\n");
    for (int i = 0; i < created; i++) {
        pthread_cancel(tids[i]);
        pthread_join(tids[i], NULL);
    }

    pthread_attr_destroy(&attr);
    printf("Готово.\n");
    return 0;
}
