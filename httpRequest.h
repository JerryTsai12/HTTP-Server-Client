#include <string>
#include <vector>

#include "const.h"
#include "./utils/tool.h"

using namespace std;

class HttpRequest
{
public:
    HttpRequest(string request)
    {
        this->request = request;
        this->header[HTTP_HEADER_VERSION] = HTTP_VERSION;
        this->header[HTTP_HEADER_USER_AGENT] = "CN2024Client/1.0";
        this->header[HTTP_HEADER_CONNECTION] = HTTP_HEADER_KEEP_ALIVE;
    };
    // vector<string> split(const string &str, const string &delim);
    HttpHeader parseRequest();
    void setHeader(string key, string value);
    string buildMessage();
    
    string message;
private:
    string request;
    HttpHeader header;
};