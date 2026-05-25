

#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <unistd.h>
#include <sys/mman.h>
#include <errno.h>

/* Константи */

/* Блоки >= MMAP_THRESHOLD виділяються через mmap(), решта — brk() */
#define MMAP_THRESHOLD  (128 * 1024)   /* 128 КБ */

/* Вирівнювання кожного блоку */
#define ALIGN           (sizeof(size_t))

/* Вирівняти x до ALIGN */
#define ALIGN_UP(x)     (((x) + ALIGN - 1) & ~(ALIGN - 1))

/* Заголовок блоку */
typedef struct block_hdr {
    size_t           size;      /* розмір корисних даних (без заголовка) */
    int              free;      /* 1 = вільний, 0 = зайнятий              */
    int              is_mmap;   /* 1 = виділено через mmap                */
    struct block_hdr *next;     /* наступний блок у списку                */
    struct block_hdr *prev;     /* попередній блок у списку               */
} block_hdr_t;

#define HDR_SIZE   ALIGN_UP(sizeof(block_hdr_t))

/* Голова двозв'язного списку всіх brk-блоків */
static block_hdr_t *heap_list = NULL;

/* Допоміжні функції */

/* Повернути вказівник на дані після заголовка */
static inline void *hdr_to_data(block_hdr_t *h) {
    return (char*)h + HDR_SIZE;
}

/* Повернути заголовок за вказівником на дані */
static inline block_hdr_t *data_to_hdr(void *ptr) {
    return (block_hdr_t*)((char*)ptr - HDR_SIZE);
}

/* Розширення купи через sbrk/brk */
static block_hdr_t *extend_heap(size_t size) {
    size_t total = HDR_SIZE + size;
    block_hdr_t *h = sbrk((intptr_t)total);
    if (h == (void*)-1) return NULL;

    h->size    = size;
    h->free    = 0;
    h->is_mmap = 0;
    h->next    = NULL;
    h->prev    = NULL;

    /* Додаємо в кінець списку */
    if (!heap_list) {
        heap_list = h;
    } else {
        block_hdr_t *cur = heap_list;
        while (cur->next) cur = cur->next;
        cur->next = h;
        h->prev   = cur;
    }
    return h;
}

/* Розбиття блоку: якщо залишок >= HDR_SIZE + ALIGN */
static void split_block(block_hdr_t *h, size_t need) {
    size_t leftover = h->size - need;
    if (leftover < HDR_SIZE + ALIGN) return; /* не варто розбивати */

    block_hdr_t *new_h = (block_hdr_t*)((char*)hdr_to_data(h) + need);
    new_h->size    = leftover - HDR_SIZE;
    new_h->free    = 1;
    new_h->is_mmap = 0;
    new_h->next    = h->next;
    new_h->prev    = h;

    if (h->next) h->next->prev = new_h;
    h->next = new_h;
    h->size = need;
}

/* Об'єднання суміжних вільних блоків (coalescing) */
static void coalesce(block_hdr_t *h) {
    /* Об'єднати з наступним */
    while (h->next && h->next->free && !h->next->is_mmap) {
        block_hdr_t *nxt = h->next;
        h->size += HDR_SIZE + nxt->size;
        h->next  = nxt->next;
        if (nxt->next) nxt->next->prev = h;
    }
    /* Об'єднати з попереднім */
    if (h->prev && h->prev->free && !h->prev->is_mmap) {
        block_hdr_t *prv = h->prev;
        prv->size += HDR_SIZE + h->size;
        prv->next  = h->next;
        if (h->next) h->next->prev = prv;
    }
}

/* my_malloc */
void *my_malloc(size_t size) {
    if (size == 0) return NULL;

    size = ALIGN_UP(size);

    /* --- Великий блок: mmap --- */
    if (size >= MMAP_THRESHOLD) {
        size_t total = HDR_SIZE + size;
        block_hdr_t *h = mmap(NULL, total,
                              PROT_READ | PROT_WRITE,
                              MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (h == MAP_FAILED) return NULL;
        h->size    = size;
        h->free    = 0;
        h->is_mmap = 1;
        h->next    = NULL;
        h->prev    = NULL;
        printf("  [malloc] mmap  %6zu байт  @ %p\n", size, hdr_to_data(h));
        return hdr_to_data(h);
    }

    /* --- Малий блок: шукаємо вільний у списку (first-fit) --- */
    block_hdr_t *cur = heap_list;
    while (cur) {
        if (cur->free && !cur->is_mmap && cur->size >= size) {
            split_block(cur, size);
            cur->free = 0;
            printf("  [malloc] reuse %6zu байт  @ %p\n", size, hdr_to_data(cur));
            return hdr_to_data(cur);
        }
        cur = cur->next;
    }

    /* --- Не знайдено — розширюємо купу --- */
    block_hdr_t *h = extend_heap(size);
    if (!h) return NULL;
    printf("  [malloc] brk   %6zu байт  @ %p\n", size, hdr_to_data(h));
    return hdr_to_data(h);
}

/* my_free */
void my_free(void *ptr) {
    if (!ptr) return;

    block_hdr_t *h = data_to_hdr(ptr);

    if (h->is_mmap) {
        printf("  [free]   munmap %6zu байт  @ %p\n", h->size, ptr);
        munmap(h, HDR_SIZE + h->size);
        return;
    }

    printf("  [free]   mark  %6zu байт  @ %p\n", h->size, ptr);
    h->free = 1;
    coalesce(h);
}

/* Вивід стану купи (тільки brk-блоки) */
static void print_heap(void) {
    printf("\n--- Стан купи (brk-блоки) ---\n");
    block_hdr_t *cur = heap_list;
    int idx = 0;
    size_t total_free = 0, total_used = 0;
    while (cur) {
        printf("  [%2d] addr=%-14p size=%6zu  %s\n",
               idx++, hdr_to_data(cur), cur->size,
               cur->free ? "ВІЛЬНИЙ" : "зайнятий");
        if (cur->free) total_free += cur->size;
        else           total_used += cur->size;
        cur = cur->next;
    }
    printf("  Зайнято: %zu байт  |  Вільно: %zu байт\n\n",
           total_used, total_free);
}

/* Демонстрація фрагментації */
static void demo_fragmentation(void) {
    printf("=== Демонстрація фрагментації ===\n\n");

    /* 1. Виділяємо 5 блоків різного розміру */
    printf("-- Крок 1: виділяємо 5 блоків --\n");
    void *a = my_malloc(64);
    void *b = my_malloc(128);
    void *c = my_malloc(64);
    void *d = my_malloc(256);
    void *e = my_malloc(64);
    print_heap();

    /* 2. Звільняємо кожен другий блок → фрагментація */
    printf("-- Крок 2: звільняємо b та d (через один) --\n");
    my_free(b);
    my_free(d);
    print_heap();

    /* 3. Намагаємося виділити 300 байт — не вліземо ні в b, ні в d окремо */
    printf("-- Крок 3: запит 300 байт (більше кожного вільного блоку) --\n");
    void *f = my_malloc(300);
    if (!f)
        printf("  [!] Фрагментація! Неможливо задовольнити запит 300 байт,\n"
               "      хоча сумарно вільно більше.\n\n");
    else
        printf("  Виділено @ %p\n\n", f);
    print_heap();

    /* 4. Звільняємо a, c, e → суміжні вільні блоки об'єднуються */
    printf("-- Крок 4: звільняємо a, c, e → coalescing --\n");
    my_free(a);
    my_free(c);
    my_free(e);
    if (f) my_free(f);
    print_heap();
}

/* Демонстрація mmap для великих блоків */
static void demo_mmap(void) {
    printf("=== Демонстрація mmap (блок >= 128 КБ) ===\n\n");

    size_t big = 256 * 1024; /* 256 КБ */
    printf("-- Виділяємо %zu КБ через mmap --\n", big / 1024);
    void *p = my_malloc(big);
    if (p) {
        memset(p, 0xAB, big);
        printf("  Записано %zu КБ у mmap-блок\n", big / 1024);
        my_free(p);
    }
    printf("\n");
}

/* main */
int main(void) {
    printf("Розмір заголовка блоку : %zu байт\n", HDR_SIZE);
    printf("Поріг mmap             : %d байт\n\n", MMAP_THRESHOLD);

    demo_fragmentation();
    demo_mmap();

    printf("=== Завершено ===\n");
    return 0;
}
