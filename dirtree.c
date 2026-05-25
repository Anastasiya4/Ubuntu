
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <pwd.h>

/* Константи */
#define MAX_DEPTH   64
#define MAX_PATH   4096
#define MAX_INODES 65536   /* максимум відвіданих inode для виявлення циклів */

/* Таблиця відвіданих inode (для виявлення циклів через bind-mount / hardlink) */
static ino_t visited[MAX_INODES];
static dev_t visited_dev[MAX_INODES];
static int   visited_count = 0;

static int inode_seen(dev_t dev, ino_t ino) {
    for (int i = 0; i < visited_count; i++)
        if (visited[i] == ino && visited_dev[i] == dev)
            return 1;
    return 0;
}

static void inode_add(dev_t dev, ino_t ino) {
    if (visited_count < MAX_INODES) {
        visited[visited_count]     = ino;
        visited_dev[visited_count] = dev;
        visited_count++;
    }
}

static void inode_remove(dev_t dev, ino_t ino) {
    for (int i = 0; i < visited_count; i++) {
        if (visited[i] == ino && visited_dev[i] == dev) {
            visited[i]     = visited[visited_count - 1];
            visited_dev[i] = visited_dev[visited_count - 1];
            visited_count--;
            return;
        }
    }
}

/* Вивід дерева з відступами */
static void print_indent(int depth, int is_last) {
    /* Прості ASCII-відступи */
    for (int i = 0; i < depth - 1; i++) printf("│   ");
    if (depth > 0)
        printf(is_last ? "└── " : "├── ");
}

/* Порівняння для qsort (алфавітний порядок) */
static int cmp_str(const void *a, const void *b) {
    return strcmp(*(const char **)a, *(const char **)b);
}

/* Рекурсивний обхід */
static void walk(const char *path, int depth, int is_last) {
    struct stat st;

    /* lstat — не розгортаємо симлінк одразу */
    if (lstat(path, &st) != 0) {
        perror(path);
        return;
    }

    const char *name = strrchr(path, '/');
    name = name ? name + 1 : path;

    /* --- Якщо це символьне посилання --- */
    if (S_ISLNK(st.st_mode)) {
        char target[MAX_PATH] = {0};
        ssize_t len = readlink(path, target, sizeof(target) - 1);
        if (len < 0) len = 0;
        target[len] = '\0';

        print_indent(depth, is_last);
        printf("\033[36m%s\033[0m -> %s\n", name, target);  /* cyan */

        /* Перевіряємо, чи симлінк веде до директорії, і якщо так —
         * відображаємо її вміст, але перевіряємо на цикл через stat */
        struct stat tst;
        if (stat(path, &tst) == 0 && S_ISDIR(tst.st_mode)) {
            if (inode_seen(tst.st_dev, tst.st_ino)) {
                print_indent(depth + 1, 1);
                printf("\033[33m[цикл — пропускаємо]\033[0m\n");
            }
            /* Не заходимо в симлінк-директорію, щоб уникнути нескінченності;
             * але посилання вже відображено — це і є "зберігаємо символьні" */
        }
        return;
    }

    /* --- Якщо це не директорія — пропускаємо (нас цікавить ієрархія) --- */
    if (!S_ISDIR(st.st_mode)) return;

    /* --- Виявлення циклу через inode --- */
    if (inode_seen(st.st_dev, st.st_ino)) {
        print_indent(depth, is_last);
        printf("\033[33m%s [цикл — пропускаємо]\033[0m\n", name);
        return;
    }
    inode_add(st.st_dev, st.st_ino);

    /* --- Виводимо поточну директорію --- */
    print_indent(depth, is_last);
    printf("\033[34;1m%s/\033[0m\n", depth == 0 ? path : name);  /* blue bold */

    /* --- Читаємо вміст директорії --- */
    DIR *dir = opendir(path);
    if (!dir) {
        print_indent(depth + 1, 1);
        printf("\033[31m[немає доступу]\033[0m\n");
        inode_remove(st.st_dev, st.st_ino);
        return;
    }

    /* Збираємо імена піддиректорій і симлінків */
    char **entries = NULL;
    int    count   = 0;
    int    cap     = 16;
    entries = malloc((size_t)cap * sizeof(char*));
    if (!entries) { closedir(dir); return; }

    struct dirent *de;
    while ((de = readdir(dir)) != NULL) {
        if (strcmp(de->d_name, ".") == 0 ||
            strcmp(de->d_name, "..") == 0) continue;

        /* Перевіряємо тип: директорія або симлінк */
        char child[MAX_PATH];
        snprintf(child, sizeof(child), "%s/%s", path, de->d_name);

        struct stat cs;
        if (lstat(child, &cs) != 0) continue;
        if (!S_ISDIR(cs.st_mode) && !S_ISLNK(cs.st_mode)) continue;

        if (count == cap) {
            cap *= 2;
            char **tmp = realloc(entries, (size_t)cap * sizeof(char*));
            if (!tmp) break;
            entries = tmp;
        }
        entries[count++] = strdup(de->d_name);
    }
    closedir(dir);

    /* Сортуємо алфавітно */
    qsort(entries, (size_t)count, sizeof(char*), cmp_str);

    /* Рекурсивно обходимо */
    for (int i = 0; i < count; i++) {
        char child[MAX_PATH];
        snprintf(child, sizeof(child), "%s/%s", path, entries[i]);
        walk(child, depth + 1, i == count - 1);
        free(entries[i]);
    }
    free(entries);

    inode_remove(st.st_dev, st.st_ino);
}

/* main */
int main(int argc, char *argv[]) {
    char start[MAX_PATH];

    if (argc >= 2) {
        strncpy(start, argv[1], MAX_PATH - 1);
    } else {
        /* За замовчуванням — домашня директорія поточного користувача */
        struct passwd *pw = getpwuid(getuid());
        if (pw)
            strncpy(start, pw->pw_dir, MAX_PATH - 1);
        else if (!getcwd(start, sizeof(start))) {
            perror("getcwd");
            return 1;
        }
    }
    start[MAX_PATH - 1] = '\0';

    /* Видаляємо кінцевий '/' (крім кореня) */
    size_t len = strlen(start);
    if (len > 1 && start[len - 1] == '/')
        start[len - 1] = '\0';

    printf("Ієрархія директорій: %s\n\n", start);
    walk(start, 0, 1);
    printf("\nВідвідано inode: %d\n", visited_count);
    return 0;
}
