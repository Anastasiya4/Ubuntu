
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>

/* Глобальний лічильник тіків
 * volatile sig_atomic_t — гарантує атомарний доступ
 * з обробника сигналу */
static volatile sig_atomic_t tick_count  = 0;
static volatile sig_atomic_t stop_after  = 0;   /* 0 = нескінченно */

/* Обробник сигналу SIGRTMIN
 * Викликається кожен раз, коли таймер спрацьовує.
 * Використовуємо тільки async-signal-safe функції: write(). */
static void timer_handler(int sig, siginfo_t *si, void *uc) {
    (void)sig; (void)si; (void)uc;

    tick_count++;   /* атомарно для sig_atomic_t */

    /* Формуємо рядок вручну (printf не async-signal-safe) */
    char buf[64];
    int  n = 0;
    const char *prefix = "Тік #";
    while (prefix[n]) buf[n++] = prefix[n];   /* unsafe but demo */

    /* Конвертуємо число */
    int val = tick_count;
    char num[16]; int ni = 15;
    num[ni] = '\n'; ni--;
    if (val == 0) { num[ni--] = '0'; }
    else { while (val > 0) { num[ni--] = '0' + val % 10; val /= 10; } }
    ni++;

    /* Збираємо рядок */
    char out[80];
    int oi = 0;
    for (int i = 0; prefix[i]; i++) out[oi++] = prefix[i];
    for (int i = ni; num[i] != '\0'; i++) out[oi++] = num[i];
    out[oi++] = '\n';

    write(STDOUT_FILENO, out, (size_t)oi);
}

/* Створення та запуск POSIX-таймера */
static timer_t create_timer(long interval_sec, long interval_nsec) {
    /* 1. Встановлюємо обробник сигналу */
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = timer_handler;
    sa.sa_flags     = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGRTMIN, &sa, NULL) < 0) {
        perror("sigaction");
        exit(1);
    }

    /* 2. Описуємо подію: сигнал SIGRTMIN */
    struct sigevent sev;
    memset(&sev, 0, sizeof(sev));
    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo  = SIGRTMIN;

    /* 3. Створюємо таймер на базі CLOCK_MONOTONIC
     *    (не залежить від зміни системного часу) */
    timer_t tid;
    if (timer_create(CLOCK_MONOTONIC, &sev, &tid) < 0) {
        perror("timer_create");
        exit(1);
    }

    /* 4. Встановлюємо інтервал */
    struct itimerspec its;
    its.it_value.tv_sec     = interval_sec;   /* перше спрацювання */
    its.it_value.tv_nsec    = interval_nsec;
    its.it_interval.tv_sec  = interval_sec;   /* повтор */
    its.it_interval.tv_nsec = interval_nsec;

    if (timer_settime(tid, 0, &its, NULL) < 0) {
        perror("timer_settime");
        exit(1);
    }

    return tid;
}

/* Вимкнення таймера */
static void disarm_timer(timer_t tid) {
    struct itimerspec its;
    memset(&its, 0, sizeof(its));   /* it_value = {0,0} → зупинити */
    timer_settime(tid, 0, &its, NULL);
    printf("Таймер зупинено.\n");
}

/* Запит поточного стану таймера */
static void query_timer(timer_t tid) {
    struct itimerspec its;
    timer_gettime(tid, &its);
    printf("  Залишилось до наступного тіку: %ld.%09ld с\n",
           its.it_value.tv_sec, its.it_value.tv_nsec);
    printf("  Інтервал повтору:              %ld.%09ld с\n",
           its.it_interval.tv_sec, its.it_interval.tv_nsec);
}

/* main */
int main(int argc, char *argv[]) {
    long interval_sec  = 1;
    long interval_nsec = 0;

    if (argc >= 2) interval_sec  = atol(argv[1]);
    if (argc >= 3) stop_after    = atoi(argv[2]);

    printf("POSIX interval timer — лічильник тіків\n");
    printf("Інтервал  : %ld с\n", interval_sec);
    printf("Зупинка   : %s\n",
           stop_after ? "після N тіків" : "Ctrl+C");
    printf("----------------------------------------\n");

    timer_t tid = create_timer(interval_sec, interval_nsec);

    printf("Стан таймера одразу після запуску:\n");
    query_timer(tid);
    printf("----------------------------------------\n");

    /* Головний цикл: чекаємо сигналів */
    while (1) {
        pause();   /* блокуємось до наступного сигналу */

        /* Перевірка умови зупинки */
        if (stop_after > 0 && tick_count >= stop_after) {
            printf("----------------------------------------\n");
            printf("Досягнуто %d тіків — зупиняємось.\n", (int)tick_count);
            disarm_timer(tid);
            break;
        }
    }

    /* Виводимо підсумок */
    printf("\n=== Підсумок ===\n");
    printf("Всього тіків : %d\n", (int)tick_count);
    printf("Інтервал     : %ld с\n", interval_sec);
    printf("Орієнтовний час роботи: ~%ld с\n",
           (long)tick_count * interval_sec);

    timer_delete(tid);
    return 0;
}
