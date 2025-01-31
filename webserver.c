#include <netinet/in.h>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <libgen.h>

#define LISTENADDR "0.0.0.0"  // Listen on all interfaces
#define BASE_DIR "./www"      // Web server root directory


void hexdump(char*,int);

/* Debugging Function */
void hexdump(char*,int);  // Displays binary data in hex format

/* HTTP Request Structure */
struct sHttpRequest {
    char method[8];       // HTTP method (GET, POST, etc.)
    char url[128];        // Requested URL/path
};
typedef struct sHttpRequest httpreq;

/* File Handling Structure */
struct sFile {
    char filename[64];    // Name of file
    char *fc;             // Pointer to file content
    int size;             // File size in bytes
};
typedef struct sFile File;

/* Global Variables */
char *error;  // Stores error messages for error handling

/**
 * Determines MIME type based on file extension
 * @param filename: Name/path of the file
 * @return: MIME type as C string
 */
const char* get_mime_type(const char *filename) {
    const char *ext = strrchr(filename, '.');  // Find last '.' occurrence
    if (!ext) return "application/octet-stream";  // Default for unknown
    
    // Compare extensions with known MIME types
    if (strcmp(ext, ".html") == 0) return "text/html";
    if (strcmp(ext, ".css") == 0) return "text/css";
    if (strcmp(ext, ".js") == 0) return "application/javascript";
    if (strcmp(ext, ".png") == 0) return "image/png";
    if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0) return "image/jpeg";
    if (strcmp(ext, ".gif") == 0) return "image/gif";
    if (strcmp(ext, ".pdf") == 0) return "application/pdf";
    
    return "text/plain";  // Fallback type
}

/**
 * Initializes server socket
 * @param portno: Port number to listen on
 * @return: Socket file descriptor or 0 on error
 */
int srv_init(int portno) {
    int s;
    struct sockaddr_in srv;

    // Create TCP socket
    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0) {
        error = "socket() error";
        return 0;
    }

    // Configure server address structure
    srv.sin_family = AF_INET;                  // IPv4
    srv.sin_addr.s_addr = inet_addr(LISTENADDR); // Bind to all interfaces
    srv.sin_port = htons(portno);              // Convert port to network byte order

    // Bind socket to address
    if (bind(s, (struct sockaddr *)&srv, sizeof(srv))) {
        close(s);
        error = "bind() error";
        return 0;
    }

    // Start listening with backlog of 5
    if (listen(s, 5)) {
        close(s);
        error = "listen() error";
        return 0;
    }

    return s;  // Return listening socket
}

/**
 * Accepts incoming client connection
 * @param s: Server socket descriptor
 * @return: Client socket descriptor or 0 on error
 */
int cli_accept(int s) {
    int c;
    struct sockaddr_in cli;
    socklen_t addrlen = sizeof(cli);

    c = accept(s, (struct sockaddr *)&cli, &addrlen);
    if (c < 0) {
        error = "accept() error";
        return 0;
    }

    return c;
}

/**
 * Parses HTTP request line
 * @param str: Raw HTTP request string
 * @return: httpreq structure or NULL on error
 */
httpreq *parse_http(char *str) {
    httpreq *req = malloc(sizeof(httpreq));
    char *p = str;

    // Extract method (first token before space)
    while (*p && *p != ' ') p++;
    if (*p != ' ') {
        error = "parse_http() NOSPACE error";
        free(req);
        return NULL;
    }
    *p = '\0';  // Null-terminate method
    strncpy(req->method, str, 7);

    // Move to URL part
    str = ++p;
    while (*p && *p != ' ') p++;
    if (*p != ' ') {
        error = "parse_http() 2NDSPACE error";
        free(req);
        return NULL;
    }
    *p = '\0';  // Null-terminate URL
    strncpy(req->url, str, 127);

    return req;
}

/**
 * Reads data from client socket
 * @param c: Client socket descriptor
 * @return: Pointer to read buffer or NULL on error
 */
char *cli_read(int c) {
    static char buf[512];  // Static buffer (non-reentrant)
    if (read(c, buf, 511) < 0) {
        error = "read() error";
        return NULL;
    }
    return buf;
}

/**
 * Sends HTTP response headers
 * @param c: Client socket
 * @param code: HTTP status code
 */
void http_headers(int c, int code) {
    char buf[512];
    snprintf(buf, sizeof(buf),
        "HTTP/1.0 %d OK\n"
        "Server: httpd.c\n"
        "Cache-Control: no-store, no-cache, max-age=0, private\n"
        "Content-Language: en\n"
        "Expires: -1\n"
        "X-Frame-Options: SAMEORIGIN\n",
        code);
    write(c, buf, strlen(buf));
}

/**
 * Sends HTTP response body
 * @param c: Client socket
 * @param contenttype: MIME type of content
 * @param data: Response body content
 */
void http_response(int c, char *contenttype, char *data) {
    char buf[512];
    snprintf(buf, sizeof(buf),
        "Content-Type: %s\n"
        "Content-Length: %zu\n\n%s",
        contenttype, strlen(data), data);
    write(c, buf, strlen(buf));
}

/**
 * Reads file into memory
 * @param filename: Path to file
 * @return: File structure or NULL on error
 */
File *readfile(char *filename) {
    int fd = open(filename, O_RDONLY);
    if (fd < 0) return NULL;

    File *f = malloc(sizeof(File));
    strncpy(f->filename, filename, 63);
    f->fc = malloc(512);  // Initial buffer
    int x = 0, n;

    while ((n = read(fd, f->fc + x, 512)) > 0) {
        x += n;
        f->fc = realloc(f->fc, x + 512);  // Expand buffer
    }

    f->size = x;
    close(fd);
    return f;
}

/**
 * Sends file through socket with proper MIME type
 * @param c: Client socket
 * @param file: File structure to send
 * @return: 1 on success, 0 on failure
 */
int sendfile(int c, File *file) {
    const char *mime_type = get_mime_type(file->filename);
    char headers[512];
    snprintf(headers, sizeof(headers),
        "Content-Type: %s\n"
        "Content-Length: %d\n\n",
        mime_type, file->size);
    write(c, headers, strlen(headers));

    // Send file content in chunks
    int remaining = file->size;
    char *p = file->fc;
    while (remaining > 0) {
        int chunk = (remaining < 512) ? remaining : 512;
        if (write(c, p, chunk) <= 0) return 0;
        p += chunk;
        remaining -= chunk;
    }
    return 1;
}

/**
 * Handles client connection lifecycle
 * @param s: Server socket (unused)
 * @param c: Client socket
 */
void cli_conn(int s, int c) {
    char *request = cli_read(c);
    if (!request) {
        fprintf(stderr, "Read error: %s\n", error);
        close(c);
        return;
    }

    httpreq *req = parse_http(request);
    if (!req) {
        fprintf(stderr, "Parse error: %s\n", error);
        close(c);
        return;
    }

    printf("Received %s request for %s\n", req->method, req->url);

    // Security: Block directory traversal
    if (strstr(req->url, "..")) {
        http_headers(c, 403);
        http_response(c, "text/plain", "403 Forbidden: Directory traversal attempt");
        goto cleanup;
    }

    // Route handling
    if (strcmp(req->method, "GET") == 0) {
        char fullpath[256];
        snprintf(fullpath, sizeof(fullpath), "%s%s", BASE_DIR, req->url);

        // Handle root path
        if (strcmp(req->url, "/") == 0) {
            snprintf(fullpath, sizeof(fullpath), "%s/index.html", BASE_DIR);
        }

        File *f = readfile(fullpath);
        if (f) {
            http_headers(c, 200);
            sendfile(c, f);
            free(f->fc);
            free(f);
        } else {
            http_headers(c, 404);
            http_response(c, "text/plain", "404 Not Found");
        }
    } else {
        http_headers(c, 501);
        http_response(c, "text/plain", "501 Not Implemented");
    }

cleanup:
    free(req);
    close(c);
}

/**
 * Main server loop
 */
int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int server_fd = srv_init(atoi(argv[1]));
    if (!server_fd) {
        fprintf(stderr, "Server init failed: %s\n", error);
        return EXIT_FAILURE;
    }

    printf("Server running on %s:%s\n", LISTENADDR, argv[1]);
    
    while(1) {
        int client_fd = cli_accept(server_fd);
        if (!client_fd) {
            fprintf(stderr, "Accept error: %s\n", error);
            continue;
        }

        // Fork child process to handle client
        if (fork() == 0) {
            close(server_fd);  // Child doesn't need listener
            cli_conn(server_fd, client_fd);
            exit(EXIT_SUCCESS);
        }
        close(client_fd);  // Parent closes client socket
    }

    return EXIT_SUCCESS;  // Never reached
}