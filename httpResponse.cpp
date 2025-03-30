#include "httpResponse.h"

#include <sstream>

#include "const.h"
#include "utils/tool.h"

HttpResponse::HttpResponse(){
    HttpHeader header;
    header[HTTP_HEADER_VERSION] = "HTTP/1.1";
    header[HTTP_HEADER_SERVER] = HTTP_SERVER_NAME;
    header[HTTP_BODY] = EMPTY;
    // header[HTTP_HEADER_CONTENT_TYPE] = MIME_TEXT_PLAIN;
    this->header = header;
};

void HttpResponse::setHeader(string key, string value) {
    this->header[key] = value;
    return;
}

void HttpResponse::setHeader404() {
    this->setHeader(HTTP_HEADER_STATUS, HTTP_404);
    this->setHeader(HTTP_HEADER_CONTENT_TYPE, MIME_TEXT_PLAIN);
    this->setHeader(HTTP_BODY, HTTP_404_BODY);
    return;
}

void HttpResponse::setHeader405(const string &allowMethod){
    this->setHeader(HTTP_HEADER_STATUS, HTTP_405);
    this->setHeader(HTTP_HEADER_CONTENT_TYPE, MIME_TEXT_PLAIN);
    this->setHeader(HTTP_HEADER_ALLOW, allowMethod);
    return;
}

void HttpResponse::setHeader500() {
    this->setHeader(HTTP_HEADER_STATUS, HTTP_500);
    this->setHeader(HTTP_HEADER_CONTENT_TYPE, MIME_TEXT_PLAIN);
    return;
}

HttpHeader HttpResponse::parse(const string& response) {
    HttpHeader header;
    vector<string> tokens = split(response, CRLF);

    // handle the start line
    vector<string> startLine = split(tokens[0], SPACE);
    header[HTTP_HEADER_VERSION] = startLine[0];
    header[HTTP_HEADER_STATUS] = startLine[1];
    header[HTTP_HEADER_STATUS_TEXT] = startLine[2];

    // for http header
    // first element is the start line
    // last element is the body
    for (size_t i = 1; i < tokens.size() - 1; i++) {
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

string HttpResponse::buildMessage() {
    stringstream httpMessage;

    // http start line
    httpMessage << this->header[HTTP_HEADER_VERSION] << SPACE
                << this->header[HTTP_HEADER_STATUS] << CRLF;

    this->header[HTTP_HEADER_CONTENT_LENGTH] =
        to_string(this->header[HTTP_BODY].length());
    
    // http header
    for (auto const& [key, val] : this->header) {
        if (key == HTTP_HEADER_VERSION || key == HTTP_HEADER_STATUS ||
            key == HTTP_BODY) {
            continue;
        }
        httpMessage << key << ": " << val << CRLF;
    }

    // blank line
    httpMessage << CRLF;

    // http body
    httpMessage << this->header[HTTP_BODY];

    return httpMessage.str();
}