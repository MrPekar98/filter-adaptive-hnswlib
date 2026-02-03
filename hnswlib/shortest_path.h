#pragma once

#include <unordered_map>
#include <unordered_set>

namespace hnswlib
{
    template<typename node_type, typename distance_type>
    class ShortestPath
    {
    public:
        virtual ~ShortestPath() = default;
        virtual distance_type distance(const node_type& source, const node_type& target) = 0;
    };

    template<typename node_type, typename distance_type>
    class Dijkstra: public ShortestPath<node_type, distance_type>
    {
    private:
        std::unordered_map<node_type, std::unordered_set<node_type>> matrix;
        uint32_t** edgeWeights;

    public:
        Dijkstra(const std::unordered_map<node_type, std::unordered_set<node_type>>& matrix, uint32_t** edgeWeights)
            : matrix(matrix), edgeWeights(edgeWeights)
        {}

        distance_type distance(const node_type& source, const node_type& target) override
        {
            return 1.0;
        }
    };
}