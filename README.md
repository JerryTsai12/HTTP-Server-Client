# HTTP Server & Client

This project implements a web server and client application that supports file uploads, downloads, video streaming with DASH, and basic authentication.

## Features

- File upload and download functionality
- Video upload and streaming with DASH (Dynamic Adaptive Streaming over HTTP)
- Authentication for secure operations
- Simple web interface for file and video management

## Directory Structure

The project has the following directory structure:
```
.
├── client.cpp
├── const.h
├── httpRequest.cpp
├── httpRequest.h
├── httpResponse.cpp
├── httpResponse.h
├── makefile
├── server.cpp
├── secret                    # Authentication data (username:password pairs)
├── utils/
│   ├── base64.c
│   ├── base64.cpp
│   ├── base64.h
│   ├── tool.cpp
│   └── tool.h
└── web/
    ├── files/               # Directory for uploaded files
    ├── index.html
    ├── listf.rhtml
    ├── listv.rhtml
    ├── player.rhtml
    ├── tmp/                 # Directory for temporary files
    ├── uploadf.html
    ├── uploadv.html
    └── videos/              # Directory for uploaded videos
```

## Setup

### Compilation

To compile the server and client:

```bash
make all
```

For debugging:

```bash
make test
```

To clean compiled files:

```bash
make clean
```

### Authentication Setup

**Important:** Create your own `secret` file in the project root directory with username and password pairs in the following format:

```
username1:password1
username2:password2
```

Each line should contain a single username-password pair separated by a colon (`:`). The file should have the following characteristics:

- One username:password pair per line
- No spaces around the colon separator (unless they are part of the username or password)
- No empty lines (they will be ignored)
- No comments or additional formatting
- UTF-8 encoding is recommended for special characters

Examples of valid entries:

```
admin:admin123
user:password
```

The server will encode these credentials using Base64 for HTTP Basic Authentication.

## Usage

### Running the Server

```bash
./server <port>
```

Example:
```bash
./server 8080
```

### Running the Client

```bash
./client <host> <port> [username:password]
```

Example:
```bash
./client localhost 8080 user:password
```

### Client Commands

- `put <file>` - Upload a file to the server
- `putv <file>` - Upload a video file to the server
- `get <file>` - Download a file from the server
- `quit` - Exit the client

## Web Interface

The server provides a web interface accessible through a browser:

- Home page: `http://<host>:<port>/`
- File list: `http://<host>:<port>/file/`
- Video list: `http://<host>:<port>/video/`
- File upload page: `http://<host>:<port>/upload/file`
- Video upload page: `http://<host>:<port>/upload/video`

## API Endpoints

- `/api/file` - POST endpoint for file uploads
- `/api/file/<filename>` - GET endpoint for file downloads
- `/api/video` - POST endpoint for video uploads
- `/api/video/<videoname>/<file>` - GET endpoint for video streaming files

## Authentication

Protected endpoints (file/video uploading) require HTTP Basic Authentication with credentials defined in the `secret` file.

## Requirements

- C++17 compatible compiler
- POSIX-compliant operating system
- FFmpeg (for video transcoding)

## Notes

- Uploaded videos are automatically converted to DASH format for adaptive streaming
- Only MP4 video files (.mp4) are supported for video uploads
- Files are stored in the `web/files` directory
- Videos are stored in the `web/videos/<videoname>` directory
