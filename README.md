# 🚀 HTTP Server in C++

A lightweight HTTP server built from scratch in C++ using raw POSIX sockets — no frameworks, no libraries.
---

## Features

- Accepts **GET requests** over TCP
- Serves **static files** (HTML, CSS, JS, images)
- Correct **MIME types** per file extension
- **404 / 403 / 405** error responses
- Protection against **path traversal** attacks (`../../etc/passwd`)
- Zero dependencies — only POSIX sockets

---

## Project Structure

```
http_server/
├── server.cpp        # Full server implementation (~150 lines)
├── Makefile
└── www/              # Static files to serve
    ├── index.html
    ├── about.html
    └── style.css
```

---

## Getting Started

### Build & Run

```bash
make run
```

Or manually:

```bash
g++ -std=c++17 -o server server.cpp
./server
```

Then open **http://localhost:8080** in your browser.

---

## How It Works

```
Browser                        Server
  |                              |
  |  TCP connect :8080           |
  |----------------------------->|
  |                              |
  |  GET /index.html HTTP/1.1    |
  |----------------------------->|
  |                              |  1. recv() — read request
  |                              |  2. Parse method + path
  |                              |  3. Read file from ./www/
  |                              |  4. Build HTTP response
  |                              |
  |  HTTP/1.1 200 OK             |
  |  Content-Type: text/html     |
  |  ...                         |
  |<-----------------------------|
```

### Key system calls used

| Call | Purpose |
|------|---------|
| `socket()` | Create a TCP socket |
| `bind()` | Attach to port 8080 |
| `listen()` | Start accepting connections |
| `accept()` | Block and wait for a client |
| `recv()` | Read the HTTP request |
| `send()` | Send the HTTP response |
| `close()` | Close the connection |

---

## What I Learned

- How TCP sockets work at the OS level
- The structure of raw HTTP/1.1 requests and responses
- File I/O and MIME type detection
- Basic security: path traversal prevention
- Network byte order (`htons`)

---

## Roadmap

- [ ] Multithreading (`std::thread` per connection)
- [ ] Keep-alive connections
- [ ] Config file (port, www directory)
- [ ] Request logging to file
- [ ] POST request support
