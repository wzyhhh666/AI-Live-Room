#include <iostream>
#include <string>
#include <thread>
#include <atomic>
#include <csignal>
#include <sstream>
#include "client.h"

using namespace chatroom::client;

static std::unique_ptr<ChatClient> g_client;
static std::atomic<bool> g_running{true};

void printHelp() {
    std::cout << "\n========================================" << std::endl;
    std::cout << "  ChatRoom Client Commands:" << std::endl;
    std::cout << "========================================\n" << std::endl;
    std::cout << "  connect <host> <port>   - Connect to server" << std::endl;
    std::cout << "  disconnect              - Disconnect from server" << std::endl;
    std::cout << "  login <username>        - Login to server" << std::endl;
    std::cout << "  logout                  - Logout from server" << std::endl;
    std::cout << "  join <room_id>          - Join a room" << std::endl;
    std::cout << "  leave <room_id>         - Leave current room" << std::endl;
    std::cout << "  danmaku <message>       - Send danmaku message" << std::endl;
    std::cout << "  gift <gift_id> <count>  - Send gift" << std::endl;
    std::cout << "  create <room_name>      - Create new room" << std::endl;
    std::cout << "  close <room_id>         - Close room" << std::endl;
    std::cout << "  status                  - Show connection status" << std::endl;
    std::cout << "  help                    - Show this help" << std::endl;
    std::cout << "  quit                    - Exit client\n" << std::endl;
    std::cout << "========================================\n" << std::endl;
}

void printStatus() {
    std::cout << "\n[Status]" << std::endl;
    std::cout << "  Connected: " << (g_client->isConnected() ? "✓ Yes" : "✗ No") << std::endl;
    std::cout << "  Logged in: " << (g_client ? "✓ Yes" : "✗ No") << std::endl;
    std::cout << "  Current Room: " << "N/A" << std::endl;
}

void signalHandler(int signum) {
    std::cout << "\nReceived signal " << signum << ", shutting down..." << std::endl;
    g_running.store(false);
    
    if (g_client) {
        g_client->disconnect();
    }
}

void commandLoop() {
    std::string line;

    while (g_running.load()) {
        std::cout << "\nchatroom> ";
        
        if (!std::getline(std::cin, line)) break;

        if (line.empty()) continue;

        std::istringstream iss(line);
        std::string cmd;
        iss >> cmd;

        if (cmd == "quit" || cmd == "exit") {
            g_running.store(false);
            break;

        } else if (cmd == "help" || cmd == "?") {
            printHelp();

        } else if (cmd == "status") {
            printStatus();

        } else if (cmd == "connect") {
            std::string host;
            uint16_t port = 8900;
            
            iss >> host;
            if (!(iss >> port)) {
                port = 8900;
            }

            if (!host.empty()) {
                if (g_client->connect(host, port)) {
                    std::cout << "[System] Connected! Use 'login' to authenticate." << std::endl;
                } else {
                    std::cerr << "[Error] Connection failed!" << std::endl;
                }
            } else {
                std::cerr << "[Usage] connect <host> [port]" << std::endl;
            }

        } else if (cmd == "disconnect") {
            g_client->disconnect();
            std::cout << "[System] Disconnected." << std::endl;

        } else if (cmd == "login") {
            std::string username;
            iss >> username;

            if (!username.empty()) {
                if (g_client->login(username)) {
                    std::cout << "[System] Login request sent!" << std::endl;
                }
            } else {
                std::cerr << "[Usage] login <username>" << std::endl;
            }

        } else if (cmd == "logout") {
            g_client->logout();
            std::cout << "[System] Logged out." << std::endl;

        } else if (cmd == "join") {
            uint64_t roomId;
            if (iss >> roomId) {
                g_client->joinRoom(roomId);
            } else {
                std::cerr << "[Usage] join <room_id>" << std::endl;
            }

        } else if (cmd == "leave") {
            uint64_t roomId;
            if (iss >> roomId) {
                g_client->leaveRoom(roomId);
            } else {
                std::cerr << "[Usage] leave <room_id>" << std::endl;
            }

        } else if (cmd == "danmaku" || cmd == "msg" || cmd == "send") {
            std::string message;
            std::getline(iss, message);

            if (!message.empty()) {
                size_t start = message.find_first_not_of(" \t");
                if (start != std::string::npos) {
                    message = message.substr(start);
                }

                uint64_t roomId = 1;
                g_client->sendDanmaku(roomId, message);
            } else {
                std::cerr << "[Usage] danmaku <message>" << std::endl;
            }

        } else if (cmd == "gift") {
            int giftId, count = 1;
            
            if (iss >> giftId) {
                iss >> count;
                
                uint64_t roomId = 1;
                g_client->sendGift(roomId, giftId, count);
            } else {
                std::cerr << "[Usage] gift <gift_id> [count]" << std::endl;
            }

        } else if (cmd == "create") {
            std::string roomName;
            std::getline(iss, roomName);

            if (!roomName.empty()) {
                size_t start = roomName.find_first_not_of(" \t");
                if (start != std::string::npos) {
                    roomName = roomName.substr(start);
                }
                g_client->createRoom(roomName);
            } else {
                std::cerr << "[Usage] create <room_name>" << std::endl;
            }

        } else if (cmd == "close") {
            uint64_t roomId;
            if (iss >> roomId) {
                g_client->closeRoom(roomId);
            } else {
                std::cerr << "[Usage] close <room_id>" << std::endl;
            }

        } else {
            std::cerr << "[Unknown command: " << cmd << ". Type 'help' for commands.]" << std::endl;
        }
    }
}

int main(int argc, char* argv[]) {
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    std::cout << "========================================" << std::endl;
    std::cout << "  ChatRoom Client v1.0" << std::endl;
    std::cout << "========================================\n" << std::endl;

    if (!TcpConnection::initWinsock()) {
        std::cerr << "[Fatal] Failed to initialize network!" << std::endl;
        return 1;
    }

    g_client = std::make_unique<ChatClient>();

    std::cout << "[System] Client initialized successfully!" << std::endl;
    std::cout << "[System] Type 'help' for available commands.\n" << std::endl;

    if (argc > 1) {
        std::string host = "127.0.0.1";
        uint16_t port = 8900;

        host = argv[1];
        if (argc > 2) {
            port = static_cast<uint16_t>(std::stoi(argv[2]));
        }

        std::cout << "[Auto-connect] Connecting to " << host << ":" << port << "..." << std::endl;
        
        if (g_client->connect(host, port)) {
            std::cout << "[Auto-connect] Connected! Logging in as TestUser..." << std::endl;
            g_client->login("TestUser");
        }
    }

    commandLoop();

    if (g_client) {
        g_client->disconnect();
    }

    TcpConnection::cleanupWinsock();

    std::cout << "\n[System] Client shutdown complete. Goodbye!\n" << std::endl;
    return 0;
}
