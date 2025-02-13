#ifndef __ACYCLIC_HPP__
#define __ACYCLIC_HPP__


#include <vector>

class Acyclic
{
public:
    Acyclic(std::vector<std::vector<int>> graph) : graph_(graph) { }

    bool detectRing();

    std::vector<std::vector<int>> removeRingTopological();

    std::vector<std::vector<int>> removeCycleEdges();

private:
    std::vector<std::vector<int>> graph_;

    std::vector<int> topologicalSort();

};

#endif