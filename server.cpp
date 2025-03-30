#include <arpa/inet.h>
#include <net/if.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <regex>
#include <set>
#include <sstream>
#include <vector>

#include "const.h"
#include "httpRequest.h"
#include "httpResponse.h"
#include "utils/base64.h"
#include "utils/tool.h"

using namespace std;

// global variables
char buffer[BUFFER_SIZE];

// function prototypes
int handle_argument(int argc, char *argv[]);

class Server {
   public:
    Server(int port, set<string> secrets);
    ~Server();
    void run();
    int newConnection();
    void closeConnection(int index, int client_fd);
    HttpHeader receiveRequest(int index, int client_fd);
    void sendResponse(int client_fd, string response);

    string route(HttpHeader header);
    string routeFile();
    string routeVideo();
    string routeVideoPlayer(string videoName);
    string routeApiFile(HttpHeader header);
    string routeApiFileGet(string fileName);
    string routeApiVideo(HttpHeader header);
    string routeApiVideoGet(string fileName);

   private:
    int server_fd;
    set<string> secrets;
    vector<pollfd> poll_fds;
};

Server::Server(int port, set<string> secrets) {
    struct sockaddr_in server_addr;

    // Get socket file descriptor
    if ((this->server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        ERR_EXIT("socket()");
    }

    // Set server address information
    bzero(&server_addr, sizeof(server_addr));  // erase the data
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port);

    // Bind the server file descriptor to the server address
    if (::bind(this->server_fd, (struct sockaddr *)&server_addr,
             sizeof(server_addr)) < 0) {
        ERR_EXIT("bind()");
    }

    // for server to listen
    if (listen(this->server_fd, QUEUE_SIZE) < 0) {
        ERR_EXIT("listen()");
    }

    struct pollfd pfd;
    pfd.fd = this->server_fd;
    pfd.events = POLLIN;
    poll_fds.push_back(pfd);

    // secrets
    this->secrets = secrets;
    return;
}

Server::~Server() {
    close(this->server_fd);
    return;
}

void Server::run() {
#ifdef DEBUG
    cout << "poll..." << endl;
#endif
    int ready = poll(poll_fds.data(), poll_fds.size(), -1);

    // poll error
    if (ready == -1) {
        ERR_EXIT("poll()");
    }

#ifdef DEBUG
    cout << "handling clients..." << endl;
#endif

    for (size_t i = 0; i < poll_fds.size(); i++) {
// since we always send the response intantly
#ifdef DEBUG
        cout << "poll size: " << poll_fds.size() << " | handling client:" << i
             << endl;
#endif

        if (poll_fds[i].revents == 0 || !(poll_fds[i].revents & POLLIN)) {
            continue;
        }

        int client_fd, index;
        if (poll_fds[i].fd == server_fd) {
            client_fd = this->newConnection();
            index = poll_fds.size() - 1;
#ifdef DEBUG
            cout << "client:" << index << " | at server_fd: " << server_fd
                 << " | is newConnection" << endl;
#endif
        } else {
            client_fd = this->poll_fds[i].fd;
            index = i;
#ifdef DEBUG
            cout << "client:" << i << " | at client_fd: " << client_fd
                 << " | is old" << endl;
#endif
        }

        HttpHeader requestHeader = this->receiveRequest(index, client_fd);
        if (requestHeader.size() == 0) {
            continue;
        }

        string messages;
        try {
            messages = this->route(requestHeader);
            if (messages == EMPTY) {
                HttpResponse http;
                http.setHeader500();
                messages = http.buildMessage();
            }
        } catch (const std::exception &e) {
            HttpResponse http;
            http.setHeader500();
            messages = http.buildMessage();
        }

        this->sendResponse(client_fd, messages);

        if (requestHeader.count(HTTP_HEADER_CONNECTION) != 0 &&
            requestHeader[HTTP_HEADER_CONNECTION] == "close") {
            this->closeConnection(index, client_fd);
        }
        break;
    }
    return;
}

// handle new client connection
int Server::newConnection() {
#ifdef DEBUG
    cout << "newConnection, ";
#endif
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    int client_fd =
        accept(this->server_fd, (struct sockaddr *)&client_addr, &client_len);

#ifdef DEBUG
    cout << "accept client_fd: " << client_fd << endl;
#endif

    if (client_fd < 0) {
        cerr << ERR_ACCEPT << endl;
        exit(-1);
    }

    struct pollfd pfd;
    pfd.fd = client_fd;
    pfd.events = POLLIN;
    poll_fds.push_back(pfd);

    return client_fd;
}

void Server::closeConnection(int index, int client_fd) {
    close(client_fd);
    this->poll_fds.erase(this->poll_fds.begin() + index);
#ifdef DEBUG
    cout << "poll fds size: " << poll_fds.size() << endl;
    cout << "client:" << index
         << " disconnected. Total clients: " << this->poll_fds.size() - 1
         << endl;
#endif

    return;
}

// return the request from client
HttpHeader Server::receiveRequest(int index, int client_fd) {
#ifdef DEBUG
    cout << "in receiveRequest" << endl;
#endif

    ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
    HttpHeader header;

    if (bytes_read <= 0) {
#ifdef DEBUG
        cout << "bytes_read=" << bytes_read << " disconnected" << endl;
#endif
        this->closeConnection(index, client_fd);
        return header;
    }

    string request = string(buffer, bytes_read);

#ifdef DEBUG
    cout << "---Received from client" << client_fd << "---" << endl;
    cout << replaceAll(replaceAll(request, "\r", "\\r"), "\n", "\\n");
#endif

    HttpRequest httpRequest(request);
    header = httpRequest.parseRequest();

    if (header[HTTP_HEADER_METHOD] == HTTP_METHOD_GET) {
        return header;
    }

    int contentLength = stoi(header[HTTP_HEADER_CONTENT_LENGTH]);
    while (header[HTTP_BODY].length() < contentLength) {
        ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
        header[HTTP_BODY] += string(buffer, bytes_read);
#ifdef DEBUG
        cout << replaceAll(replaceAll(buffer, "\r", "\\r"), "\n", "\\n")
             << endl;
#endif
    }

#ifdef DEBUG
    cout << endl;
    cout << "---Received end---" << endl << endl;
#endif

    return header;
}

void Server::sendResponse(int client_fd, string response) {
#ifdef DEBUG
    cout << "---Response to client" << client_fd << " ---" << endl;
    cout << response << endl;
    cout << "---Response end---" << endl << endl;
#endif
    send(client_fd, response.c_str(), response.length(), 0);
    return;
}

// handle the request routing.
// return the response http message
string Server::route(HttpHeader requestHeader) {
    string messages;

    // if the endpoint is not valid
    string requestPath = requestHeader[HTTP_HEADER_PATH];
    string validPathRegex = string("(/)") + "|(/upload/(file|video))" +
                            "|(/file/)" + "|(/video/.*)" +
                            "|(/api/(file|video))" + "|(/api/(file|video)/.*)";

    if (!regex_match(requestPath, regex(validPathRegex))) {
        HttpResponse http;
        http.setHeader404();
        return http.buildMessage();
    }

    // .substr(1) to remove the first "/"
    // due to the split function will stop when it meet first empty string
    // split("/video", "/") -> {"video"}
    // split("/video/aaaa", "/") -> {"video/aaaa"}
    vector<string> pathSplited = split(requestPath.substr(1), SLASH);
    string method = requestHeader[HTTP_HEADER_METHOD];

    // check method
    // for api/file/{filename} and api/video/{filename}, only allow POST method
    if (regex_match(requestPath, regex("/api/(file|video)")) &&
        method != HTTP_METHOD_POST) {
        HttpResponse http;
        http.setHeader405(HTTP_METHOD_POST);
        return http.buildMessage();
    }
    if (!regex_match(requestPath, regex("/api/(file|video)")) &&
        method != HTTP_METHOD_GET) {
        HttpResponse http;
        http.setHeader405(HTTP_METHOD_GET);
        return http.buildMessage();
    }

    // check auth
    // for "/upload/file", "/upload/video",  "api/file/", "api/video/", check
    // the auth
    if (regex_match(requestPath,
                    regex("(/api/(file|video))|(/upload/(file|video))"))) {

        HttpResponse http;
        http.setHeader(HTTP_HEADER_STATUS, HTTP_401);
        http.setHeader(HTTP_HEADER_CONTENT_TYPE, MIME_TEXT_PLAIN);
        http.setHeader(HTTP_HEADER_WWWAUTH, "Basic realm=b11902040");
        http.setHeader(HTTP_BODY, HTTP_401_BODY);

        if (requestHeader.find(HTTP_HEADER_AUTH) == requestHeader.end()) {
            return http.buildMessage();
        }

#ifdef DEBUG
        cout << "auth: " << requestHeader[HTTP_HEADER_AUTH] << endl;
#endif

        size_t pos = requestHeader[HTTP_HEADER_AUTH].find(" ");
        string authMethod = requestHeader[HTTP_HEADER_AUTH].substr(0, pos);
        string credentials = requestHeader[HTTP_HEADER_AUTH].substr(pos + 1);

        if (toLowerCase(authMethod) != "basic" ||
            this->secrets.find(credentials) == this->secrets.end()) {
            return http.buildMessage();
        }
    }

    // for "/"
    if (regex_match(requestPath, regex("/"))) {
        HttpResponse http;
        http.setHeader(HTTP_HEADER_STATUS, HTTP_200);
        http.setHeader(HTTP_HEADER_CONTENT_TYPE, MIME_TEXT_HTML);
        http.setHeader(HTTP_BODY, openFile(FILE_PATH_INDEX));

        return http.buildMessage();
    }

    // for "/upload/file"
    if (regex_match(requestPath, regex("/upload/file"))) {
        HttpResponse http;
        http.setHeader(HTTP_HEADER_STATUS, HTTP_200);
        http.setHeader(HTTP_HEADER_CONTENT_TYPE, MIME_TEXT_HTML);
        http.setHeader(HTTP_BODY, openFile(FILE_PATH_UPLOAD_FILE));

        return http.buildMessage();
    }

    // for "/upload/video"
    if (regex_match(requestPath, regex("/upload/video"))) {
        HttpResponse http;
        http.setHeader(HTTP_HEADER_STATUS, HTTP_200);
        http.setHeader(HTTP_HEADER_CONTENT_TYPE, MIME_TEXT_HTML);
        http.setHeader(HTTP_BODY, openFile(FILE_PATH_UPLOAD_VIDEO));

        return http.buildMessage();
    }

    // for "/file/"
    if (regex_match(requestPath, regex("/file/"))) {
        return this->routeFile();
    }

    // for "/video/"
    if (regex_match(requestPath, regex("/video/"))) {
        return this->routeVideo();
    }

    // for "/video/{videoname}"
    if (regex_match(requestPath, regex("/video/.+"))) {
        return this->routeVideoPlayer(urlDecode(pathSplited[1]));
    }

    // for "/api/file"
    if (regex_match(requestPath, regex("/api/file"))) {
        return this->routeApiFile(requestHeader);
    }

    // for "/api/file/{filepath}"
    if (regex_match(requestPath, regex("/api/file/.+"))) {
        return this->routeApiFileGet(urlDecode(pathSplited[2]));
    }

    // for "/api/video"
    if (regex_match(requestPath, regex("/api/video"))) {
        return this->routeApiVideo(requestHeader);
    }

    // for "/api/video/{filepath}"
    if (regex_match(requestPath, regex("/api/video/.+"))) {
        return this->routeApiVideoGet(
            urlDecode(pathSplited[2] + "/" + pathSplited[3]));
    }

    HttpResponse http;
    http.setHeader500();
    return http.buildMessage();
}

string Server::routeFile() {
    string fileContent = openFile(FILE_PATH_LIST_FILE);
    size_t posBegin = fileContent.find(TAG_FILE_LIST);
    fileContent.erase(posBegin, TAG_FILE_LIST.length());

    set<string> fileNames = listFile(FOLDER_PATH_FILES);

    for (auto name : fileNames) {
        string tmp = "<tr><td><a href=\"/api/file/" + urlEncode(name) + "\">" +
                     name + "</a></td></tr>\n";
        fileContent.insert(posBegin, tmp);
    }

    HttpResponse http;
    http.setHeader(HTTP_HEADER_STATUS, HTTP_200);
    http.setHeader(HTTP_HEADER_CONTENT_TYPE, HTTP_200);
    http.setHeader(HTTP_HEADER_CONTENT_LENGTH, MIME_TEXT_HTML);
    http.setHeader(HTTP_BODY, fileContent);
    return http.buildMessage();
}

string Server::routeVideo() {
    string fileContent = openFile(FILE_PATH_LIST_VIDEO);
    size_t posBegin = fileContent.find(TAG_VIDEO_LIST);
    fileContent.erase(posBegin, TAG_VIDEO_LIST.length());

    set<string> fileNames = listFile(FOLDER_PATH_VIDEOS);

    for (auto name : fileNames) {
        string tmp = "<tr><td><a href=\"/video/" + urlEncode(name) + "\">" +
                     name + "</a></td></tr>\n";
        fileContent.insert(posBegin, tmp);
    }

    HttpResponse http;
    http.setHeader(HTTP_HEADER_STATUS, HTTP_200);
    http.setHeader(HTTP_HEADER_CONTENT_TYPE, MIME_TEXT_HTML);
    http.setHeader(HTTP_BODY, fileContent);

    return http.buildMessage();
}

string Server::routeVideoPlayer(string videoName) {
    if (!isFileExist(FOLDER_PATH_VIDEOS + videoName)) {
        HttpResponse http;
        http.setHeader404();
        return http.buildMessage();
    }

    // open the player.rhtml
    string fileContent = openFile(FILE_PATH_PLAYER);

    // replace "<?VIDEO_NAME?>" with the video name
    size_t posBegin = fileContent.find(TAG_VIDEO_NAME);
    fileContent.erase(posBegin, TAG_VIDEO_NAME.length());
    fileContent.insert(posBegin, videoName);

    // replace "<?MPD_PATH?>" with the MPD URL
    posBegin = fileContent.find(TAG_MPD_PATH);
    fileContent.erase(posBegin, TAG_MPD_PATH.length());
    fileContent.insert(posBegin, "\"/api/video/" + urlEncode(videoName) + "/" +
                                     NAME_MPD + "\"");

    HttpResponse http;
    http.setHeader(HTTP_HEADER_STATUS, HTTP_200);
    http.setHeader(HTTP_HEADER_CONTENT_TYPE, MIME_TEXT_HTML);
    http.setHeader(HTTP_BODY, fileContent);

    return http.buildMessage();
}

string Server::routeApiFile(HttpHeader header) {
    vector<string> contentType = split(header[HTTP_HEADER_CONTENT_TYPE], "; ");
    if (contentType.size() != 2) {
        return EMPTY;
    }

    // get the boundry
    string boundry = split(contentType[1], "=")[1];
    // delete the last part
    vector<string> firstPart =
        split(header[HTTP_BODY], "--" + boundry + "--" + CRLF);

    // split parts
    vector<string> parts = split(firstPart[0], "--" + boundry + CRLF);
    parts = split(parts[0], "--" + boundry + CRLF);

    for (auto part : parts) {
        // split the header and body
        size_t pos = part.find(CRLF + CRLF);
        vector<string> partSplit;
        partSplit.push_back(part.substr(0, pos));
        partSplit.push_back(part.substr(pos + 4));

        string body = partSplit[1];
        pos = body.rfind("\r\n");
        if (pos != std::string::npos) {
            body.replace(pos, 2, "");
        }

        // split the header
        map<string, string> headers;
        for (auto header : split(partSplit[0], CRLF)) {
            vector<string> k = split(header, ": ");
            headers[toLowerCase(k[0])] = k[1];
        }

        // split the content disposition
        vector<string> b = split(headers[HTTP_HEADER_CONTENT_DISPOSITION], "; ");
        map<string, string> contentDisposition;

        // find the filename
        string fileName = "";
        for (auto j : b) {
            vector<string> k = split(j, "=");
            if (k[0] == "name" && k[1] != "\"upfile\"") {
                break;
            }
            if (k[0] == "filename" && k[1].length() > 2) {
                fileName = k[1].substr(1, k[1].length() - 2);
#ifdef DEBUG
                cout << "-----filename: " << fileName << endl;
#endif
                writeFile(FOLDER_PATH_FILES + fileName, body);
            }
        }
    }

    HttpResponse http;
    http.setHeader(HTTP_HEADER_STATUS, HTTP_200);
    http.setHeader(HTTP_HEADER_CONTENT_TYPE, MIME_TEXT_PLAIN);
    http.setHeader(HTTP_BODY, "File Uploaded\n");

    return http.buildMessage();
}

string Server::routeApiFileGet(string fileName) {
    string filePath = FOLDER_PATH_FILES + fileName;
    if (!isFileExist(filePath)) {
#ifdef DEBUG
        cout << "file not found:" << filePath << endl;
#endif
        HttpResponse http;
        http.setHeader404();
        return http.buildMessage();
    }

    HttpResponse http;
    http.setHeader(HTTP_HEADER_STATUS, HTTP_200);
    http.setHeader(HTTP_HEADER_CONTENT_TYPE, decideMimeType(fileName));
    http.setHeader(HTTP_BODY, openFile(filePath));
    http.setHeader(HTTP_HEADER_CONTENT_DISPOSITION,
                   "attachment; filename=\"" + fileName + "\"");

    return http.buildMessage();
}

string Server::routeApiVideo(HttpHeader header) {
    vector<string> contentType = split(header[HTTP_HEADER_CONTENT_TYPE], "; ");
    if (contentType.size() != 2) {
        return EMPTY;
    }

    // get the boundry
    string boundry = split(contentType[1], "=")[1];
    // delete the last part
    vector<string> firstPart =
        split(header[HTTP_BODY], "--" + boundry + "--" + CRLF);

    // split parts
    vector<string> parts = split(firstPart[0], "--" + boundry + CRLF);
    parts = split(parts[0], "--" + boundry + CRLF);

    for (auto part : parts) {
        // split the header and body
        vector<string> partSplit = split(part, CRLF + CRLF);

        string body = partSplit[1];
        size_t pos = body.rfind("\r\n");
        if (pos != std::string::npos) {
            body.replace(pos, 2, "");
        }

        // string body = replaceAll(partSplit[1], CRLF, "");

        // split the header
        map<string, string> headers;
        for (auto header : split(partSplit[0], CRLF)) {
            vector<string> k = split(header, ": ");
            headers[toLowerCase(k[0])] = k[1];
        }

        // split the content disposition
        vector<string> b = split(headers[HTTP_HEADER_CONTENT_DISPOSITION], "; ");
        map<string, string> contentDisposition;

        // find the filename
        string videoName = "";
        for (auto j : b) {
            vector<string> k = split(j, "=");
            if (k[0] == "name" && k[1] != "\"upfile\"") {
                break;
            }
            if (k[0] == "filename" && k[1].length() > 2) {
                videoName = k[1].substr(1, k[1].length() - 2);
#ifdef DEBUG
                cout << "-----filename: " << videoName << endl;
#endif
                writeFile(FOLDER_PATH_TMP + videoName, body);

                string fileExtension = getFileExtension(videoName);
                videoName = videoName.replace(videoName.rfind(fileExtension),
                                              fileExtension.length(), "");

                makeFolder(FOLDER_PATH_VIDEOS + videoName);
                string dashCommand =
                    string("ffmpeg -re -i ") + FOLDER_PATH_TMP + videoName +
                    ".mp4 " + "-c:a aac -c:v libx264 " +
                    "-map 0 -b:v:1 6M -s:v:1 1920x1080 -profile:v:1 high " +
                    "-map 0 -b:v:0 144k -s:v:0 256x144 -profile:v:0 baseline " +
                    "-bf 1 -keyint_min 120 -g 120 -sc_threshold 0 -b_strategy "
                    "0 " +
                    "-ar:a:1 22050 -use_timeline 1 -use_template 1 " +
                    "-adaptation_sets \"id=0, streams=v id=1, streams=a\" -f "
                    "dash " +
                    FOLDER_PATH_VIDEOS + videoName + "/" + NAME_MPD + " &";
                system(dashCommand.c_str());
            }
        }
    }

    HttpResponse http;
    http.setHeader(HTTP_HEADER_STATUS, HTTP_200);
    http.setHeader(HTTP_HEADER_CONTENT_TYPE, MIME_TEXT_PLAIN);
    http.setHeader(HTTP_BODY, "Video Uploaded\n");

    return http.buildMessage();
}

string Server::routeApiVideoGet(string fileName) {
#ifdef DEBUG
    cout << "\nfile name:" << fileName << endl;
#endif
    string filePath = FOLDER_PATH_VIDEOS + fileName;
    if (!isFileExist(filePath)) {
#ifdef DEBUG
        cout << "file not found:" << filePath << endl;
#endif
        HttpResponse http;
        http.setHeader(HTTP_HEADER_STATUS, HTTP_404);
        http.setHeader(HTTP_HEADER_CONTENT_TYPE, MIME_TEXT_PLAIN);
        http.setHeader(HTTP_BODY, HTTP_404_BODY);

        return http.buildMessage();
    }

    HttpResponse http;
    http.setHeader(HTTP_HEADER_STATUS, HTTP_200);
    http.setHeader(HTTP_HEADER_CONTENT_TYPE, decideMimeType(fileName));
    http.setHeader(HTTP_BODY, openFile(filePath));
    http.setHeader(HTTP_HEADER_CONTENT_DISPOSITION,
                   "attachment; filename=\"" + fileName + "\"");

    return http.buildMessage();
}

// handle the argument input
int handle_argument(int argc, char *argv[]) {
    if (argc == 2) {
        return stoi(argv[1]);
    }

    cerr << ERR_INPUT << endl;
    exit(-1);
}

set<string> readSecretFile(string file_path_secret) {
    if (!isFileExist(file_path_secret)) {
        cerr << "Secret file not found" << endl;
        exit(-1);
    }

    ifstream file(file_path_secret);
    string line;
    set<string> secrets;
#ifdef DEBUG
    cout << "----Reading Serect START----" << endl;
#endif
    while (getline(file, line)) {
        if (!line.empty()) {
            size_t csize;
            char *encoded = base64_encode((const unsigned char *)(line.c_str()),
                                          line.length(), &csize);
            // secrets.insert("Basic " + string(encoded, csize));
            secrets.insert(string(encoded, csize));
#ifdef DEBUG
            cout << line << "|" << string(encoded, csize) << endl;
#endif
        }
    }
#ifdef DEBUG
    cout << "----Reading Serect END----\n" << endl;
#endif

    return secrets;
}

int main(int argc, char *argv[]) {
    int serverPort = handle_argument(argc, argv);
    set<string> secrets = readSecretFile(FILE_PATH_SECRET);

    makeFolder(FOLDER_PATH_FILES);
    makeFolder(FOLDER_PATH_VIDEOS);
    makeFolder(FOLDER_PATH_TMP);

    Server server(serverPort, secrets);
    while (true) {
        server.run();
    };
}
