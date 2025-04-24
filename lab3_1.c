#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h> // для работы с директориями: opendir, readdir, closedir
#include <sys/stat.h>
#include <errno.h>
#include <libgen.h>
#include <fcntl.h> // для O_RDONLY
#include <unistd.h>  // close и др сисколов
#include <limits.h>

#define BLOCK_SIZE 1024 // блок для чтения и записи

// Переворачиваем строку в предоставленный буфер
void reverse_string(const char *str, char *output, size_t max_len) {
    size_t len = strnlen(str, max_len-1);
    for (size_t i = 0; i < len; i++) {
        output[i] = str[len - 1 - i];
    }
    output[len] = '\0';
}

// Переворачиваем блок байтов
void reverse_block(char *buffer, size_t size) {
    for (size_t i = 0; i < size / 2; i++) {
        char temp = buffer[i];
        buffer[i] = buffer[size - 1 - i];
        buffer[size - 1 - i] = temp;
    }
}

int copy_file_reversed(const char *src_path, const char *dest_path) {
    struct stat src_stat;
    if (stat(src_path, &src_stat) < 0) {
        fprintf(stderr, "Error getting metadata for: %s\n", src_path);
        return -1;
    }
     // открываем исходный файл для чтения 
    int src_fd = open(src_path, O_RDONLY);
    if (src_fd == -1) {
        perror("Error opening source file");
        return -1;
    }
    // создал ЦЕЛЕВОЙ файл с теми же правами доступа
    int dest_fd = open(dest_path, O_WRONLY | O_CREAT | O_TRUNC, src_stat.st_mode);
    if (dest_fd == -1) {
        perror("Error creating destination file");
        close(src_fd);
        return -1;
    }
     // man: Функции truncate и ftruncate устанавливают длину файлового дексриптора dest_fd в src_stat.st_size байт
    if (ftruncate(dest_fd, src_stat.st_size) == -1) {
        perror("Error truncating destination file");
         close(src_fd);
         close(dest_fd);
         return -1;
    }
    // буфер для блока
    char buffer[BLOCK_SIZE];
    // обрабатываем файл блоками с конца
    size_t remaining = src_stat.st_size; // количество оставшихся байт - сначала это просто размер исходного файла в байтах
    size_t dest_pos = 0;

    while (remaining > 0) {
        size_t block_size = remaining > BLOCK_SIZE ? BLOCK_SIZE : remaining; // размер текущего блока
        size_t read_pos = remaining - block_size;
        // чтение блока ИЗ src_fd максимум block_size байтов в буфер buffer с позиции read_pos
        ssize_t bytes_read = pread(src_fd, buffer, block_size, read_pos);
        if (bytes_read == -1) {
            perror("Error reading file");
             close(src_fd);
             close(dest_fd);
             return -1;
        }

        reverse_block(buffer, bytes_read);

        ssize_t bytes_written = pwrite(dest_fd, buffer, bytes_read, dest_pos);
        if (bytes_written == -1) {
            perror("Error writing file");
             close(src_fd);
             close(dest_fd);
             return -1;
        }

        dest_pos += bytes_written;
        remaining -= bytes_read;
    }

    close(src_fd);
    close(dest_fd);
    return 0;

}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <directory>\n", argv[0]);
        return 1;
    }

    struct stat st;
    if (stat(argv[1], &st) < 0 || !S_ISDIR(st.st_mode)) {
        fprintf(stderr, "Invalid directory: %s\n", argv[1]);
        return 1;
    }

    // Обрабатываем пути
    char src_path_copy[PATH_MAX];
    strncpy(src_path_copy, argv[1], sizeof(src_path_copy));
    src_path_copy[sizeof(src_path_copy)-1] = '\0';
    
    char parent_dir[PATH_MAX];
    char src_dir_name[PATH_MAX];
    char reversed_dir_name[PATH_MAX];
    
    // Получаем родительский каталог
    strncpy(parent_dir, dirname(src_path_copy), sizeof(parent_dir));
    
    // Получаем имя исходного каталога
    strncpy(src_path_copy, argv[1], sizeof(src_path_copy));
    strncpy(src_dir_name, basename(src_path_copy), sizeof(src_dir_name));
    reverse_string(src_dir_name, reversed_dir_name, sizeof(reversed_dir_name));

    // Формируем целевой путь
    char target_path[PATH_MAX];
    if (snprintf(target_path, sizeof(target_path), "%s/%s", 
                parent_dir, reversed_dir_name) >= sizeof(target_path)) {
        fprintf(stderr, "Path too long: %s/%s\n", parent_dir, reversed_dir_name);
        return 1;
    }

    // Создаем целевой каталог
    if (mkdir(target_path, st.st_mode) < 0 && errno != EEXIST) {
        perror("Error creating target directory");
        return 1;
    }

    DIR *dir = opendir(argv[1]);
    if (!dir) {
        perror("Error opening source directory");
        return 1;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
        continue;
        }
        char full_src_path[PATH_MAX];
        if (snprintf(full_src_path, sizeof(full_src_path), 
                    "%s/%s", argv[1], entry->d_name) >= sizeof(full_src_path)) {
            fprintf(stderr, "Path too long: %s/%s\n", argv[1], entry->d_name);
            continue;
        }

        struct stat file_stat;
        if (stat(full_src_path, &file_stat) < 0 || !S_ISREG(file_stat.st_mode)) {
            fprintf(stderr, "Skipping non-regular file: %s\n", full_src_path);
            continue;
        }

        char reversed_file_name[NAME_MAX];
        reverse_string(entry->d_name, reversed_file_name, sizeof(reversed_file_name));

        char full_dest_path[PATH_MAX];
        if (snprintf(full_dest_path, sizeof(full_dest_path), 
                    "%s/%s", target_path, reversed_file_name) >= sizeof(full_dest_path)) {
            fprintf(stderr, "Path too long: %s/%s\n", target_path, reversed_file_name);
            continue;
        }

        if (copy_file_reversed(full_src_path, full_dest_path) != 0) {
            fprintf(stderr, "Failed to copy: %s -> %s\n", 
                    full_src_path, full_dest_path);
        }
    }

    closedir(dir);
    return 0;
}
