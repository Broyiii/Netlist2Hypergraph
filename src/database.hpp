

#ifndef __DATABASE_HPP__
#define __DATABASE_HPP__

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>

using InstanceName = std::string;
using NetName = std::string;
using TypeName = std::string;
using FanInNetName = std::string;
using FanOutNetName = std::string;
using PortName = std::string;


struct InstanceType
{
    const TypeName name_;
    std::unordered_set<PortName> inputs;
    std::unordered_set<PortName> outputs;

    InstanceType(TypeName name) : name_(name) { }
};


struct Instance
{
    const InstanceName name_;
    const TypeName type_;
    std::unordered_set<NetName> inputs_;
    std::unordered_set<NetName> outputs_;
    
    int ID_ = -1;
    // std::vector<NetName> connectedNets_;

    Instance(InstanceName name, TypeName type) : name_(name), type_(type) { }
};

// enum NetTypr {INPUT=0, OUTPUT, INOUT, WIRE};
struct Net
{
    const NetName name_;
    const int index_;
    const int highIndex_;
    const int lowIndex_;
    const TypeName type_;
    std::unordered_set<InstanceName> destinations_;
    InstanceName source_ = "";

    std::weak_ptr<Net> fanInNet_;

    int ID_ = -1;

    Net(NetName name, int index, int highIndex, int lowIndex, TypeName type) :
        name_(name), index_(index), highIndex_(highIndex), lowIndex_(lowIndex), type_(type) { }
};


class NetUnionFind {
private:
    std::unordered_map<NetName, NetName> parent;
    std::unordered_map<NetName, int> rank;
public:
    // 检查元素是否存在
    bool check(const NetName& x) {
        if (rank.find(x) == rank.end())
            return false;
        return true;
    }

    // 查找根节点并进行路径压缩
    NetName find(const NetName& x) {
        if (parent[x] != x) {
            parent[x] = find(parent[x]);
        }
        return parent[x];
    }

    // 初始化并查集，将每个元素的父节点设为自身
    void makeSet(const NetName& x) {
        if (parent.find(x) == parent.end()) {
            parent[x] = x;
            rank[x] = 0;
        }
    }
    // 合并两个集合
    void unionSets(const NetName& x, const NetName& y) {
        NetName rootX = find(x);
        NetName rootY = find(y);
        if (rootX != rootY) {
            if (rank[rootX] < rank[rootY]) {
                parent[rootX] = rootY;
            } else if (rank[rootX] > rank[rootY]) {
                parent[rootY] = rootX;
            } else {
                parent[rootY] = rootX;
                rank[rootX]++;
            }
        }
    }
    // 判断两个元素是否属于同一个集合
    bool connected(const NetName& x, const NetName& y) {
        return find(x) == find(y);
    }
};

#endif