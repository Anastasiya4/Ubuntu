# Практична робота №6 — Завдання 1

> **Завдання:** Реалізувати власний динамічний алокатор пам'яті на основі `sbrk()` з підтримкою списку вільних блоків і коалесценції та перевірити його коректність за допомогою Valgrind.

---

## Архітектура алокатора

### Структура блоку

```
┌──────────────────────────────────────────┐
│  size │ free │ magic │ next  ← block_t   │  HDR байт
├──────────────────────────────────────────┤
│          дані користувача                │  size байт
└──────────────────────────────────────────┘
         ↑ повертається з my_malloc()
```

| Поле | Тип | Призначення |
|---|---|---|
| `size` | `size_t` | Розмір корисних даних |
| `free` | `unsigned int` | 1 = вільний, 0 = зайнятий |
| `magic` | `unsigned int` | `0xCAFEBABE` (used) / `0xDEADBEEF` (free) |
| `next` | `block_t*` | Наступний блок у списку |

### Magic-числа
Поле `magic` дозволяє виявити **пошкодження метаданих купи** — якщо значення не збігається з очікуваним, алокатор повідомляє про помилку.

---

## Алгоритми

### Виділення — First Fit
```
my_malloc(size):
  для кожного блоку у free_list:
    якщо блок вільний і достатньо великий:
      split(блок, size)   ← відрізаємо залишок
      повернути блок
  heap_extend(size)       ← sbrk() якщо нічого не знайдено
```

### Розбиття (split)
```
До:    [     FREE 512 байт     ]
       my_malloc(64)
Після: [ USED 64 ][ FREE 432 ]
```

### Коалесценція (coalesce) — при кожному free()
```
До:    [ USED ][ FREE 64 ][ FREE 128 ][ USED ]
Після: [ USED ][   FREE 192          ][ USED ]
```

---

## Компіляція та запуск

```bash
gcc -Wall -Wextra -g -O0 allocator.c -o allocator
./allocator
```

---

## Перевірка через Valgrind

```bash
valgrind \
  --leak-check=full \
  --track-origins=yes \
  --show-leak-kinds=all \
  --error-exitcode=1 \
  ./allocator
```

### Очікуваний звіт Valgrind (коректна програма)
```
==PID== HEAP SUMMARY:
==PID==     in use at exit: 0 bytes in 0 blocks
==PID==   total heap usage: N allocs, N frees, X bytes allocated
==PID==
==PID== All heap blocks were freed -- no leaks are possible
==PID== ERROR SUMMARY: 0 errors from 0 contexts
```

### Якщо додати навмисний витік (для тесту)
```c
void *leak = my_malloc(100);  /* без my_free(leak) */
```
```
==PID== LEAK SUMMARY:
==PID==    definitely lost: 100 bytes in 1 blocks
```

---

## Тести у програмі

| Тест | Що перевіряється |
|---|---|
| `test_null()` | `malloc(0)` → NULL, `free(NULL)` → без краху |
| `test_basic()` | Alloc→Free→Coalesce→Reuse: злитий блок повторно використовується |
| `test_double_free()` | Подвійне звільнення виявляється через прапор `free` |
| `test_data()` | Записані дані читаються коректно (перевірка через `assert`) |

---

## Відмінність від лабораторної №4

| | Лаб. №4 | Лаб. №6 |
|---|---|---|
| Великі блоки | `mmap()` | тільки `sbrk()` |
| Magic-захист | Немає | `MAGIC_FREE` / `MAGIC_USED` |
| Подвійний free | Немає | Виявляється |
| Валідація | Немає | Valgrind + `assert` |

---

## Додаткові команди

```bash
# Побачити системні виклики sbrk/brk
strace -e brk ./allocator

# Перевірка через AddressSanitizer
gcc -fsanitize=address -g -O0 allocator.c -o allocator_asan
./allocator_asan
```
