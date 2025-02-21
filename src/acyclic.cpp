#include "acyclic.hpp"
#include <queue>
#include <iostream>
#include <unordered_map>
#include <stack>
#include <unordered_set>
#include <functional>
#include <algorithm>
#include <fstream>


/// @brief 检测超图是否有环
/// @return true->存在环
bool Acyclic::detectCycle()
{
    // 找到图中最大的节点编号
    int nodesNum = ID2instance_.size();
    // int maxNode = -1;
    // for (const auto& edge : hypergraph_) {
    //     for (int node : edge) {
    //         maxNode = std::max(maxNode, node);
    //     }
    // }

    // 初始化入度数组
    std::vector<int> inDegree(nodesNum, 0);
    // 计算每个节点的入度
    for (const auto& edge : hypergraph_) {
        for (size_t i = 1; i < edge.size(); ++i) {
            int dstID = edge[i];
            ++inDegree[dstID];
        }
    }

    // 初始化队列，将入度为 0 的节点入队
    std::queue<int> q;
    for (int i = 0; i < nodesNum; ++i) {
        if (inDegree[i] == 0) {
            q.push(i);
        } else if (ID2instance_[i]->type_.find("dff") != std::string::npos) {
            // 对于DFF 拓扑序为0
            inDegree[i] = 0;
            q.push(i);
        }
    }

    // 记录访问过的节点数量
    int visited = 0;
    // BFS 过程
    while (!q.empty()) {
        int node = q.front();
        q.pop();
        ++visited;

        // 遍历所有当前节点为源节点的边
        for (int netID : sourceNets_[node])
        {
            const std::vector<int>& edge = hypergraph_[netID];
            for (size_t i = 1; i < edge.size(); ++i) {
                int dstID = edge[i];
                // 跳过DFF
                if (ID2instance_[dstID]->type_.find("dff") != std::string::npos)
                    continue;
                // 将邻接节点的入度减 1
                --inDegree[dstID];
                // 若邻接节点的入度变为 0，则将其入队
                if (inDegree[dstID] == 0) {
                    q.push(dstID);
                }
            }
        }
    }
    std::cout << "visited = " << visited << ", nodesNum = " << nodesNum << "\n" << std::endl;
    // 若所有节点都被访问过，则不存在环；反之，则存在环
    return visited != nodesNum;
}


void Acyclic::calTopoOrder()
{
    int maxNode = ID2instance_.size() - 1;

    // 初始化入度数组
    std::vector<int> inDegree(maxNode+1, 0);
    topoOrder_.resize(maxNode+1, -1);
    // 计算每个节点的入度
    for (const auto& edge : hypergraph_) {
        for (size_t i = 1; i < edge.size(); ++i) {
            ++inDegree[edge[i]];
        }
    }

    // 初始化队列，将入度为 0 的节点入队
    std::queue<int> q;
    for (int i = 0; i <= maxNode; ++i) {
        if (inDegree[i] == 0) {
            q.push(i);
            topoOrder_[i] = 0;
        } else if (ID2instance_[i]->type_.find("dff") != std::string::npos) {
            // 对于DFF 拓扑序为0
            inDegree[i] = 0;
            q.push(i);
            topoOrder_[i] = 0;
        }
    }

    // BFS 过程
    while (!q.empty()) {
        int node = q.front();
        q.pop();

        // 遍历所有当前节点为源节点的边
        for (int netID : sourceNets_[node])
        {
            const std::vector<int>& edge = hypergraph_[netID];
            for (size_t i = 1; i < edge.size(); ++i) {
                int dstID = edge[i];
                // 跳过DFF
                if (ID2instance_[dstID]->type_.find("dff") != std::string::npos)
                    continue;
                if (topoOrder_[dstID] < topoOrder_[edge[0]] + 1)
                    topoOrder_[dstID] = topoOrder_[edge[0]] + 1;
                // 将邻接节点的入度减 1
                inDegree[dstID]--;
                // 若邻接节点的入度变为 0，则将其入队
                if (inDegree[dstID] == 0) {
                    q.push(dstID);
                }
            }
        }
    }

    for (int i = 0; i <= maxNode; ++i) {
        if (topoOrder_[i] == -1) {
            std::cout << "ERROR ! Node " << i << "." << ID2instance_[i]->type_ << "." << ID2instance_[i]->name_ << " has no tb !" << "indegree = " << inDegree[i] << std::endl;
            std::cout << "\t";
            for (int netID : sourceNets_[i]) {
                const std::vector<int>& edge = hypergraph_[netID];
                for (size_t j = 1; j < edge.size(); ++j) {
                    std::cout << edge[j] << "." << ID2instance_[i]->type_  << "." << ID2instance_[edge[j]]->name_ << ", ";
                }
                std::cout << std::endl;
            }
            std::cout << std::endl;
        }
    }
    std::cout << "get topo order !!!" << std::endl;
}


// Tarjan算法：查找所有强连通分量
std::vector<std::vector<int>> Acyclic::findStronglyConnectedComponents() {
    std::unordered_map<int, bool> onStack;
    std::unordered_map<int, int> lowLink;
    std::unordered_map<int, int> index;
    std::stack<int> stack;
    std::vector<std::vector<int>> sccs;
    int currentIndex = 0;

    // 对图中的每个节点进行DFS遍历
    // for (const auto& pair : sourceNets_) {
    //     int node = pair.first;
    //     if (index.find(node) == index.end()) {
    //         dfs(node, currentIndex, stack, onStack, lowLink, index, sccs);
    //     }
    // }
    for (int node = 0; node < ID2instance_.size(); ++node) {
        if (index.find(node) == index.end()) {
            dfs(node, currentIndex, stack, onStack, lowLink, index, sccs);
        }
    }
    std::cout << "!!!sccs.size = " << sccs.size() << std::endl;
    return sccs;
}


// Tarjan算法的DFS实现
void Acyclic::dfs(int node, int& currentIndex, std::stack<int>& stack,
            std::unordered_map<int, bool>& onStack, std::unordered_map<int, int>& lowLink,
            std::unordered_map<int, int>& index, std::vector<std::vector<int>>& sccs) {
    // 初始化当前节点的索引和low-link值
    index[node] = currentIndex;
    lowLink[node] = currentIndex;
    currentIndex++;
    stack.push(node);
    onStack[node] = true;

    // 遍历当前节点的所有邻接节点
    for (int neighbor : adjacencyList_[node])
    {
        // for (int i = 1; i < hypergraph_[edgeID].size(); ++i)
        {
            // int neighbor = hypergraph_[edgeID][i];
            if (ID2instance_[neighbor]->type_.find("dff") != std::string::npos)
                continue;
            if (index.find(neighbor) == index.end()) {
                // 如果邻接节点未被访问，则递归访问
                dfs(neighbor, currentIndex, stack, onStack, lowLink, index, sccs);
                lowLink[node] = std::min(lowLink[node], lowLink[neighbor]);
            } else if (onStack[neighbor]) {
                // 如果邻接节点在栈中，更新low-link值
                lowLink[node] = std::min(lowLink[node], index[neighbor]);
            }
        }
    }

    // 如果当前节点是一个强连通分量的根节点，收集该分量
    // 在SCC中 最后一个节点是从Input出发最早访问到的节点
    if (lowLink[node] == index[node]) {
        std::vector<int> scc;
        while (true) {
            int w = stack.top();
            stack.pop();
            onStack[w] = false;
            scc.push_back(w);
            if (w == node) break;
        }
        sccs.push_back(scc);
    }
}


bool Acyclic::removeCycle()
{
    // calTopoOrder();
    auto sccs = findStronglyConnectedComponents();

    std::sort(sccs.begin(), sccs.end(), 
        [](const std::vector<int>& lhs_, const std::vector<int>& rhs_){ return lhs_.size() > rhs_.size(); });
    std::unordered_set<int> nodeIDinMaxSCC(sccs[0].begin(), sccs[0].end());

{
    // std::ofstream file("./graph");
    // for (int nodeID : sccs[0])
    // {
    //     for (int netID : sourceNets_[nodeID])
    //     {
    //         const auto& edge = hypergraph_[netID];
    //         int srcID = edge[0];
    //         for (int i = 1; i < edge.size(); ++i)
    //         {
    //             int dstID = edge[i];
    //             if (nodeIDinMaxSCC.count(dstID))
    //             {
    //                 file << srcID << "." << ID2instance_[srcID]->type_ << " " 
    //                      << dstID << "." << ID2instance_[dstID]->type_ << std::endl;
    //             }
    //         }
    //     }
    // }
    // file.close();
}

    // std::cout << "SCC:\n" << std::endl;
    int num = 0;
    int maxSize = 0;
    std::vector<int> nums_scc;
    for (auto &s : sccs)
    {
        if (s.size() == 1)
            continue;
        ++num;
        nums_scc.emplace_back(s.size());
        if (maxSize < s.size())
        {
            maxSize = s.size();
        }
    }
    std::cout << "> 1 num = " << num << ", maxsize = " << maxSize << "\n" << std::endl;
    for (int n : nums_scc)
    {
        std::cout << n << ", ";
    }
    std::cout << "\n" << std::endl;

    std::cout << "DFSRemoveCycle !" << std::endl;

    std::vector<int> inDegree(ID2instance_.size(), 0);
    // 计算每个节点的入度
    for (const auto& edge : hypergraph_) {
        for (size_t i = 1; i < edge.size(); ++i) {
            int dstID = edge[i];
            // // 对于DFF，不统计入度
            // if (ID2instance_[dstID]->type_.find("dff") != std::string::npos)
            //     continue;
            inDegree[dstID]++;
        }
    }

    // 初始化队列，将入度为 0 的节点入队
    std::queue<int> q;
    int removeEdgeNum = 0;
    for (int i = 0; i < ID2instance_.size(); ++i) {
        std::unordered_set<int> vis;
        if (inDegree[i] == 0) {
            DFSRemoveCycle(vis, i, removeEdgeNum);
        } else if (ID2instance_[i]->type_.find("dff") != std::string::npos) {
            DFSRemoveCycle(vis, i, removeEdgeNum);
        }
    }
    std::cout << "removeEdgeNum = " << removeEdgeNum << std::endl;
    


    calTopoOrder();
    sccs = findStronglyConnectedComponents();

    std::sort(sccs.begin(), sccs.end(), 
        [](const std::vector<int>& lhs_, const std::vector<int>& rhs_){ return lhs_.size() > rhs_.size(); });

    // std::cout << "SCC:\n" << std::endl;
    num = 0;
    maxSize = 0;
    nums_scc.clear();
    for (auto &s : sccs)
    {
        if (s.size() == 1)
            continue;
        ++num;
        nums_scc.emplace_back(s.size());
        if (maxSize < s.size())
        {
            maxSize = s.size();
        }
    }
    std::cout << "sccs.size = " << sccs.size() << ", > 1 num = " << num << ", maxsize = " << maxSize << "\n" << std::endl;
    for (int n : nums_scc)
    {
        std::cout << n << ", ";
    }
    std::cout << "\n" << std::endl;



    bool cycle = detectCycle();
    if (!cycle)
    {
        // std::cout << "Remove SUCCESS !\n" << std::endl;
        return true;
    }
    else 
    {
        // std::cout << "Remove FAILED !\n" << std::endl;
        return false;
    }

}


void Acyclic::mergeSCC(std::vector<std::vector<int>> &SCCs)
{
    // 新的超图将同一个SCC内的节点合并 新的节点ID就是SCC的ID
    std::unordered_map<int, int> node2SCCID;
    SCCsWeights_.resize(SCCs.size(), 0);  // 新节点的权重
    for (int sccID = 0; sccID < SCCs.size(); ++sccID)
    {
        SCCsWeights_[sccID] = SCCs[sccID].size();
        for (int nodeID : SCCs[sccID])
        {
            node2SCCID.insert({nodeID, sccID});
        }
    }

    // 合并节点
    for (auto &edge : hypergraph_)
    {
        for (auto &node : edge)
        {
            node = node2SCCID[node];
        }
        // 去重
        int sourceID = edge[0];
        std::sort(edge.begin(), edge.end());
        auto last = std::unique(edge.begin(), edge.end());
        edge.erase(last, edge.end());
        // 将source节点放到最前面
        auto it = std::lower_bound(edge.begin(), edge.end(), sourceID);
        std::swap(*edge.begin(), *it);
    } 
}


/// @brief 基于DFS算法的去环
void Acyclic::DFSRemoveCycle(std::unordered_set<int> &visited, int nodeID, int &removeEdgeNum)
{
    visited.insert(nodeID);
    for (int netID : sourceNets_[nodeID])
    {
        auto& edge = hypergraph_[netID];
        for (int i = 1; i < edge.size(); ++i)
        {
            int dstID = edge[i];

            // 对于DFF 跳过
            if (ID2instance_[dstID]->type_.find("dff") != std::string::npos)
            // if (ID2instance_[dstID]->type_ == "dffeas")
                continue;

            if (visited.count(dstID))  // 出现了环 将dstID从net中删除
            {
                std::swap(edge[i], edge[edge.size()-1]);
                edge.pop_back();
                --i;
                ++removeEdgeNum;
                continue;
            }
            
            DFSRemoveCycle(visited, dstID, removeEdgeNum);
        }
    }
    visited.erase(nodeID);
}