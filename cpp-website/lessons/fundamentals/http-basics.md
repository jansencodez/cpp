# HTTP Protocol Basics

The Hypertext Transfer Protocol (HTTP) is the foundation of web communication. Understanding HTTP is crucial for building web servers and APIs.

## HTTP Request Structure

Every HTTP request follows a specific format with three main parts:

1. **Request Line**: Method, URI, and HTTP version
2. **Headers**: Key-value pairs with metadata
3. **Body**: Optional data payload

### HTTP Request Example

```
GET /api/users HTTP/1.1
Host: localhost:8080
User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7)
Accept: application/json
Accept-Language: en-US,en;q=0.9
Accept-Encoding: gzip, deflate
Connection: keep-alive
Cache-Control: no-cache

{
    "name": "John Doe",
    "email": "john@example.com"
}
```

### HTTP Response Structure

```
HTTP/1.1 200 OK
Content-Type: application/json
Content-Length: 45
Access-Control-Allow-Origin: *
Connection: close
Date: Mon, 01 Jan 2024 00:00:00 GMT
Server: C++ HTTP Server/1.0

{"status": "success", "message": "User created"}
```

## HTTP Methods

| Method      | Purpose                       | Example                 |
| ----------- | ----------------------------- | ----------------------- |
| **GET**     | Retrieve data from the server | `GET /api/users`        |
| **POST**    | Create new resources          | `POST /api/users`       |
| **PUT**     | Update existing resources     | `PUT /api/users/123`    |
| **DELETE**  | Remove resources              | `DELETE /api/users/123` |
| **PATCH**   | Partial updates               | `PATCH /api/users/123`  |
| **HEAD**    | Get headers only              | `HEAD /api/users`       |
| **OPTIONS** | Get allowed methods           | `OPTIONS /api/users`    |

## HTTP Status Codes

### 2xx Success

- **200 OK**: Request succeeded
- **201 Created**: Resource created successfully
- **204 No Content**: Success but no body returned

### 4xx Client Error

- **400 Bad Request**: Invalid syntax or parameters
- **401 Unauthorized**: Authentication required
- **403 Forbidden**: Access denied
- **404 Not Found**: Resource doesn't exist
- **405 Method Not Allowed**: HTTP method not supported
- **429 Too Many Requests**: Rate limit exceeded

### 5xx Server Error

- **500 Internal Server Error**: Server encountered an error
- **502 Bad Gateway**: Upstream server error
- **503 Service Unavailable**: Server temporarily unavailable
- **504 Gateway Timeout**: Upstream server timeout

## HTTP Headers

### Common Request Headers

- **Host**: Target hostname and port
- **User-Agent**: Client application identifier
- **Accept**: Preferred response content types
- **Authorization**: Authentication credentials
- **Content-Type**: Request body format
- **Content-Length**: Request body size

### Common Response Headers

- **Content-Type**: Response body format
- **Content-Length**: Response body size
- **Cache-Control**: Caching directives
- **Set-Cookie**: Cookie setting
- **Access-Control-Allow-Origin**: CORS policy

## Implementing HTTP Parser

Here's a basic HTTP request parser implementation:

```cpp
#include <string>
#include <map>
#include <sstream>
#include <algorithm>

struct HttpRequest {
    std::string method;
    std::string path;
    std::string version;
    std::map<std::string, std::string> headers;
    std::map<std::string, std::string> queryParams;
    std::string body;

    bool isValid() const {
        return !method.empty() && !path.empty() && !version.empty();
    }
};

class RequestParser {
public:
    static HttpRequest parse(const std::string& rawRequest) {
        HttpRequest request;
        std::istringstream stream(rawRequest);
        std::string line;

        // Parse request line
        if (std::getline(stream, line)) {
            parseRequestLine(line, request);
        }

        // Parse headers
        while (std::getline(stream, line) && !line.empty() && line != "\r") {
            parseHeader(line, request);
        }

        // Parse body
        parseBody(stream, request);

        return request;
    }

private:
    static void parseRequestLine(const std::string& line, HttpRequest& request) {
        std::istringstream lineStream(line);
        lineStream >> request.method >> request.path >> request.version;

        // Extract query parameters from path
        size_t queryPos = request.path.find('?');
        if (queryPos != std::string::npos) {
            std::string queryString = request.path.substr(queryPos + 1);
            request.path = request.path.substr(0, queryPos);
            parseQueryString(queryString, request);
        }
    }

    static void parseHeader(const std::string& line, HttpRequest& request) {
        size_t colonPos = line.find(':');
        if (colonPos != std::string::npos) {
            std::string key = line.substr(0, colonPos);
            std::string value = line.substr(colonPos + 1);

            // Remove \r and trim whitespace
            if (!value.empty() && value.back() == '\r') {
                value.pop_back();
            }

            // Trim leading whitespace
            value.erase(0, value.find_first_not_of(" \t"));

            request.headers[key] = value;
        }
    }

    static void parseQueryString(const std::string& queryString, HttpRequest& request) {
        std::istringstream stream(queryString);
        std::string param;

        while (std::getline(stream, param, '&')) {
            size_t equalPos = param.find('=');
            if (equalPos != std::string::npos) {
                std::string key = param.substr(0, equalPos);
                std::string value = param.substr(equalPos + 1);
                request.queryParams[key] = value;
            }
        }
    }

    static void parseBody(std::istringstream& stream, HttpRequest& request) {
        std::string body;
        std::string line;

        while (std::getline(stream, line)) {
            body += line + "\n";
        }

        if (!body.empty()) {
            request.body = body;
        }
    }
};
```

## Interactive Exercise

Implement a function to validate HTTP methods:

```cpp
bool isValidHttpMethod(const std::string& method) {
    // TODO: Implement method validation
    // Valid methods: GET, POST, PUT, DELETE, PATCH, HEAD, OPTIONS
    // Return true if valid, false otherwise

    return false;
}

std::string getHttpMethodDescription(const std::string& method) {
    // TODO: Return description for each HTTP method
    // Example: GET -> "Retrieve data from server"

    return "Unknown method";
}
```

## Next Steps

Now that you understand HTTP basics, let's learn about multi-threading for handling multiple clients.

<div class="next-lesson">
    <a href="/course/fundamentals/threading" class="btn btn-primary">Next: Multi-threading and Concurrency →</a>
</div>
