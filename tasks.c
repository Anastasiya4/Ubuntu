

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <ctype.h>
#include <limits.h>

/* Завдання 1.1 */
void task1_1(void) {
    printf("\n=== Завдання 1.1: erf та довірчі інтервали ===\n");

    double z;
    printf("Введіть Z-оцінку: ");
    if (scanf("%lf", &z) != 1) {
        fprintf(stderr, "Помилка введення!\n");
        return;
    }

    if (isnan(erf(z * sqrt(0.5)))) {
        fprintf(stderr, "Помилка обчислення erf!\n");
        return;
    }

    printf("Інтеграл нормального розподілу N(0,1) між -Z та Z: %g\n",
           erf(z * sqrt(0.5)));

    /* Довірчі інтервали для трьох рівнів */
    double levels[]    = {0.90, 0.95, 0.99};
    double z_values[]  = {1.645, 1.960, 2.576};
    int n = sizeof(levels) / sizeof(levels[0]);

    printf("\nДовірчі інтервали (стандартний нормальний розподіл):\n");
    for (int i = 0; i < n; i++) {
        printf("  %.0f%%: Z = %.3f  =>  інтеграл = %.6f\n",
               levels[i] * 100,
               z_values[i],
               erf(z_values[i] * sqrt(0.5)));
    }
}

/* Завдання 1.6 */
void task1_6(void) {
    printf("\n=== Завдання 1.6: конвертація секунд ===\n");

    long seconds;
    printf("Введіть кількість секунд: ");
    if (scanf("%ld", &seconds) != 1) {
        fprintf(stderr, "Помилка введення!\n");
        return;
    }

    const char *prefix = "";
    long abs_sec = seconds;
    if (seconds < 0) {
        prefix   = "(у минулому) ";
        abs_sec  = -seconds;
    }

    long h = abs_sec / 3600;
    long m = (abs_sec % 3600) / 60;
    long s = abs_sec % 60;

    printf("%s%ld секунд еквівалентно %ld годинам %ld хвилинам %ld секундам.\n",
           prefix, seconds, h, m, s);
}

/* Завдання 1.7 */
void task1_7(void) {
    printf("\n=== Завдання 1.7: перетворення систем числення ===\n");

    int base;
    char buf[256];

    printf("Введіть основу системи числення: ");
    if (scanf("%d", &base) != 1 || base < 2 || base > 36) {
        fprintf(stderr, "Недопустима основа!\n");
        return;
    }

    printf("Введіть число в системі числення з основою %d: ", base);
    if (scanf("%255s", buf) != 1) {
        fprintf(stderr, "Помилка введення!\n");
        return;
    }

    long result = 0;
    for (int i = 0; buf[i] != '\0'; i++) {
        int digit;
        char c = toupper((unsigned char)buf[i]);
        if (c >= '0' && c <= '9')       digit = c - '0';
        else if (c >= 'A' && c <= 'Z')  digit = c - 'A' + 10;
        else                            continue; /* ігноруємо недопустимі символи */

        if (digit >= base) {
            fprintf(stderr, "Цифра '%c' недопустима для основи %d, пропущено.\n",
                    buf[i], base);
            continue;
        }
        result = result * base + digit;
    }

    printf("Значення %s (основа %d) у десятковій системі = %ld\n",
           buf, base, result);
}

/* Завдання 1.8 */
#define ARR_SIZE 20

void task1_8(void) {
    printf("\n=== Завдання 1.8: масив та пошук індексів ===\n");

    float arr[ARR_SIZE];
    srand((unsigned int)time(NULL));

    for (int i = 0; i < ARR_SIZE; i++)
        arr[i] = (float)(rand() % 10);

    printf("Масив: ");
    for (int i = 0; i < ARR_SIZE; i++)
        printf("%.0f ", arr[i]);
    printf("\n");

    float target;
    printf("Введіть число для пошуку: ");
    if (scanf("%f", &target) != 1) {
        fprintf(stderr, "Помилка введення!\n");
        return;
    }

    printf("Індекси елементу %.0f: ", target);
    int found = 0;
    for (int i = 0; i < ARR_SIZE; i++) {
        if (arr[i] == target) {
            printf("%d ", i);
            found++;
        }
    }
    if (!found)
        printf("(не знайдено)");
    printf("\n");
}

/* Завдання 1.9 */
int replace(char *str, char from, char to) {
    int count = 0;
    while (*str) {
        if (*str == from) {
            *str = to;
            count++;
        }
        str++;
    }
    return count;
}

void task1_9(void) {
    printf("\n=== Завдання 1.9: replace пробілів ===\n");

    char buf[256];
    printf("Введіть рядок: ");
    /* читаємо рядок з пробілами */
    if (scanf(" %255[^\n]", buf) != 1) {
        fprintf(stderr, "Помилка введення!\n");
        return;
    }

    int n = replace(buf, ' ', '-');
    printf("Результат: \"%s\", кількість замін: %d\n", buf, n);
}

/* Завдання 1. 10 */
typedef enum { JAN=1,FEB,MAR,APR,MAY,JUN,JUL,AUG,SEP,OCT,NOV,DEC } Month;

static int is_leap(int y) {
    return (y % 4 == 0 && y % 100 != 0) || (y % 400 == 0);
}

static int days_in_month(Month m, int y) {
    int days[] = {0,31,28,31,30,31,30,31,31,30,31,30,31};
    if (m == FEB && is_leap(y)) return 29;
    return days[m];
}

void task1_10(void) {
    printf("\n=== Завдання 1.10: завтрашня дата ===\n");

    int day, month, year;
    printf("Введіть дату (день місяць рік): ");
    if (scanf("%d %d %d", &day, &month, &year) != 3 ||
        month < 1 || month > 12 || day < 1) {
        fprintf(stderr, "Неправильна дата!\n");
        return;
    }

    Month m = (Month)month;
    day++;
    if (day > days_in_month(m, year)) {
        day = 1;
        m++;
        if (m > DEC) { m = JAN; year++; }
    }

    printf("Завтрашня дата: %02d.%02d.%04d\n", day, (int)m, year);
}

/* Завдання 1. 11 */
#define MAX_STUDENTS 100
#define NAME_LEN     64
#define ADDR_LEN    128

typedef struct {
    char name[NAME_LEN];
    char birthdate[12];  /* DD.MM.YYYY */
    char address[ADDR_LEN];
} Student;

static Student db[MAX_STUDENTS];
static int db_count = 0;

void student_add(const char *name, const char *birth, const char *addr) {
    if (db_count >= MAX_STUDENTS) { printf("База заповнена!\n"); return; }
    strncpy(db[db_count].name,      name,  NAME_LEN - 1);
    strncpy(db[db_count].birthdate, birth, 11);
    strncpy(db[db_count].address,   addr,  ADDR_LEN - 1);
    db_count++;
    printf("Студента додано.\n");
}

void student_list(void) {
    for (int i = 0; i < db_count; i++)
        printf("  [%d] %s | %s | %s\n",
               i, db[i].name, db[i].birthdate, db[i].address);
}

void student_search(const char *query) {
    printf("Результати пошуку за \"%s\":\n", query);
    for (int i = 0; i < db_count; i++) {
        if (strstr(db[i].name, query) ||
            strstr(db[i].address, query)) {
            printf("  [%d] %s | %s | %s\n",
                   i, db[i].name, db[i].birthdate, db[i].address);
        }
    }
}

void task1_11(void) {
    printf("\n=== Завдання 1.11: база даних студентів ===\n");

    student_add("Іваненко Олексій", "01.01.2003", "м. Київ, вул. Хрещатик 1");
    student_add("Петренко Марія",   "15.06.2002", "м. Львів, вул. Шевченка 5");
    student_add("Сидоренко Іван",   "22.03.2004", "м. Харків, пл. Свободи 2");

    printf("Список студентів:\n");
    student_list();

    student_search("Київ");
}

/* Завдання 1.12 — масиви */
int linear_search_rec(int *arr, int n, int val, int idx) {
    if (idx >= n) return -1;
    if (arr[idx] == val) return idx;
    return linear_search_rec(arr, n, val, idx + 1);
}

int binary_search(int *arr, int n, int val) {
    int lo = 0, hi = n - 1;
    while (lo <= hi) {
        int mid = (lo + hi) / 2;
        if      (arr[mid] == val) return mid;
        else if (arr[mid]  < val) lo = mid + 1;
        else                      hi = mid - 1;
    }
    return -1;
}

static int cmp_int(const void *a, const void *b) {
    return (*(int*)a - *(int*)b);
}

void task1_12(void) {
    printf("\n=== Завдання 1.12: пошук у масиві ===\n");

    int arr[10];
    srand((unsigned int)time(NULL));
    for (int i = 0; i < 10; i++) arr[i] = rand() % 20;

    printf("Масив: ");
    for (int i = 0; i < 10; i++) printf("%d ", arr[i]);
    printf("\n");

    int val = arr[3]; /* шукаємо відомий елемент */
    int idx = linear_search_rec(arr, 10, val, 0);
    printf("Рекурсивний пошук %d: індекс %d\n", val, idx);

    qsort(arr, 10, sizeof(int), cmp_int);
    printf("Відсортований масив: ");
    for (int i = 0; i < 10; i++) printf("%d ", arr[i]);
    printf("\n");

    idx = binary_search(arr, 10, val);
    printf("Бінарний пошук %d: індекс %d\n", val, idx);
}

/* Завдання 1.13 */
void task1_13(void) {
    printf("\n=== Завдання 1.13: динамічний масив ===\n");

    int n;
    printf("Введіть розмір масиву (1–100): ");
    if (scanf("%d", &n) != 1 || n < 1 || n > 100) {
        fprintf(stderr, "Недопустимий розмір!\n");
        return;
    }

    int *arr = (int*)malloc((size_t)n * sizeof(int));
    if (!arr) { fprintf(stderr, "Помилка виділення пам'яті!\n"); return; }

    for (int i = 0; i < n; i++) arr[i] = n;
    printf("Масив (%d елементів): ", n);
    for (int i = 0; i < n; i++) printf("%d ", arr[i]);
    printf("\n");

    /* збільшуємо розмір */
    int new_n = n * 2;
    int *tmp = (int*)realloc(arr, (size_t)new_n * sizeof(int));
    if (!tmp) { free(arr); fprintf(stderr, "Помилка realloc!\n"); return; }
    arr = tmp;
    for (int i = n; i < new_n; i++) arr[i] = 0;
    printf("Після realloc (%d елементів): ", new_n);
    for (int i = 0; i < new_n; i++) printf("%d ", arr[i]);
    printf("\n");

    free(arr);
    printf("Пам'ять звільнено.\n");
}

/* Завдання 1.14 */
#define MAX_LINE 1024
#define BUF_LINES 20

void task1_14(void) {
    printf("\n=== Завдання 1.14: останні N рядків ===\n");

    int n = 5;
    char ring[BUF_LINES][MAX_LINE];
    int  head  = 0, count = 0;

    FILE *f = fopen("tasks.c", "r"); /* читаємо сам себе як демонстрацію */
    if (!f) { fprintf(stderr, "Не вдалося відкрити файл!\n"); return; }

    while (fgets(ring[head], MAX_LINE, f)) {
        head = (head + 1) % BUF_LINES;
        if (count < BUF_LINES) count++;
    }
    fclose(f);

    if (n > count) n = count;
    printf("Останні %d рядків файлу tasks.c:\n", n);
    int start = (head - n + BUF_LINES) % BUF_LINES;
    for (int i = 0; i < n; i++) {
        printf("  %s", ring[(start + i) % BUF_LINES]);
    }
}

/* Завдання 1.15 */
void merge_sort(int *arr, int n) {
    if (n < 2) return;
    int mid = n / 2;
    int *L = (int*)malloc((size_t)mid * sizeof(int));
    int *R = (int*)malloc((size_t)(n - mid) * sizeof(int));
    if (!L || !R) { free(L); free(R); return; }

    memcpy(L, arr,       (size_t)mid       * sizeof(int));
    memcpy(R, arr + mid, (size_t)(n - mid) * sizeof(int));

    merge_sort(L, mid);
    merge_sort(R, n - mid);

    int i = 0, j = 0, k = 0;
    while (i < mid && j < n - mid)
        arr[k++] = (L[i] <= R[j]) ? L[i++] : R[j++];
    while (i < mid)   arr[k++] = L[i++];
    while (j < n-mid) arr[k++] = R[j++];

    free(L); free(R);
}

void task1_15(void) {
    printf("\n=== Завдання 1.15: сортування ===\n");

    int arr[] = {42, 7, 19, 3, 55, 1, 28, 14};
    int n = sizeof(arr) / sizeof(arr[0]);

    printf("До сортування:    ");
    for (int i = 0; i < n; i++) printf("%d ", arr[i]);
    printf("\n");

    merge_sort(arr, n);

    printf("Після merge sort: ");
    for (int i = 0; i < n; i++) printf("%d ", arr[i]);
    printf("\n");

    /* qsort — спадання */
    int arr2[] = {42, 7, 19, 3, 55, 1, 28, 14};
    qsort(arr2, n, sizeof(int), cmp_int);
    printf("Після qsort (зрост.): ");
    for (int i = 0; i < n; i++) printf("%d ", arr2[i]);
    printf("\n");
}

/* Завдання 1.16 */
typedef struct {
    char keyword[32];
    int  frequency;
} KeywordEntry;

static int cmp_keyword(const void *a, const void *b) {
    const KeywordEntry *ka = (const KeywordEntry*)a;
    const KeywordEntry *kb = (const KeywordEntry*)b;
    int c = strcmp(ka->keyword, kb->keyword);
    if (c != 0) return c;
    return ka->frequency - kb->frequency;
}

void task1_16(void) {
    printf("\n=== Завдання 1.16: сортування структур ===\n");

    KeywordEntry data[] = {
        {"while",   5}, {"for",    12}, {"if",     8},
        {"return",  3}, {"struct", 7},  {"for",     2}
    };
    int n = sizeof(data) / sizeof(data[0]);

    qsort(data, n, sizeof(KeywordEntry), cmp_keyword);

    printf("Відсортовано (keyword, frequency):\n");
    for (int i = 0; i < n; i++)
        printf("  %-12s %d\n", data[i].keyword, data[i].frequency);

    /* bsearch */
    KeywordEntry key = {"if", 0};
    KeywordEntry *found = (KeywordEntry*)
        bsearch(&key, data, n, sizeof(KeywordEntry), cmp_keyword);
    printf("bsearch \"if\": %s\n", found ? "знайдено" : "не знайдено");
}

/* Завдання 1.17 */
void insort(void *base, size_t n, size_t size,
            int (*cmp)(const void*, const void*)) {
    char *arr = (char*)base;
    char *tmp = (char*)malloc(size);
    if (!tmp) return;

    for (size_t i = 1; i < n; i++) {
        memcpy(tmp, arr + i * size, size);
        size_t j = i;
        while (j > 0 && cmp(arr + (j-1)*size, tmp) > 0) {
            memcpy(arr + j*size, arr + (j-1)*size, size);
            j--;
        }
        memcpy(arr + j * size, tmp, size);
    }
    free(tmp);
}

void task1_17(void) {
    printf("\n=== Завдання 1.17: insertion sort ===\n");

    int arr[] = {9, 3, 7, 1, 5, 8, 2, 6, 4};
    int n = sizeof(arr) / sizeof(arr[0]);

    printf("До сортування:     ");
    for (int i = 0; i < n; i++) printf("%d ", arr[i]);
    printf("\n");

    insort(arr, n, sizeof(int), cmp_int);

    printf("Після insort:      ");
    for (int i = 0; i < n; i++) printf("%d ", arr[i]);
    printf("\n");
}

/* Завдання 1.18  */
void print_binary(unsigned long long val, int bits) {
    for (int i = bits - 1; i >= 0; i--) {
        printf("%llu", (val >> i) & 1ULL);
        if (i % 4 == 0 && i > 0) printf(" ");
    }
    printf("\n");
}

void task1_18(void) {
    printf("\n=== Завдання 1.18: двійковий формат ===\n");

    unsigned char  b8  = 0b10101010;
    unsigned short b16 = 0xABCD;
    unsigned int   b32 = 0xDEADBEEF;

    printf("8-бітне  %3u: ", b8);
    print_binary(b8,  8);

    printf("16-бітне %5u: ", b16);
    print_binary(b16, 16);

    printf("32-бітне %10u: ", b32);
    print_binary(b32, 32);

    /* ASCII → binary */
    const char *text = "Hi";
    printf("Текст \"%s\" у двійковому:\n", text);
    for (int i = 0; text[i]; i++) {
        printf("  '%c' = ", text[i]);
        print_binary((unsigned char)text[i], 8);
    }
}

/* Завдання 1.19 */
void task1_19(void) {
    printf("\n=== Завдання 1.19: частота символів ===\n");

    /* для демонстрації аналізуємо рядок */
    const char *sample = "Hello, World! Programming in C is fun.";
    int freq[256] = {0};

    for (int i = 0; sample[i]; i++)
        freq[(unsigned char)sample[i]]++;

    /* сортуємо за частотою (прості вставки) */
    int order[256];
    for (int i = 0; i < 256; i++) order[i] = i;
    for (int i = 1; i < 256; i++) {
        int key = order[i];
        int j = i - 1;
        while (j >= 0 && freq[order[j]] < freq[key]) {
            order[j+1] = order[j]; j--;
        }
        order[j+1] = key;
    }

    printf("Рядок: \"%s\"\nЧастоти (спадання):\n", sample);
    for (int i = 0; i < 256 && freq[order[i]] > 0; i++) {
        if (isprint(order[i]))
            printf("  '%c': %d\n", order[i], freq[order[i]]);
        else
            printf("  [%d]: %d\n", order[i], freq[order[i]]);
    }
}

/* Завдання 1.20 */
void task1_20(void) {
    printf("\n=== Завдання 1.20: CSV таблиця ===\n");

    /* вбудований CSV для демонстрації */
    const char *csv =
        "Ім'я,Вік,Місто\n"
        "Олексій,21,Київ\n"
        "Марія,22,Львів\n"
        "Іван,20,Харків\n";

    char buf[1024];
    strncpy(buf, csv, sizeof(buf) - 1);

    /* визначення ширин стовпців */
    int widths[8] = {0};
    char tmp[1024];
    strncpy(tmp, buf, sizeof(tmp)-1);
    char *line = strtok(tmp, "\n");
    while (line) {
        char row[256]; strncpy(row, line, 255);
        int col = 0;
        char *cell = strtok(row, ",");
        while (cell && col < 8) {
            int w = (int)strlen(cell);
            if (w > widths[col]) widths[col] = w;
            cell = strtok(NULL, ",");
            col++;
        }
        line = strtok(NULL, "\n");
    }

    /* виведення таблиці */
    strncpy(tmp, buf, sizeof(tmp)-1);
    line = strtok(tmp, "\n");
    int first = 1;
    while (line) {
        char row[256]; strncpy(row, line, 255);
        int col = 0;
        char *cell = strtok(row, ",");
        printf("|");
        while (cell && col < 8) {
            printf(" %-*s |", widths[col], cell);
            cell = strtok(NULL, ",");
            col++;
        }
        printf("\n");
        if (first) {
            printf("|");
            for (int c = 0; c < col; c++)
                printf("%.*s|", widths[c]+2, "--------------------");
            printf("\n");
            first = 0;
        }
        line = strtok(NULL, "\n");
    }
}

/* Завдання 1.21 */
void task1_21(void) {
    printf("\n=== Завдання 1.21: корені квадратного рівняння ===\n");

    double a, b, c;
    printf("Введіть a, b, c: ");
    if (scanf("%lf %lf %lf", &a, &b, &c) != 3) {
        fprintf(stderr, "Помилка введення!\n");
        return;
    }

    if (a == 0.0) {
        if (b == 0.0) {
            printf(c == 0.0 ? "Нескінченно багато розв'язків.\n"
                            : "Немає розв'язків.\n");
        } else {
            printf("Лінійне рівняння, x = %.6g\n", -c / b);
        }
        return;
    }

    double D = b*b - 4.0*a*c;
    if (D > 0.0) {
        printf("Два дійсних корені:\n");
        printf("  x1 = %.6g\n", (-b + sqrt(D)) / (2.0*a));
        printf("  x2 = %.6g\n", (-b - sqrt(D)) / (2.0*a));
    } else if (D == 0.0) {
        printf("Один корінь: x = %.6g\n", -b / (2.0*a));
    } else {
        double re =  -b       / (2.0*a);
        double im = sqrt(-D)  / (2.0*a);
        printf("Комплексні корені:\n");
        printf("  x1 = %.6g + %.6g*i\n", re,  im);
        printf("  x2 = %.6g - %.6g*i\n", re,  im);
    }
}

/* Завдання 1.22 */
#define RING_CAP 8

typedef struct {
    int  buf[RING_CAP];
    int  head, tail, size;
} RingBuf;

void rb_init(RingBuf *rb)              { rb->head = rb->tail = rb->size = 0; }
int  rb_push(RingBuf *rb, int val) {
    if (rb->size == RING_CAP) return 0;
    rb->buf[rb->tail] = val;
    rb->tail = (rb->tail + 1) % RING_CAP;
    rb->size++;
    return 1;
}
int  rb_pop(RingBuf *rb, int *val) {
    if (rb->size == 0) return 0;
    *val = rb->buf[rb->head];
    rb->head = (rb->head + 1) % RING_CAP;
    rb->size--;
    return 1;
}

void task1_22(void) {
    printf("\n=== Завдання 1.22: кільцевий буфер ===\n");

    RingBuf rb; rb_init(&rb);
    for (int i = 1; i <= 6; i++) rb_push(&rb, i * 10);

    printf("Буфер після вставки 10..60 (розмір %d):\n", rb.size);
    int v;
    while (rb_pop(&rb, &v)) printf("  вилучено: %d\n", v);
    printf("Буфер порожній: розмір = %d\n", rb.size);
}

/* Завдання 1.23 */
unsigned char invert_bits(unsigned char x) { return (unsigned char)(~x); }

unsigned char invert_range(unsigned char x, int lo, int hi) {
    unsigned char mask = 0;
    for (int i = lo; i <= hi; i++) mask |= (unsigned char)(1 << i);
    return (unsigned char)(x ^ mask);
}

void task1_23(void) {
    printf("\n=== Завдання 1.23: інверсія бітів ===\n");

    unsigned char x = 0b10101010;
    unsigned char y = invert_bits(x);

    printf("Вхід:    "); print_binary(x, 8);
    printf("Інверсія:"); print_binary(y, 8);
    printf("Десятк.: %u\n", y);

    /* часткова інверсія бітів 2..5 */
    unsigned char z = invert_range(x, 2, 5);
    printf("Інверсія бітів [2..5]: "); print_binary(z, 8);
}

/* Завдання 1.24 */
#define swap(t, x, y) do { t _tmp = (x); (x) = (y); (y) = _tmp; } while(0)

void task1_24(void) {
    printf("\n=== Завдання 1.24: swap макрос ===\n");

    int a = 5, b = 10;
    printf("До:    a=%d, b=%d\n", a, b);
    swap(int, a, b);
    printf("Після: a=%d, b=%d\n", a, b);

    double x = 3.14, y = 2.71;
    printf("До:    x=%.2f, y=%.2f\n", x, y);
    swap(double, x, y);
    printf("Після: x=%.2f, y=%.2f\n", x, y);
}

/* Завдання 1.25 */
double rand_float(double max, unsigned int seed) {
    srand(seed);
    return max * ((double)rand() / (double)RAND_MAX);
}

void task1_25(void) {
    printf("\n=== Завдання 1.25: випадкове число ===\n");

    double max  = 100.0;
    unsigned int seed = 42;

    printf("Випадкове число в [0, %.1f] із seed=%u: %.6f\n",
           max, seed, rand_float(max, seed));

    /* без seed — з поточного часу */
    srand((unsigned int)time(NULL));
    printf("Випадкове число в [0, %.1f] без seed:    %.6f\n",
           max, max * ((double)rand() / (double)RAND_MAX));
}

/* main — меню для вибору завдання */
int main(void) {
    printf("Лабораторна робота №1 — Ubuntu та програмування на C\n");
    printf("Введіть номер завдання (1–25, 0 = вихід):\n");
    printf("  1=1.1  2=1.6  3=1.7  4=1.8  5=1.9  6=1.10\n");
    printf("  7=1.11 8=1.12 9=1.13 10=1.14 11=1.15 12=1.16\n");
    printf(" 13=1.17 14=1.18 15=1.19 16=1.20 17=1.21 18=1.22\n");
    printf(" 19=1.23 20=1.24 21=1.25\n> ");

    int choice;
    if (scanf("%d", &choice) != 1) { fprintf(stderr, "Помилка!\n"); return 1; }

    switch (choice) {
        case  1: task1_1();  break;
        case  2: task1_6();  break;
        case  3: task1_7();  break;
        case  4: task1_8();  break;
        case  5: task1_9();  break;
        case  6: task1_10(); break;
        case  7: task1_11(); break;
        case  8: task1_12(); break;
        case  9: task1_13(); break;
        case 10: task1_14(); break;
        case 11: task1_15(); break;
        case 12: task1_16(); break;
        case 13: task1_17(); break;
        case 14: task1_18(); break;
        case 15: task1_19(); break;
        case 16: task1_20(); break;
        case 17: task1_21(); break;
        case 18: task1_22(); break;
        case 19: task1_23(); break;
        case 20: task1_24(); break;
        case 21: task1_25(); break;
        case  0: printf("Вихід.\n"); break;
        default: printf("Невідомий вибір.\n");
    }
    return 0;
}
