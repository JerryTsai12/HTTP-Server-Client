#include "tool.h"
#include "../const.h"
#include <cctype>

// split the string by the delimiter
// if incounter the empty string, then stop the split
// put rest of the string into the last element of the vector
vector<string> split(const string &str, const string &delim)
{
    vector<string> tokens;
    size_t prev = 0, curr = 0;
    do
    {
        curr = str.find(delim, prev);

        if (curr != prev)
        {
            tokens.push_back(str.substr(prev, curr - prev));
        }
        else
        {
            tokens.push_back(str.substr(curr + delim.length(), str.length()));
            break;
        }
        prev = curr + delim.length();
    } while (curr != string::npos);

    return tokens;
}

bool isFileExist(const string &name)
{
    return filesystem::exists(name);
}

string openFile(const string &filePath)
{
    ifstream file(filePath.c_str(), ios::binary);
    if (!file.is_open())
    {
        string err = "open '" + filePath + "'Failed";
        ERR_EXIT(err.c_str());
    }

    stringstream ss;
    ss << file.rdbuf();

    return ss.str();
}

set<string> listFile(const string &folderPath)
{
    set<string> files;
    for (const auto &entry : filesystem::directory_iterator(folderPath))
    {
        files.insert(entry.path().filename());
    }

    return files;
}

void makeFolder(string folderPath)
{
    if (!filesystem::exists(folderPath))
        filesystem::create_directory(folderPath);
    return;
}

string replaceAll(string input, string replace_word, string replace_by)
{

    size_t pos = input.find(replace_word);

    // Iterate through the string and replace all
    // occurrences
    while (pos != string::npos)
    {
        // Replace the substring with the specified string
        input.replace(pos, replace_word.size(), replace_by);

        // Find the next occurrence of the substring
        pos = input.find(replace_word,
                         pos + replace_by.size());
    }

    return input;
}

bool writeFile(const string &filePath, const string &content)
{
    ofstream file(filePath.c_str(), ios::binary);
    if (!file.is_open())
    {
        string err = "open '" + filePath + "'Failed";
        ERR_EXIT(err.c_str());
    }

    file << content;
    file.close();
    return true;
}

string getFileExtension(const string &filePath)
{
    filesystem::path path = filePath;
    return path.extension();
}

string urlEncode(string url)
{
    stringstream ss;
    for (auto c : url)
    {
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
        {
            ss << c;
        }
        else
        {
            ss << '%';
            ss << hex << uppercase << (int)c;
        }
    }
    return ss.str();
}

string urlDecode(string url)
{
    auto next = url.find("%");
    while (next != string::npos)
    {
        string sHex = url.substr(next + 1, 2);
        // cout <<  sHex << endl;
        url.replace(next, 3, 1, (char)stoi(sHex, 0, 16));
        next = url.find("%", next + 1);
    }
    // cout <<  url << endl;
    return (url);
}

string decideMimeType(const string &fileName) {
    string mimetype = MIME_TEXT_PLAIN;

    if (getFileExtension(fileName) == ".html" ||
        getFileExtension(fileName) == ".rhtml") {
        mimetype = MIME_TEXT_HTML;
    } else if (getFileExtension(fileName) == ".mp4" ||
               getFileExtension(fileName) == ".m4v") {
        mimetype = MIME_VIDEO_MP4;
    } else if (getFileExtension(fileName) == ".m4s") {
        mimetype = MIME_VIDEO_IOS;
    } else if (getFileExtension(fileName) == ".m4a") {
        mimetype = MIME_AUDIO_MP4;
    } else if (getFileExtension(fileName) == ".mpd") {
        mimetype = MIME_APP_DASH;
    }

    return mimetype;
}

string toLowerCase(string str){
    for (int i = 0; i < str.length(); i++) {
        str[i] = tolower(str[i]);
    }
    return str;

}