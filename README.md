# Практична робота 12-13 — Варіант 1

> **Завдання:** Написати програму, яка ловить сигнал `SIGSEGV`, зберігає fault address, поточний стек та PID у лог-файл, і намагається перезапустити себе.

---

## Компіляція та запуск

```bash
gcc -Wall -Wextra -g -O0 sigsegv_handler.c -o sigsegv_handler
./sigsegv_handler
```

> Прапорець `-g` обов'язковий — без нього `backtrace_symbols_fd()` не виведе імена функцій.

---

## Очікуваний вивід

```
PID=5678  Запуск #0  (лог: /tmp/sigsegv_crash.log)
Симулюємо SIGSEGV через 1 секунду...
[crash] Перезапуск #1

PID=5679  Запуск #1  (лог: /tmp/sigsegv_crash.log)
Симулюємо SIGSEGV через 1 секунду...
[crash] Перезапуск #2

PID=5680  Запуск #2  (лог: /tmp/sigsegv_crash.log)
Симулюємо SIGSEGV через 1 секунду...
[crash] Перезапуск #3

PID=5681  Запуск #3  (лог: /tmp/sigsegv_crash.log)
Симулюємо SIGSEGV через 1 секунду...
[crash] Досягнуто MAX_RESTART — завершення.
```

### Вміст лог-файлу `/tmp/sigsegv_crash.log`

```
========== CRASH REPORT ==========
Time     : Mon May 25 12:00:01 2026
PID      : 5678
Signal   : 11 (SIGSEGV)
Fault @  : 0x0000000000000000
si_code  : 1 (SEGV_MAPERR)
RIP      : 0x000000000040123a
--- Stack trace ---
./sigsegv_handler(sigsegv_handler+0x...)[0x...]
/lib/x86_64-linux-gnu/libc.so.6(+0x...)[0x...]
./sigsegv_handler(main+0x...)[0x...]
===================================
```

---

## Архітектура програми

```
main()
  │
  ├─ install_handler()  ← sigaction(SIGSEGV, SA_SIGINFO)
  │
  ├─ sleep(1)
  │
  └─ *null_ptr = 42    ← SIGSEGV
          │
          ▼
   sigsegv_handler(sig, siginfo_t*, ucontext_t*)
          │
          ├─ open(LOG_FILE, O_APPEND)
          ├─ safe_write(PID, fault addr, si_code, RIP)
          ├─ backtrace_symbols_fd()
          ├─ close(fd)
          │
          └─ restart_count < MAX_RESTART ?
               ├─ Так → setenv() + execv(argv[0])  ← перезапуск
               └─ Ні  → _exit(1)
```

---

## Ключові рішення

### Чому `safe_write` замість `printf`?

Обробник сигналів виконується **асинхронно** — може перервати будь-яку функцію. `printf` використовує внутрішні буфери (`FILE*`), які можуть бути в непослідовному стані в момент переривання. `write()` — async-signal-safe системний виклик.

### Чому `execv()` замість `fork()`?

`fork()` у обробнику сигналів — небезпечний (не async-signal-safe). `execv()` замінює **поточний** процес новим запуском без створення дочірнього.

### Як передається лічильник перезапусків?

Через змінну середовища `CRASH_RESTART_COUNT` — вона успадковується після `execv()`.

### `si_code` значення

| Значення | Константа | Причина |
|---|---|---|
| 1 | `SEGV_MAPERR` | Звернення до не відображеної адреси (NULL) |
| 2 | `SEGV_ACCERR` | Порушення прав доступу (read-only сторінка) |

---

## Перегляд лог-файлу

```bash
cat /tmp/sigsegv_crash.log

# Очистити лог перед новим запуском
rm /tmp/sigsegv_crash.log
```
