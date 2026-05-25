# Практична робота №5 — Варіант 1

> **Завдання:** Змоделювати читання за межами масиву, яке не викликає segfault, але змінює логіку обчислень через "витік" сусідньої змінної зі стеку.

---

## Опис

Програма містить три демонстрації одного класу помилки — **out-of-bounds читання зі стеку**:

| Демонстрація | Ефект |
|---|---|
| 1. `i <= 4` замість `i < 4` | `sum_bad` містить "сміттєве" значення сусідньої змінної |
| 2. Перевірка прав (`flags[3]`) | Звичайний користувач отримує права адміністратора |
| 3. Адреси змінних | Наочно показує розташування змінних на стеку |

---

## Чому немає segfault?

```
Стек — одна велика сторінка пам'яті (зазвичай 8 МБ).
arr[4] виходить за межі МАСИВУ, але НЕ за межі СТОРІНКИ.
MMU не бачить порушення → segfault не виникає.

Вищі адреси
┌──────────────┐
│  secret = 99 │  ← arr[4] читає тут
├──────────────┤
│  arr[3] = 40 │
│  arr[2] = 30 │
│  arr[1] = 20 │
│  arr[0] = 10 │
└──────────────┘
Нижчі адреси
```

---

## Компіляція та запуск

```bash
# Базова компіляція (без захисту стека для відтворюваності)
gcc -Wall -Wextra -O0 -fno-stack-protector stack_leak.c -o stack_leak
./stack_leak
```

### Аналіз через Valgrind
```bash
valgrind --track-origins=yes --error-exitcode=1 ./stack_leak
```

Valgrind виведе щось на кшталт:
```
==PID== Invalid read of size 4
==PID==    at 0x...: demo_stack_read (stack_leak.c:52)
==PID==  Address 0x... is 0 bytes after a block of size 16
```

### Аналіз через AddressSanitizer (ASan)
```bash
gcc -fsanitize=address,undefined -O0 -fno-stack-protector \
    stack_leak.c -o stack_asan
./stack_asan
```

ASan зупинить програму одразу при виході за межі:
```
ERROR: AddressSanitizer: stack-buffer-overflow on address 0x...
READ of size 4 at 0x... thread T0
    #0 demo_stack_read stack_leak.c:52
```

---

## Небезпека у реальному коді

Демонстрація 2 моделює реальний клас вразливостей:

```c
/* ВРАЗЛИВИЙ КОД */
int flags[3] = {0, 0, 0};
int is_admin = 1;            /* на стеку після flags */

for (int i = 0; i <= 3; i++) /* має бути i < 3 */
    if (flags[i]) return 1;  /* зчитує is_admin → повертає "адмін"! */
```

Це нагадує реальні CVE, де off-by-one у перевірці меж призводить до обходу авторизації.

---

## Порівняння інструментів виявлення

| Інструмент | Команда | Виявляє? |
|---|---|---|
| GCC `-Wall` | `gcc -Wall` | Частково (попередження) |
| Valgrind | `valgrind --track-origins=yes` | Так (runtime) |
| ASan | `-fsanitize=address` | Так (runtime, швидше) |
| UBSan | `-fsanitize=undefined` | Так (UB) |
| Static analyzer | `gcc -fanalyzer` | Іноді (compile-time) |
