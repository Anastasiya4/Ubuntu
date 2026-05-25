
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Стратегія: виділяємо пам'ять блоками по BLOCK_MB мегабайт,
 * поки malloc не поверне NULL. Зберігаємо вказівники, щоб уникнути
 * оптимізації компілятором. Після вимірювання звільняємо все.
 */

#define BLOCK_MB    16UL
#define BLOCK_BYTES (BLOCK_MB * 1024UL * 1024UL)
#define MAX_BLOCKS  65536  /* до ~1 ТБ — завідомо більше за будь-яку купу */

int main(void) {
    void **blocks = (void**)malloc(MAX_BLOCKS * sizeof(void*));
    if (!blocks) {
        fprintf(stderr, "Не вдалося виділити масив вказівників.\n");
        return 1;
    }

    size_t count = 0;

    printf("Виділення пам'яті блоками по %lu МБ...\n", BLOCK_MB);

    while (count < MAX_BLOCKS) {
        void *ptr = malloc(BLOCK_BYTES);
        if (!ptr) break;

        /* Записуємо один байт у кожен блок, щоб сторінки реально виділились
         * і компілятор не викинув виклик як мертвий код. */
        memset(ptr, 0xAB, BLOCK_BYTES);

        blocks[count++] = ptr;

        if (count % 16 == 0)
            printf("  Виділено: %5lu МБ\n", count * BLOCK_MB);
    }

    double total_mb  = (double)(count * BLOCK_MB);
    double total_gb  = total_mb / 1024.0;

    printf("\n=== Результат ===\n");
    printf("Блоків виділено : %zu\n",   count);
    printf("Розмір блоку    : %lu МБ\n", BLOCK_MB);
    printf("Максимальна купа: %.0f МБ (%.2f ГБ)\n", total_mb, total_gb);

    /* Звільняємо всю пам'ять */
    for (size_t i = 0; i < count; i++)
        free(blocks[i]);
    free(blocks);

    printf("Пам'ять звільнено.\n");
    return 0;
}
