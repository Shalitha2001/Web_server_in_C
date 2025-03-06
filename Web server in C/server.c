
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef __APPLE__
#include <sys/uio.h>
#elif defined(__linux__)
#include <sys/sendfile.h>
#endif
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

/* Constants */
#define HTTP_BUFFER_SIZE 4096     // Size of our buffer for reading/writing
#define HTTP_PORT 8080           // Port number for the server
#define HTTP_ROOT_DIR "./public"  // Root directory for serving files

/* Function declarations */
char *parse_http_method(const char *request);
char *parse_file_path(const char *request);
char *get_file_extension(const char *file_path);
void handle_client_request(int client_socket, const char *method, const char *file_path, const char *file_ext);
void send_http_response(int client_socket, const char *file_path, int status_code, const char *status_msg, const char *content_type);
void create_http_header(char *buffer, int status_code, const char *status_msg, const char *content_type, long content_length);
const char *get_content_type(const char *extension);

/* Main function - Entry point of the web server */
int main(void) {
    int server_socket;
    struct sockaddr_in server_addr;
    
    // Create server socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Failed to create server socket");
        return EXIT_FAILURE;
    }
    
    // Set socket options for reuse
    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("Failed to set socket options");
        close(server_socket);
        return EXIT_FAILURE;
    }
    
    // Configure server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(HTTP_PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    
    // Bind socket to address
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Failed to bind server socket");
        close(server_socket);
        return EXIT_FAILURE;
    }
    
    // Listen for connections
    if (listen(server_socket, 10) < 0) {
        perror("Failed to listen on server socket");
        close(server_socket);
        return EXIT_FAILURE;
    }
    
    printf("Server is running on port %d\n", HTTP_PORT);
    
    // Main server loop
    while (1) {
        struct sockaddr_in http_client_addr;
        socklen_t client_addr_len = sizeof(http_client_addr);
        int client_socket;
        
        // Accept client connection
        client_socket = accept(server_socket, (struct sockaddr *)&http_client_addr, &client_addr_len);
        if (client_socket < 0) {
            perror("Failed to accept client connection");
            continue;
        }
        
        // Read HTTP request
        char request_buffer[HTTP_BUFFER_SIZE] = {0};
        ssize_t bytes_read = recv(client_socket, request_buffer, HTTP_BUFFER_SIZE - 1, 0);
        if (bytes_read < 0) {
            perror("Failed to read client request");
            close(client_socket);
            continue;
        }
        
        // Parse HTTP request
        char *method = parse_http_method(request_buffer);
        char *request_path = parse_file_path(request_buffer);
        char *file_extension = get_file_extension(request_path);

        // Log the request
        printf("Request: %s %s\n", 
               method ? method : "<invalid>", 
               request_path ? request_path : "<invalid>");
        
        // Handle the request
        handle_client_request(client_socket, method, request_path, file_extension);
        
        // Clean up
        free(method);
        free(request_path);
    }
    
    close(server_socket);
    return EXIT_SUCCESS;
}

/* Parse the HTTP method from the request */
char *parse_http_method(const char *request_data) {
    if (!request_data) return NULL;

    char *method = malloc(16);
    if (!method) {
        perror("Failed to allocate memory for HTTP method");
        return NULL;
    }
    
    if (sscanf(request_data, "%15s", method) != 1) {
        free(method);
        return NULL;
    }
    return method;
}

/* Parse the requested file path from the request */
char *parse_file_path(const char *request_data) {
    if (!request_data) return NULL;

    char *path = malloc(HTTP_BUFFER_SIZE);
    if (!path) {
        perror("Failed to allocate memory for request path");
        return NULL;
    }
    
    if (sscanf(request_data, "%*s %s", path) != 1) {
        free(path);
        return NULL;
    }
    return path;
}

/* Get file extension from the file path */
char *get_file_extension(const char *file_path) {
    if (!file_path) return NULL;
    return strrchr(file_path, '.');
}

/* Handle client request and generate appropriate response */
void handle_client_request(int client_fd, const char *http_method, const char *request_path, const char *file_ext) {
    // Check for valid request
    if (!http_method || !request_path) {
        send_http_response(client_fd, "./err/400.html", 400, "Bad Request", "text/html");
        return;
    }
    
    // Only handle GET requests
    if (strcmp(http_method, "GET") != 0) {
        send_http_response(client_fd, "./err/405.html", 405, "Method Not Allowed", "text/html");
        return;
    }
    
    // Build full file path
    char full_path[HTTP_BUFFER_SIZE];
    snprintf(full_path, HTTP_BUFFER_SIZE, "%s%s", HTTP_ROOT_DIR, request_path);
    
    // Handle directory requests (add index.html)
    if (!file_ext) {
        strcat(full_path, "/index.html");
        file_ext = ".html";
    }
    
    // Check content type
    const char *content_type = get_content_type(file_ext);
    if (!content_type) {
        send_http_response(client_fd, "./err/415.html", 415, "Unsupported Media Type", "text/html");
        return;
    }
    
    // Send the response
    send_http_response(client_fd, full_path, 200, "OK", content_type);
}

/* Send HTTP response to client */
void send_http_response(int client_fd, const char *file_path, int status_code, const char *status_msg, const char *content_type) {
    char http_header[HTTP_BUFFER_SIZE];
    struct stat file_info;
    int file_fd;
    
    // Try to open the requested file
    file_fd = open(file_path, O_RDONLY);
    if (file_fd < 0) {
        // File not found, send 404
        file_fd = open("./err/404.html", O_RDONLY);
        if (file_fd < 0) {
            perror("Failed to open error page");
            close(client_fd);
            return;
        }
        status_code = 404;
        status_msg = "Not Found";
        content_type = "text/html";
    }
    
    // Get file size
    if (fstat(file_fd, &file_info) < 0) {
        perror("Failed to get file information");
        close(file_fd);
        close(client_fd);
        return;
    }
    
    // Create and send header
    create_http_header(http_header, status_code, status_msg, content_type, file_info.st_size);
    send(client_fd, http_header, strlen(http_header), 0);
    
    // Send file content
    #ifdef __APPLE__
    off_t len = file_info.st_size;
    sendfile(file_fd, client_fd, 0, &len, NULL, 0);
    #elif defined(__linux__)
    sendfile(client_fd, file_fd, 0, file_info.st_size);
    #endif
    
    // Clean up
    close(file_fd);
    close(client_fd);
}

/* Create HTTP header */
void create_http_header(char *buffer, int status_code, const char *status_msg, const char *content_type, long content_length) {
    snprintf(buffer, HTTP_BUFFER_SIZE,
            "HTTP/1.1 %d %s\r\n"
            "Content-Type: %s\r\n"
            "Content-Length: %ld\r\n"
            "Connection: close\r\n"
            "\r\n",
            status_code, status_msg, content_type, content_length);
}

/* Get content type based on file extension */
const char *get_content_type(const char *ext) {
    if (!ext) return "text/html";
    
    struct {
        const char *ext;
        const char *type;
    } mime_types[] = {
        {".html", "text/html"},
        {".css", "text/css"},
        {".js", "application/javascript"},
        {".png", "image/png"},
        {".jpg", "image/jpeg"},
        {".jpeg", "image/jpeg"},
        {".mp4", "video/mp4"},
        {".webm", "video/webm"},
        {".ogg", "video/ogg"},
        {".avi", "video/x-msvideo"},
        {".mpeg", "video/mpeg"},
        {NULL, NULL}
    };
    
    for (int i = 0; mime_types[i].ext; i++) {
        if (strcmp(ext, mime_types[i].ext) == 0)
            return mime_types[i].type;
    }
    return NULL;
}