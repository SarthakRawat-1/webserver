
# HTTP Web Server

## Overview

This project implements a **HTTP Web Server** with the following features:

-   **Lightweight & Efficient**: Minimalist design optimized for handling multiple client `GET` requests.
-   **Dynamic MIME Type Handling**: Determines file types dynamically to serve correct content types.
-   **Basic Routing Support**: Serves static files like `Text`, `HTML`, `CSS`, `JS`, `Images` and `PDFs`. Note that serving `CSS` and `JS` files directly in browser means that only their raw content will appear which is their default behavior.
-   **Security Measures**: Blocks directory traversal attacks to prevent unauthorized file access.
-   **Multi-Client Handling**: Manages multiple connections using fork-based processing.


## Setup Instructions

1.  **Compilation**:
    
    ```bash
    gcc webserver.c -o webserver
    ```
    
2.  **Execution**:
    
    ```bash
    ./webserver <port_number>
    ```
    
    Example:
    
    ```bash
    ./webserver 8080
    ```
   

## Testing Endpoints 

1.  **Home Route**:
    
    ```
    http://localhost:<port_number>/
    ```
    
2.  **Route for serving CSS**:
    
    ```
    http://localhost:<port_number>/css/style.css
    ```
    
3.  **Route for serving JS**:
    
    ```
    http://localhost:<port_number>/js/script.js
    ```
    
4.  **Route for serving Images**:
    
    ```
    http://localhost:<port_number>/img/test.png
    http://localhost:<port_number>/img/shogun.jpg
    ```

5.  **Route for serving PDF**:
    
    ```
    http://localhost:<port_number>/download/sample.pdf
    ```
    

## Code Overview

1. **Main Components**:
    - `sHttpRequest`/`httpreq`: Structure storing HTTP method and requested URL
    - `sFile`/`File`: Structure containing filename, file content buffer, and size
    - `srv_init()`: Initializes server socket and starts listening
    - `cli_accept()`: Accepts incoming client connections
    - `parse_http()`: Parses raw HTTP request into structured format
    - `cli_conn()`: Main client connection handler function

2. **Concurrency Model**:
    - Uses `fork()` to create child processes for each client connection.

3. **HTTP Protocol Implementation**:
    - `http_headers()`: Sends HTTP response headers with status code
    - `http_response()`: Constructs complete HTTP response body
    - `sendfile()`: Streams files with proper MIME type headers
    - `get_mime_type()`: Determines Content-Type from file extension
    - Supports GET method and common HTTP status codes (200, 404, 403, 501)

4. **File Handling**:
    - `readfile()`: Reads file contents into memory buffer
    - Serves static files from `BASE_DIR` (./www directory)
    - Handles common web formats (HTML/CSS/JS/Images/PDF)
    - Dynamic path resolution for `/`, `/img/`, `/css/`, `/js/`, and `/download/`

5. **Security & Validation**:
    - Directory traversal prevention (blocks URLs containing "..")
    - HTTP method validation (rejects non-GET methods with 501)
    - MIME type verification for safe content delivery

6. **Error Handling**:
    - Global `error` string for tracking error messages
    - `http_response()` used for sending error messages to clients
    - Handles file not found (404), forbidden paths (403), and unimplemented methods (501)
    - Socket error handling for bind/listen/accept failures

## Notes

-   The server can only serve GET requests and Static files.
-   The server uses port 8080 by default but can be customized.

## Dependencies

-   GCC compiler

## License

This project is licensed under the MIT License.

