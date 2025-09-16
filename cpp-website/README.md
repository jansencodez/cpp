# C++ HTTP Server

A lightweight, high-performance HTTP server built with C++ using the Crow framework.

## Features

- **Fast & Lightweight**: Built with modern C++17
- **RESTful API**: JSON-based API endpoints
- **CORS Support**: Cross-origin resource sharing enabled
- **Multi-threaded**: Handles concurrent requests efficiently
- **Graceful Shutdown**: Proper signal handling for clean exits
- **Health Monitoring**: Built-in health check endpoint

## Prerequisites

- **C++17 compatible compiler** (GCC 7+, Clang 5+, MSVC 2017+)
- **CMake 3.16+**
- **Git** (for fetching dependencies)

## Building the Server

### 1. Clone and Navigate

```bash
cd /path/to/your/project
```

### 2. Create Build Directory

```bash
mkdir build
cd build
```

### 3. Configure with CMake

```bash
cmake ..
```

### 4. Build the Project

```bash
make -j$(nproc)
```

## Running the Server

### Basic Usage

```bash
# Run on default port 8080
./server

# Run on custom port
./server 3000
```

### Server Information

- **Default Port**: 8080
- **URL**: http://localhost:8080
- **Stop Server**: Press `Ctrl+C`

## API Endpoints

### 1. Root Endpoint

- **URL**: `/`
- **Method**: `GET`
- **Description**: Welcome page with server information
- **Response**: HTML page

### 2. Health Check

- **URL**: `/health`
- **Method**: `GET`
- **Description**: Server health status
- **Response**: JSON

```json
{
  "status": "healthy",
  "timestamp": 1234567890,
  "uptime": "running",
  "version": "1.0.0"
}
```

### 3. Get All Users

- **URL**: `/api/users`
- **Method**: `GET`
- **Description**: Retrieve all users
- **Response**: JSON

```json
{
  "success": true,
  "count": 3,
  "users": [
    {
      "id": "1",
      "name": "John Doe",
      "email": "john@example.com",
      "age": 30
    }
  ]
}
```

### 4. Get User by ID

- **URL**: `/api/users/{id}`
- **Method**: `GET`
- **Description**: Retrieve user by ID
- **Response**: JSON

```json
{
  "success": true,
  "user": {
    "id": "1",
    "name": "John Doe",
    "email": "john@example.com",
    "age": 30
  }
}
```

## Project Structure

```
cpp-server/
├── CMakeLists.txt          # Build configuration
├── config.json             # Server configuration
├── README.md               # This file
├── include/
│   └── Server.h           # Server class header
└── src/
    ├── main.cpp            # Main entry point
    └── Server.cpp          # Server implementation
```

## Configuration

Edit `config.json` to customize server settings:

- **Port**: Server listening port
- **Host**: Binding address
- **Threads**: Number of worker threads
- **CORS**: Cross-origin settings

## Development

### Adding New Endpoints

1. Add route handler declaration in `Server.h`
2. Implement the handler in `Server.cpp`
3. Register the route in `setupRoutes()`

### Example: Adding a POST endpoint

```cpp
// In Server.h
void handleCreateUser(const crow::request& req, crow::response& res);

// In Server.cpp
void Server::handleCreateUser(const crow::request& req, crow::response& res) {
    // Parse JSON body
    auto body = nlohmann::json::parse(req.body);

    // Process request...

    res = createJsonResponse({{"success", true}});
}

// In setupRoutes()
CROW_ROUTE((*app_), "/api/users").methods("POST"_method)
([this](const crow::request& req, crow::response& res) {
    handleCreateUser(req, res);
});
```

## Performance

- **Concurrent Requests**: Multi-threaded architecture
- **Memory Usage**: Efficient C++ memory management
- **Response Time**: Sub-millisecond response times for simple requests
- **Scalability**: Can handle thousands of concurrent connections

## Troubleshooting

### Common Issues

1. **Port Already in Use**

   ```bash
   # Check what's using the port
   lsof -i :8080

   # Kill the process or use different port
   ./server 3000
   ```

2. **Build Errors**

   - Ensure C++17 support is enabled
   - Check CMake version (3.16+)
   - Verify all dependencies are available

3. **Runtime Errors**
   - Check server logs
   - Verify port permissions
   - Ensure firewall allows connections

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests if applicable
5. Submit a pull request

## License

This project is open source and available under the MIT License.

## Support

For issues and questions:

- Check the troubleshooting section
- Review the API documentation
- Open an issue on GitHub
