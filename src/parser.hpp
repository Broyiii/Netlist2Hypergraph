#ifndef __PARSER_HPP__
#define __PARSER_HPP__

#include "database.hpp"
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <fstream>
#include <string>
#include <cctype>
#include <iostream>
#include <sstream>
#include "acyclic.hpp"


/// @brief 可以直接索引nets_的net信息
struct NetInfo
{
    NetName name_;  // 能够直接在nets_中索引到的name
    int lowIndex_;  // net的低位index
    int highIndex_; // net的高位index
};


class Parser
{
public:
    Parser(std::string netlist_dir, std::string port_dir, std::string hgr_dir) {
        netlistFile_ = std::ifstream(netlist_dir);
        if (!netlistFile_.is_open()) {
            std::cerr << "无法打开文件: " << netlist_dir << std::endl;
            return;
        }
        portFileName_ = port_dir;
        hgrFile_ = hgr_dir;
    }

    ~Parser() {
        netlistFile_.close();
    }

    bool parserFlow();

private:
    std::unordered_map<InstanceName, std::shared_ptr<Instance>> instances_;
    // For multi-bits net, store as [low, low+1, ... , high-1, high]
    std::unordered_map<NetName, std::vector<std::shared_ptr<Net>>> nets_;

    std::unordered_set<TypeName> totalInstanceTypes_;
    std::unordered_map<TypeName, std::shared_ptr<InstanceType>> instancePortInfos_;

    std::unordered_map<FanOutNetName, FanInNetName> assignNets_;  // assign fanout = fanin;

    std::ifstream netlistFile_;
    std::string portFileName_;
    std::string hgrFile_;

    std::vector<std::vector<int>> hyperedges_;
    int validInstanceNum = 0;
    int validNetNum = 0;

    bool hasInstancePortInfo = false;  // 是否输入port信息标志位 false时net不存在方向性 忽略source报错

    std::string trim(const std::string& str);
    std::vector<std::string> split(const std::string& str, char delimiter);
    std::vector<NetName> splitSignals(const std::string& signalStr);
    std::pair<std::string, std::string> splitNameBitindx(const std::string& input);

    std::string getNextSegment();

    bool checkFirstWord(const std::string& segment, const std::string& targetWord);

    bool parsePort();

    void parseNet(std::string &segment);

    void parseAssign(std::string &segment);

    void parseInstance(std::string &segment);

    void showInstanceType() {
        std::cout << "type num = " << totalInstanceTypes_.size() << std::endl;
        for (TypeName type : totalInstanceTypes_)
            std::cout << type << std::endl;
    }

    void handleNets();

    void handleAssign();

    void getHyperedges();

    void writeHgr();

    void writeInstanceInfo();

    void writeNetInfo();

    NetInfo getNetInfo(std::string &str);
};

#endif