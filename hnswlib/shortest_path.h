#pragma once

#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <vector>
#include <stdexcept>
#include <limits>

namespace hnswlib
{
    template<typename node_type, typename distance_type>
    class ShortestPath
    {
    public:
        virtual ~ShortestPath() = default;
        virtual distance_type distance(const node_type& source, const node_type& target) = 0;
    };

    template<typename node_type>
    class BreadthFirstSearch: public ShortestPath<node_type, int>
    {
    private:
        std::unordered_map<node_type, std::unordered_set<node_type>> matrix;

    public:
        BreadthFirstSearch(const std::unordered_map<node_type, std::unordered_set<node_type>>& matrix)
            : matrix(matrix)
        {}

        int distance(const node_type& source, const node_type& target) override
        {
            if (source == target)
            {
                return 0;
            }

            else if (matrix.find(source) == matrix.end() || matrix.find(target) == matrix.end())
            {
                return -1;
            }

            std::queue<std::pair<node_type, int>> distances;
            std::unordered_set<node_type> visited;
            distances.emplace(source, 0);
            visited.insert(source);

            while (!distances.empty())
            {
                auto tag = distances.front();
                distances.pop();

                for (const node_type& related : matrix.at(tag.first))
                {
                    if (related == target)
                    {
                        return tag.second + 1;
                    }

                    else if (visited.find(related) == visited.end())
                    {
                        visited.insert(related);
                        distances.emplace(related, tag.second + 1);
                    }
                }
            }

            return -1;
        }
    };

    template<typename node_type, typename distance_type>
    class Dijkstra: public ShortestPath<node_type, distance_type>
    {
    private:
        std::unordered_map<node_type, std::unordered_set<node_type>> matrix;
        uint32_t** edgeWeights;
        std::vector<distance_type> distanses;
        std::unordered_map<node_type, std::vector<distance_type>> distances;

    public:
        Dijkstra(const std::unordered_map<node_type, std::unordered_set<node_type>>& matrix, uint32_t** edgeWeights)
            : matrix(matrix), edgeWeights(edgeWeights)
        {
            for (const auto& pair : matrix)
            {
                std::vector<distance_type> nodeDistances(matrix.size(), -1);
                distances.insert({pair.first, nodeDistances});
            }
        }

        // This function stores the intermediate results, which can be reused when calling this function again
        distance_type distance(const node_type& source, const node_type& target) override
        {
            if (source < 0 || source > matrix.size() || target < 0 || target > matrix.size())
            {
                throw std::runtime_error("Invalid node ID");
            }

            distance_type existingDistance = distances[source][target - 1];

            if (existingDistance != -1)
            {
                return existingDistance;
            }

            std::priority_queue<std::pair<double, node_type>, std::vector<std::pair<double, node_type>>, std::greater<std::pair<double, node_type>>> q;
            std::vector<distance_type> dist;
            std::vector<node_type> prev;
            dist.assign(matrix.size(), std::numeric_limits<distance_type>::max());
            prev.assign(matrix.size(), -1);
            dist[source - 1] = 0;
            q.push(std::make_pair(0.0, source));

            for (const auto& pair : matrix)
            {
                auto& v = pair.first;

                if (pair.first != source)
                {
                    prev[v - 1] = -1;
                    dist[v - 1] = std::numeric_limits<distance_type>::max();
                    q.push(std::make_pair(std::numeric_limits<distance_type>::max(), v));
                }
            }

            while (!q.empty())
            {
                auto& u = q.top();
                q.pop();

                for (const auto& v : matrix.at(u.second))
                {
                    distance_type weight = edgeWeights[u.second - 1][v - 1];
                    distance_type alt = dist[u.second - 1] + weight;

                    if (alt < dist[v - 1])
                    {
                        prev[v - 1] = u.second;
                        dist[v - 1] = alt;
                        q.push(std::make_pair(alt, v));
                    }
                }
            }

            distances[source] = dist;
            return dist[target - 1];
        }
    };
}