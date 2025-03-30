#ifndef CONST_H
#define CONST_H

#include <string>
#include <map>

using namespace std;

// typedef for the http header
typedef map<string, string> HttpHeader;

// Error messages
const string ERR_INPUT = "Usage: ./server <port>";
const string ERR_ACCEPT = "Accept failed";

// int constants
const int BUFFER_SIZE = 200000000;
const int QUEUE_SIZE = 500;


// string constants
const string CRLF = "\r\n";
const string SPACE = " ";
const string SLASH = "/";
const string EMPTY = "";


// HTTP
// HTTP methods

const string HTTP_METHOD_GET = "GET";
const string HTTP_METHOD_POST = "POST";

// HTTP version
const string HTTP_VERSION = "HTTP/1.1";

// HTTP indentifiers
const string HTTP_BODY = "body";
const string HTTP_SERVER_NAME = "CN2024Server/1.0";

//HTTP header start line
const string HTTP_HEADER_METHOD = "method";
const string HTTP_HEADER_PATH = "path";
const string HTTP_HEADER_VERSION = "version";

// HTTP header fields
const string HTTP_HEADER_SPLIT = ": ";
const string HTTP_HEADER_SERVER = "server";
const string HTTP_HEADER_CONNECTION = "connection";
const string HTTP_HEADER_ALLOW = "allow";
const string HTTP_HEADER_HOST = "host";
const string HTTP_HEADER_CONTENT_TYPE = "content-type";
const string HTTP_HEADER_CONTENT_LENGTH = "content-length";
const string HTTP_HEADER_CONTENT_DISPOSITION = "content-disposition";
const string HTTP_HEADER_AUTH = "authorization";
const string HTTP_HEADER_WWWAUTH = "www-authenticate";
const string HTTP_HEADER_USER_AGENT = "user-agent";
const string HTTP_HEADER_STATUS = "status";
const string HTTP_HEADER_STATUS_TEXT = "status-text";
const string HTTP_HEADER_BOUNDARY = "boundary";

const string HTTP_HEADER_MULTY_PART = "multipart/form-data";
const string HTTP_HEADER_KEEP_ALIVE = "keep-alive";

// HTTP status code
const string HTTP_200 = "200 OK";
const string HTTP_401 = "401 Unauthorized";
const string HTTP_404 = "404 Not Found";
const string HTTP_405 = "405 Method Not Allowed";
const string HTTP_500 = "500 Internal Server Error";
const string HTTP_401_BODY = "Unauthorized\n";
const string HTTP_404_BODY = "Not Found\n";


// PATH
const string PATH_INDEX = "";
const string PATH_UPLOAD = "upload";
const string PATH_FILE = "file";
const string PATH_VIDEO = "video";
const string PATH_API = "api";

// MIME types
const string MIME_TEXT_PLAIN = "text/plain";
const string MIME_TEXT_HTML = "text/html";
const string MIME_VIDEO_MP4 = "video/mp4";
const string MIME_VIDEO_IOS = "video/ios.segment";
const string MIME_AUDIO_MP4 = "audio/mp4";
const string MIME_APP_DASH = "application/dash+xml";


// pseudo-HTML tag
const string TAG_FILE_LIST = "<?FILE_LIST?>";
const string TAG_VIDEO_LIST = "<?VIDEO_LIST?>";
const string TAG_VIDEO_NAME = "<?VIDEO_NAME?>";
const string TAG_MPD_PATH = "<?MPD_PATH?>";

// file path
const string FILE_PATH_SECRET = "./secret";
const string FILE_PATH_INDEX = "./web/index.html";
const string FILE_PATH_UPLOAD_FILE = "./web/uploadf.html";
const string FILE_PATH_UPLOAD_VIDEO = "./web/uploadv.html";
const string FILE_PATH_LIST_FILE = "./web/listf.rhtml";
const string FILE_PATH_LIST_VIDEO = "./web/listv.rhtml";
const string FILE_PATH_PLAYER = "./web/player.rhtml";

// folder path
const string FOLDER_PATH_FILES = "./web/files/";
const string FOLDER_PATH_VIDEOS = "./web/videos/";
const string FOLDER_PATH_TMP = "./web/tmp/";


const string NAME_MPD = "dash.mpd";

#endif