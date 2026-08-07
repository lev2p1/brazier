# lightlib Framework

lightlib is a modern C++ MVC framework designed for creating high-performance asynchronous APIs.
The framework combines the power of Boost.Asio with modern C++20/23 features.

## Features

- **Full asynchrony** based on Boost.Asio coroutines
- **MVC architecture** with clear separation of concerns
- **Built-in database migration** system
- **JWT authentication** support
- **Redis caching**
- **WebSockets** technology support
- **High performance** with minimal overhead

## Requirements

### **Required components:**

- **PostgreSQL** 12+ (for database operations)
- **CMake** 3.14+ (build system)
- **vcpkg** (C++ dependency manager)
- **C++ compiler** with C++20 support (GCC 10+, Clang 10+, MSVC 2019 16.8+)
- **.env file** with environment configuration
- **lightlib libraries** (pre-built)

### **Supported platforms:**

- **Windows** 10/11
- **Linux** (Ubuntu 20.04+, CentOS 8+)
- **macOS** 11+

## Project structure

```text
your_project/
├── config.json                  # Environment configuration
├── CMakeLists.txt               # Build file
├── vcpkg.json                   # vcpkg dependencies
├── your_project/
│   └── Proj.cpp           # Main application file
├── libs/
│   └── lightlib/
│       ├── x64-windows/        # Windows libraries
│       └── x64-linux/          # Linux libraries
└── include/                    # Header files
```

## Configuration files

### CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.14)

set(PROJECT_NAME "your_project")
project(${PROJECT_NAME} VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE AND DEFINED ENV{VCPKG_ROOT})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif()

if(MSVC)
    add_compile_options(/utf-8)
endif()

find_package(Boost CONFIG REQUIRED COMPONENTS beast asio filesystem system thread date_time smart_ptr optional variant algorithm)
find_package(fmt CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(jwt-cpp CONFIG REQUIRED)
find_package(hiredis CONFIG REQUIRED)
find_package(ZLIB REQUIRED)
find_package(PostgreSQL REQUIRED)
find_package(OpenSSL REQUIRED)
# Add all the other packages you need here

if(WIN32)
    set(lightlib_DIR "${CMAKE_SOURCE_DIR}/libs/lightlib/x64-windows")
elseif(UNIX)
    set(lightlib_DIR "${CMAKE_SOURCE_DIR}/libs/lightlib/x64-linux")
endif()

add_library(lightlib SHARED IMPORTED)

if(WIN32)
    set_target_properties(lightlib PROPERTIES
        IMPORTED_IMPLIB "${lightlib_DIR}/lib/lightlib.lib"
        IMPORTED_LOCATION "${lightlib_DIR}/bin/lightlib.dll"
        INTERFACE_INCLUDE_DIRECTORIES "${lightlib_DIR}/include"
    )
elseif(UNIX)
    set_target_properties(lightlib PROPERTIES
        IMPORTED_IMPLIB "${lightlib_DIR}/lib/liblightlib.a"
        IMPORTED_LOCATION "${lightlib_DIR}/bin/liblightlib.dylib"
        INTERFACE_INCLUDE_DIRECTORIES "${lightlib_DIR}/include"
    )
endif()

target_include_directories(lightlib INTERFACE
    "${lightlib_DIR}"
    "${lightlib_DIR}/include"
)

target_link_libraries(lightlib INTERFACE
    Boost::beast  
    Boost::asio
    Boost::filesystem
    Boost::system
    Boost::thread
    Boost::date_time
    Boost::smart_ptr
    Boost::optional
    Boost::variant
    Boost::algorithm
    fmt::fmt
    nlohmann_json::nlohmann_json
    jwt-cpp::jwt-cpp
    hiredis::hiredis
    ZLIB::ZLIB
    PostgreSQL::PostgreSQL
    OpenSSL::SSL
    OpenSSL::Crypto

    # Add all the other packages you need here
)

add_executable(${PROJECT_NAME} ${PROJECT_NAME}/${PROJECT_NAME}.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE lightlib)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

if(WIN32)
    target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/vcpkg_installed/x64-windows/include)
endif()

if(WIN32)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    
        COMMAND ${CMAKE_COMMAND} -E copy
            "${lightlib_DIR}/bin/lightlib.dll"
            $<TARGET_FILE_DIR:${PROJECT_NAME}>

        COMMENT "Copying lightlib.dll to output directory"
    )
elseif(UNIX)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
            "${lightlib_DIR}/bin/liblightlib.dylib"
            $<TARGET_FILE_DIR:${PROJECT_NAME}>

        COMMENT "Copying liblightlib.dylib to output directory"
    )
endif()
```

### vcpkg.json

```json
{
  "name": "your_project",
  "version": "1.0.0",
  "dependencies": [
    "boost-algorithm",
    "boost-asio",
    "boost-beast",
    "boost-date-time",
    "boost-filesystem",
    "boost-optional",
    "boost-smart-ptr",
    "boost-system",
    "boost-thread",
    "boost-variant",
    "fmt",
    "hiredis",
    "jwt-cpp",
    "libpq",
    "nlohmann-json",
    "zlib"
  ]
}
```

Add all the other packages you need to the end of the list

## Building the project

```text
# Windows
.\build.bat

# Linux/macOS
sh build.sh
```

## lightlib configuration

### Overview

`ConfigManager` is a flexible configuration management system in lightlib that provides:
- Loading configuration from JSON files
- Working with nested parameters using dot notation
- Automatic saving of changes
- Support for different filesystem drivers
- Type-safe value retrieval with default values

### Configuration File Structure

#### Complete Structure with Comments

```json
{
  "server": {
    "host": "0.0.0.0",
    "port": 3501,
    "keep-alive-timeout": 60
  },
  "database": {
    "host": "127.0.0.1",
    "port": 5432,
    "username": "kirill",
    "password": "qwerty",
    "database": "light"
  },
  "redis": {
    "host": "127.0.0.1",
    "port": 6379
  },
  "nosql": {
    "host": "127.0.0.1",
    "port": 6379
  },
  "filesystem": {
    "drivers": {
      "local": {
        "root": "./storage"
      },
      "root": {
        "root": "./"
      },
      "default": "local"
    }
  }
}
```

To initialize the global configuration manager, you need to run `ConfigManager::initGlobal()` at the beginning of program execution.

```cpp
#include <lightlib/vendor/ConfigManager.hpp>

int main() {
    ConfigManager::initGlobal();
    
    std::string host = global_config->get("server.host", "127.0.0.1");
    int port = global_config->get("server.port", 3501);
    
    lightlib::Server server(host, port);
    server.initialize();
    server.run();
    
    return 0;
}
```

You can immediately convert data to the required types and work with nested config parameters.
```cpp
std::string host = config.getNested<std::string>("server.host", "localhost");
int port = config.getNested<int>("server.port", 8080);
bool debug = config.getNested<bool>("app.debug", false);
json database = config.getNested<json>("database", json::object());
```

#### Saving and reloading
```cpp
auto& config = *global_config;

config.save();               // Manual save
config.reload();             // Reload from file
config.setAutoSave(true);    // Auto-save on changes
config.setAutoSave(false);   // Manual save only
config.clear();              // Clear all values
```
## lightlib Logging System

lightlib provides a high-performance logging system with colored output, log rotation, and signal handling.
Logging is done both to file and console with color coding.


### Key Features

- **Colored logging** to console
- **Automatic log rotation**
- **Thread safety** using mutexes
- **High performance** with time caching
- **Signal handling** for graceful shutdown
- **File logging** with buffering

| Level     | Color      | Description              |
|-----------|------------|--------------------------|
| ERROR     |  Red       | Critical errors          |
| WARNING   |  Yellow    | Warnings                 |
| INFO      |  Blue      | Informational messages   |
| SUCCESS   |  Green     | Successful operations    |
| HELP      |  Dark blue | Support messages         |
| CMD       |  Purple    | Commands to use          |
| DEBUG     |            | Debug information        |

### Usage examples

```cpp
Logger::log("Database connection established", "SUCCESS");

std::string username = "john_doe";
Logger::log("User " + username + " logged in", "INFO");
Logger::log("Debugging username", "DEBUG");

try {
    
} catch (const std::exception& e) {
    Logger::log(std::string("Exception: ") + e.what(), "ERROR");
}
```

### Automatic log rotation

All logs are saved and can be found at your_project_path/Storage/Logs

```text
your_project_path/
└── Storage/
    └── Logs/
        ├── app.log      # Current log file
        └── app_old.log  # Previous log after rotation

```

The system automatically rotates logs when the line limit is reached:

- Current file: `debug.log`
- Old file: `debug_old.log`
- Default limit: 10,000 lines

### Custom log levels

```cpp
Logger::log("Custom business event", "BUSINESS");
Logger::log("Performance metric", "METRIC");
Logger::log("Security audit", "AUDIT");
```

## Log format

### Console output (with colors):

```text
2024-01-15 14:30:25 [INFO] Application started
2024-01-15 14:30:26 [SUCCESS] Database connected
2024-01-15 14:30:27 [ERROR] Connection timeout
```

The lightlib logging system is ready for use in production environments and provides all necessary functions for effective application monitoring and debugging.

## lightlib Routing System

lightlib provides a powerful routing system with support for asynchronous handlers, parameterized paths, and CORS.
The system is built on Boost.Beast and Boost.Asio coroutines.

### 1. Router

Central class for managing routes and handling requests.

### 2. Route

Represents a single endpoint with parameters and a handler.

### 3. RouterRegisterer

Simplifies route declaration through macros.

### Route registration

```cpp
auto userController = std::make_shared<UserController>();
    
// Basic routes
R(GET, "/api/users", userController, getUsers);
R(POST, "/api/users", userController, createUser);
    
// Parameterized routes
R(GET, "/api/users/:id", userController, getUserById);
R(PUT, "/api/users/:id", userController, updateUser);
R(DELETE_, "/api/users/:id", userController, deleteUser);
    
// Nested parameters
R(GET, "/api/users/:userId/posts/:postId", userController, getUserPost);
    
// CORS for API endpoints
CORS("/api/users", userController);
CORS("/api/users/:id", userController);
```

### Route types

#### Static routes

```cpp
R(GET, "/api/health", healthController, checkHealth);
R(GET, "/api/version", infoController, getVersion);
```

#### Parameterized routes

```cpp
R(GET, "/api/users/:id", userController, getUserById);
R(GET, "/api/categories/:categoryId/products/:productId", productController, getProduct);
```

### Request handling

#### Accessing parameters

```cpp
boost::asio::awaitable<void> getUserPost(const Request& req, Response& res, const Params& params) {
    auto userId = params.at("userId");
    auto postId = params.at("postId");
    
    auto user = co_await userService.findById(userId);
    auto post = co_await postService.findByUserAndId(userId, postId);
    
    nlohmann::json response = {
        {"user", user},
        {"post", post}
    };
    
    res.result(http::status::ok);
    res.body() = response.dump();
    res.set(http::field::content_type, "application/json");
    co_return;
}
```

#### Working with request body

```cpp
boost::asio::awaitable<void> createUser(const Request& req, Response& res, const Params& params) {
    try {
        auto body = nlohmann::json::parse(req.body());
        
        if (!body.contains("email") || !body.contains("name")) {
            res.result(http::status::bad_request);
            co_return;
        }
        
        auto user = co_await userService.create(body);
        
        res.result(http::status::created);
        res.body() = user.toJson().dump();
        res.set(http::field::content_type, "application/json");
        
    } catch (const std::exception& e) {
        res.result(http::status::bad_request);
    }
    co_return;
}
```

## lightlib WebSocket Routing System

lightlib provides a complete WebSocket routing system with support for parameterized paths, multiple message types, and global handlers. The system integrates seamlessly with the existing HTTP router.

### WebSocket Router

WebSocketRouter is a singleton class that manages all WebSocket routes and their handlers.

| Method	                          | Description                          |
|-------------------------------------|--------------------------------------|
| ``instance()	 ``                     | Returns singleton instance           |
| ``add_route(path, handler)``	          | Registers a new WebSocket route      |
| ``find_route(target, params)``	      | Finds route matching the target path |
| ``set_global_message_handler(handler)`` |	Sets global text message handler     |
| ``set_global_binary_handler(handler)``  |	Sets global binary message handler   |
| ``set_global_close_handler(handler)``	  | Sets global close handler            |
| ``set_global_error_handler(handler)``	  | Sets global error handler            |

### WebSocket Route Types

#### 1. Connect Route (WS_R)

Handles new WebSocket connection establishment.

**Handler Signature:**

```cpp
void handler(std::shared_ptr<WebSocketSession> session, const Params& params)
```

**Use Case:** Initialize connection, send welcome message, set up session-specific data.

```cpp
class ChatController {
public:
    void onConnect(std::shared_ptr<WebSocketSession> session, const Params& params) {
        std::string room = params.at("room");
        session->send("Welcome to room: " + room);
    }
};

WS_R("/chat/:room", ChatController, onConnect);
```

#### 2. Text Message Route (WS_MSG)

Handles incoming text messages.

**Handler Signature:**

```cpp
void handler(const std::string& message, std::shared_ptr<WebSocketSession> session)
```

**Use Case:** Process chat messages, commands, JSON data.

```cpp
class ChatController {
public:
    void onMessage(const std::string& message, std::shared_ptr<WebSocketSession> session) {
        if (message == "ping") {
            session->send("pong");
        } else {
            session->send("Echo: " + message);
        }
    }
};


WS_MSG("/chat/:room", ChatController, onMessage);
```

#### 3. Binary Message Route (WS_BIN)

Handles incoming binary messages.

**Handler Signature:**

```cpp
void handler(std::vector<uint8_t>&& data, std::shared_ptr<WebSocketSession> session)
```

**Use Case:** File uploads, image transfers, binary protocols.

```cpp
class FileController {
public:
    void onBinary(std::vector<uint8_t>&& data, std::shared_ptr<WebSocketSession> session) {
        std::ofstream file("upload_" + std::to_string(time(nullptr)) + ".bin", std::ios::binary);
        file.write(reinterpret_cast<const char*>(data.data()), data.size());
        session->send("File received: " + std::to_string(data.size()) + " bytes");
    }
};

WS_BIN("/upload", FileController, onBinary);
```

#### 4. Close Route (WS_CLOSE)

Handles connection closure events.

**Handler Signature:**

```cpp
void handler(std::shared_ptr<WebSocketSession> session)
```

**Use Case:** Cleanup resources, update user lists, log disconnections.

```cpp
class ChatController {
public:
    void onClose(std::shared_ptr<WebSocketSession> session) {
        active_users_.erase(session->get_id());
        WebSocketManager::instance().broadcast("User " + std::string(session->get_id()) + " left");
    }
};

WS_CLOSE("/chat/:room", ChatController, onClose);
```

#### 5. Error Route (WS_ERR)

Handles WebSocket errors.

**Handler Signature:**

```cpp
void handler(const std::string& error, std::shared_ptr<WebSocketSession> session)
```

**Use Case:** Logging, error recovery, client notification.

```cpp
class ChatController {
public:
    void onError(const std::string& error, std::shared_ptr<WebSocketSession> session) {
        Logger::log("WebSocket error: " + error, "ERROR");
        session->send("Error occurred: " + error);
    }
};

WS_ERR("/chat/:room", ChatController, onError);
```
### WebSocket Macros

|Macro	| Description |
|-------|-------------|
|``WS_R(path, controller, handler)``	|Register WebSocket connect handler|
|``WS_MSG(path, controller, handler)``	|Register text message handler|
|``WS_BIN(path, controller, handler)``	|Register binary message handler|
|``WS_CLOSE(path, controller, handler)`` |	Register close handler|
|``WS_ERR(path, controller, handler)``	|Register error handler|

### Global Handlers

Global handlers are called for every WebSocket connection, regardless of the route path.

#### Global Handler Macros

|Macro	|Description|
|-------|-----------|
|``WS_G_MSG(controller, handler)``|	Global text message handler|
|``WS_G_BIN(controller, handler) ``|	Global binary message handler|
|``WS_G_CLOSE(controller, handler)``|	Global close handler|
|``WS_G_ERR(controller, handler)``|Global error handler|

#### Global Handler Example

```cpp
class GlobalController {
public:
    void onGlobalMessage(const std::string& message, std::shared_ptr<WebSocketSession> session) {
        // Log every message
        Logger::log("Message from " + std::string(session->get_id()) + ": " + message);
        
        // Common commands available on any path
        if (message == "help") {
            session->send("Available commands: stats, time, ping");
        } else if (message == "stats") {
            session->send("Active sessions: " + 
                std::to_string(WebSocketManager::instance().get_session_count()));
        }
    }
    
    void onGlobalClose(std::shared_ptr<WebSocketSession> session) {
        Logger::log("Session closed: " + std::string(session->get_id()));
    }
};

// Register global handlers (called once)
WS_G_MSG(GlobalController, onGlobalMessage);
WS_G_CLOSE(GlobalController, onGlobalClose);
```

### Handler Priority

Route-specific handlers have priority over global handlers

Global handlers are called when no route-specific handler exists

Global close and error handlers are always called in addition to route-specific ones

```text
Client connects
    ↓
Find matching route
    ↓
┌─────────────────────────────────────┐
│ If route has message handler:       │
│   → Use route handler               │
│ Else:                               │
│   → Use global message handler      │
└─────────────────────────────────────┘
    ↓
Close and Error: route + global both called
```

## Controllers in lightlib Framework

Controllers in lightlib are central components that handle HTTP requests and return responses.
They implement application business logic, work with data models, and form HTTP responses.

### Base structure

```cpp
#include <lightlib/Core>
#include <lightlib/DB>
#include <lightlib/Http>

class UserController : public lightlib::Controller {
public:
    using Request = http::request<http::string_body>;
    using Response = http::response<http::string_body>;
    using Params = std::unordered_map<std::string, std::string>;

    // Override REST methods
    boost::asio::awaitable<void> show(const Request& req, Response& res, const Params& params) override;
    boost::asio::awaitable<void> store(const Request& req, Response& res, const Params& params) override;
    boost::asio::awaitable<void> update(const Request& req, Response& res, const Params& params) override;
    boost::asio::awaitable<void> delete_(const Request& req, Response& res, const Params& params) override;
    
    // Custom methods
    boost::asio::awaitable<void> login(const Request& req, Response& res, const Params& params);
    boost::asio::awaitable<void> logout(const Request& req, Response& res, const Params& params);
    
    // CORS handler
    void setCors(const Request& req, Response& res) override;
};
```

### Base Controller Methods

The base Controller class provides default implementations that return 405 Method Not Allowed.

You can override the REST methods.

|Method	|HTTP Method	|Default Behavior	|Typical Use|
|-------|------------|------------------|-----------|
|``show(req, res, params)``	|GET	|405 Not Allowed|	Get single resource by ID
|``store(req, res, params)``	|POST	|405 Not Allowed|	Create new resource
|``update(req, res, params)``	|PUT	|405 Not Allowed|	Update existing resource
|``delete_(req, res, params)``|	DELETE|	405 Not Allowed|	Delete resource

#### Overriding REST methods:

```cpp
class UserController : public lightlib::Controller {
public:
    // GET /users/:id - Get single user
    boost::asio::awaitable<void> show(const Request& req, Response& res, const Params& params) override {
        auto userId = std::stoi(params.at("id"));
        auto user = co_await User::find(userId);
        
        if (!user) {
            res.result(http::status::not_found);
            co_return;
        }
        
        res.result(http::status::ok);
        res.body() = user.toJson().dump();
        res.set(http::field::content_type, "application/json");
        co_return;
    }
    
    // POST /users - Create new user
    boost::asio::awaitable<void> store(const Request& req, Response& res, const Params& params) override {
        auto body = nlohmann::json::parse(req.body());
        
        auto user = co_await User::create(body);
        
        res.result(http::status::created);
        res.body() = user.toJson().dump();
        res.set(http::field::content_type, "application/json");
        co_return;
    }
    
    // PUT /users/:id - Update user
    boost::asio::awaitable<void> update(const Request& req, Response& res, const Params& params) override {
        auto userId = std::stoi(params.at("id"));
        auto body = nlohmann::json::parse(req.body());
        
        auto user = co_await User::update(userId, body);
        
        res.result(http::status::ok);
        res.body() = user.toJson().dump();
        co_return;
    }
    
    // DELETE /users/:id - Delete user
    boost::asio::awaitable<void> delete_(const Request& req, Response& res, const Params& params) override {
        auto userId = std::stoi(params.at("id"));
        
        co_await User::remove(userId);
        
        res.result(http::status::no_content);
        co_return;
    }
};
```

### Custom Controller Methods

You can add any number of custom methods beyond the base REST methods:

```cpp
class UserController : public lightlib::Controller {
public:
    // Custom method - not in base Controller
    boost::asio::awaitable<void> login(const Request& req, Response& res, const Params& params) {
        auto body = nlohmann::json::parse(req.body());
        std::string email = body["email"];
        std::string password = body["password"];
        
        auto user = co_await User::authenticate(email, password);
        
        if (!user) {
            res.result(http::status::unauthorized);
            co_return;
        }
        
        std::string token = generateJWT(user);
        
        nlohmann::json response;
        response["token"] = token;
        response["user"] = user.toJson();
        
        res.result(http::status::ok);
        res.body() = response.dump();
        res.set(http::field::content_type, "application/json");
        co_return;
    }
    
    boost::asio::awaitable<void> logout(const Request& req, Response& res, const Params& params) {
        // Clear session, invalidate token
        res.result(http::status::ok);
        co_return;
    }
    
    boost::asio::awaitable<void> profile(const Request& req, Response& res, const Params& params) {
        // Get current user profile
        auto userId = getCurrentUserId(req);
        auto user = co_await User::find(userId);
        
        res.result(http::status::ok);
        res.body() = user.toJson().dump();
        co_return;
    }
};
```

### CORS Handlers

#### Default CORS implementation

The base Controller class provides default CORS handlers:

```cpp
void Controller::setCors(const Request& req, Response& res) {
    res.set(http::field::access_control_allow_origin, "*");
    res.set(http::field::access_control_allow_credentials, "true");
    res.set(http::field::access_control_allow_methods, "GET, POST, PUT, DELETE, OPTIONS");
    res.set(http::field::access_control_allow_headers, "*");
    res.set(http::field::access_control_expose_headers, "Set-Cookie");
}

void Controller::setCorsHeaders(const Request& req, Response& res) {
    this->setCors(req, res);
}
```
#### Overriding CORS for specific controllers

```cpp
class ApiController : public lightlib::Controller {
public:
    void setCors(const Request& req, Response& res) override {
        // Restrict to specific origin
        res.set(http::field::access_control_allow_origin, "https://myapp.com");
        res.set(http::field::access_control_allow_credentials, "true");
        res.set(http::field::access_control_allow_methods, "GET, POST, PUT, DELETE");
        res.set(http::field::access_control_allow_headers, "Content-Type, Authorization, X-API-Key");
        res.set(http::field::access_control_expose_headers, "X-Total-Count");
    }
};

class PublicController : public lightlib::Controller {
public:
    void setCors(const Request& req, Response& res) override {
        // Allow any origin (public API)
        std::string origin = req.at(http::field::origin);
        res.set(http::field::access_control_allow_origin, origin);
        res.set(http::field::access_control_allow_methods, "GET, OPTIONS");
    }
};
```

### Working with Requests

#### Parsing request body

```cpp
boost::asio::awaitable<void> createUser(const Request& req, Response& res, const Params& params) {
    try {
        // Parse JSON body
        auto body = nlohmann::json::parse(req.body());
        
        // Validate required fields
        if (!body.contains("name") || !body.contains("email")) {
            res.result(http::status::bad_request);
            res.body() = R"({"error":"Missing required fields"})";
            co_return;
        }
        
        std::string name = body["name"];
        std::string email = body["email"];
        
        // Create user
        auto user = co_await User::create(name, email);
        
        res.result(http::status::created);
        res.body() = user.toJson().dump();
        res.set(http::field::content_type, "application/json");
        
    } catch (const nlohmann::json::parse_error& e) {
        res.result(http::status::bad_request);
        res.body() = R"({"error":"Invalid JSON"})";
    } catch (const std::exception& e) {
        res.result(http::status::internal_server_error);
        res.body() = R"({"error":"Internal server error"})";
    }
    
    co_return;
}
```

#### Working with URL parameters

```cpp
boost::asio::awaitable<void> getUser(const Request& req, Response& res, const Params& params) {
    // Get parameters from URL (e.g., /users/:id)
    auto userId = params.at("id");
    
    // Optional parameters with default
    std::string fields = params.count("fields") ? params.at("fields") : "all";
    
    auto user = co_await User::find(std::stoi(userId));
    
    if (!user) {
        res.result(http::status::not_found);
        res.body() = R"({"error":"User not found"})";
        co_return;
    }
    
    nlohmann::json response;
    if (fields == "all") {
        response = user.toJson();
    } else {
        response = user.toJson(fields);
    }
    
    res.result(http::status::ok);
    res.body() = response.dump();
    res.set(http::field::content_type, "application/json");
    co_return;
}
```

#### Working with query parameters

```cpp
boost::asio::awaitable<void> getUsers(const Request& req, Response& res, const Params& params) {
    // Parse query string
    auto target = std::string(req.target());
    auto query_pos = target.find('?');
    
    std::unordered_map<std::string, std::string> query;
    if (query_pos != std::string::npos) {
        std::string query_string = target.substr(query_pos + 1);
        // Parse key=value&key2=value2
        // ...
    }
    
    int page = std::stoi(query.count("page") ? query["page"] : "1");
    int limit = std::stoi(query.count("limit") ? query["limit"] : "20");
    
    auto users = co_await User::paginate(page, limit);
    
    nlohmann::json response;
    response["data"] = users;
    response["page"] = page;
    response["limit"] = limit;
    
    res.result(http::status::ok);
    res.body() = response.dump();
    res.set(http::field::content_type, "application/json");
    co_return;
}
```

#### Accessing headers

```cpp
boost::asio::awaitable<void> authEndpoint(const Request& req, Response& res, const Params& params) {
    // Get authorization header
    auto auth = req.find(http::field::authorization);
    if (auth == req.end()) {
        res.result(http::status::unauthorized);
        co_return;
    }
    
    std::string token = auth->value().to_string();
    
    // Remove "Bearer " prefix if present
    if (token.rfind("Bearer ", 0) == 0) {
        token = token.substr(7);
    }
    
    auto user = co_await validateToken(token);
    
    if (!user) {
        res.result(http::status::unauthorized);
        co_return;
    }
    
    // Continue with request
    co_return;
}
```

#### Working with Responses

```cpp
// 200 OK with JSON
res.result(http::status::ok);
res.body() = json.dump();
res.set(http::field::content_type, "application/json");
res.prepare_payload();

// 201 Created
res.result(http::status::created);
res.body() = createdResource.toJson().dump();
res.set(http::field::location, "/api/users/" + std::to_string(id));
res.prepare_payload();

// 204 No Content (for DELETE)
res.result(http::status::no_content);
res.body() = "";
res.prepare_payload();

// 400 Bad Request
res.result(http::status::bad_request);
res.body() = R"({"error":"Invalid input"})";
res.prepare_payload();

// 401 Unauthorized
res.result(http::status::unauthorized);
res.set(http::field::www_authenticate, "Bearer");
res.body() = R"({"error":"Authentication required"})";
res.prepare_payload();

// 403 Forbidden
res.result(http::status::forbidden);
res.body() = R"({"error":"Access denied"})";
res.prepare_payload();

// 404 Not Found
res.result(http::status::not_found);
res.body() = R"({"error":"Resource not found"})";
res.prepare_payload();

// 405 Method Not Allowed
res.result(http::status::method_not_allowed);
res.set(http::field::allow, "GET, POST");
res.body() = R"({"error":"Method not allowed"})";
res.prepare_payload();

// 500 Internal Server Error
res.result(http::status::internal_server_error);
res.body() = R"({"error":"Internal server error"})";
res.prepare_payload();
```

#### Setting custom headers

```cpp
res.set(http::field::content_type, "application/json");
res.set(http::field::cache_control, "no-cache");
res.set(http::field::expires, "0");
res.set(http::field::x_custom_header, "custom_value");
res.set("X-RateLimit-Limit", "100");
res.set("X-RateLimit-Remaining", "95");
```

## Helpers in lightlib Framework

lightlib provides a comprehensive set of utility classes (helpers) for common tasks:
- working with cookies; 
- code generation;
- data validation; 
- HTTP requests;
- email sending; 
- other helper functions.

### Code Class - Code Generation

**Purpose**

Generate random codes for various purposes: verification codes, one-time passwords, authentication tokens, backup codes, and API keys.

|Method	|Description	|Return Type|
|-------|-------------|-----------|
|``generateRandomCode(length)``	|Generate random alphanumeric code	|``std::string``
|``generateNumericCode(length)``	|Generate numeric-only code	|``std::string``
|``generateMultipleCodes(count, length, numeric)``	|Generate multiple codes at once |``std::vector<std::string>``
|``base64_encode(bytes)``	|Base64 encode string	|``std::string``

#### Usage Examples

Single code generation

```cpp
#include <lightlib/App/Http/Helpers/Code.hpp>

// Alphanumeric code (letters + digits)
std::string verificationCode = Code::generateRandomCode(16);
// Example output: "Ab3kL9mN2pXy8zQ1w"

// Numeric code (only digits)
std::string smsCode = Code::generateNumericCode(6);
// Example output: "384290"

// Email verification token
std::string emailToken = Code::generateRandomCode(32);
Multiple codes generation
cpp
// Generate 10 backup codes (alphanumeric, 8 chars each)
auto backupCodes = Code::generateMultipleCodes(10, 8);
// Example output: ["Ab3kL9mN", "Xy8zQ1wV", "r4F7tY2p", ...]

// Generate 5 numeric OTP codes (4 digits each)
auto otpCodes = Code::generateMultipleCodes(5, 4, true);
// Example output: ["3842", "9017", "5623", "7481", "2395"]

// Generate API keys
auto apiKeys = Code::generateMultipleCodes(100, 32);
Base64 encoding
cpp
std::string original = "Hello World";
std::string encoded = Code::base64_encode(original);
// Output: "SGVsbG8gV29ybGQ="

// For SMTP authentication
std::string authString = '\0' + username + '\0' + password;
std::string encodedAuth = Code::base64_encode(authString);
Real-world examples
cpp
// Generate verification code for email
std::string generateEmailVerificationCode() {
    return Code::generateRandomCode(32);
}

// Generate 2FA backup codes
std::vector<std::string> generate2FABackupCodes() {
    return Code::generateMultipleCodes(10, 8);
}

// Generate session token
std::string generateSessionToken() {
    return Code::generateRandomCode(64);
}

// Generate numeric OTP
std::string generateOTP() {
    return Code::generateNumericCode(6);
}
```

### Cookie Class - HTTP Cookies

**Purpose**

Simplified HTTP cookie handling: parsing cookies from requests and setting cookies in responses.

|Method	|Description	|Return Type|
|-------|---------------|-----------|
|``parseCookies(cookieHeader)``|	Parse cookie header string into key-value map	|``std::map<std::string, std::string>``
|``set(response, cookies)``	|Set multiple cookies in HTTP response|	``void``

#### Usage Examples

**Parsing cookies from request**

```cpp
#include <lightlib/App/Http/Helpers/Cookie.hpp>

boost::asio::awaitable<void> show(const Request& req, Response& res, const Params& params) {
    // Get cookie header from request
    auto cookieHeader = req.find(http::field::cookie);
    
    if (cookieHeader != req.end()) {
        auto cookies = Cookie::parseCookies(std::string(cookieHeader->value()));
        
        std::string sessionId = cookies["session_id"];
        std::string userId = cookies["user_id"];
        std::string theme = cookies["theme"];
        
        // Use cookie values
        if (cookies.find("auth_token") != cookies.end()) {
            // User is authenticated
        }
    }
    
    co_return;
}
```

**Setting cookies in response**

```cpp
boost::asio::awaitable<void> login(const Request& req, Response& res, const Params& params) {
    // Single cookie
    std::map<std::string, std::string> cookies;
    cookies["session_id"] = Code::generateRandomCode(32);
    Cookie::set(res, cookies);
    
    // Multiple cookies
    std::map<std::string, std::string> authCookies = {
        {"auth_token", "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9"},
        {"user_id", "12345"},
        {"session_id", Code::generateRandomCode(32)},
        {"refresh_token", Code::generateRandomCode(64)}
    };
    Cookie::set(res, authCookies);
    
    res.result(http::status::ok);
    co_return;
}
```

**Complete authentication example**
```cpp
class AuthController : public lightlib::Controller {
public:
    boost::asio::awaitable<void> login(const Request& req, Response& res, const Params& params) {
        auto body = nlohmann::json::parse(req.body());
        std::string email = body["email"];
        std::string password = body["password"];
        
        auto user = co_await authenticate(email, password);
        
        if (!user) {
            res.result(http::status::unauthorized);
            co_return;
        }
        
        // Set authentication cookies
        std::map<std::string, std::string> cookies = {
            {"access_token", generateAccessToken(user)},
            {"refresh_token", generateRefreshToken(user)},
            {"user_id", std::to_string(user.id)},
            {"session_id", Code::generateRandomCode(32)}
        };
        Cookie::set(res, cookies);
        
        res.result(http::status::ok);
        co_return;
    }
    
    boost::asio::awaitable<void> logout(const Request& req, Response& res, const Params& params) {
        // Clear cookies by setting expired values
        std::map<std::string, std::string> expiredCookies = {
            {"access_token", ""},
            {"refresh_token", ""},
            {"user_id", ""},
            {"session_id", ""}
        };
        Cookie::set(res, expiredCookies);
        
        res.result(http::status::ok);
        co_return;
    }
};
```

### Validator Class - Data Validation

**Purpose**

Validate user input data: passwords, email addresses, and other common data types.

|Method	|Description	|Return Type|
|-------|-------------|------------|
|``password(password)``|	Validate password| ``bool``
|``email(email)`` |	Validate email format (RFC 5322)	| ``bool``
|``passwordError(password)`` | Get detailed password error message|	``std::string``

**Password Requirements:**
- Length: 8-20 characters
- At least one digit (0-9)
- At least one uppercase letter (A-Z)
- At least one special character (!@#$%^&* etc.)

#### Usage Examples

**Basic validation**

```cpp
#include <lightlib/App/Http/Helpers/Validator.hpp>

// Password validation
if (Validator::password("MySecure123!")) {
    // Password is valid
} else {
    std::string error = Validator::passwordError("MySecure123!");
}

// Email validation
if (Validator::email("user@example.com")) {
    // Email is valid
} else {
    // Invalid email format
}
```

**Complete registration example**

```cpp
boost::asio::awaitable<void> register(const Request& req, Response& res, const Params& params) {
    auto body = nlohmann::json::parse(req.body());
    
    std::string email = body["email"];
    std::string password = body["password"];
    
    // Validate email
    if (!Validator::email(email)) {
        res.result(http::status::bad_request);
        res.body() = R"({"error":"Invalid email format"})";
        co_return;
    }
    
    // Validate password
    if (!Validator::password(password)) {
        std::string error = Validator::passwordError(password);
        nlohmann::json response;
        response["error"] = error;
        res.body() = response.dump();
        res.result(http::status::bad_request);
        co_return;
    }
    
    // Create user
    auto user = co_await User::create(email, password);
    
    res.result(http::status::created);
    co_return;
}
```

**Password validation with detailed errors**

```cpp
std::string validatePasswordWithDetails(const std::string& password) {
    if (password.size() < 8) {
        return "Password must be at least 8 characters long";
    }
    if (password.size() > 20) {
        return "Password must not exceed 20 characters";
    }
    
    if (Validator::password(password)) {
        return ""; // Valid
    }
    
    return Validator::passwordError(password);
}

// Usage
std::string error = validatePasswordWithDetails("weak");
if (!error.empty()) {
    // Show error to user
}
```

### HttpClient Class - HTTP Requests

**Purpose**

Make HTTP/HTTPS requests to external APIs with support for GET, POST, PUT, DELETE methods and JSON data.

|Method	|Description	|Return Type
|-|-|-
``get(url, body)``	|Perform HTTP GET request	|``Response``
``post(url, body)``	|Perform HTTP POST request	|``Response``
``put(url, body)``	|Perform HTTP PUT request	|``Response``
``del(url, body)``	|Perform HTTP DELETE request	|``Response``
``set_timeout(timeout)``	|Set request timeout	|``void``
``is_success(response)``	|Check if response is successful	|``bool``

#### Usage Examples

**Basic GET request**

```cpp
#include <lightlib/App/Http/Helpers/HttpClient.hpp>

HttpClient client;
nlohmann::json body; // Empty for GET

// GET request
Response response = client.get("https://api.example.com/users");

if (client.is_success(response)) {
    auto users = nlohmann::json::parse(response.body());
    // Process users
}
```

**POST request with JSON body**

```cpp
HttpClient client;

nlohmann::json userData = {
    {"name", "John Doe"},
    {"email", "john@example.com"},
    {"age", 30}
};

Response response = client.post("https://api.example.com/users", userData);

if (response.result() == http::status::created) {
    auto createdUser = nlohmann::json::parse(response.body());
    std::cout << "User created with ID: " << createdUser["id"] << std::endl;
}
```

**PUT request for update**

```cpp
HttpClient client;

nlohmann::json updateData = {
    {"name", "Jane Doe"},
    {"age", 31}
};

Response response = client.put("https://api.example.com/users/123", updateData);

if (client.is_success(response)) {
    // User updated successfully
}
```

**DELETE request**

```cpp
HttpClient client;
nlohmann::json emptyBody;

Response response = client.del("https://api.example.com/users/123", emptyBody);

if (response.result() == http::status::no_content) {
    // User deleted successfully
}
```
**GET request with query parameters**

```cpp
HttpClient client;

nlohmann::json queryParams = {
    {"page", 1},
    {"limit", 20},
    {"sort", "desc"}
};

// Query parameters will be appended to URL automatically
Response response = client.get("https://api.example.com/users", queryParams);

auto users = nlohmann::json::parse(response.body());
for (const auto& user : users["data"]) {
    std::cout << user["name"] << std::endl;
}
```

**Setting timeout**
```cpp
HttpClient client;
client.set_timeout(std::chrono::seconds(60));

// This request will timeout after 60 seconds
Response response = client.get("https://slow-api.example.com/data");
```

**Complete API integration example**

```cpp
class WeatherService {
private:
    HttpClient client_;
    std::string apiKey_;
    
public:
    WeatherService(const std::string& apiKey) : apiKey_(apiKey) {}
    
    nlohmann::json getWeather(const std::string& city) {
        nlohmann::json params = {
            {"q", city},
            {"appid", apiKey_},
            {"units", "metric"}
        };
        
        Response response = client_.get("https://api.openweathermap.org/data/2.5/weather", params);
        
        if (client_.is_success(response)) {
            return nlohmann::json::parse(response.body());
        }
        
        return nlohmann::json();
    }
    
    nlohmann::json getForecast(const std::string& city, int days = 5) {
        nlohmann::json params = {
            {"q", city},
            {"appid", apiKey_},
            {"units", "metric"},
            {"cnt", days * 8}
        };
        
        Response response = client_.get("https://api.openweathermap.org/data/2.5/forecast", params);
        
        if (client_.is_success(response)) {
            return nlohmann::json::parse(response.body());
        }
        
        return nlohmann::json();
    }
};
```

### SmtpClient Class - Email Sending

**Purpose**

Send emails using SMTP protocol with SSL/TLS encryption support.

|Method|	Description	|Return Type
|-|-|-
``send(recipient, message, sender_name)``|	Send email to recipient	| ``net::awaitable<int>``

**Configuration**

The SmtpClient uses environment variables for configuration:

|Variable	|Description
|-|-
``SMTP_HOST``	|SMTP server hostname
``SMTP_PORT``	|SMTP server port
``SMTP_USERNAME``	|Authentication username
``SMTP_PASSWORD``	|Authentication password
``S_HOST``	|Local hostname for EHLO

#### Usage Examples

**Basic email sending**

```cpp
#include <lightlib/App/Http/Helpers/SmtpClient.hpp>

boost::asio::awaitable<void> sendEmail(const Request& req, Response& res, const Params& params) {
    SmtpClient smtp;
    
    std::string recipient = "user@example.com";
    std::string message = "<h1>Welcome!</h1><p>Thank you for registering.</p>";
    std::string senderName = "My App Support";
    
    int result = co_await smtp.send(recipient, message, senderName);
    
    if (result == 0) {
        res.result(http::status::ok);
        res.body() = "Email sent successfully";
    } else {
        res.result(http::status::internal_server_error);
        res.body() = "Failed to send email";
    }
    
    co_return;
}
```

**HTML email with formatting**

```cpp
std::string createHtmlEmail(const std::string& username, const std::string& verificationCode) {
    std::string html = R"(
        <!DOCTYPE html>
        <html>
        <head>
            <style>
                body { font-family: Arial, sans-serif; }
                .container { padding: 20px; background-color: #f4f4f4; }
                .content { background-color: white; padding: 20px; border-radius: 5px; }
                .code { font-size: 24px; font-weight: bold; color: #0066cc; }
            </style>
        </head>
        <body>
            <div class="container">
                <div class="content">
                    <h2>Welcome, )" + username + R"(!</h2>
                    <p>Please verify your email address using the code below:</p>
                    <p class="code">)" + verificationCode + R"(</p>
                    <p>This code expires in 24 hours.</p>
                </div>
            </div>
        </body>
        </html>
    )";
    
    return html;
}

boost::asio::awaitable<void> sendVerificationEmail(const std::string& email, const std::string& username) {
    SmtpClient smtp;
    std::string verificationCode = Code::generateRandomCode(32);
    std::string htmlMessage = createHtmlEmail(username, verificationCode);
    
    int result = co_await smtp.send(email, htmlMessage, "Verification Service");
    
    if (result == 0) {
        // Save verification code to database
        co_await saveVerificationCode(email, verificationCode);
    }
    
    co_return;
}
```

**Complete registration with email**

```cpp
class AuthController : public lightlib::Controller {
public:
    boost::asio::awaitable<void> register(const Request& req, Response& res, const Params& params) {
        auto body = nlohmann::json::parse(req.body());
        
        std::string email = body["email"];
        std::string username = body["username"];
        std::string password = body["password"];
        
        // Validate input
        if (!Validator::email(email)) {
            res.result(http::status::bad_request);
            res.body() = R"({"error":"Invalid email"})";
            co_return;
        }
        
        if (!Validator::password(password)) {
            nlohmann::json response;
            response["error"] = Validator::passwordError(password);
            res.body() = response.dump();
            res.result(http::status::bad_request);
            co_return;
        }
        
        // Create user
        auto user = co_await User::create(email, username, password);
        
        // Send welcome email
        SmtpClient smtp;
        std::string welcomeMessage = createWelcomeEmail(username);
        
        co_await smtp.send(email, welcomeMessage, "MyApp Team");
        
        // Send verification email
        std::string verificationCode = Code::generateRandomCode(32);
        std::string verifyMessage = createVerificationEmail(username, verificationCode);
        
        co_await smtp.send(email, verifyMessage, "Verification Service");
        
        // Save verification code
        co_await saveVerificationCode(email, verificationCode);
        
        res.result(http::status::created);
        nlohmann::json response;
        response["message"] = "User registered. Please check your email for verification.";
        res.body() = response.dump();
        co_return;
    }
    
private:
    std::string createWelcomeEmail(const std::string& username) {
        return R"(
            <h1>Welcome, )" + username + R"(!</h1>
            <p>Thank you for joining MyApp!</p>
            <p>We're excited to have you on board.</p>
        )";
    }
    
    std::string createVerificationEmail(const std::string& username, const std::string& code) {
        return R"(
            <h1>Verify Your Email</h1>
            <p>Hello )" + username + R"(,</p>
            <p>Please click the link below to verify your email address:</p>
            <p><a href="https://myapp.com/verify?code=)" + code + R"(">Verify Email</a></p>
            <p>This link expires in 24 hours.</p>
        )";
    }
};
```