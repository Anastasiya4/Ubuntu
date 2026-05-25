

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <execinfo.h>      /* backtrace() / backtrace_symbols_fd() */
#include <fcntl.h>
#include <time.h>
#include <sys/types.h>
#include <ucontext.h>

#define LOG_FILE   "/tmp/sigsegv_crash.log"
#define MAX_FRAMES 32
#define MAX_RESTART 3      /* максимум автоматичних перезапусків */

/* Argv програми зберігаємо глобально для передачі в execv() */
static char **g_argv = NULL;

/* Лічильник перезапусків передається через змінну середовища */
#define RESTART_ENV "CRASH_RESTART_COUNT"

/* Асинхронно-безпечний запис рядка у fd (printf НЕ можна використовувати в обробнику сигналів) */
static void safe_write(int fd, const char *msg) {
    write(fd, msg, strlen(msg));
}

/* Перетворення числа в рядок (async-signal-safe) */
static void itoa_hex(unsigned long val, char *buf, int len) {
    const char hex[] = "0123456789abcdef";
    int i = len - 1;
    buf[i--] = '\0';
    if (val == 0) { buf[i] = '0'; return; }
    while (val && i >= 0) {
        buf[i--] = hex[val & 0xF];
        val >>= 4;
    }
}

static void itoa_dec(long val, char *buf, int len) {
    int i = len - 1;
    buf[i--] = '\0';
    int neg = (val < 0);
    if (neg) val = -val;
    if (val == 0) { buf[i] = '0'; return; }
    while (val && i >= 0) {
        buf[i--] = '0' + (val % 10);
        val /= 10;
    }
    if (neg && i >= 0) buf[i] = '-';
}

/* Обробник SIGSEGV */
static void sigsegv_handler(int sig, siginfo_t *info, void *uctx) {
    char buf[32];

    /* Відкриваємо лог-файл (O_APPEND — якщо перезапуск, дописуємо) */
    int fd = open(LOG_FILE, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd < 0) fd = STDERR_FILENO;

    /* Заголовок */
    safe_write(fd, "\n========== CRASH REPORT ==========\n");

    /* Час (не async-safe, але прийнятно для логування) */
    time_t t = time(NULL);
    char *ts = ctime(&t);
    if (ts) { safe_write(fd, "Time     : "); safe_write(fd, ts); }

    /* PID */
    safe_write(fd, "PID      : ");
    itoa_dec(getpid(), buf, sizeof(buf));
    safe_write(fd, buf + strlen(buf) - 6); /* тільки цифри */
    safe_write(fd, "\n");

    /* Сигнал */
    safe_write(fd, "Signal   : ");
    itoa_dec(sig, buf, sizeof(buf));
    safe_write(fd, buf);
    safe_write(fd, " (SIGSEGV)\n");

    /* Fault address */
    safe_write(fd, "Fault @  : 0x");
    itoa_hex((unsigned long)info->si_addr, buf, sizeof(buf));
    safe_write(fd, buf);
    safe_write(fd, "\n");

    /* si_code */
    safe_write(fd, "si_code  : ");
    itoa_dec(info->si_code, buf, sizeof(buf));
    safe_write(fd, buf);
    safe_write(fd, (info->si_code == SEGV_MAPERR)  ? " (SEGV_MAPERR)\n"  :
                   (info->si_code == SEGV_ACCERR)  ? " (SEGV_ACCERR)\n"  :
                                                      "\n");

    /* Адреса регістра RIP/EIP з ucontext */
#if defined(__x86_64__)
    ucontext_t *uc = (ucontext_t*)uctx;
    safe_write(fd, "RIP      : 0x");
    itoa_hex((unsigned long)uc->uc_mcontext.gregs[REG_RIP], buf, sizeof(buf));
    safe_write(fd, buf);
    safe_write(fd, "\n");
#elif defined(__i386__)
    ucontext_t *uc = (ucontext_t*)uctx;
    safe_write(fd, "EIP      : 0x");
    itoa_hex((unsigned long)uc->uc_mcontext.gregs[REG_EIP], buf, sizeof(buf));
    safe_write(fd, buf);
    safe_write(fd, "\n");
#else
    (void)uctx;
    safe_write(fd, "RIP      : (архітектура не підтримується)\n");
#endif

    /* --- Backtrace --- */
    safe_write(fd, "--- Stack trace ---\n");
    void *frames[MAX_FRAMES];
    int  nframes = backtrace(frames, MAX_FRAMES);
    backtrace_symbols_fd(frames, nframes, fd);
    safe_write(fd, "===================================\n");

    if (fd != STDERR_FILENO) close(fd);

    /* --- Перезапуск --- */
    const char *cnt_str = getenv(RESTART_ENV);
    int cnt = cnt_str ? atoi(cnt_str) : 0;

    if (cnt < MAX_RESTART && g_argv) {
        /* Записуємо новий лічильник у змінну середовища */
        char new_cnt[16];
        snprintf(new_cnt, sizeof(new_cnt), "%d", cnt + 1);
        setenv(RESTART_ENV, new_cnt, 1);

        /* Виводимо повідомлення в stderr (async-safe через write) */
        safe_write(STDERR_FILENO, "[crash] Перезапуск #");
        itoa_dec(cnt + 1, buf, sizeof(buf));
        safe_write(STDERR_FILENO, buf);
        safe_write(STDERR_FILENO, "\n");

        /* execv() замінює поточний процес новим запуском програми */
        execv(g_argv[0], g_argv);
        /* якщо execv не спрацював — завершуємось */
    }

    safe_write(STDERR_FILENO, "[crash] Досягнуто MAX_RESTART — завершення.\n");
    _exit(1);
}

/* Встановлення обробника через sigaction */
static void install_handler(void) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = sigsegv_handler;
    sa.sa_flags     = SA_SIGINFO;   /* отримуємо siginfo_t та ucontext */
    sigemptyset(&sa.sa_mask);
    sigaction(SIGSEGV, &sa, NULL);
}

/* main */
int main(int argc, char *argv[]) {
    (void)argc;
    g_argv = argv;

    install_handler();

    const char *cnt_str = getenv(RESTART_ENV);
    int restart_num = cnt_str ? atoi(cnt_str) : 0;

    printf("PID=%d  Запуск #%d  (лог: %s)\n",
           getpid(), restart_num, LOG_FILE);
    printf("Симулюємо SIGSEGV через 1 секунду...\n");
    sleep(1);

    /* Навмисне звернення до нульового вказівника → SIGSEGV */
    volatile int *null_ptr = NULL;
    *null_ptr = 42;   /* <-- тут виникне SIGSEGV */

    /* Сюди не дійдемо */
    printf("Цей рядок не повинен з'явитись.\n");
    return 0;
}
