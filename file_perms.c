
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>

#define TEST_DIR  "/tmp/lab9_test"
#define FILE_A    TEST_DIR "/file_other_owner.txt"
#define FILE_B    TEST_DIR "/file_group_write.txt"
#define FILE_C    TEST_DIR "/file_world_write.txt"
#define FILE_D    TEST_DIR "/file_sticky.txt"

/* Допоміжні функції */

/* Вивести права доступу у форматі rwxrwxrwx */
static void print_perms(const char *path) {
    struct stat st;
    if (stat(path, &st) != 0) { perror(path); return; }

    mode_t m = st.st_mode;
    char bits[11];
    bits[0]  = S_ISDIR(m)  ? 'd' : '-';
    bits[1]  = (m & S_IRUSR) ? 'r' : '-';
    bits[2]  = (m & S_IWUSR) ? 'w' : '-';
    bits[3]  = (m & S_IXUSR) ? 'x' : '-';
    bits[4]  = (m & S_IRGRP) ? 'r' : '-';
    bits[5]  = (m & S_IWGRP) ? 'w' : '-';
    bits[6]  = (m & S_IXGRP) ? 'x' : '-';
    bits[7]  = (m & S_IROTH) ? 'r' : '-';
    bits[8]  = (m & S_IWOTH) ? 'w' : '-';
    bits[9]  = (m & S_IXOTH) ? 'x' : '-';
    bits[10] = '\0';

    struct passwd *pw = getpwuid(st.st_uid);
    struct group  *gr = getgrgid(st.st_gid);

    printf("  %s  %s %s  %s\n", bits,
           pw ? pw->pw_name : "?",
           gr ? gr->gr_name : "?",
           path);
}

/* Спроба запису у файл — повертає 1 якщо успішно */
static int try_write(const char *path, const char *data) {
    int fd = open(path, O_WRONLY | O_APPEND);
    if (fd < 0) {
        printf("  write → ВІДМОВЛЕНО (%s)\n", strerror(errno));
        return 0;
    }
    ssize_t n = write(fd, data, strlen(data));
    close(fd);
    if (n < 0) {
        printf("  write → ПОМИЛКА (%s)\n", strerror(errno));
        return 0;
    }
    printf("  write → УСПІХ (%zd байт)\n", n);
    return 1;
}

/* Спроба читання */
static int try_read(const char *path) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        printf("  read  → ВІДМОВЛЕНО (%s)\n", strerror(errno));
        return 0;
    }
    char buf[64] = {0};
    ssize_t n = read(fd, buf, sizeof(buf) - 1);
    close(fd);
    if (n < 0) {
        printf("  read  → ПОМИЛКА (%s)\n", strerror(errno));
        return 0;
    }
    printf("  read  → УСПІХ: \"%.*s\"\n", (int)(n > 32 ? 32 : n), buf);
    return 1;
}

/* Підготовка тестового середовища */
static void setup(void) {
    mkdir(TEST_DIR, 0755);

    /* Створюємо файли з різними правами */
    int fd;

    /* FILE_A: власник root (симулюємо через chmod 000 — ми не root) */
    fd = open(FILE_A, O_CREAT | O_WRONLY | O_TRUNC, 0600);
    if (fd >= 0) { write(fd, "owner_data\n", 11); close(fd); }
    chmod(FILE_A, 0600);  /* тільки власник може читати/писати */

    /* FILE_B: group-write — члени групи можуть писати */
    fd = open(FILE_B, O_CREAT | O_WRONLY | O_TRUNC, 0664);
    if (fd >= 0) { write(fd, "group_data\n", 11); close(fd); }
    chmod(FILE_B, 0664);  /* rw-rw-r-- */

    /* FILE_C: world-write — будь-хто може писати */
    fd = open(FILE_C, O_CREAT | O_WRONLY | O_TRUNC, 0666);
    if (fd >= 0) { write(fd, "world_data\n", 11); close(fd); }
    chmod(FILE_C, 0666);  /* rw-rw-rw- */

    /* FILE_D: world-write у sticky-директорії */
    fd = open(FILE_D, O_CREAT | O_WRONLY | O_TRUNC, 0666);
    if (fd >= 0) { write(fd, "sticky_data\n", 12); close(fd); }
    chmod(FILE_D, 0666);
}

static void cleanup(void) {
    unlink(FILE_A); unlink(FILE_B);
    unlink(FILE_C); unlink(FILE_D);
    rmdir(TEST_DIR);
}

/*  Сценарій 1: файл з правами 0600 (тільки власник) */
static void scenario_owner_only(void) {
    printf("\n--- Сценарій 1: файл 0600 (тільки власник) ---\n");
    print_perms(FILE_A);
    printf("  Поточний UID: %d\n", getuid());

    struct stat st;
    stat(FILE_A, &st);
    if (st.st_uid == getuid()) {
        printf("  Ми є власником → доступ дозволено\n");
    } else {
        printf("  Ми НЕ власник → очікується відмова\n");
    }
    try_read(FILE_A);
    try_write(FILE_A, "append_by_non_owner\n");
}

/* Сценарій 2: файл з правами 0664 (group-write) */
static void scenario_group_write(void) {
    printf("\n--- Сценарій 2: файл 0664 (group-write) ---\n");
    print_perms(FILE_B);

    struct stat st;
    stat(FILE_B, &st);

    /* Перевіряємо, чи ми у групі файлу */
    gid_t groups[64];
    int ng = getgroups(64, groups);
    int in_group = (getgid() == st.st_gid);
    for (int i = 0; i < ng && !in_group; i++)
        if (groups[i] == st.st_gid) in_group = 1;

    printf("  Група файлу: %d, ми %sу цій групі\n",
           st.st_gid, in_group ? "" : "НЕ ");
    printf("  Якщо ми у групі — можемо писати незважаючи на власника\n");

    try_read(FILE_B);
    try_write(FILE_B, "group_append\n");
}

/* Сценарій 3: файл з правами 0666 (world-write) */
static void scenario_world_write(void) {
    printf("\n--- Сценарій 3: файл 0666 (world-write) ---\n");
    print_perms(FILE_C);
    printf("  Будь-який користувач може читати та писати\n");

    try_read(FILE_C);
    try_write(FILE_C, "written_by_anyone\n");
}

/* Сценарій 4: sticky bit на директорії */
static void scenario_sticky(void) {
    printf("\n--- Сценарій 4: sticky bit на директорії ---\n");

    /* Встановлюємо sticky bit на TEST_DIR */
    chmod(TEST_DIR, 01777);  /* rwxrwxrwt */
    print_perms(TEST_DIR);
    print_perms(FILE_D);

    printf("  Sticky bit (+t): навіть якщо файл 0666,\n"
           "  видалити його може тільки власник або root.\n"
           "  Приклад: /tmp має sticky bit.\n");

    /* Запис у файл — можливий (0666) */
    try_write(FILE_D, "write_ok_sticky_dir\n");

    /* Спроба видалення */
    if (unlink(FILE_D) == 0)
        printf("  unlink → УСПІХ (ми власник файлу)\n");
    else
        printf("  unlink → ВІДМОВЛЕНО (%s)\n", strerror(errno));

    chmod(TEST_DIR, 0755);
}

/* Сценарій 5: ACL — концептуальне пояснення */
static void scenario_acl_info(void) {
    printf("\n--- Сценарій 5: розширені списки доступу (ACL) ---\n");
    printf("  ACL дозволяє надати доступ конкретному користувачеві\n"
           "  незалежно від стандартних прав UNIX.\n\n"
           "  Команди:\n"
           "    setfacl -m u:alice:rw file.txt   # alice може rw\n"
           "    getfacl file.txt                  # переглянути ACL\n\n"
           "  Перевірити підтримку ACL:\n"
           "    mount | grep acl\n"
           "    tune2fs -l /dev/sda1 | grep acl\n\n"
           "  Якщо ACL встановлено — звичайні chmod-права обходяться\n"
           "  для конкретних користувачів.\n");
}

/* Підсумкова таблиця */
static void print_summary(void) {
    printf("\n=== Підсумок сценаріїв ===\n");
    printf("  %-35s %s\n", "Сценарій", "Можна змінити без власника?");
    printf("  %-35s %s\n", "---", "---");
    printf("  %-35s %s\n", "0600 (тільки власник)",    "Ні");
    printf("  %-35s %s\n", "0664 (group-write)",       "Так, якщо у групі");
    printf("  %-35s %s\n", "0666 (world-write)",       "Так, будь-хто");
    printf("  %-35s %s\n", "Sticky dir + 0666",        "Писати: Так / Видалити: Ні");
    printf("  %-35s %s\n", "ACL (setfacl)",            "Так, явно дозволено");
    printf("  %-35s %s\n", "root",                     "Завжди Так");
}

/* main */
int main(void) {
    printf("Практична робота №9 — Варіант 1\n");
    printf("Зміна файлу без прав власника\n");
    printf("================================\n");
    printf("Запущено як UID=%d GID=%d\n", getuid(), getgid());

    setup();

    scenario_owner_only();
    scenario_group_write();
    scenario_world_write();
    scenario_sticky();
    scenario_acl_info();
    print_summary();

    cleanup();
    printf("\nТестові файли видалено.\n");
    return 0;
}
