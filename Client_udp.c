#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h> // inet_ntoa

#define BUFFER_SIZE 1024
#define PORT 5000

int main() {
    int client_fd;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];

    // создаем UDP сокет
    /*
    socket() создает сокет, возвращает дескриптор сокета
        __domain - AF_INET - протокол IPv4
        __type - SOCK_DGRAM - UDP
        __protocol - 0 - выбирается автоматически
    */
    client_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (client_fd < 0) {
        printf("socket creation failed");
        exit(EXIT_FAILURE);
    }

    // настройка адреса сервера server_addr
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    
    // inet_pton преобразует IP адрес из строкового формата (2 аргумент) в бинарный, понятный сетевым функциям (3-й)
    if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0) {
        printf("address conversion failed");
        close(client_fd);
        exit(EXIT_FAILURE);
    }

    printf("введите сообщение серверу: ");
    fgets(buffer, BUFFER_SIZE, stdin);
    buffer[strcspn(buffer, "\n")] = '\0'; // удалил символ новой строки 

    /*
    sendto() отправляет сообщение серверу через сокет: 
        client_fd - дескриптор сокета
        buffer - указатель на буфер с данными
        strlen(buffer) - размер сообщения
        0 - флаги
        (sockaddr*)&server_addr - адрес получателя 
        sizeof(server_addr) - его размер соответственно
    */
    ssize_t sent_len = sendto(client_fd, buffer, strlen(buffer), 0,
        (struct sockaddr*)&server_addr, sizeof(server_addr));
    
    if (sent_len < 0) {
        printf("sendto failed");
        close(client_fd);
        exit(EXIT_FAILURE);
    }

    /*
    recfvrom() получает сообщение от сервера через сокет:
        client_fd - дескриптор сокета
        buffer - буфер для данными
        BUFFER_SIZE - сколько байт считать из буффера 
        0 - флаги
        NULL - адрес получателя (нас не интересует)
        NULL - размер получателя
    */
    // можно использовать recv 
    ssize_t recv_len = recvfrom(client_fd, buffer, BUFFER_SIZE, 0, NULL, NULL);
    if (recv_len < 0) {
        printf("recvfrom failed");
        close(client_fd);
        exit(EXIT_FAILURE);
    }
    buffer[recv_len] = '\0';

    printf("получено эхо-сообщение: %s\n", buffer);

    close(client_fd);
    return 0;
}
