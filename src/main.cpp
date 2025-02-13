#include <iostream>
#include <chrono>

#include "parser.hpp"


void Usage();
void printHead();

int main(int argc, char* argv[]) {
    auto start = std::chrono::system_clock::now();
    printHead();
    if ((argc == 1) || (argc % 2 == 0))
    {
        Usage();
        return false;
    }
    int argIndex = 1;
    std::string netlistFile = "";
    std::string portFile = "";
    std::string hgrFile = "";
    while (argIndex < (argc - 1))
    {
        std::string arg_str = argv[argIndex++];
        if (arg_str == "-v")
        {
            netlistFile = argv[argIndex++];
        }
        else if (arg_str == "-o")
        {
            hgrFile = argv[argIndex++];
        }
        else if (arg_str == "-p")
        {
            portFile = argv[argIndex++];
        }
        else
        {
            std::cout << "Error command : " << arg_str << "\n" << std::endl;
            Usage();
            return false;
        }
    }

    Parser parser(netlistFile, portFile, hgrFile);
    parser.parserFlow();

    auto end = std::chrono::system_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(end - start).count();
    std::cout << "Parse total takes: " << duration << " s." << std::endl;
    return 0;
}


void printHead()
{
    // 获取当前时间 精确至秒
    time_t today = time(0);
    char tmp[21];
    strftime(tmp, sizeof(tmp), "%Y-%m-%d %H:%M:%S", localtime(&today));

    printf("+========================================================================+\n");
    printf("|                                                                        |\n");
    printf("|                     Netlist To Hypergraph Converter                    |\n");
    printf("|                           Author : @Broyi                              |\n");
    printf("|                           Version: 2025-02-13 (V1.00)                  |\n");
    printf("|                           Run    : %s                 |\n", tmp);
    printf("+========================================================================+\n");
    std::cout << std::endl;
}


void Usage()
{
    std::cout << "Usage:\n\t./nl2hg -v <netlist_dir> -o <hgr_file_fir> [optional command]\n" << std::endl;
    std::cout << "Optional Command:" << std::endl;
    std::cout << "\t-p <port_info_dir>" << "\n" << std::endl;
}