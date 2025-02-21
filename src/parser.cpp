#include "parser.hpp"
#include <regex>


bool Parser::parserFlow()
{
    hasInstancePortInfo = parsePort();

    std::cout << "Parse Netlist ..." << std::endl;
    std::string segment;
    while ((segment = getNextSegment()) != "") {
        if (checkFirstWord(segment, "module") ||
            checkFirstWord(segment, "endmodule") ||
            checkFirstWord(segment, "defparam"))
        {
            // std::cout << "invalid syntax :\n" << segment << std::endl;
            continue;
        }
        else if (checkFirstWord(segment, "input") ||
                 checkFirstWord(segment, "output") ||
                 checkFirstWord(segment, "inout") ||
                 checkFirstWord(segment, "wire"))
        {
            // std::cout << segment << std::endl;
            parseNet(segment);
            // std::cout << std::endl;
        }
        else if (checkFirstWord(segment, "assign"))
        {
            parseAssign(segment);
        }
        else
        {
            parseInstance(segment);
        }
    }
    std::cout << "Parse Netlist SUCCESS !\n" << std::endl;

    // showInstanceType();
    handleNets();
    handleAssign();

    getHyperedges();
    writeHgr();
    writeInstanceInfo();
    writeNetInfo();
    return true;
}



// 该函数用于从文件中每次获取一段以分号分隔的内容
std::string Parser::getNextSegment()
{
    std::string segment;
    std::string line;
    char ch;
    // bool inComment = false;
    while (std::getline(netlistFile_, line)) {
        // 跳过空行
        bool inComment = false;
        bool isEmpty = true;
        for (char c : line) {
            if (!std::isspace(c)) {
                isEmpty = false;
                break;
            }
        }
        if (isEmpty) continue;
        for (size_t i = 0; i < line.length(); ++i) {
            ch = line[i];
            if (inComment) {
                if (ch == '\n') {
                    inComment = false; // 注释结束
                }
                continue;
            }
            if (ch == '/' && i + 1 < line.length() && line[i + 1] == '/') {
                inComment = true; // 开始注释
                // i++; // 跳过第二个 /
                break;;
            }
            segment += ch;
            if (ch == ';') {
                return segment;
            }
        }
        if (inComment)
            continue;
    }
    // 如果文件结束都没遇到分号，返回剩余内容
    if (!segment.empty()) {
        return segment;
    }
    return "";
}

// 分割字符串
std::vector<std::string> Parser::split(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;
    while (std::getline(ss, token, delimiter)) {
        token = trim(token);
        if (!token.empty() && token != ".") {
            tokens.push_back(token);
        }
    }
    return tokens;
}

/// @brief 检查第一个单词是否为targetWord
/// @param segment 
/// @param targetWord 
/// @return 
bool Parser::checkFirstWord(const std::string& segment, const std::string& targetWord)
{
    std::istringstream iss(segment);
    std::string firstWord;
    iss >> firstWord;
    return firstWord == targetWord;
}


/// @brief 输入符合verilog语法的net语句 返回可以直接索引nets_的net信息
/// @param str net语句
/// @return NetInfo
NetInfo Parser::getNetInfo(std::string &str)
{
    NetInfo ret;
    if (nets_.find(str) != nets_.end())
    {
        ret.name_ = str;
        ret.lowIndex_ = nets_[str][0]->lowIndex_;
        ret.highIndex_ = nets_[str][0]->highIndex_;
        return ret;
    }

    if (str.find("unconnected_wire") != str.npos)
    {
        ret.name_ = "unconnected_wire";
        return ret;
    }

    auto result = splitNameBitindx(str);
    NetName netname = result.first;
    if (nets_.find(netname) == nets_.end())
    {
        if (str.find("OBSERVABLED") == str.npos)
        {
            std::cout << "Parser::getNetInfo() Error ! " << str << " not found !" << std::endl;
            exit(1);
        }
        ret.name_ = str;
        ret.lowIndex_ = 0;
        ret.highIndex_ = 0;
        return ret; 
    }

    int low_index, high_index;
    size_t colonPos = result.second.find(':');
    if (colonPos == result.second.npos)
    {
        low_index = std::stoi(result.second);
        high_index = low_index;
    }
    else 
    {
        if (colonPos != std::string::npos) {
            high_index = std::stoi(trim(result.second.substr(0, colonPos)));
            low_index = std::stoi(trim(result.second.substr(colonPos + 1)));
        }
        if (high_index < low_index)
            std::swap(high_index, low_index);
    }

    if (nets_[netname].size() < high_index)
    {
        std::cout << "Parser::getNetInfo() Error ! " << str << " out of range index !" << std::endl;
        exit(1);
    }

    ret.name_ = netname;
    ret.lowIndex_ = low_index;
    ret.highIndex_ = high_index;
    return ret;
}


bool Parser::parsePort()
{
    std::cout << "Parse Port [ " << portFileName_ << " ] ..." << std::endl;
    std::ifstream file(portFileName_);
    if (!file.is_open()) {
        std::cerr << "Cannot open or find port info file " << portFileName_ << std::endl;
        return false;
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string input = buffer.str();
    file.close();

    std::regex modulePattern(R"(module\s+(\w+)\s*\{([^}]+)\})");
    auto moduleBegin = std::sregex_iterator(input.begin(), input.end(), modulePattern);
    auto moduleEnd = std::sregex_iterator();
    for (std::sregex_iterator i = moduleBegin; i != moduleEnd; ++i) {
        std::smatch moduleMatch = *i;
        TypeName name = moduleMatch[1].str();
        std::shared_ptr<InstanceType> instType = std::make_shared<InstanceType>(name);
        std::string content = moduleMatch[2].str();
        std::regex inputPattern(R"(input:\s*([^;]+);)");
        std::regex outputPattern(R"(output:\s*([^;]+);)");
        std::smatch inputMatch;
        if (std::regex_search(content, inputMatch, inputPattern)) {
            std::string inputList = inputMatch[1].str();
            auto inputs = split(inputList, ',');
            instType->inputs = std::unordered_set<TypeName>(inputs.begin(), inputs.end());
        }
        std::smatch outputMatch;
        if (std::regex_search(content, outputMatch, outputPattern)) {
            std::string outputList = outputMatch[1].str();
            auto outputs = split(outputList, ',');
            instType->outputs = std::unordered_set<TypeName>(outputs.begin(), outputs.end());
        }
        instancePortInfos_.insert({name, instType});
    }
    std::cout << "Parse Port [ " << portFileName_ << " ] SUCCESS !\n" << std::endl;
    return true;
}

void Parser::parseNet(std::string &segment)
{
    // std::cout << "Parse net ..." << std::endl;
    // 正则表达式模式
    std::regex pattern(R"((output|inout|input|wire)\s*(?:\[([^\]]+)\])?\s*([^;]+);)");
    std::smatch match;
    if (std::regex_search(segment, match, pattern)) {
        std::string type = trim(match[1].str());
        std::string widthStr = trim(match[2].str());
        std::string name = trim(match[3].str());

        // 处理位宽信息
        std::string widthLeft, widthRight;
        if (!widthStr.empty()) {
            size_t colonPos = widthStr.find(':');
            if (colonPos != std::string::npos) {
                widthLeft = trim(widthStr.substr(0, colonPos));
                widthRight = trim(widthStr.substr(colonPos + 1));
            }
        }

        if (nets_.find(name) != nets_.end())
        {
            std::cout << "Parser::parseNet() Warning : Net " << name << " has existed !" << std::endl;
            return;
        }
        
        // 构建Net
        if (widthStr.empty())  // 单位宽net
        {
            std::shared_ptr<Net> net = std::make_shared<Net>(name, 0, 0, 0, type);
            if (type == "input")
            {
                std::string instName = "__INPUT_INSTANCE_" + name + "__";
                std::shared_ptr<Instance> inputInst = std::make_shared<Instance>(instName, "input");
                inputInst->outputs_.insert(name);
                instances_.insert({instName, inputInst});
            }
            else if (type == "output" || type == "inout")
            {
                std::string instName = "__OUTPUT_INSTANCE_" + name + "__";
                std::shared_ptr<Instance> inputInst = std::make_shared<Instance>(instName, "output");
                inputInst->inputs_.insert(name);
                instances_.insert({instName, inputInst});
            }
            nets_.insert({name, {net}});
        }
        else  // 多位宽net
        {
            // std::cout << "Name = " << name << ", type = " << type << ", lw = " << widthLeft << ":rw = " << widthRight << std::endl;
            int high = std::stoi(widthLeft);
            int low = std::stoi(widthRight);
            if (low > high)
                std::swap(low, high);
            for (int index = low; index <= high; ++index)
            {
                std::shared_ptr<Net> net = std::make_shared<Net>(name, index, high, low, type);
                if (type == "input")
                {
                    std::string netSingleBitName = name+"["+std::to_string(index)+"]";
                    std::string instName = "__INPUT_INSTANCE_" + netSingleBitName + "__";
                    std::shared_ptr<Instance> inputInst = std::make_shared<Instance>(instName, "input");
                    inputInst->outputs_.insert(netSingleBitName);
                    instances_.insert({instName, inputInst});
                }
                else if (type == "output" || type == "inout")
                {
                    std::string netSingleBitName = name+"["+std::to_string(index)+"]";
                    std::string instName = "__OUTPUT_INSTANCE_" + netSingleBitName + "__";
                    std::shared_ptr<Instance> inputInst = std::make_shared<Instance>(instName, "input");
                    inputInst->inputs_.insert(netSingleBitName);
                    instances_.insert({instName, inputInst});
                }
                nets_[name].emplace_back(net);
            }
        }
    }
}


void Parser::parseAssign(std::string &segment)
{
    // std::cout << "Parse assign ..." << std::endl;
    // std::cout << segment << std::endl;

    auto splitAssignStatement = [](const std::string& statement) {
        // 查找等号的位置
        size_t equalPos = statement.find('=');
        if (equalPos == std::string::npos) {
            return std::make_pair(std::string(), std::string());
        }

        // 提取等号左侧的字符串
        std::string left = statement.substr(0, equalPos);
        // 去除左侧字符串中 "assign" 关键字
        size_t start = left.find("assign");
        if (start != std::string::npos) {
            left = left.substr(start + std::string("assign").length());
        }
        // 去除左侧字符串的前导空格
        while (!left.empty() && std::isspace(left.front())) {
            left.erase(0, 1);
        }
        // 去除左侧字符串的尾随空格
        while (!left.empty() && std::isspace(left.back())) {
            left.pop_back();
        }

        // 提取等号右侧的字符串
        std::string right = statement.substr(equalPos + 1);
        // 去除右侧字符串的前导空格
        while (!right.empty() && std::isspace(right.front())) {
            right.erase(0, 1);
        }
        // 删除取反符号
        if (right[0] == '~') {
            right.erase(0, 1);
        }
        // 去除右侧字符串的前导空格
        while (!right.empty() && std::isspace(right.front())) {
            right.erase(0, 1);
        }
        // 去除右侧字符串的尾随空格
        while (!right.empty() && std::isspace(right.back())) {
            right.pop_back();
        }

        // 去掉右侧字符串最后的分号
        if (!right.empty() && right.back() == ';') {
            right.pop_back();
        }
        // 去除右侧字符串的尾随空格
        while (!right.empty() && std::isspace(right.back())) {
            right.pop_back();
        }

        return std::make_pair(left, right);
    };

    // 正则表达式匹配assign，忽略右边信号前的取反符号
    // std::regex pattern(R"(assign\s+([^=]+)=\s*([^;]+);)");
    // std::smatch match;
    // if (std::regex_search(segment, match, pattern)) 
    {
        // FanOutNetName leftWire = trim(match[1].str());
        // FanInNetName rightWire = trim(match[2].str());
        auto pair_ = splitAssignStatement(segment);
        FanOutNetName leftWire = pair_.first;
        FanInNetName rightWire = pair_.second;
        if (rightWire.find("1'b0") != rightWire.npos || rightWire.find("1'b1") != rightWire.npos)
        {
            std::cout << "Parser::parseAssign() Warning : segment parse assign fail : \n\t" << segment << std::endl;
            return;
        }
        assignNets_.insert({leftWire, rightWire});
    //     std::vector<NetName> leftConnectedSignals;
    //     std::vector<NetName> rightConnectedSignals;
    //     if (!leftWire.empty() && leftWire.front() == '{' && leftWire.back() == '}') {
    //         // 去除花括号
    //         std::string innerSignals = leftWire.substr(1, leftWire.length() - 2);
    //         leftConnectedSignals = splitSignals(innerSignals);
    //     }
    //     if (!rightWire.empty() && rightWire.front() == '{' && rightWire.back() == '}') {
    //         // 去除花括号
    //         std::string innerSignals = rightWire.substr(1, rightWire.length() - 2);
    //         rightConnectedSignals = splitSignals(innerSignals);
    //     }
    //     if (leftConnectedSignals.empty() && rightConnectedSignals.empty())
    //         // 无花括号的情况
    //         assignNets_.insert({leftWire, rightWire});
    //     else if (leftConnectedSignals.size() == rightConnectedSignals.size())
    //     {
    //         // 两边都有花括号
    //         for (int i = 0; i < leftConnectedSignals.size(); ++i)
    //             assignNets_.insert({leftConnectedSignals[i], rightConnectedSignals[i]});
    //     }
    //     else if (!leftConnectedSignals.empty() && rightConnectedSignals.empty())
    //     {
    //         // 左边有花括号
    //         for (int i = 0; i < leftConnectedSignals.size(); ++i)
    //         {
    //             assignNets_.insert({leftConnectedSignals[i], rightWire + "[" + std::to_string(i) + "]"});
    //         }
    //     }
    //     else if (leftConnectedSignals.empty() && !rightConnectedSignals.empty())
    //     {
    //         // 右边有花括号
    //         for (int i = 0; i < rightConnectedSignals.size(); ++i)
    //         {
    //             assignNets_.insert({leftWire + "[" + std::to_string(i) + "]", rightConnectedSignals[i]});
    //         }
    //     }
    }
    // else
    // {
    //     std::cout << "Parser::parseAssign() Warning : segment parse assign fail : \n" << segment << std::endl;
    // }
}


void Parser::parseInstance(std::string &segment)
{
    // std::cout << "Parse instance ..." << std::endl;
    // 提取模块名和实例名
    std::regex moduleInstancePattern(R"((\w+)\s+([^(\s]+))");
    std::smatch moduleInstanceMatch;
    InstanceName name;
    TypeName type;
    if (std::regex_search(segment, moduleInstanceMatch, moduleInstancePattern)) {
        type = trim(moduleInstanceMatch[1].str());
        name = trim(moduleInstanceMatch[2].str());
    }
    if (instancePortInfos_.find(type) == instancePortInfos_.end())
    {
        if (hasInstancePortInfo)
        {
            std::cout << "Parser::parseInstance() Error ! Not find instance type " << type << " !" << std::endl;
            exit(1);
        }
    }
    totalInstanceTypes_.insert(type);
    // std::cout << "type = " << type << ", name = " << name << std::endl;
    std::shared_ptr<Instance> instance = std::make_shared<Instance>(name, type);
    // 提取端口和连接信号
    std::regex portPattern(R"(\.(\w+)\(([^)]+)\))");
    auto portBegin = std::sregex_iterator(segment.begin(), segment.end(), portPattern);
    auto portEnd = std::sregex_iterator();
    for (std::sregex_iterator i = portBegin; i != portEnd; ++i) {
        std::smatch portMatch = *i;
        std::string portName = trim(portMatch[1].str());
        std::string connectedSignal = trim(portMatch[2].str());

        // 判断端口是输入还是输出
        bool inputPort = true;
        if (instancePortInfos_.find(type) == instancePortInfos_.end())
        {
            // instancePortInfos_ 没有找到 type
            instancePortInfos_.insert({type, std::make_shared<InstanceType>(type)});
        }
        if (instancePortInfos_[type]->inputs.find(portName) != instancePortInfos_[type]->inputs.end())
            inputPort = true;
        else if (instancePortInfos_[type]->outputs.find(portName) != instancePortInfos_[type]->outputs.end())
            inputPort = false;
        else 
        {
            std::cout << "Parser::parseInstance() Warning : Instance type " << type << ": port " << portName << " not found !" << std::endl;
            if (std::next(i) == portEnd || portName.find("out") != portName.npos)
                inputPort = false;  // 如果这是最后一个端口 认为是输出
            if (inputPort)
                instancePortInfos_[type]->inputs.insert(portName);
            else
                instancePortInfos_[type]->outputs.insert(portName);
        }

        if (!connectedSignal.empty() && connectedSignal.front() == '{' && connectedSignal.back() == '}') {
            // 去除花括号
            std::string innerSignals = connectedSignal.substr(1, connectedSignal.length() - 2);
            std::vector<NetName> connectedSignals = splitSignals(innerSignals);
            // instance->connectedNets_.insert(instance->connectedNets_.end(), connectedSignals.begin(), connectedSignals.end());
            if (inputPort)
                instance->inputs_.insert(connectedSignals.begin(), connectedSignals.end());
            else
                instance->outputs_.insert(connectedSignals.begin(), connectedSignals.end());
        } else {
            // instance->connectedNets_.emplace_back(connectedSignal);
            if (inputPort)
                instance->inputs_.insert(connectedSignal);
            else
                instance->outputs_.insert(connectedSignal);
        }
    }
    instances_.insert({name, instance});
}

// 去除字符串前后空白符的函数
std::string Parser::trim(const std::string& str) {
    auto first = std::find_if_not(str.begin(), str.end(), [](int c) { return std::isspace(c); });
    auto last = std::find_if_not(str.rbegin(), str.rend(), [](int c) { return std::isspace(c); }).base();
    return (first < last) ? std::string(first, last) : "";
}

// 分割花括号内的信号
std::vector<NetName> Parser::splitSignals(const std::string& signalStr) {
    std::vector<std::string> signals;
    std::stringstream ss(signalStr);
    std::string token;
    while (std::getline(ss, token, ',')) {
        token = trim(token);
        if (!token.empty()) {
            signals.push_back(token);
        }
    }
    return signals;
}


// 分割xxxxx[index] -> xxxxx, index
std::pair<std::string, std::string> Parser::splitNameBitindx(const std::string& input)
{
    std::regex pattern(R"((.*)\s*\[([\d:]+)\])");
    std::smatch match;
    if (std::regex_search(input, match, pattern)) {
        return {trim(match[1].str()), trim(match[2].str())};
    }
    return {"", ""};
}


// 将instance中的input与output连接到对应net上
void Parser::handleNets()
{
    std::cout << "Handle nets ..." << std::endl;
    // 插入电源和地instance
    std::shared_ptr<Instance> vccInst = std::make_shared<Instance>("__VCC_INSTANCE__", "_VCC_");
    std::shared_ptr<Instance> gndInst = std::make_shared<Instance>("__GND_INSTANCE__", "_GND_");
    instances_.insert({"__VCC_INSTANCE__", vccInst});
    instances_.insert({"__GND_INSTANCE__", gndInst});
    
    if (nets_.find("vcc") != nets_.end())
        vccInst->outputs_.insert("vcc");
    if (nets_.find("gnd") != nets_.end())
        vccInst->outputs_.insert("gnd");

    if (nets_.find("\\vcc") == nets_.end())
    {
        nets_.insert({"\\vcc", {std::make_shared<Net>("\\vcc", 0, 0, 0, "input")}});
        vccInst->outputs_.insert("\\vcc");
    }
    if (nets_.find("\\gnd") == nets_.end())
    {
        nets_.insert({"\\gnd", {std::make_shared<Net>("\\gnd", 0, 0, 0, "input")}});
        gndInst->outputs_.insert("\\gnd");
    }
    for (auto &instance_iter : instances_)
    {
        auto instance = instance_iter.second;

        for (NetName netname : instance->inputs_)
        {
            NetInfo netinfo = getNetInfo(netname);
            if (netinfo.name_ == "unconnected_wire")
                continue;
            
            for (int index = netinfo.lowIndex_; index <= netinfo.highIndex_; ++index)
            {
                auto net = nets_[netinfo.name_][nets_[netinfo.name_][0]->lowIndex_+index];
                if (net->index_ != index)
                {
                    std::cout << "Parser::handleNets() Error ! Net " << netname << " index should be " << net->index_ << ", wrong index is " << index << std::endl;
                    exit(1);
                }
                net->destinations_.insert(instance->name_);
            }
        }

        for (NetName netname : instance->outputs_)
        {
            NetInfo netinfo = getNetInfo(netname);
            if (netinfo.name_ == "unconnected_wire")
                continue;
            if (netinfo.name_.find("OBSERVABLED") != std::string::npos)
            {
                // 对外的数据观察线 新建一个output instance
                if (nets_.find(netinfo.name_) == nets_.end())
                {
                    std::shared_ptr<Net> obNet = std::make_shared<Net>(netinfo.name_, 0, 0, 0, "output");
                    nets_.insert({netinfo.name_, {obNet}});
                    std::string instName = "__OUTPUT_INSTANCE_" + netinfo.name_ + "__";
                    std::shared_ptr<Instance> inst = std::make_shared<Instance>(instName, "OUTPUT");
                    inst->inputs_.insert(netinfo.name_);
                    instances_.insert({instName, inst});
                    obNet->destinations_.insert(instName);
                    obNet->source_ = instance->name_;
                    continue;
                }
            }

            for (int index = netinfo.lowIndex_; index <= netinfo.highIndex_; ++index)
            {
                auto net = nets_[netinfo.name_][nets_[netinfo.name_][0]->lowIndex_+index];
                if (net->index_ != index)
                {
                    std::cout << "Parser::handleNets() Error ! Net " << netname << " index should be " << net->index_ << ", wrong index is " << index << std::endl;
                    exit(1);
                }
                if (net->source_ != "")
                {
                    if (hasInstancePortInfo)
                    {
                        std::cout << "Parser::handleNets() Error ! Net " << netname << " has multi sources !\n\t";
                        std::cout << "source[1] = " << net->source_ << ", source[2] = " << instance->name_ << std::endl;
                        exit(1);
                    }
                    // 如果没有 port 文件  则将 source 点放入 destination 中
                    net->destinations_.insert(net->source_);
                    net->source_ = "";
                }
                net->source_ = instance->name_;
            }
        }
    }
}


// 处理assign的net 找到所有fanoutnet的最前fainnet
// 将fanout的destinations放入fanin中 并删除fanout的destinations
void Parser::handleAssign()
{
    std::cout << "Handle assign ..." << std::endl;
    std::cout << "Total assign num = " << assignNets_.size() << std::endl;
    for (auto &assign_iter : assignNets_)
    {
        FanOutNetName fanout = assign_iter.first;
        FanInNetName fanin = assign_iter.second;
        if (fanout == "\\pcx_spc_grant_px[2]~input")
            std::cout << "\n";
        if (!fanout.empty() && fanout.front() == '{' && fanout.back() == '}') {
            // 去除花括号
            fanout = trim(fanout.substr(1, fanout.length() - 2));
        }
        if (!fanin.empty() && fanin.front() == '{' && fanin.back() == '}') {
            // 去除花括号
            fanin = trim(fanin.substr(1, fanin.length() - 2));
        }
        std::vector<FanOutNetName> lhs_ = splitSignals(fanout);
        std::vector<FanInNetName> rhs_ = splitSignals(fanin);
        std::vector<std::weak_ptr<Net>> fanoutNets;
        std::vector<std::weak_ptr<Net>> faninNets;
        for (FanOutNetName netname : lhs_)
        {
            NetInfo fanoutInfo = getNetInfo(netname);
            for (int index = fanoutInfo.highIndex_; index >= fanoutInfo.lowIndex_; --index)
            {
                fanoutNets.push_back(nets_[fanoutInfo.name_][index-nets_[fanoutInfo.name_][0]->lowIndex_]);
            }
        }
        for (FanInNetName netname : rhs_)
        {
            NetInfo faninInfo = getNetInfo(netname);
            for (int index = faninInfo.highIndex_; index >= faninInfo.lowIndex_; --index)
            {
                faninNets.push_back(nets_[faninInfo.name_][index-nets_[faninInfo.name_][0]->lowIndex_]);
            }
        }

        if (fanoutNets.size() != faninNets.size())
        {
            // 位宽不相等
            std::cout << "Parser::handleAssign() Error ! Bit width not equal ! \n" 
                      << "\tFanout " << fanout << " = " << fanoutNets.size() 
                      << ", fan in " << fanin << " = " << faninNets.size() << std::endl;
            exit(1);
        }

        for (int offset = 0; offset < fanoutNets.size(); ++offset)
        {
            auto netFanin = faninNets[offset].lock();
            auto netFanout = fanoutNets[offset].lock();
            if (netFanin->fanInNet_.expired())  
                // faninnet没有前序fanin 将fanoutnet映射到faninnet上
                netFanout->fanInNet_ = netFanin;
            else  
                // faninnet也有前序fanin 将fanoutnet映射带前序fanin上
                netFanout->fanInNet_ = netFanin->fanInNet_;
        }
    }

    for (auto &nets_iter : nets_)
    {
        for (auto &net : nets_iter.second)
        {
            if (net->destinations_.size() == 0)
                continue;
            auto faninnet = net->fanInNet_.lock();
            if (!faninnet)  // fanin是nullptr 说明没有作为fanout被assign过
                continue;

            // 找到源头 fanin
            while (!faninnet->fanInNet_.expired()) {
                faninnet = faninnet->fanInNet_.lock();
            }
            if (net->name_ == "\\pcx_spc_grant_px[2]~input")
                std::cout << "\n";
            // // 否则 将destination赋值到fanin上
            faninnet->destinations_.insert(net->destinations_.begin(),
                                           net->destinations_.end());
            // 删除faninnet的destinations
            net->destinations_.clear();

            // // 否则 将source赋值到fanout上
            // if (net->name_ == "\\GPIO_1[10]~input")
            //     std::cout << "net" << std::endl;
            // net->source_ = faninnet->source_;
        }
    }
}


void Parser::getHyperedges()
{
    validInstanceNum = 0;
    validNetNum = 0;
    ID2instance_.reserve(instances_.size());
    // 为了保证节点ID连续 通过net索引instance 被索引到的instance才会被分配ID
    for (auto &nets_iter : nets_)
    {
        for (auto &net : nets_iter.second)
        {
            if (net->destinations_.size() == 0)
                continue;
            if (net->source_ == "")
            {
                if (net->type_ != "inout")
                {
                    if (hasInstancePortInfo)
                    {
                        std::cout << "Parser::writeHgr() Error ! Net " << net->name_ << ", index " << net->index_ << " has no source !" << std::endl;
                        exit(1);
                    }
                    goto GEN_HYPEREDGE;
                }
                std::string instName = "__INPUT_INSTANCE_";
                std::string netname = net->name_;
                if (net->highIndex_ == 0)
                    instName += net->name_ + "__";
                else 
                {
                    instName += net->name_ + "[" + std::to_string(net->index_) + "]__";
                    netname += "[" + std::to_string(net->index_) + "]";
                }
                std::shared_ptr<Instance> inputInst = std::make_shared<Instance>(instName, "input");
                inputInst->outputs_.insert(netname);
                net->source_ = instName;
                instances_.insert({instName, inputInst});
            }

GEN_HYPEREDGE:
            hyperedges_.push_back({});
            net->ID_ = validNetNum;
            ++validNetNum;
            // 为instance赋ID
            if (net->source_ != "")  // 当没有输入port信息文件 net没有source的情况是允许的
            {
                if (instances_[net->source_]->ID_ == -1)
                {
                    instances_[net->source_]->ID_ = validInstanceNum;
                    ID2instance_.emplace_back(instances_[net->source_]);
                    ++validInstanceNum;
                }
                hyperedges_.back().emplace_back(instances_[net->source_]->ID_);
            }
            for (auto &dstInst : net->destinations_)
            {
                if (instances_[dstInst]->ID_ == -1)
                {
                    instances_[dstInst]->ID_ = validInstanceNum;
                    ID2instance_.emplace_back(instances_[dstInst]);
                    ++validInstanceNum;
                }
                hyperedges_.back().emplace_back(instances_[dstInst]->ID_);
            }
            
        }
    }

    if (!genDAG)
        return;

    // 环检测并剔除环
    Acyclic acyclic(hyperedges_, ID2instance_);
    if (acyclic.detectCycle())
    {
        acyclicFlag = false;
        std::cout << "Hypergraph is NOT Acyclic ! Do Remove Cycle !" << std::endl;
        if (acyclic.removeCycle())
        {
            hyperedges_ = acyclic.getHypergraph();
            weights_ = acyclic.getWeights();
            validInstanceNum = weights_.size();
            acyclicFlag = true;
            std::cout << "Remove cycle SUCCESS !\n" << std::endl;
        }
        else
        {
            std::cout << "Remove cycle FAIL !\n" << std::endl;
            acyclicFlag = false;
        }
    }
    else
    {
        acyclicFlag = true;
        std::cout << "Hypergraph is Acyclic !\n" << std::endl;
    }
}


void Parser::writeHgr()
{
    std::ofstream file(hgrFile_);
    if (!file.is_open())
    {
        std::cout << "Parser::writeHgr() Error ! Hgr file " << hgrFile_ << " open failed !" << std::endl;
        exit(1);
    }

    std::cout << "Net num = " << validNetNum << ", Instance num = " << validInstanceNum << std::endl;
    if (weights_.empty())
        file << validNetNum << " " << validInstanceNum << std::endl;
    else
        file << validNetNum << " " << validInstanceNum << " 10" << std::endl;
    for (auto &hyperedge : hyperedges_)
    {
        for (int instID : hyperedge)
        {
            // hgr 编号从1开始
            file << instID + 1 << " ";
        }
        file << std::endl;
    }

    if (!weights_.empty())
    {
        for (int &wt : weights_)
        {
            file << wt << std::endl;
        }
    }

    file.close();
    std::cout << "Write hgr file [ " << hgrFile_ << " ] SUCCESS !" << std::endl;
}


void Parser::writeInstanceInfo()
{
    std::string instInfoFile = hgrFile_ + ".instinfo";
    std::ofstream file(instInfoFile);
    if (!file.is_open())
    {
        std::cout << "Parser::writeHgr() Error ! Instance Info file " << instInfoFile << " open failed !" << std::endl;
        exit(1);
    }

    for (auto &inst_iter : instances_)
    {
        auto &inst = inst_iter.second;
        if (inst->ID_ == -1)
            continue;
        file << inst->ID_ << ", " << inst->name_ << ", " << inst->type_ << std::endl;
    }

    file.close();
    std::cout << "Write Instance Info file [ " << instInfoFile << " ] SUCCESS !" << std::endl;
}


void Parser::writeNetInfo()
{
    std::string netInfoFile = hgrFile_ + ".netinfo";
    std::ofstream file(netInfoFile);
    if (!file.is_open())
    {
        std::cout << "Parser::writeHgr() Error ! Net Info file " << netInfoFile << " open failed !" << std::endl;
        exit(1);
    }

    for (auto &net_iter : nets_)
    {
        // auto &inst = inst_iter.second;
        for (auto &net : net_iter.second)
        {
            if (net->ID_ == -1)
                continue;
            file << net->ID_ << ", " << net->name_ << ", " << net->type_ << std::endl;
        }
    }

    file.close();
    std::cout << "Write Net Info file [ " << netInfoFile << " ] SUCCESS !" << std::endl;
}