#include <iostream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

int main(){
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(8080);

    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    bind(server_socket, (sockaddr*)&addr, sizeof(addr));
    listen(server_socket, 10);

    std::cout << "Server listening on port 8080..." << std::endl;
    while (true) {
        int client_socket = accept(server_socket, nullptr, nullptr);
        char buffer[1024];
        memset(buffer, 0, sizeof(buffer));
        read(client_socket, buffer, sizeof(buffer) - 1);
        std::cout << "Received: " << buffer << std::endl;
        const char* response = "HTTP/1.1 200 OK\r\nContent-Length: 13\r\n\r\nHello, World!";
        send(client_socket, response, strlen(response), 0);
        close(client_socket);
    }
    close(server_socket);
    return 0;
}