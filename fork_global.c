

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

/* Глобальна змінна — існує до fork() в одному екземплярі, після fork() кожен процес отримує власну КОПІЮ */
int global_counter = 100;

int main(void) {
    printf("=== До fork() ===\n");
    printf("  PID=%d  global_counter=%d  &global=%p\n",
           getpid(), global_counter, (void*)&global_counter);

    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        /* ---- Дочірній процес ---- */
        printf("\n=== Дочірній процес (PID=%d, PPID=%d) ===\n",
               getpid(), getppid());
        printf("  global_counter до зміни  = %d\n", global_counter);
        printf("  &global (дочірній)        = %p\n", (void*)&global_counter);

        /* Дочірній НЕ змінює змінну — лише читає */
        printf("  [дочірній] змін не вносимо\n");
        printf("  global_counter після      = %d\n", global_counter);

        exit(0);

    } else {
        /* Батьківський процес */

        /* Невелика пауза, щоб дочірній встиг вивести свій рядок першим */
        sleep(1);

        printf("\n=== Батьківський процес (PID=%d) ===\n", getpid());
        printf("  global_counter до зміни  = %d\n", global_counter);
        printf("  &global (батьківський)    = %p\n", (void*)&global_counter);

        /* Змінюємо глобальну змінну ТІЛЬКИ в батьківському процесі */
        global_counter = 999;
        printf("  [батьківський] global_counter = 999\n");
        printf("  global_counter після зміни = %d\n", global_counter);

        /* Чекаємо завершення дочірнього */
        int status;
        waitpid(pid, &status, 0);

        printf("\n=== Підсумок ===\n");
        printf("  Батьківський: global_counter = %d\n", global_counter);
        printf("  Дочірній бачив:              = 100  (незмінено)\n");
        printf("  Висновок: fork() копіює пам'ять — зміни незалежні!\n");
        printf("  Хоча адреси однакові — це віртуальні адреси,\n"
               "  які відображаються на різні фізичні сторінки (COW).\n");
    }

    return 0;
}
