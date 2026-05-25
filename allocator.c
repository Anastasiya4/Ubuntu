
#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <assert.h>

/* Налаштування */
#define ALIGN        8                          /* вирівнювання байт          */
#define ALIGN_UP(n)  (((n)+(ALIGN-1))&~(ALIGN-1))
#define MIN_SPLIT    (sizeof(block_t) + ALIGN)  /* мінімум для розбиття      */

/* =========================================================
 * Заголовок блоку
 *
 *  ┌────────────────────────────────┐
 *  │  size   │ free │  magic │ next │  ← block_t  (заголовок)
 *  ├────────────────────────────────┤
 *  │         дані користувача       │  ← повертається з my_malloc()
 *  └────────────────────────────────┘
 *
 *  magic використовується для виявлення пошкодження метаданих.
 * ========================================================= */
#define MAGIC_FREE  0xDEADBEEF
#define MAGIC_USED  0xCAFEBABE

typedef struct block {
    size_t       size;   /* розмір корисних даних (байт) */
    unsigned int free;   /* 1 = вільний                  */
    unsigned int magic;  /* MAGIC_FREE або MAGIC_USED    */
    struct block *next;  /* наступний блок у списку      */
} block_t;

#define HDR  ALIGN_UP(sizeof(block_t))

static block_t *free_list = NULL;  /* голова єдиного списку всіх блоків */

/*  Внутрішні допоміжні функції */

static inline void *blk_data(block_t *b) { return (char*)b + HDR; }
static inline block_t *data_blk(void *p) { return (block_t*)((char*)p - HDR); }

/* Перевірка magic-числа — виявлення пошкодження купи */
static void check_magic(block_t *b) {
    if (b->magic != MAGIC_FREE && b->magic != MAGIC_USED) {
        fprintf(stderr,
            "[allocator] FATAL: пошкодження метаданих @ %p "
            "(magic = 0x%08X)\n", (void*)b, b->magic);
        /* У реальному коді — abort(); тут продовжуємо для наочності */
    }
}

/* Розширення купи через sbrk() */
static block_t *heap_extend(size_t size) {
    block_t *b = sbrk((intptr_t)(HDR + size));
    if (b == (void*)-1) return NULL;
    b->size  = size;
    b->free  = 0;
    b->magic = MAGIC_USED;
    b->next  = NULL;

    /* Додаємо в кінець списку */
    if (!free_list) {
        free_list = b;
    } else {
        block_t *cur = free_list;
        while (cur->next) cur = cur->next;
        cur->next = b;
    }
    return b;
}

/* Розбиття блоку */
static void split(block_t *b, size_t need) {
    if (b->size < need + MIN_SPLIT) return;   /* залишок надто малий */

    block_t *new_b = (block_t*)((char*)blk_data(b) + need);
    new_b->size    = b->size - need - HDR;
    new_b->free    = 1;
    new_b->magic   = MAGIC_FREE;
    new_b->next    = b->next;

    b->next = new_b;
    b->size = need;
}

/*  Коалесценція (об'єднання суміжних вільних блоків) */
static void coalesce(void) {
    block_t *cur = free_list;
    while (cur && cur->next) {
        check_magic(cur);
        if (cur->free && cur->next->free) {
            /* Зливаємо cur і cur->next */
            cur->size  += HDR + cur->next->size;
            cur->next   = cur->next->next;
            /* Не рухаємо cur вперед — перевіряємо знову */
        } else {
            cur = cur->next;
        }
    }
}

/* my_malloc */
void *my_malloc(size_t size) {
    if (!size) return NULL;
    size = ALIGN_UP(size);

    /* Шукаємо вільний блок (first-fit) */
    block_t *cur = free_list;
    while (cur) {
        check_magic(cur);
        if (cur->free && cur->size >= size) {
            split(cur, size);
            cur->free  = 0;
            cur->magic = MAGIC_USED;
            printf("  [malloc] reuse @ %-14p  size=%4zu\n",
                   blk_data(cur), size);
            return blk_data(cur);
        }
        cur = cur->next;
    }

    /* Не знайшли — розширюємо купу */
    block_t *b = heap_extend(size);
    if (!b) { perror("sbrk"); return NULL; }
    printf("  [malloc] sbrk  @ %-14p  size=%4zu\n", blk_data(b), size);
    return blk_data(b);
}

/* my_free */
void my_free(void *ptr) {
    if (!ptr) return;

    block_t *b = data_blk(ptr);
    check_magic(b);

    if (b->free) {
        fprintf(stderr, "[allocator] WARN: подвійне звільнення @ %p!\n", ptr);
        return;
    }

    printf("  [free]         @ %-14p  size=%4zu\n", ptr, b->size);
    b->free  = 1;
    b->magic = MAGIC_FREE;
    coalesce();
}

/* Вивід стану алокатора */
static void dump_heap(const char *label) {
    printf("\n--- %s ---\n", label);
    block_t *cur = free_list;
    int idx = 0;
    size_t used = 0, freed = 0;
    while (cur) {
        printf("  [%2d]  %-14p  size=%4zu  %s  magic=%08X\n",
               idx++, (void*)cur, cur->size,
               cur->free ? "FREE    " : "USED    ", cur->magic);
        cur->free ? (freed += cur->size) : (used += cur->size);
        cur = cur->next;
    }
    printf("  Зайнято: %zu Б  |  Вільно: %zu Б\n\n", used, freed);
}

/* Тести */

/* Тест 1: базові alloc/free та коалесценція */
static void test_basic(void) {
    printf("=== Тест 1: базовий alloc/free/coalesce ===\n");

    void *a = my_malloc(64);
    void *b = my_malloc(128);
    void *c = my_malloc(64);
    dump_heap("після трьох alloc");

    my_free(a);
    my_free(b);         /* суміжні a та b мають злитися */
    dump_heap("після free(a), free(b) — coalesce a+b");

    void *d = my_malloc(180);   /* повинен потрапити в злитий блок */
    dump_heap("після alloc 180 (reuse злитого блоку)");

    my_free(c);
    my_free(d);
    dump_heap("після звільнення всього");
}

/* Тест 2: корректна обробка NULL */
static void test_null(void) {
    printf("=== Тест 2: malloc(0) та free(NULL) ===\n");
    void *p = my_malloc(0);
    printf("  my_malloc(0) = %p  (очікується NULL)\n", p);
    my_free(NULL);
    printf("  my_free(NULL) — без помилок\n\n");
}

/* Тест 3: виявлення подвійного звільнення */
static void test_double_free(void) {
    printf("=== Тест 3: подвійне звільнення ===\n");
    void *p = my_malloc(32);
    my_free(p);
    my_free(p);   /* алокатор повинен виявити і попередити */
    printf("\n");
}

/* Тест 4: запис та читання даних */
static void test_data(void) {
    printf("=== Тест 4: запис і читання даних ===\n");
    int *arr = my_malloc(5 * sizeof(int));
    assert(arr != NULL);
    for (int i = 0; i < 5; i++) arr[i] = i * 10;
    printf("  arr = ");
    for (int i = 0; i < 5; i++) printf("%d ", arr[i]);
    printf("\n");
    assert(arr[4] == 40);
    my_free(arr);
    printf("  Дані коректні.\n\n");
}

/* main */
int main(void) {
    printf("Власний алокатор на sbrk()  |  HDR=%zu байт  ALIGN=%d\n\n",
           HDR, ALIGN);

    test_null();
    test_basic();
    test_double_free();
    test_data();

    printf("=== Завершено ===\n");
    printf("\nЗапустіть: valgrind --leak-check=full --track-origins=yes "
           "--show-leak-kinds=all ./allocator\n");
    return 0;
}
