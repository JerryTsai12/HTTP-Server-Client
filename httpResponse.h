#include <string>
#include "const.h"

using namespace std;

class HttpResponse
{
public:
    HttpResponse();
    void setHeader(string key, string value);
    void setHeader404();
    void setHeader405(const string &allowMethod);
    void setHeader500();
    string buildMessage();
    HttpHeader parse(const string &response);
    
private:
    HttpHeader header;
};