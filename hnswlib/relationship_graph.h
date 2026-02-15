#pragma once

#include <fstream>
#include <unordered_map>
#include <unordered_set>
#include <stdexcept>
#include <cstdlib>
#include <algorithm>
#include "shortest_path.h"

namespace hnswlib
{
    // This implementation assumes that IDs er continuous integers starting from 1
    template<typename tag_type>
    class RelationshipGraph
    {
    private:
        using distance_type = double;

        std::unordered_map<tag_type, std::string>& lookup;
        std::unordered_map<std::string, tag_type>& inverted;
        std::unordered_map<tag_type, std::unordered_set<tag_type>> adjMatrix;
        distance_type** distances = nullptr;
        uint32_t** frequencies = nullptr;
        size_t distancesCapacity, distancesCount = 0;

    public:
        RelationshipGraph(std::unordered_map<tag_type, std::string>& lookup,
            std::unordered_map<std::string, tag_type>& inverted)
            : lookup(lookup), inverted(inverted), distancesCapacity(10)
        {
            resizeDistances(10);
        }

        ~RelationshipGraph()
        {
            freeAll();
        }

        void freeAll()
        {
            for (int i = 0; i < distancesCapacity; i++)
            {
                free(distances[i]);
                free(frequencies[i]);
            }

            free(distances);
            free(frequencies);
        }

        void initDistances(distance_type* array, size_t n)
        {
            for (int i = 0; i < n; i++)
            {
                array[i] = 0.0;
            }
        }

        void initFrequencies(uint32_t* array, size_t n)
        {
            for (int i = 0; i < n; i++)
            {
                array[i] = 0;
            }
        }

        // Synchronizes the index of weighted tag distances by expanding capacity when needed and inserted new tags in the tag index
        void syncDistances(const std::unordered_set<tag_type>& newTags)
        {
            tag_type maxTagId = 0;

            for (const tag_type& tagId : newTags)
            {
                if (tagId > maxTagId)
                {
                    maxTagId = tagId;
                }
            }

            if (maxTagId > distancesCapacity)
            {
                resizeDistances(std::max(static_cast<unsigned>(maxTagId), static_cast<unsigned>(distancesCapacity + 10)));
            }

            Dijkstra<tag_type, distance_type> dijkstra(adjMatrix, frequencies);
            std::unordered_set<tag_type> allTags;

            for (const auto& pair : adjMatrix)
            {
                allTags.insert(pair.first);
            }

            for (const tag_type& newTag : newTags)
            {
                distance_type distance = dijkstra.distance(newTag, newTag); // Dijkstra does not need a target, since it computes distances to all other nodes

                for (const tag_type& oldTag : allTags)
                {
                    if (oldTag == newTag)
                    {
                        continue;
                    }

                    distances[newTag - 1][oldTag - 1] = distance;   // -1 because node IDs start at 1
                    distances[oldTag - 1][newTag - 1] = distance;
                }
            }
        }

        void resizeDistances(size_t newCapacity)
        {
            if (!distances)
            {
                distances = (distance_type**) malloc(sizeof(distance_type*) * newCapacity);
                frequencies = (uint32_t**) malloc(sizeof(uint32_t*) * newCapacity);

                if (!distances || !frequencies)
                {
                    throw std::runtime_error("Not enough memory");
                }

                for (int i = 0; i < newCapacity; i++)
                {
                    distances[i] = (distance_type*) malloc(sizeof(distance_type) * newCapacity);
                    frequencies[i] = (uint32_t*) malloc(sizeof(uint32_t) * newCapacity);

                    if (!distances[i] || !frequencies[i])
                    {
                        throw std::runtime_error("Not enough memory");
                    }

                    initDistances(distances[i], newCapacity);
                    initFrequencies(frequencies[i], newCapacity);
                }
            }

            else
            {
                if (newCapacity <= distancesCapacity)
                {
                    throw std::runtime_error("New distance capacity most be greater than the previous");
                }

                distances = (distance_type**) realloc(distances, sizeof(distance_type*) * newCapacity);
                frequencies = (uint32_t**) realloc(frequencies, sizeof(uint32_t*) * newCapacity);

                if (!distances || !frequencies)
                {
                    throw std::runtime_error("Not enough memory");
                }

                for (int i = 0; i < distancesCapacity; i++)
                {
                    distances[i] = (distance_type*) realloc(distances[i], sizeof(distance_type) * newCapacity);
                    frequencies[i] = (uint32_t*) realloc(frequencies[i], sizeof(uint32_t) * newCapacity);

                    if (!distances[i] || !frequencies[i])
                    {
                        throw std::runtime_error("Not enough memory");
                    }

                    initDistances(distances[i] + distancesCapacity, newCapacity - distancesCapacity);
                    initFrequencies(frequencies[i] + distancesCapacity, newCapacity - distancesCapacity);
                }

                for (int i = distancesCapacity; i < newCapacity; i++)
                {
                    distances[i] = (distance_type*) malloc(sizeof(distance_type) * newCapacity);
                    frequencies[i] = (uint32_t*) malloc(sizeof(uint32_t) * newCapacity);

                    if (!distances[i] || !frequencies[i])
                    {
                        throw std::runtime_error("Not enough memory");
                    }

                    initDistances(distances[i], newCapacity);
                    initFrequencies(frequencies[i], newCapacity);
                }
            }

            distancesCapacity = newCapacity;
        }

        RelationshipGraph& operator=(const RelationshipGraph& other)
        {
            lookup = other.lookup;
            inverted = other.inverted;
            adjMatrix = other.adjMatrix;

            return *this;
        }

        void relate(const std::unordered_set<tag_type>& tagIds) noexcept
        {
            std::unordered_set<tag_type> newTags;

            for (const tag_type& tagId : tagIds)
            {
                std::unordered_set<tag_type> related = tagIds;
                related.erase(tagId);

                if (adjMatrix.find(tagId) != adjMatrix.end())   // If a tag already exists
                {
                    adjMatrix[tagId].insert(related.begin(), related.end());
                }

                else
                {
                    adjMatrix.insert({tagId, related});
                    newTags.insert(tagId);
                }
            }

            if (!newTags.empty())
            {
                syncDistances(newTags);
            }

            for (const tag_type& tagId : tagIds)
            {
                std::unordered_set<tag_type> related = tagIds;
                related.erase(tagId);

                for (const tag_type& relatedTag : related)  // Increments frequencies for existing tag edges
                {
                    frequencies[tagId - 1][relatedTag - 1]++;
                    frequencies[relatedTag - 1][tagId - 1]++;
                }
            }
        }

        [[nodiscard]] int hops(const tag_type& fromTag, const tag_type& toTag) const noexcept
        {
            BreadthFirstSearch<tag_type> bfs(adjMatrix);
            return bfs.distance(fromTag, toTag);
        }

        [[nodiscard]] distance_type distance(const tag_type& fromTag, const tag_type& toTag) const
        {
            if (fromTag >= distancesCount || toTag >= distancesCount || fromTag < 0 || toTag < 0)
            {
                throw std::runtime_error("Invalid tag ID in distance function: fromTag (" + std::to_string(fromTag) +
                    ") and toTag (" + std::to_string(toTag) + ") must be greater than 0 and smaller than " + std::to_string(distancesCount));
            }

            return distances[fromTag][toTag];
        }

        void clear() noexcept
        {
            adjMatrix.clear();

            for (int i = 0; i < distancesCapacity; i++)
            {
                initDistances(distances[i], distancesCapacity);
                initFrequencies(frequencies[i], distancesCapacity);
            }
        }

        void saveIndex(std::ofstream& output) const
        {
            std::size_t size = adjMatrix.size();
            output.write(reinterpret_cast<const char*>(&size), sizeof(size));
            output.write(reinterpret_cast<const char*>(&distancesCapacity), sizeof(distancesCapacity));
            output.write(reinterpret_cast<const char*>(&distancesCount), sizeof(distancesCount));

            for (const auto& pair : adjMatrix)
            {
                std::size_t related = pair.second.size();
                output.write((char*) &related, sizeof(std::size_t));
                output.write((char*) &pair.first, sizeof(tag_type));

                for (const tag_type& relatedTag : pair.second)
                {
                    output.write((char*) &relatedTag, sizeof(tag_type));
                }
            }

            for (int i = 0; i < distancesCapacity; i++)
            {
                output.write(reinterpret_cast<const char*>(distances[i]), sizeof(distance_type) * distancesCapacity);
                output.write(reinterpret_cast<const char*>(frequencies[i]), sizeof(double) * distancesCapacity);
            }
        }

        void loadIndex(std::ifstream& input, const std::unordered_map<tag_type, std::string>& lookup,
            const std::unordered_map<std::string, tag_type>& inverted)
        {
            freeAll();
            distances = nullptr;
            resizeDistances(distancesCapacity);

            std::size_t size;
            input.read(reinterpret_cast<char*>(&size), sizeof(size));
            input.read(reinterpret_cast<char*>(&distancesCapacity), sizeof(distancesCapacity));
            input.read(reinterpret_cast<char*>(&distancesCount), sizeof(distancesCount));

            for (std::size_t i = 0; i < size; i++)
            {
                std::size_t related;
                tag_type tag;
                std::unordered_set<tag_type> relatedTags;
                input.read((char*) &related, sizeof(std::size_t));
                input.read((char*) &tag, sizeof(tag_type));

                for (std::size_t j = 0; j < related; j++)
                {
                    tag_type relatedTag;
                    input.read((char*) &relatedTag, sizeof(tag_type));
                    relatedTags.insert(relatedTag);
                }

                adjMatrix.insert({tag, relatedTags});
            }

            for (int i = 0; i < distancesCapacity; i++)
            {
                input.read(reinterpret_cast<char*>(distances[i]), sizeof(distance_type) * distancesCapacity);
                input.read(reinterpret_cast<char*>(frequencies[i]), sizeof(uint32_t) * distancesCapacity);
            }


            this->lookup = lookup;
            this->inverted = inverted;
        }
    };
}