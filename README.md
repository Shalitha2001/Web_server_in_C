### **Lightweight HTTP server in C**

This is my web server project in C is a lightweight HTTP server that listens on port 8080 and serves static files from a public directory. Here’s a brief overview of its key features:

### **Key Features:**

- **Socket Programming:**
Uses the **`socket()`**, **`bind()`**, and **`listen()`** system calls to establish a server that accepts incoming HTTP requests.

- **Handling HTTP Requests:**
It parses **GET** requests, extracts the requested file path, and determines the corresponding content type.

- **Serving Static Files:**
Reads files from the **public** directory and sends them to the client. If the requested path is a directory, it serves `index.html` by default.

- **Error Handling:**
Responds with appropriate HTTP status codes:
  - **404 Not Found** if the file doesn’t exist.
  - **400 Bad Request** for malformed requests.
  - **405 Method Not Allowed** for unsupported methods (only **GET** is allowed).
  - **415 Unsupported Media Type** if the file type is unrecognized.
- **MIME Type Detection:**
Determines the content type based on file extensions (`.html`, `.css`, `.js`, `.png`, `.jpg`, etc.).

- **Sendfile Optimization:**
Uses **`sendfile()`** (Linux) or **`sendfile()` with `off_t`** (macOS) to efficiently transfer files.

### **How It Works:**
1. The server starts and listens on port **8080**.
2. When a client connects, it reads the HTTP request.
3. It extracts the request method and file path.
4. It determines the content type and serves the requested file.
5. If the file is missing, it serves an appropriate error page.
6. The connection is closed after sending the response.

This server is a great foundational project for learning **network programming**, **HTTP fundamentals**, and **file handling** in C.

### **How to Run This Web Server**  

#### **For Linux** 
1. **Install necessary tools (if not installed):**  
   ```sh
   sudo apt update
   sudo apt install build-essential
   ```
2. **Compile the server:**  
   ```sh
   gcc -o web_server web_server.c
   ```
3. **Run the server:**  
   ```sh
   ./web_server
   ```
4. **Access the server:**  
   Open a browser and visit:  
   ```
   http://localhost:8080
   ```

#### **For macOS**   
1. **Install necessary tools (if not installed):**  
   ```sh
   xcode-select --install
   ```
2. **Compile the server:**  
   ```sh
   gcc -o web_server web_server.c
   ```
3. **Run the server:**  
   ```sh
   ./web_server
   ```
4. **Access the server:**  
   Open a browser and visit:  
   ```
   http://localhost:8080
   ```
