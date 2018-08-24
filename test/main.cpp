
#include <fstream>
#include <sstream>
#include <locale>

#include "cpp_class_grammar.h"

int _tmain(int argc, char* argv[])
{
    std::vector<std::wstring> filelist;
    filelist.reserve(argc - 1);
    for (int i = 1; i < argc; ++i) {
        std::wstring file = argv[i];
        if (file.substr(file.length() - 2, 2) == L".h") {
            filelist.push_back(argv[i]);
        }
    }

    if (filelist.empty())
        return 0;

    std::ifstream fs;
    fs.open(filelist.front().c_str());
    if (!fs.is_open()) {
        return 0;
    }

    std::stringstream buffer;
    buffer << fs.rdbuf();
    std::string input = buffer.str();

    CppClassGrammar gramar;
    bool ret = gramar.doParse(input); // 解析后，生成一个class信息树


    return 0;
}