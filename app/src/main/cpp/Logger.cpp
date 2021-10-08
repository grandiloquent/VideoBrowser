#include "Logger.h"
#include <fstream>

using namespace std;

void WriteFile(const string &fileName, const string &content) {
    string path = "/storage/emulated/0/Documents/";
    ofstream file(path + fileName);
    if (file.is_open()) {
        file << content;
        file.close();
    }
}