#ifndef __ACYCLIC_HPP__
#define __ACYCLIC_HPP__


#include <vector>
#include <stack>
#include <unordered_map>
#include <database.hpp>

class Acyclic
{
public:
    Acyclic(const std::vector<std::vector<int>> &graph,
            const std::vector<std::shared_ptr<Instance>> &ID2instance) 
            : hypergraph_(graph), ID2instance_(ID2instance) {
        for (int i = 0; i < graph.size(); ++i)
        {
            sourceNets_[graph[i][0]].push_back(i);
            for (int j = 1; j < graph[i].size(); ++j)
                adjacencyList_[graph[i][0]].insert(graph[i][j]);
        }
    }

    bool detectCycle();

    bool removeCycle();

    std::vector<std::vector<int>> getHypergraph() { return hypergraph_; }
    std::vector<int> getWeights() { return SCCsWeights_; }

private:
    std::vector<std::vector<int>> hypergraph_;
    const std::vector<std::shared_ptr<Instance>> &ID2instance_;
    std::unordered_map<int, std::vector<int>> sourceNets_;  // 存储srcID的netIDs
    std::unordered_map<int, std::unordered_set<int>> adjacencyList_;  // 邻接表
    std::vector<int> SCCsWeights_;
    std::vector<int> topoOrder_;

    std::vector<std::vector<int>> findStronglyConnectedComponents();
    void dfs(int node, int& currentIndex, std::stack<int>& stack,
            std::unordered_map<int, bool>& onStack, std::unordered_map<int, int>& lowLink,
            std::unordered_map<int, int>& index, std::vector<std::vector<int>>& sccs);
    
    void mergeSCC(std::vector<std::vector<int>> &SCCs);

    void DFSRemoveCycle(std::unordered_set<int> &visited, int nodeID, int &removeEdgeNum); 
    
    void calTopoOrder();
};

#endif