#include "acyclic.hpp"
#include <queue>
#include <iostream>
#include <unordered_map>
#include <stack>
#include <unordered_set>
#include <functional>


/// @brief 检测超图是否有环
/// @return true->存在环
bool Acyclic::detectRing()
{
    // 找到图中最大的节点编号
    int maxNode = 0;
    for (const auto& edge : graph_) {
        for (int node : edge) {
            maxNode = std::max(maxNode, node);
        }
    }

    // 初始化入度数组
    std::vector<int> inDegree(maxNode + 1, 0);
    // 计算每个节点的入度
    for (const auto& edge : graph_) {
        for (size_t i = 1; i < edge.size(); ++i) {
            inDegree[edge[i]]++;
        }
    }

    // 初始化队列，将入度为 0 的节点入队
    std::queue<int> q;
    for (int i = 0; i <= maxNode; ++i) {
        if (inDegree[i] == 0) {
            q.push(i);
        }
    }

    // 记录访问过的节点数量
    int visited = 0;
    // BFS 过程
    while (!q.empty()) {
        int node = q.front();
        q.pop();
        visited++;

        // 遍历所有边，找到以当前节点为源节点的边
        for (const auto& edge : graph_) {
            if (edge[0] == node) {
                for (size_t i = 1; i < edge.size(); ++i) {
                    // 将邻接节点的入度减 1
                    inDegree[edge[i]]--;
                    // 若邻接节点的入度变为 0，则将其入队
                    if (inDegree[edge[i]] == 0) {
                        q.push(edge[i]);
                    }
                }
            }
        }
    }

    // 若所有节点都被访问过，则不存在环；反之，则存在环
    return visited != maxNode + 1;
}


// 拓扑排序函数
std::vector<int> Acyclic::topologicalSort() {
    int maxNode = 0;
    for (const auto& edge : graph_) {
        for (int node : edge) {
            maxNode = std::max(maxNode, node);
        }
    }

    std::vector<int> inDegree(maxNode + 1, 0);
    for (const auto& edge : graph_) {
        for (size_t i = 1; i < edge.size(); ++i) {
            inDegree[edge[i]]++;
        }
    }

    std::queue<int> q;
    for (int i = 0; i <= maxNode; ++i) {
        if (inDegree[i] == 0) {
            q.push(i);
        }
    }

    std::vector<int> topOrder;
    while (!q.empty()) {
        int node = q.front();
        q.pop();
        topOrder.push_back(node);

        for (const auto& edge : graph_) {
            if (edge[0] == node) {
                for (size_t i = 1; i < edge.size(); ++i) {
                    inDegree[edge[i]]--;
                    if (inDegree[edge[i]] == 0) {
                        q.push(edge[i]);
                    }
                }
            }
        }
    }

    return topOrder;
}


// 移除环的函数
std::vector<std::vector<int>> Acyclic::removeRingTopological() {
    std::vector<int> topOrder = topologicalSort();
    std::unordered_map<int, int> orderMap;
    for (size_t i = 0; i < topOrder.size(); ++i) {
        orderMap[topOrder[i]] = i;
    }

    std::vector<std::vector<int>> acyclicHypergraph;
    for (const auto& edge : graph_) {
        bool valid = true;
        for (size_t i = 1; i < edge.size(); ++i) {
            if (orderMap[edge[0]] > orderMap[edge[i]]) {
                valid = false;
                break;
            }
        }
        if (valid) {
            acyclicHypergraph.push_back(edge);
        }
    }

    return acyclicHypergraph;
}


// 检测环并移除环中的边
std::vector<std::vector<int>> Acyclic::removeCycleEdges() {
    int n = 0;
    for (const auto& edge : graph_) {
        for (int node : edge) {
            n = std::max(n, node + 1);
        }
    }
    std::vector<std::vector<int>> adj(n);
    std::vector<int> inDegree(n, 0);
    for (const auto& edge : graph_) {
        int u = edge[0];
        for (size_t i = 1; i < edge.size(); ++i) {
            int v = edge[i];
            adj[u].push_back(v);
            inDegree[v]++;
        }
    }

    std::vector<bool> visited(n, false);
    std::vector<bool> onStack(n, false);
    std::stack<int> cycle;
    bool foundCycle = false;

    std::function<void(int)> dfs = [&](int u) {
        visited[u] = true;
        onStack[u] = true;
        cycle.push(u);
        for (int v : adj[u]) {
            if (!visited[v]) {
                dfs(v);
            } else if (onStack[v]) {
                foundCycle = true;
                while (cycle.top() != v) {
                    cycle.pop();
                }
                return;
            }
            if (foundCycle) {
                return;
            }
        }
        onStack[u] = false;
        cycle.pop();
    };

    for (int i = 0; i < n; ++i) {
        if (!visited[i]) {
            dfs(i);
            if (foundCycle) {
                break;
            }
        }
    }

    if (!foundCycle) {
        return graph_;
    }

    std::unordered_set<int> cycleNodes;
    while (!cycle.empty()) {
        cycleNodes.insert(cycle.top());
        cycle.pop();
    }

    std::vector<std::vector<int>> newGraph;
    for (const auto& edge : graph_) {
        int u = edge[0];
        bool valid = true;
        for (size_t i = 1; i < edge.size(); ++i) {
            int v = edge[i];
            if (cycleNodes.count(u) && cycleNodes.count(v)) {
                valid = false;
                break;
            }
        }
        if (valid) {
            newGraph.push_back(edge);
        }
    }

    return newGraph;
}

