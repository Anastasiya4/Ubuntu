

#include <stdio.h>
#include <string.h>

/*
 * Сценарій:
 *   У функції calc() поруч на стеку розташовані два об'єкти:
 *     int secret  = 42;   ← конфіденційне значення
 *     int arr[4]  = ...;  ← масив, читання якого вийде за межі
 *
 *   Залежно від компілятора та ABI стек виглядає приблизно так:
 *
 *   Вищі адреси
 *   ┌──────────────┐
 *   │  secret (4B) │  ← arr[4]  (читаємо тут — вже за межами!)
 *   ├──────────────┤
 *   │  arr[3] (4B) │
 *   │  arr[2] (4B) │
 *   │  arr[1] (4B) │
 *   │  arr[0] (4B) │
 *   └──────────────┘
 *   Нижчі адреси
 *
 *   Читання arr[4] повертає вміст secret або іншої змінної,
 *   що змінює результат обчислення без будь-якого segfault.
 */

/* Демонстрація 1: читання arr[N] зчитує сусідню змінну */
static void demo_stack_read(void) {
    int arr[4]  = {10, 20, 30, 40};
    int secret  = 99;          /* сусідня змінна на стеку           */
    int sum_ok  = 0;
    int sum_bad = 0;

    /* Правильний підрахунок — тільки 4 елементи */
    for (int i = 0; i < 4; i++)
        sum_ok += arr[i];

    /*
     * ПОМИЛКА: цикл іде до i <= 4 (на один крок більше).
     * arr[4] — це вже за межами масиву; тут може лежати secret.
     * segfault НЕ виникає, бо адреса все ще у межах сторінки стеку.
     */
    for (int i = 0; i <= 4; i++)   /* <-- i <= 4 замість i < 4 */
        sum_bad += arr[i];

    printf("=== Демонстрація 1: читання за межами масиву ===\n");
    printf("  arr      = {%d, %d, %d, %d}\n",
           arr[0], arr[1], arr[2], arr[3]);
    printf("  secret   = %d\n", secret);
    printf("  &arr[0]  = %p\n", (void*)&arr[0]);
    printf("  &arr[4]  = %p  (за межами)\n", (void*)&arr[4]);
    printf("  &secret  = %p\n", (void*)&secret);
    printf("\n");
    printf("  sum_ok  (i < 4)  = %d  (правильно: 10+20+30+40)\n", sum_ok);
    printf("  sum_bad (i <= 4) = %d  (включено arr[4] = ??? зі стеку)\n",
           sum_bad);

    if (sum_ok != sum_bad) {
        printf("\n  [!] Результати ВІДРІЗНЯЮТЬСЯ на %d!\n",
               sum_bad - sum_ok);
        printf("  [!] arr[4] = %d — зчитано зі стеку (можливо, secret).\n",
               arr[4]);  /* навмисне читання для демонстрації */
    }
    printf("\n");
}

/* Демонстрація 2: умовна логіка залежить від "сміттєвого" значення */
static int is_admin_buggy(void) {
    int flags[3]  = {0, 0, 0}; /* флаги прав: 0 = звичайний користувач */
    int is_admin  = 1;          /* наступна змінна на стеку             */

    /*
     * ПОМИЛКА: перевіряємо flags[3] — поза масивом.
     * Якщо там лежить is_admin == 1, функція повертає "адміністратор",
     * хоча всі три прапори == 0.
     */
    for (int i = 0; i <= 3; i++) {  /* має бути i < 3 */
        if (flags[i]) return 1;
    }
    (void)is_admin; /* прибираємо попередження "unused" */
    return 0;
}

static int is_admin_fixed(void) {
    int flags[3]  = {0, 0, 0};
    int is_admin  = 1;          /* залишаємо для симетрії зі стеком */

    for (int i = 0; i < 3; i++) {   /* правильна межа */
        if (flags[i]) return 1;
    }
    (void)is_admin;
    return 0;
}

static void demo_logic_corruption(void) {
    printf("=== Демонстрація 2: помилкова перевірка прав ===\n");
    printf("  is_admin_buggy() = %d  (очікувано 0, отримано %d)\n",
           0, is_admin_buggy());
    printf("  is_admin_fixed() = %d  (правильно)\n",
           is_admin_fixed());
    if (is_admin_buggy() != 0)
        printf("\n  [!] Помилка: користувач отримав права адміністратора!\n");
    printf("\n");
}

/* Демонстрація 3: адреси змінних на стеку (наочна схема) */
static void demo_addresses(void) {
    int a = 1, b = 2, c = 3;
    int arr[3] = {10, 20, 30};

    printf("=== Демонстрація 3: розташування змінних на стеку ===\n");
    printf("  &a      = %p  (значення %d)\n", (void*)&a,      a);
    printf("  &b      = %p  (значення %d)\n", (void*)&b,      b);
    printf("  &c      = %p  (значення %d)\n", (void*)&c,      c);
    printf("  &arr[0] = %p  (значення %d)\n", (void*)&arr[0], arr[0]);
    printf("  &arr[1] = %p  (значення %d)\n", (void*)&arr[1], arr[1]);
    printf("  &arr[2] = %p  (значення %d)\n", (void*)&arr[2], arr[2]);
    printf("  &arr[3] = %p  (за межами — тут: %d)\n",
           (void*)&arr[3], arr[3]); /* UB — навмисно для демонстрації */
    printf("\n");
    printf("  Різниця між елементами: %td байт\n",
           (char*)&arr[1] - (char*)&arr[0]);
    printf("  Стек зростає %s\n",
           (char*)&arr[1] > (char*)&arr[0] ? "вгору" : "вниз");
    printf("\n");
}

/* main */
int main(void) {
    printf("Практична робота №5 — Варіант 1\n");
    printf("Out-of-bounds читання зі стеку без segfault\n");
    printf("============================================\n\n");

    demo_addresses();
    demo_stack_read();
    demo_logic_corruption();

    printf("Для аналізу запустіть:\n");
    printf("  valgrind --track-origins=yes ./stack_leak\n");
    printf("  gcc -fsanitize=address,undefined -O0 stack_leak.c -o stack_asan && ./stack_asan\n");
    return 0;
}
