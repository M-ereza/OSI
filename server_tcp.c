#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h> // inet_ntoa
#include <sys/wait.h>
#include <signal.h>

#define PORT 5000
#define BUFFER_SIZE 1024

ssize_t send_all(int sock_fd, const void *buf, size_t len) {
    ssize_t bytes_sent = 0;
    while (bytes_sent < len) {
        ssize_t bytes_sent_now = send(sock_fd, buf + bytes_sent, len - bytes_sent, 0);
        if (bytes_sent_now < 0) { // если при отправке произошла ошибка (оборвался сокет или что-то еще
            return -1;
        }
        bytes_sent += bytes_sent_now;
    }
    return bytes_sent;
}

/* b. TCP-сервер создает новый процесс, в котором: 
i.читает данные от клиента;
ii.пересылает их ему обратно.
*/
void handle_client(int client_sock_fd) {
    char buffer[BUFFER_SIZE]; // буфер для приема/отправки данных клиента
    ssize_t bytes_read;
    
    // цикл работает пока присвоенный bytes_read результат работы recv - чтения данных с сокета != 0
    while (bytes_read = recv(client_sock_fd, buffer, BUFFER_SIZE, 0)) {
        if (bytes_read < 0) {
            printf("recv failed");
            break;
        }
        buffer[bytes_read] = '\0';
        printf("получено %s\n", buffer);
        // эхо-ответ обратно клиенту 
        // send(client_sock_fd, buffer, bytes_read, 0);
        if (send_all(client_sock_fd, buffer, bytes_read) < 0) {
            printf("send_all (send(s)) failed");
            break;
        }
        memset(buffer, 0, BUFFER_SIZE);
    }

    close(client_sock_fd);
    exit(0);
}

int main() {
    int server_fd, client_sock_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    // создаем TCP сокет
    /*
    socket() создает сокет, возвращает дескриптор сокета
        __domain - AF_INET - протокол IPv4
        __type - SOCK_STREAM - TCP
        __protocol - 0 - выбирается автоматически
    */
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        printf("socket creation failed");
        exit(EXIT_FAILURE);
    }

    // SO_REUSEADDR позволяет переиспользовать порт сразу после завершения сервера. даже если он в состоянии TIME_WAIT
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        printf("setsockopt failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // настройка адреса сервера server_addr
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_family = AF_INET; // IP адрес к которому будет привязан сокет (IPv4)
    server_addr.sin_port = htons(PORT); // номер порта (в сетевом порядке байт) к которому будет привязан сокет

    // привязка сокета к адресу server_addr
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        printf("bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // начало прослушивания (5 потому что в мане прочитал что обычно <= 5)
    if (listen(server_fd, 5) < 0) {
        printf("listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("TCP сервер запущен на порту %d\n", PORT);

    while (1) {
        // принятие нового соединения
        client_sock_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_sock_fd < 0) {
            printf("accept failed");
            continue;
        }

        printf("новое подключение от %s:%d\n", inet_ntoa(client_addr.sin_addr), client_addr.sin_port);

        // создание дочернего процесса
        pid_t pid = fork();
        if (pid < 0) {
            printf("fork failed");
            close(client_sock_fd);
        } 
        
        if (pid == 0) { // дочерний процесс
            close(server_fd); // закрываем слушающий сокет
            handle_client(client_sock_fd);
        } else {
            close(client_sock_fd); // закрываем клиентский сокет
        }
    }

    close(server_fd);
    return 0;
}
