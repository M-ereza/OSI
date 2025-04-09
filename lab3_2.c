#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> // close и др сисколы
#include <dirent.h> // для работы с директориями: opendir, readdir, closedir
#include <sys/stat.h> // mkdir
#include <libgen.h> // basename
#include <fcntl.h> // O_WRONLY, O_CREAT, O_TRUNC и тд

struct command {
    char *name;
    int (*func)(char *argv[]);
};

struct command entries[] = {
    {"create_dir", create_dir},
    {"list_dir", list_dir},
    {"remove_dir", remove_dir},

    {"create_file", create_file},
    {"read_file", print_file},
    {"remove_file", remove_file},

    {"create_symlink", create_symlink},
    {"print_symlink", print_symlink},
    {"print_symlink_file", print_symlink_file},
    {"remove_symlink", remove_symlink},

    {"create_hardlink", create_hardlink},
    {"remove_hardlink", remove_hardlink},
    {"print_file_info", print_file_info},
    {"change_file_rights", change_file_rights},
    
    {NULL, NULL} // для завершения цикла
};

// a - создать каталог, переданный аргументом
int create_dir(char *argv[]) {
    if (mkdir(argv[1], 0755) != 0) {
        printf("ошибка mkdir при создании каталога: %s\n", argv[1]);
        return -1;
    }

    return 0;
}

// b - вывести содержимое каталога, указанного в аргументе
int list_dir(char *argv[]) {
    DIR *dir = opendir(argv[1]);
    if (!dir) {
        printf("ошибка opendir при открытии каталога: %s\n", argv[1]);
        return -1;
    }

    printf("Содержимое каталога: %s\n", argv[1]);

    struct dirent *entry;
    /* логика: readdir-ом читаю по элементу исходного каталога dir, в entry лежит укзаатель на структуру dirent
       dirent есть стракт в котором лежит 2 поля: d_ino - file serial number; d_name - name of file
    */
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        printf("%s\n", entry->d_name);
    }

    closedir(dir);
    return 0;
}

// c - удалить каталог, указанный в аргументе
int remove_dir(char *argv[]) {
    if (rmdir(argv[1]) != 0) {
        printf("ошибка rmdir при удалении каталога: %s\n", argv[1]);
        return -1;
    }

    return 0;
}

// d - создать файл, указанный в аргументе
int create_file(char *argv[]) {
    int fd = open(argv[1], O_WRONLY | O_CREAT | O_TRUNC, 0755);
    if (fd == -1) {
        printf("ошибка open при создании файла: %s\n", argv[1]);
        return -1;
    }

    close(fd);
    return 0;
}

// e - вывести содержимое файла, указанного в аргументе
int print_file(char *argv[]) {
    FILE *file = fopen(argv[1], "r");
    if (!file) {
        printf("ошибка fopen при открытии файла: %s\n", argv[1]);
        return -1;
    }

    printf("Содержимое файла: %s\n", argv[1]);

    char c;
    while ((c = fgetc(file)) != EOF) {
        printf("%c", c);
    }

    fclose(file);
    return 0;
}

// f - удалить файл, указанный в аргументе
int remove_file(char *argv[]) {
    if (remove(argv[1]) != 0) {
        printf("ошибка remove при удалении файла: %s\n", argv[1]);
        return -1;
    }

    return 0;
}

// g - создать символьную ссылку на файл, указанный в аргументе
int create_symlink(char *argv[]) {
    if (symlink(argv[1], argv[2])) {
        printf("ошибка symlink при создании символьной ссылки: %s -> %s\n", argv[1], argv[2]);
        return -1;
    }

    return 0;
}

// h - вывести содержимое символьной ссылки, указанный в аргументе
int print_symlink(char *argv[]) {
    char buf[1024];
    // int readlink(const char *path, char *buf, size_t bufsiz);
    // помещает содержимое символьной ссылки path в буфер buf длиной bufsiz
    int len = readlink(argv[1], buf, sizeof(buf)-1);
    if (len == -1) {
        printf("ошибка при чтении симлинка: %s\n", argv[1]);
        return -1;
    }
    buf[len] = '\0'; // из мана: readlink не добавляет в buf символ NUL. Сам добавляю идентификатор конца строки
    
    printf("Симлинк %s указывает на: %s\n", argv[1], buf);
    
    return 0;
}

// i - вывести содержимое файла, на который указывает символьная ссылка, указанная в аргументе
int print_symlink_file(char *argv[]) {
    char buf[1024];
    int len = readlink(argv[1], buf, sizeof(buf)-1);
    if (len == -1) {
        printf("Ошибка при чтении симлинка: %s\n", argv[1]);
        return -1;
    }
    buf[len] = '\0';

    // хочу применить функцию print_file
    char *file_argv[2];
    file_argv[0] = "print_file"; // любое значение, не важно
    file_argv[1] = buf;
    print_file(file_argv);

    return 0;
}

// j - удалить символьную ссылку на файл, указанный в аргументе
int remove_symlink(char *argv[]) {
    if (unlink(argv[1]) != 0) {
        printf("ошибка unlink при удалении символьной ссылки: %s\n", argv[1]);
        return -1;
    }

    return 0;
}

// k - создать жесткую ссылку на файл, указанный в аргументе
int create_hardlink(char *argv[]) {
    if (link(argv[1], argv[2]) != 0) {
        printf("Ошибка link при создании жесткой ссылки: %s -> %s\n", argv[1], argv[2]);
        return -1;
    }

    return 0;
}

// l - удалить жесткую ссылку на файл, указанный в аргументе
int remove_hardlink(char *argv[]) {
    if (unlink(argv[1]) != 0) {
        printf("Ошибка удаления ссылки: %s\n", argv[1]);
        return -1;
    }

    return 0;
}

// m - вывести права доступа к файлу, указанному в аргументе и количество жестких ссылок на него
int print_file_info(char *argv[]) {
    struct stat st;
    if (stat(argv[1], &st) != 0) {
        printf("Ошибка stat при получении метаданных файла: %s\n", argv[1]);
        return -1;
    }
    
    printf("Информация о файле %s:\n", argv[1]);
    printf("1 - права доступа: %o\n", st.st_mode & 0777); // 0777 - маской зануляю биты не отвечающие за права доступа
    printf("2 - количество жестких ссылок: %ld\n", st.st_nlink);

    return 0;
}

// n - изменить права доступа к файлу, указанному в аргументе
int change_file_rights(char *argv[]) {
    // Convert a string to a long integer.
    // long strtol(const char *start, char **end, int radix)
    // выбрал восьмиричную систему счисления тк права доступа до 7 включительно
    mode_t new_mode = strtol(argv[2], NULL, 8);
    if (chmod(argv[1], new_mode) != 0) {
        printf("ошибка chmod при изменении прав доступа к файлу: %s\n", argv[1]);
        return -1;
    }

    return 0;
}



int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("неверное количество аргументов\n");
        return -1;
    }

    char *cmd = basename(argv[0]);

    int i;
    for (i = 0; entries[i].name != NULL; i++) {
        if (strcmp(cmd, entries[i].name) == 0) {
            if (entries[i].func(argv) == 0) {
                printf("\nmain: команда %s выполнена: %s\n", cmd, argv[1]);
            } else {
                printf("\nmain: команда %s не выполнена: %s\n", cmd, argv[1]);
            }
            return 0;
        }
    }

    return 0;
}
