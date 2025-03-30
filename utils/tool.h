#include <string>
#include <vector>
#include <set>
#include <fstream>
#include <filesystem>
#include <sstream>
#include <stdlib.h>
#include <string.h>
#include <iostream>

using namespace std;

#define ERR_EXIT(a) \
    {               \
        perror(a);  \
        exit(1);    \
    }


vector<string> split(const string &str, const string &delim);
bool isFileExist(const string &name);
string openFile(const string &filePath);
bool writeFile(const string &filePath, const string &content);
set<string> listFile(const string &folderPath);
void makeFolder(string folderPath);
string replaceAll(string input, string replace_word, string replace_by);
string getFileExtension(const string &filePath);
string urlEncode(string url);
string urlDecode(string url);
string decideMimeType(const string &fileName);
string toLowerCase(string str);