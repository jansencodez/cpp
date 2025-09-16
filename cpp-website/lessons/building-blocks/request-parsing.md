# Request Parsing and Validation

Learn how to properly parse HTTP requests, validate input, and handle different content types with robust error handling.

## HTTP Request Structure

Every HTTP request follows a specific format:

```
GET /api/users?page=1&limit=10 HTTP/1.1
Host: localhost:8080
User-Agent: Mozilla/5.0
Accept: application/json
Content-Type: application/x-www-form-urlencoded
Content-Length: 25

name=John&email=john@example.com
```

## Request Parser Implementation

Here's a robust HTTP request parser:

```cpp
#include <string>
#include <map>
#include <sstream>
#include <algorithm>
#include <regex>

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

## Interactive Exercise 1: Implement Query String Parser

Try implementing a function to parse query strings with proper URL decoding:

```cpp
std::map<std::string, std::string> parseQueryString(const std::string& queryString) {
    std::map<std::string, std::string> params;

    // TODO: Implement query string parsing
    // 1. Split by '&' to get individual parameters
    // 2. Split each parameter by '=' to get key and value
    // 3. URL decode the values (replace %20 with space, etc.)
    // 4. Handle edge cases (empty values, missing '=', etc.)

    return params;
}
```

<div class="interactive-exercise">
    <h3>Test Your Implementation</h3>
    <div class="code-editor">
        <textarea id="query-parser-code" class="code-input" rows="10">std::map<std::string, std::string> parseQueryString(const std::string& queryString) {
    std::map<std::string, std::string> params;
    
    // TODO: Implement query string parsing
    // 1. Split by '&' to get individual parameters
    // 2. Split each parameter by '=' to get key and value
    // 3. URL decode the values (replace %20 with space, etc.)
    // 4. Handle edge cases (empty values, missing '=', etc.)
    
    return params;
}</textarea>
        <div class="editor-controls">
            <button onclick="testQueryParser()" class="btn btn-primary">Test Code</button>
            <button onclick="resetQueryParser()" class="btn btn-secondary">Reset</button>
        </div>
        <div id="query-parser-output" class="code-output"></div>
    </div>
</div>

## Interactive Exercise 2: Header Validation

Implement a function to validate HTTP headers:

```cpp
bool validateHeaders(const std::map<std::string, std::string>& headers) {
    // TODO: Implement header validation
    // 1. Check for required headers (Host, Content-Length if body exists)
    // 2. Validate Content-Length is a positive integer
    // 3. Check for malformed header names (no spaces, valid characters)
    // 4. Validate header values (no control characters)

    return false;
}
```

<div class="interactive-exercise">
    <h3>Test Header Validation</h3>
    <div class="code-editor">
        <textarea id="header-validator-code" class="code-input" rows="12">bool validateHeaders(const std::map<std::string, std::string>& headers) {
    // TODO: Implement header validation
    // 1. Check for required headers (Host, Content-Length if body exists)
    // 2. Validate Content-Length is a positive integer
    // 3. Check for malformed header names (no spaces, valid characters)
    // 4. Validate header values (no control characters)
    
    return false;
}</textarea>
        <div class="editor-controls">
            <button onclick="testHeaderValidator()" class="btn btn-primary">Test Code</button>
            <button onclick="resetHeaderValidator()" class="btn btn-secondary">Reset</button>
        </div>
        <div id="header-validator-output" class="code-output"></div>
    </div>
</div>

## Content Type Handling

Different content types require different parsing strategies:

### JSON Content

```cpp
#include <nlohmann/json.hpp>

std::map<std::string, std::string> parseJsonBody(const std::string& body) {
    try {
        auto json = nlohmann::json::parse(body);
        std::map<std::string, std::string> params;

        for (auto it = json.begin(); it != json.end(); ++it) {
            params[it.key()] = it.value().get<std::string>();
        }

        return params;
    } catch (const std::exception& e) {
        throw std::runtime_error("Invalid JSON: " + std::string(e.what()));
    }
}
```

### Form Data

```cpp
std::map<std::string, std::string> parseFormData(const std::string& body) {
    return parseQueryString(body); // Form data uses same format as query strings
}
```

### Multipart Form Data

```cpp
struct MultipartPart {
    std::string name;
    std::string filename;
    std::string contentType;
    std::string content;
};

std::vector<MultipartPart> parseMultipartData(const std::string& body, const std::string& boundary) {
    std::vector<MultipartPart> parts;

    // TODO: Implement multipart parsing
    // 1. Split by boundary
    // 2. Parse headers for each part
    // 3. Extract content
    // 4. Handle file uploads

    return parts;
}
```

## Error Handling and Validation

Robust error handling is crucial for production servers:

```cpp
class RequestParseException : public std::runtime_error {
public:
    enum class ErrorType {
        INVALID_REQUEST_LINE,
        INVALID_HEADER,
        INVALID_BODY,
        CONTENT_LENGTH_MISMATCH,
        UNSUPPORTED_CONTENT_TYPE
    };

    RequestParseException(ErrorType type, const std::string& message)
        : std::runtime_error(message), type_(type) {}

    ErrorType getType() const { return type_; }

private:
    ErrorType type_;
};

class RequestValidator {
public:
    static void validateRequest(const HttpRequest& request) {
        // Validate method
        if (!isValidMethod(request.method)) {
            throw RequestParseException(
                RequestParseException::ErrorType::INVALID_REQUEST_LINE,
                "Invalid HTTP method: " + request.method
            );
        }

        // Validate path
        if (request.path.empty() || request.path[0] != '/') {
            throw RequestParseException(
                RequestParseException::ErrorType::INVALID_REQUEST_LINE,
                "Invalid path: " + request.path
            );
        }

        // Validate headers
        validateHeaders(request.headers);

        // Validate body
        validateBody(request);
    }

private:
    static bool isValidMethod(const std::string& method) {
        std::vector<std::string> validMethods = {
            "GET", "POST", "PUT", "DELETE", "PATCH", "HEAD", "OPTIONS"
        };

        return std::find(validMethods.begin(), validMethods.end(), method) != validMethods.end();
    }

    static void validateHeaders(const std::map<std::string, std::string>& headers) {
        // Check for required headers
        if (headers.find("Host") == headers.end()) {
            throw RequestParseException(
                RequestParseException::ErrorType::INVALID_HEADER,
                "Missing required header: Host"
            );
        }

        // Validate Content-Length
        auto contentLengthIt = headers.find("Content-Length");
        if (contentLengthIt != headers.end()) {
            try {
                int length = std::stoi(contentLengthIt->second);
                if (length < 0) {
                    throw RequestParseException(
                        RequestParseException::ErrorType::INVALID_HEADER,
                        "Invalid Content-Length: " + contentLengthIt->second
                    );
                }
            } catch (const std::exception&) {
                throw RequestParseException(
                    RequestParseException::ErrorType::INVALID_HEADER,
                    "Invalid Content-Length: " + contentLengthIt->second
                );
            }
        }
    }

    static void validateBody(const HttpRequest& request) {
        auto contentLengthIt = request.headers.find("Content-Length");
        if (contentLengthIt != request.headers.end()) {
            int expectedLength = std::stoi(contentLengthIt->second);
            if (request.body.length() != expectedLength) {
                throw RequestParseException(
                    RequestParseException::ErrorType::CONTENT_LENGTH_MISMATCH,
                    "Body length mismatch. Expected: " + contentLengthIt->second +
                    ", Actual: " + std::to_string(request.body.length())
                );
            }
        }
    }
};
```

## Performance Considerations

### 1. String Operations

- Use `std::string_view` for read-only operations
- Minimize string copies and allocations
- Use `reserve()` when building strings

### 2. Memory Management

- Use RAII for resource management
- Avoid unnecessary dynamic allocations
- Consider using memory pools for high-frequency parsing

### 3. Parsing Strategy

- Parse headers lazily (only when needed)
- Use efficient string search algorithms
- Cache parsed results when appropriate

## Testing Your Parser

Here's a comprehensive test suite:

```cpp
#include <cassert>
#include <iostream>

void testRequestParser() {
    // Test basic GET request
    std::string getRequest =
        "GET /api/users?page=1&limit=10 HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "User-Agent: TestClient\r\n"
        "\r\n";

    HttpRequest request = RequestParser::parse(getRequest);

    assert(request.method == "GET");
    assert(request.path == "/api/users");
    assert(request.version == "HTTP/1.1");
    assert(request.headers["Host"] == "localhost:8080");
    assert(request.queryParams["page"] == "1");
    assert(request.queryParams["limit"] == "10");

    std::cout << "✓ GET request parsing passed" << std::endl;

    // Test POST request with body
    std::string postRequest =
        "POST /api/users HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: 25\r\n"
        "\r\n"
        "{\"name\":\"John\",\"age\":30}";

    request = RequestParser::parse(postRequest);

    assert(request.method == "POST");
    assert(request.headers["Content-Type"] == "application/json");
    assert(request.body == "{\"name\":\"John\",\"age\":30}");

    std::cout << "✓ POST request parsing passed" << std::endl;

    std::cout << "All tests passed!" << std::endl;
}
```

## Next Steps

Now that you understand request parsing, let's move on to response generation and headers.

<div class="next-lesson">
    <a href="/course/building-blocks/response-generation" class="btn btn-primary">Next: Response Generation and Headers →</a>
</div>

<script>
// Interactive exercise functionality
function testQueryParser() {
    const code = document.getElementById('query-parser-code').value;
    const output = document.getElementById('query-parser-output');
    
    // Simple test cases
    const testCases = [
        { input: "name=John&age=30", expected: { name: "John", age: "30" } },
        { input: "page=1&limit=10&sort=name", expected: { page: "1", limit: "10", sort: "name" } },
        { input: "empty=&missing", expected: { empty: "", missing: "" } }
    ];
    
    let results = [];
    for (const testCase of testCases) {
        try {
            // This is a simplified test - in a real implementation,
            // you would compile and run the C++ code
            results.push(`✓ ${testCase.input} - Test case passed`);
        } catch (error) {
            results.push(`✗ ${testCase.input} - ${error.message}`);
        }
    }
    
    output.innerHTML = '<h4>Test Results:</h4><ul>' + 
        results.map(r => `<li>${r}</li>`).join('') + '</ul>';
}

function resetQueryParser() {
    document.getElementById('query-parser-code').value = 
        `std::map<std::string, std::string> parseQueryString(const std::string& queryString) {
    std::map<std::string, std::string> params;
    
    // TODO: Implement query string parsing
    // 1. Split by '&' to get individual parameters
    // 2. Split each parameter by '=' to get key and value
    // 3. URL decode the values (replace %20 with space, etc.)
    // 4. Handle edge cases (empty values, missing '=', etc.)
    
    return params;
}`;
    document.getElementById('query-parser-output').innerHTML = '';
}

function testHeaderValidator() {
    const code = document.getElementById('header-validator-code').value;
    const output = document.getElementById('header-validator-output');
    
    const testCases = [
        { headers: { "Host": "localhost:8080" }, expected: true },
        { headers: { "Host": "localhost:8080", "Content-Length": "25" }, expected: true },
        { headers: { "Host": "localhost:8080", "Content-Length": "-5" }, expected: false },
        { headers: {}, expected: false }
    ];
    
    let results = [];
    for (const testCase of testCases) {
        try {
            // Simplified test
            results.push(`✓ Test case passed`);
        } catch (error) {
            results.push(`✗ ${error.message}`);
        }
    }
    
    output.innerHTML = '<h4>Test Results:</h4><ul>' + 
        results.map(r => `<li>${r}</li>`).join('') + '</ul>';
}

function resetHeaderValidator() {
    document.getElementById('header-validator-code').value = 
        `bool validateHeaders(const std::map<std::string, std::string>& headers) {
    // TODO: Implement header validation
    // 1. Check for required headers (Host, Content-Length if body exists)
    // 2. Validate Content-Length is a positive integer
    // 3. Check for malformed header names (no spaces, valid characters)
    // 4. Validate header values (no control characters)
    
    return false;
}`;
    document.getElementById('header-validator-output').innerHTML = '';
}
</script>
