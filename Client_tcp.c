#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 5000
#define BUFFER_SIZE 1024

int main() {
    int client_fd = 0;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE] = {0};

    // создаем TCP сокет
    /*
    socket() создает сокет, возвращает дескриптор сокета
        __domain - AF_INET - протокол IPv4
        __type - SOCK_STREAM - TCP
        __protocol - 0 - выбирается автоматически
    */
    client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd < 0) {
        printf("socket creation failed");
        exit(EXIT_FAILURE);
    }

    // настройка адреса сервера server_addr
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_family = AF_INET; // IP адрес к которому будет привязан сокет (IPv4)
    server_addr.sin_port = htons(PORT); // номер порта (в сетевом порядке байт) к которому будет привязан сокет
    
    if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0) {
        printf("address conversion failed");
        close(client_fd);
        exit(EXIT_FAILURE);
    }

    // подключение к серверу
    if (connect(client_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        printf("connect failed");
        close(client_fd);
        exit(EXIT_FAILURE);
    }

    printf("введите сообщение серверу: ");
    fgets(buffer, BUFFER_SIZE, stdin);
    buffer[strcspn(buffer, "\n")] = '\0'; // удалил символ новой строки

    // отправка данных на сервер
    if (send(client_fd, buffer, strlen(buffer), 0) < 0) {
        printf("send error");
        close(client_fd);
        exit(EXIT_FAILURE);
    }
    printf("отправлено: %s\n", buffer);

    // получение от сервера ответа
    ssize_t recv_len = recv(client_fd, buffer, BUFFER_SIZE, 0);
    if (recv_len < 0) {
        printf("recv failed");
        close(client_fd);
        exit(EXIT_FAILURE);
    }
    buffer[recv_len] = '\0';

    printf("получено эхо-сообщение: %s\n", buffer);

    close(client_fd);
    return 0;
}
