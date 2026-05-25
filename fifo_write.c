
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#define FIFO_PATH "/tmp/test_fifo_lab8"
#define BUF_SIZE  4096

/* Обробник SIGPIPE — фіксуємо факт отримання сигналу */
static volatile sig_atomic_t sigpipe_received = 0;

static void sigpipe_handler(int sig) {
    (void)sig;
    sigpipe_received = 1;
}

/* Демонстрація 1: write() у FIFO без читача → SIGPIPE */
static void demo_sigpipe(void) {
    printf("=== Демонстрація 1: SIGPIPE при записі без читача ===\n");

    /* Встановлюємо обробник SIGPIPE замість завершення процесу */
    struct sigaction sa = {0};
    sa.sa_handler = sigpipe_handler;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGPIPE, &sa, NULL);

    /* Створюємо FIFO */
    unlink(FIFO_PATH);
    if (mkfifo(FIFO_PATH, 0600) != 0) {
        perror("mkfifo");
        return;
    }

    /*
     * Відкриваємо FIFO для запису в неблокуючому режимі (O_NONBLOCK).
     * Без O_NONBLOCK open() заблокувався б, очікуючи читача.
     */
    int fd = open(FIFO_PATH, O_WRONLY | O_NONBLOCK);
    if (fd < 0) {
        /* ENXIO = немає читача з боку O_RDONLY */
        printf("  open(O_WRONLY|O_NONBLOCK) → помилка: %s\n", strerror(errno));
        printf("  Причина: FIFO не можна відкрити на запис без читача\n"
               "  (ядро повертає ENXIO у неблокуючому режимі)\n\n");
        unlink(FIFO_PATH);
        return;
    }

    /* Спроба запису */
    char buf[BUF_SIZE];
    memset(buf, 'A', sizeof(buf));

    ssize_t n = write(fd, buf, sizeof(buf));

    if (n < 0) {
        printf("  write() → -1, errno = %d (%s)\n", errno, strerror(errno));
    } else {
        printf("  write() → %zd байт записано\n", n);
    }

    if (sigpipe_received)
        printf("  [!] Отримано SIGPIPE — читача не було!\n");

    close(fd);
    unlink(FIFO_PATH);
    printf("\n");
}

/* Демонстрація 2: write() у FIFO — читач є, потім зникає */
static void demo_reader_disappears(void) {
    printf("=== Демонстрація 2: читач є, потім закриває кінець ===\n");

    sigpipe_received = 0;

    unlink(FIFO_PATH);
    if (mkfifo(FIFO_PATH, 0600) != 0) { perror("mkfifo"); return; }

    pid_t pid = fork();
    if (pid < 0) { perror("fork"); return; }

    if (pid == 0) {
        /* Дочірній процес: відкриває FIFO для читання, читає один раз, потім закривається — симулює "читач зник" */
        int rfd = open(FIFO_PATH, O_RDONLY);
        if (rfd < 0) { perror("child: open"); exit(1); }

        char rbuf[64];
        ssize_t r = read(rfd, rbuf, sizeof(rbuf));
        printf("  [читач] прочитано %zd байт\n", r);
        close(rfd);   /* читач закрив кінець — наступний write() отримає SIGPIPE */
        exit(0);
    }

    /* Батьківський процес: записувач */
    int wfd = open(FIFO_PATH, O_WRONLY);
    if (wfd < 0) { perror("parent: open"); wait(NULL); return; }

    char buf[BUF_SIZE];
    memset(buf, 'B', sizeof(buf));

    /* Перший запис — читач ще живий */
    ssize_t n = write(wfd, buf, 32);
    printf("  [записувач] 1-й write → %zd байт\n", n);

    /* Чекаємо, поки дочірній процес закриє свій кінець */
    usleep(200000);  /* 200 мс */

    /* Другий запис — читача вже немає */
    n = write(wfd, buf, sizeof(buf));
    if (n < 0)
        printf("  [записувач] 2-й write → -1, errno = %s\n", strerror(errno));
    else
        printf("  [записувач] 2-й write → %zd байт\n", n);

    if (sigpipe_received)
        printf("  [!] Отримано SIGPIPE — читач закрив кінець!\n");

    close(wfd);
    wait(NULL);
    unlink(FIFO_PATH);
    printf("\n");
}

/* Демонстрація 3: ігнорування SIGPIPE через SIG_IGN */
static void demo_sigpipe_ignored(void) {
    printf("=== Демонстрація 3: SIGPIPE ігнорується (SIG_IGN) ===\n");

    /* Ігноруємо SIGPIPE повністю */
    signal(SIGPIPE, SIG_IGN);

    unlink(FIFO_PATH);
    if (mkfifo(FIFO_PATH, 0600) != 0) { perror("mkfifo"); return; }

    pid_t pid = fork();
    if (pid < 0) { perror("fork"); return; }

    if (pid == 0) {
        int rfd = open(FIFO_PATH, O_RDONLY);
        if (rfd >= 0) { close(rfd); }  /* одразу закриваємо */
        exit(0);
    }

    usleep(100000);  /* чекаємо, поки дочірній закриє кінець */

    int wfd = open(FIFO_PATH, O_WRONLY | O_NONBLOCK);
    if (wfd < 0) {
        printf("  open → %s\n", strerror(errno));
        wait(NULL);
        unlink(FIFO_PATH);
        return;
    }

    char buf[128] = "test data";
    ssize_t n = write(wfd, buf, sizeof(buf));

    if (n < 0)
        printf("  write() → -1, errno = %d (%s)\n  "
               "(SIGPIPE ігноровано — write повертає -1/EPIPE замість краху)\n",
               errno, strerror(errno));
    else
        printf("  write() → %zd байт\n", n);

    close(wfd);
    wait(NULL);
    unlink(FIFO_PATH);

    /* Відновлюємо обробник */
    signal(SIGPIPE, SIG_DFL);
    printf("\n");
}

/* Демонстрація 4: атомарність write() у FIFO (PIPE_BUF) */
static void demo_pipe_buf(void) {
    printf("=== Демонстрація 4: атомарність write() — PIPE_BUF ===\n");

    long pipe_buf = fpathconf(STDIN_FILENO, _PC_PIPE_BUF);
    printf("  PIPE_BUF на цій системі = %ld байт\n", pipe_buf);
    printf("  Записи <= PIPE_BUF є атомарними (не перемішуються між процесами)\n");
    printf("  Записи >  PIPE_BUF можуть бути розбиті ядром\n\n");
}

/* main */
int main(void) {
    printf("Практична робота №8 — Варіант 1\n");
    printf("write() у FIFO без читача\n");
    printf("================================\n\n");

    demo_sigpipe();
    demo_reader_disappears();
    demo_sigpipe_ignored();
    demo_pipe_buf();

    printf("=== Висновки ===\n");
    printf("1. write() у FIFO без читача → ядро надсилає SIGPIPE процесу.\n");
    printf("2. За замовчуванням SIGPIPE завершує процес.\n");
    printf("3. При перехопленні/ігноруванні SIGPIPE → write() = -1, errno = EPIPE.\n");
    printf("4. open(O_WRONLY|O_NONBLOCK) без читача → -1, errno = ENXIO.\n");
    printf("5. Записи <= PIPE_BUF є атомарними.\n");

    return 0;
}
