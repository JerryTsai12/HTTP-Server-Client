#include <arpa/inet.h>
#include <fcntl.h>
#include <net/if.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

#include "./httpRequest.h"
#include "./httpResponse.h"
#include "./utils/base64.h"
#include "./utils/tool.h"
#include "const.h"

char buffer[BUFFER_SIZE];

string genBoundary(const string &fileContent) {
    string rands;
    string boundary = "----WebKitFormBoundary";
    while (true) {
        for (int i = 0; i < 16; i++) {
            rands += (char)(rand() % 26 + 'A');
        }
        if (fileContent.find(boundary + rands) == string::npos) break;
    }
    return (boundary + rands);
}

class Client {
   public:
    Client(string port, string host, string account);
    bool run();
    void closeClient();

   private:
    int serverfd;
    string host, port, account;
    HttpHeader headerRequest, headerResponse;
    void checkAccount();
    void upload(string url, string filePath);
    void download(string url, string filePath);
    void commandNotFound();
    void commandFailed();
    void commandSuccess();
    void commandUsage(string msg);
    void wrongAccount();
    void sendRequest(string request);
    void receiveResponse();
    void clientConnect();
};

Client::Client(string port, string host, string account) { 
    this->account = account;
    this->host = host;
    this->port = port;

    this->clientConnect();
    this->checkAccount();
    return;
}

void Client::clientConnect() {
    int sfd, s;
    struct addrinfo hints;
    struct addrinfo *result, *rp;

#ifdef DEBUG
    printf("port:%s, host:%s, account:%s\n", this->port.c_str(),
           this->host.c_str(), this->account.c_str());
#endif

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; /* Only allow IPv4 */
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = 0;
    hints.ai_protocol = 0; /* Any protocol */

    s = getaddrinfo(this->host.c_str(), this->port.c_str(), &hints, &result);
    if (s != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        exit(EXIT_FAILURE);
    }

    for (rp = result; rp != NULL; rp = rp->ai_next) {
        // Fix: Add null check before printing ai_canonname
        if (rp->ai_canonname != nullptr) {
            cout << rp->ai_canonname << endl;
        }
        sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sfd == -1) continue;
        if (connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1)
            break; /* Success */

        close(sfd);
    }

    freeaddrinfo(result);

    if (rp == NULL) {
        ERR_EXIT("Could not connect");
    }

    int flags = fcntl(sfd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl");
        exit(1);
    }

    flags &= ~O_NONBLOCK;

    if (fcntl(sfd, F_SETFL, flags) == -1) {
        perror("fcntl");
        exit(1);
    }

    this->serverfd = sfd;
    return;
}

void Client::closeClient() {
    close(this->serverfd);
    return;
}

void Client::sendRequest(string request) {
    this->headerRequest.clear();

#ifdef DEBUG
    printf("---Response to server %s ---\n", this->host.c_str());
    printf("%s\n", request.c_str());
    printf("---Response end---\n\n");
#endif
    send(this->serverfd, request.c_str(), request.length(), 0);

    return;
}

void Client::receiveResponse() {
    this->headerResponse.clear();
#ifdef DEBUG
    printf("in receiveResponse\n");
#endif

    ssize_t bytes_read;
    bytes_read = recv(this->serverfd, buffer, sizeof(buffer) - 1, 0);
    HttpHeader header;

    if (bytes_read <= 0) {
        close(this->serverfd);
        this->clientConnect();
        this->headerResponse = header;
        return;
    }

    string response = string(buffer, bytes_read);

#ifdef DEBUG
    printf("---%d bytes Received from server %s---\n", int(bytes_read),
           this->host.c_str());

    printf("%s\n",
           replaceAll(replaceAll(response, "\r", "\\r"), "\n", "\\n").c_str());
#endif

    HttpResponse httpResponse;
    header = httpResponse.parse(response);

    if (header.count(HTTP_HEADER_CONTENT_LENGTH) == 0) {
        this->headerResponse = header;
        return;
    }

    long unsigned int contentLength = stoul(header[HTTP_HEADER_CONTENT_LENGTH]);
    while (header[HTTP_BODY].length() < contentLength) {
        ssize_t bytes_read =
            recv(this->serverfd, buffer, sizeof(buffer) - 1, 0);
        header[HTTP_BODY] += string(buffer, bytes_read);
#ifdef DEBUG
        printf(
            "%s\n",
            replaceAll(replaceAll(buffer, "\r", "\\r"), "\n", "\\n").c_str());
#endif
    }

#ifdef DEBUG
    printf("\n---Received end---\n\n");
#endif

    this->headerResponse = header;
    return;
}

bool Client::run() {
    this->headerResponse.clear();
    this->headerRequest.clear();
    close(this->serverfd);

    string line;
    printf("> ");
    getline(cin, line);
    istringstream iss(line);

    string command;

    if (!(iss >> command)) {
        this->commandNotFound();
        return true;
    }

    string filePath;
    getline(iss, filePath);
    filePath.erase(0, filePath.find_first_not_of(" "));
    filePath.erase(filePath.find_last_not_of(" ") + 1,
                   filePath.size() - filePath.find_last_not_of(" "));

    if (command == "quit") {
        printf("Bye.\n");
        return false;
    }

    if (!regex_match(command, regex("(put)|(putv)|(get)"))) {
        this->commandNotFound();
        return true;
    }

    if (filePath.length() == 0) {
        this->commandUsage(command);
        return true;
    }

    if (command == "get") {
        this->clientConnect();
        this->download("/api/file/", filePath);
        return true;
    }

    if (!isFileExist(filePath)) {
        #ifdef DEBUG
        printf("File:%s not found.\n", filePath.c_str());
        #endif
        this->commandFailed();
        return true;
    }

    if (command == "put") {
        this->clientConnect();
        upload("/api/file", filePath);
        return true;
    }

    if (command == "putv") {
        this->clientConnect();
        upload("/api/video", filePath);
        return true;
    }

    return true;
}

void Client::checkAccount() {
    if (this->account == EMPTY) {
        return;
    }

    size_t csize;
    char *encoded = base64_encode((const unsigned char *)(account.c_str()),
                                  account.length(), &csize);

    this->account = "Basic " + string(encoded, csize);
    free(encoded);

    HttpRequest http(EMPTY);
    http.setHeader(HTTP_HEADER_METHOD, HTTP_METHOD_GET);
    http.setHeader(HTTP_HEADER_HOST, this->host + ":" + this->port);
    http.setHeader(HTTP_HEADER_PATH, "/upload/file");
    http.setHeader(HTTP_HEADER_AUTH, this->account);

    this->sendRequest(http.buildMessage());

    this->receiveResponse();
    if (this->headerResponse[HTTP_HEADER_STATUS] == "401") {
        this->wrongAccount();
    }

    return;
}

void Client::upload(string url, string filePath) {
    string fileContent = openFile(filePath);

    string fileName = filesystem::path(filePath).filename().string();
    string mimetype = decideMimeType(fileName);

    string boundary = genBoundary(fileContent);

    HttpRequest http(EMPTY);
    http.setHeader(HTTP_HEADER_METHOD, HTTP_METHOD_POST);
    http.setHeader(HTTP_HEADER_HOST, this->host + ":" + this->port);
    http.setHeader(HTTP_HEADER_PATH, url);
    http.setHeader(HTTP_HEADER_CONTENT_TYPE,
                   HTTP_HEADER_MULTY_PART + "; boundary=" + boundary);
    if (this->account != EMPTY) {
        http.setHeader(HTTP_HEADER_AUTH, this->account);
    }

    stringstream multipart;
    string body;
    body += "--" + boundary + CRLF;
    body += "Content-Disposition: form-data; name=\"upfile\"; filename=\"" +
            fileName + "\"" + CRLF;
    body += "Content-Type: " + mimetype + CRLF;
    body += CRLF;
    body += fileContent + CRLF;
    body += "--" + boundary + CRLF;
    body += "Content-Disposition: form-data; name=\"submit\"" + CRLF;
    body += CRLF;
    body += "Upload" + CRLF;
    body += "--" + boundary + "--" + CRLF;

    http.setHeader(HTTP_BODY, body);
    http.setHeader(HTTP_HEADER_CONTENT_LENGTH, to_string(body.length()));

#ifdef DEBUG
    for (auto c : replaceAll(replaceAll(http.buildMessage(), "\r", "\\r"), "\n",
                             "\\n")) {
        printf("%c", c);
    }
#endif

    do {
        this->sendRequest(http.buildMessage());
        this->receiveResponse();
    } while (this->headerResponse.size() == 0);

    if (headerResponse.count(HTTP_HEADER_STATUS) != 0 &&
        this->headerResponse[HTTP_HEADER_STATUS] == "200") {
        this->commandSuccess();
    } else {
        this->commandFailed();
    }

    return;
}

void Client::download(string url, string filePath) {
    HttpRequest http(EMPTY);
    http.setHeader(HTTP_HEADER_METHOD, HTTP_METHOD_GET);
    http.setHeader(HTTP_HEADER_HOST, this->host + ":" + this->port);
    http.setHeader(HTTP_HEADER_PATH, url + urlEncode(filePath));

    if (this->account != EMPTY) {
        http.setHeader(HTTP_HEADER_AUTH, this->account);
    }

    do {
        this->sendRequest(http.buildMessage());
        this->receiveResponse();
    } while (this->headerResponse.size() == 0);

    string downloadPath = "./files/" + filePath;
    if (this->headerResponse.count(HTTP_HEADER_STATUS) != 0 &&
        this->headerResponse[HTTP_HEADER_STATUS] == "200") {
        writeFile(downloadPath, this->headerResponse[HTTP_BODY]);
        this->commandSuccess();
    } else {
        this->commandFailed();
    }
    return;
}

void Client::commandSuccess() { printf("Command succeeded.\n"); }
void Client::commandNotFound() { 
    fprintf(stderr, "Command Not Found.\n");
    fprintf(stderr, "Available commands:\n");
    fprintf(stderr, "  put [file]   - Upload a file to server\n");
    fprintf(stderr, "  putv [file]  - Upload a video file to server\n");
    fprintf(stderr, "  get [file]   - Download a file from server\n");
    fprintf(stderr, "  quit         - Exit the client\n");
}
void Client::commandFailed() { fprintf(stderr, "Command failed.\n"); }
void Client::commandUsage(string command) {
    fprintf(stderr, "Usage: %s [file]\n", command.c_str());
}
void Client::wrongAccount() {
    fprintf(stderr, "Invalid user or wrong password.\n");
    exit(-1);
}

map<string, string> handle_argument(int argc, char *argv[]) {
    if (argc < 2 || argc > 4) {
        fprintf(stderr, "Usage: ./client [host] [port] [username:password]");
        exit(-1);
    }

    map<string, string> argm;
    argm["host"] = string(argv[1]);
    argm["port"] = string(argv[2]);
    if (argc == 4) {
        argm["account"] = string(argv[3]);
    } else {
        argm["account"] = EMPTY;
    }
    return argm;
}

int main(int argc, char *argv[]) {

#ifdef DEBUG
    cout << "Client start" << endl;
#endif

    map<string, string> argm = handle_argument(argc, argv);
    Client client(argm["port"], argm["host"], argm["account"]);
    makeFolder("./files/");

    while (client.run());

    client.closeClient();

    return 0;
}
