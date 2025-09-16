# Socket Programming Fundamentals

Sockets are the foundation of network communication in modern computing. They provide an interface between your application and the network stack, allowing you to send and receive data over various protocols.

## What are Sockets?

A socket is an endpoint for communication between two machines. Think of it as a "phone" that your application uses to talk to other applications across the network.

### Socket Types

- **Stream Sockets (SOCK_STREAM)**: TCP-based, reliable, ordered data transfer
- **Datagram Sockets (SOCK_DGRAM)**: UDP-based, unreliable, unordered data transfer
- **Raw Sockets**: Direct access to lower-level protocols

## Basic Socket Creation

Here's a complete example of creating and configuring a socket:

```cpp
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <cstring>

int main() {
    // Create a socket
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        std::cerr << "Failed to create socket: " << strerror(errno) << std::endl;
        return 1;
    }

    // Set socket options for address reuse
    int opt = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "setsockopt failed: " << strerror(errno) << std::endl;
        close(serverSocket);
        return 1;
    }

    // Configure socket address
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(8080);

    // Bind socket to address
    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Bind failed: " << strerror(errno) << std::endl;
        close(serverSocket);
        return 1;
    }

    // Listen for connections
    if (listen(serverSocket, 10) < 0) {
        std::cerr << "Listen failed: " << strerror(errno) << std::endl;
        close(serverSocket);
        return 1;
    }

    std::cout << "Socket created and bound successfully on port 8080!" << std::endl;

    // Accept a connection
    struct sockaddr_in clientAddr;
    socklen_t clientLen = sizeof(clientAddr);
    int clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientLen);

    if (clientSocket < 0) {
        std::cerr << "Accept failed: " << strerror(errno) << std::endl;
    } else {
        std::cout << "Client connected from " << inet_ntoa(clientAddr.sin_addr) << std::endl;
        close(clientSocket);
    }

    close(serverSocket);
    return 0;
}
```

## Key Concepts

### Address Families

- **AF_INET**: IPv4 addresses (most common)
- **AF_INET6**: IPv6 addresses
- **AF_UNIX**: Local communication (Unix domain sockets)

### Socket Options

- **SO_REUSEADDR**: Allow address reuse (prevents "Address already in use" errors)
- **SO_KEEPALIVE**: Keep connection alive
- **SO_LINGER**: Control close behavior
- **SO_RCVBUF/SO_SNDBUF**: Set receive/send buffer sizes

### Error Handling

Always check return values and use proper error reporting:

```cpp
if (result < 0) {
    std::cerr << "Operation failed: " << strerror(errno) << std::endl;
    // Clean up resources
    return false;
}
```

## Interactive Exercise

Try implementing a socket creation function with proper error handling:

```cpp
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>

int createSocket(int port) {
    // TODO: Implement socket creation with error handling
    // 1. Create socket
    // 2. Validate port range (1-65535)
    // 3. Set socket options
    // 4. Bind to address
    // 5. Handle errors gracefully
    // 6. Return socket descriptor or -1 on error

    return -1; // Placeholder
}

bool validatePort(int port) {
    // TODO: Implement port validation
    // Port should be between 1 and 65535
    return false;
}
```

## Common Socket Functions

| Function        | Purpose                         | Return Value                |
| --------------- | ------------------------------- | --------------------------- |
| `socket()`      | Create a new socket             | Socket descriptor or -1     |
| `bind()`        | Bind socket to address          | 0 on success, -1 on failure |
| `listen()`      | Start listening for connections | 0 on success, -1 on failure |
| `accept()`      | Accept incoming connection      | Client socket or -1         |
| `connect()`     | Connect to remote server        | 0 on success, -1 on failure |
| `send()/recv()` | Send/receive data               | Bytes sent/received or -1   |
| `close()`       | Close socket                    | 0 on success, -1 on failure |

## Next Steps

Now that you understand socket basics, let's move on to HTTP protocol implementation.

<div class="next-lesson">
    <a href="/course/fundamentals/http-basics" class="btn btn-primary">Next: HTTP Protocol Basics →</a>
</div>
