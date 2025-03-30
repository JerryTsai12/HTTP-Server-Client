// class to handle the http request
#include "httpRequest.h"


void HttpRequest::setHeader(string key, string value)
{
    this->header[key] = value;
    return;
}

string HttpRequest::buildMessage(){
    stringstream httpMessage;

    // http start line
    httpMessage << this->header[HTTP_HEADER_METHOD] << SPACE;
    httpMessage << this->header[HTTP_HEADER_PATH] << SPACE;
    httpMessage << this->header[HTTP_HEADER_VERSION] << CRLF;

    // http header
    for (auto const& [key, val] : this->header)
    {
        if (key == HTTP_HEADER_METHOD || key == HTTP_HEADER_PATH || key == HTTP_HEADER_VERSION || key == HTTP_BODY)
        {
            continue;
        }
        httpMessage << key << ": " << val << CRLF;
    }

    // blank line
    httpMessage << CRLF;

    // http body
    if(header.count(HTTP_BODY) != 0){
        httpMessage << this->header[HTTP_BODY];
    }
    this->message = httpMessage.str();

    return httpMessage.str();
}

HttpHeader
HttpRequest::parseRequest()
{
    HttpHeader header;
    vector<string> tokens = split(this->request, CRLF);

    // handle the start line
    vector<string> startLine = split(tokens[0], SPACE);
    
    header[HTTP_HEADER_METHOD] = startLine[0];
    header[HTTP_HEADER_PATH] = startLine[1];
    header[HTTP_HEADER_VERSION] = startLine[2];

    // for http header
    // first element is the start line
    // last element is the body
    for (size_t i = 1; i < tokens.size() - 1; i++)
    {
        vector<string> line = split(tokens[i], ": ");
        if(toLowerCase(line[0]) == HTTP_HEADER_CONNECTION ){
            header[toLowerCase(line[0])] = toLowerCase(line[1]);
        }
        else{
            header[toLowerCase(line[0])] = line[1];
        }
    }

    header[HTTP_BODY] = tokens.back();

    return header;
}