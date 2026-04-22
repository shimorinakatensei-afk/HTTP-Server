#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <cstring>

// Network headers (Linux / macOS)
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

// ─────────────────────────────────────────────
//  SETTINGS
// ─────────────────────────────────────────────
const int         PORT    = 8080;
const int         BACKLOG = 10;      // max queued connections
const std::string WWW     = "./www"; // folder with static files

// ─────────────────────────────────────────────
//  MIME types by file extension
// ─────────────────────────────────────────────
std::map<std::string, std::string> MIME = {
    {".html", "text/html; charset=utf-8"},
    {".css",  "text/css"},
    {".js",   "application/javascript"},
    {".png",  "image/png"},
    {".jpg",  "image/jpeg"},
    {".ico",  "image/x-icon"},
    {".txt",  "text/plain"},
};

// Get file extension: "index.html" → ".html"
std::string get_extension(const std::string& path) {
    size_t dot = path.rfind('.');
    if (dot == std::string::npos) return "";
    return path.substr(dot);
}

// Read entire file into a string
bool read_file(const std::string& path, std::string& out) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) return false;
    std::ostringstream ss;
    ss << file.rdbuf();
    out = ss.str();
    return true;
}

// Parse the first line of an HTTP request: "GET /index.html HTTP/1.1"
bool parse_request_line(const std::string& raw, std::string& method, std::string& path) {
    std::istringstream ss(raw);
    std::string version;
    ss >> method >> path >> version;
    return !method.empty() && !path.empty();
}

// Build an HTTP response
std::string build_response(int code, const std::string& reason,
                           const std::string& content_type,
                           const std::string& body) {
    std::ostringstream resp;
    resp << "HTTP/1.1 " << code << " " << reason << "\r\n";
    resp << "Content-Type: "   << content_type      << "\r\n";
    resp << "Content-Length: " << body.size()        << "\r\n";
    resp << "Connection: close\r\n";
    resp << "\r\n";
    resp << body;
    return resp.str();
}

// Build a simple HTML error page
std::string error_page(int code, const std::string& reason) {
    std::string body =
        "<!DOCTYPE html><html><head><title>" + std::to_string(code) + "</title></head>"
        "<body><h1>" + std::to_string(code) + " " + reason + "</h1></body></html>";
    return build_response(code, reason, "text/html; charset=utf-8", body);
}

// Handle a single client connection
void handle_client(int client_fd) {
    char buf[4096] = {};
    ssize_t n = recv(client_fd, buf, sizeof(buf) - 1, 0);
    if (n <= 0) { close(client_fd); return; }

    std::string raw(buf);
    std::string method, url_path;
    if (!parse_request_line(raw, method, url_path)) {
        std::string resp = error_page(400, "Bad Request");
        send(client_fd, resp.c_str(), resp.size(), 0);
        close(client_fd);
        return;
    }

    std::cout << "[->] " << method << " " << url_path << "\n";

    if (method != "GET") {
        std::string resp = error_page(405, "Method Not Allowed");
        send(client_fd, resp.c_str(), resp.size(), 0);
        close(client_fd);
        return;
    }

    // Strip query string: "/page?id=1" -> "/page"
    size_t q = url_path.find('?');
    if (q != std::string::npos) url_path = url_path.substr(0, q);

    // Root path -> index.html
    if (url_path == "/") url_path = "/index.html";

    // Block path traversal attacks: "../../etc/passwd"
    if (url_path.find("..") != std::string::npos) {
        std::string resp = error_page(403, "Forbidden");
        send(client_fd, resp.c_str(), resp.size(), 0);
        close(client_fd);
        return;
    }

    std::string file_path = WWW + url_path;
    std::string body;
    if (!read_file(file_path, body)) {
        std::string resp = error_page(404, "Not Found");
        send(client_fd, resp.c_str(), resp.size(), 0);
        close(client_fd);
        return;
    }

    std::string ext  = get_extension(url_path);
    std::string mime = MIME.count(ext) ? MIME[ext] : "application/octet-stream";

    std::string resp = build_response(200, "OK", mime, body);
    send(client_fd, resp.c_str(), resp.size(), 0);

    std::cout << "[OK] 200 -> " << file_path << "\n";
    close(client_fd);
}

int main() {
    // Create a TCP/IPv4 socket
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) { perror("socket"); return 1; }

    // Allow reusing the port immediately after restart
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // Bind to port
    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;   // listen on all interfaces
    addr.sin_port        = htons(PORT);  // htons: host -> network byte order

    if (bind(server_fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind"); return 1;
    }

    if (listen(server_fd, BACKLOG) < 0) { perror("listen"); return 1; }

    std::cout << "Server running at http://localhost:" << PORT << "\n";
    std::cout << "Serving files from: " << WWW << "\n\n";

    // Main loop: accept connections one by one
    while (true) {
        sockaddr_in client_addr{};
        socklen_t   client_len = sizeof(client_addr);

        int client_fd = accept(server_fd, (sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) { perror("accept"); continue; }

        handle_client(client_fd);
    }

    
    close(server_fd);
    return 0;
}
