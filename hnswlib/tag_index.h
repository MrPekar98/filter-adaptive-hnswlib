#pragma once

#include "relationship_graph.h"
#include <string>
#include <unordered_map>
#include <vector>
#include <set>
#include <stdexcept>
#include <cstdlib>
#include <cstring>
#include <fstream>

namespace hnswlib
{
    using tag_type = unsigned;

    template<typename T>
    class TagIndex
    {
    private:
        const size_t expand_size = 10000;
        const size_t internal_expand_size = 16;

        size_t capacity, levels = 1, count = 0;
        unsigned tagIdCounter = 1;
        size_t* capacities = nullptr;
        size_t* entries = nullptr;
        tag_type** tags = nullptr;

        std::vector<std::unordered_map<int, size_t>> levelTagFrequency;
        size_t *maxFrequenciesPerLevel;

        std::unordered_map<tag_type, std::string> lookup;
        std::unordered_map<std::string, tag_type> inverted;

        RelationshipGraph<tag_type> relationship_graph;

        // Expands number of internal ID entries
        void expand(const size_t& minExpansion)
        {
            size_t expansion = expand_size;

            while (expansion < minExpansion)
            {
                expansion += expand_size;
            }

            tag_type** tagsCopy = (tag_type**) realloc(tags, sizeof(tag_type*) * (capacity + expansion));
            size_t* capacitiesCopy = (size_t*) realloc(capacities, sizeof(size_t) * (capacity + expansion));
            size_t* entriesCopy = (size_t*) realloc(entries, sizeof(size_t) * (capacity + expansion));

            if (!tagsCopy || !capacitiesCopy || !entriesCopy)
            {
                throw std::runtime_error("Not enough memory");
            }

            for (int entry = capacity; entry < capacity + expansion; entry++)
            {
                tagsCopy[entry] = (tag_type*) malloc(internal_expand_size * sizeof(tag_type));

                if (!tagsCopy)
                {
                    throw std::runtime_error("Not enough memory");
                }

                size_t* cap = capacitiesCopy + entry;
                *cap = internal_expand_size;
            }

            tags = tagsCopy;
            capacities = capacitiesCopy;
            entries = entriesCopy;
            capacity += expansion;
            memset(entries + capacity, 0, expansion);
        }

        // Expands number of tag entries for a given internal ID
        void expand(const T& internalId, const size_t& minExpansion)
        {
            size_t expansion = expand_size;

            while (expansion < minExpansion)
            {
                expansion += internal_expand_size;
            }

            size_t* capacity = (size_t*) capacities + internalId;
            *capacity += expansion;
            tag_type* idTags = tags[internalId];
            tag_type* idTagsCopy = (tag_type*) realloc(idTags, sizeof(tag_type) * *capacity);

            if (!idTagsCopy)
            {
                throw std::runtime_error("Not enough memory");
            }

            tags[internalId] = idTagsCopy;
        }

        [[nodiscard]] static unsigned unionSize(const std::vector<tag_type>& s1, const std::vector<tag_type>& s2) noexcept
        {
            std::set<tag_type> unique;

            for (const tag_type& tag : s1)
            {
                unique.insert(tag);
            }

            for (const tag_type& tag : s2)
            {
                unique.insert(tag);
            }

            return unique.size();
        }

    public:
        explicit TagIndex(size_t capacity)
            : capacity(capacity)
        {
            tags = (tag_type**) malloc(sizeof(tag_type*) * capacity);
            capacities = (size_t*) malloc(sizeof(size_t) * capacity);
            entries = (size_t*) malloc(sizeof(size_t) * capacity);
            maxFrequenciesPerLevel = (size_t*) malloc(sizeof(size_t) * levels);

            if (!tags || !capacities || !entries || !maxFrequenciesPerLevel)
            {
                throw std::runtime_error("Not enough memory");
            }

            memset(entries, 0, capacity * sizeof(size_t));
            levelTagFrequency.emplace_back();

            for (size_t i = 0; i < capacity; i++)
            {
                size_t* entry = capacities + i;
                *entry = internal_expand_size;
                tags[i] = (tag_type*) malloc(internal_expand_size * sizeof(tag_type));

                if (!tags[i])
                {
                    throw std::runtime_error("Not enough memory");
                }
            }
        }

        void clear() noexcept
        {
            count = 0;
            capacity = 0;
            tagIdCounter = 1;
            free(capacities);
            free(entries);
            free(maxFrequenciesPerLevel);
            relationship_graph.clear();

            for (int i = 0; i < capacity; i++)
            {
                free(tags[i]);
            }

            free(tags);
        }

        void insert(const T& internalId, const std::vector<std::string>& internalTags, unsigned level)
        {
            if (level >= levels)
            {
                size_t* maxLevelFrequencyCopy = (size_t*) realloc(maxFrequenciesPerLevel, sizeof(size_t) * (level + 1));

                if (!maxLevelFrequencyCopy)
                {
                    throw std::runtime_error("Not enough memory");
                }

                maxFrequenciesPerLevel = maxLevelFrequencyCopy;

                for (int i = levels; i < (level + 1) - levels; i++)
                {
                    size_t* maxFrequency = maxFrequenciesPerLevel + i;
                    *maxFrequency = 0;
                    levelTagFrequency.emplace_back();
                }

                levels = level + 1;
            }

            else if (internalId >= capacity)
            {
                expand(internalId + 1);
            }

            size_t* internalCapacity = capacities + internalId;
            size_t* internalEntries = entries + internalId;
            std::vector<tag_type> tagIds;
            tagIds.reserve(internalTags.size());

            for (const std::string& tag : internalTags)
            {
                if (inverted.find(tag) == inverted.end())
                {
                    lookup.insert({tagIdCounter, tag});
                    inverted.insert({tag, tagIdCounter++});
                }

                if (*internalEntries >= *internalCapacity)
                {
                    expand(internalId, *internalEntries + 1);
                }

                tag_type tagId = inverted[tag];
                memcpy(tags[internalId] + *internalEntries, &tagId, sizeof(tag_type));
                (*internalEntries)++;
                tagIds.push_back(tagId);

                for (int levelCopy = static_cast<int>(level); levelCopy >= 0; levelCopy--)
                {
                    if (levelTagFrequency[levelCopy].find(tagId) == levelTagFrequency[levelCopy].end())
                    {
                        levelTagFrequency[levelCopy][tagId] = 0;
                    }

                    levelTagFrequency[levelCopy][tagId]++;
                    size_t* maxFrequency = maxFrequenciesPerLevel + levelCopy;
                    *maxFrequency = std::max(*maxFrequency, levelTagFrequency[levelCopy][tagId]);
                }
            }

            count++;
            relationship_graph.relate(tagIds);
        }

        [[nodiscard]] std::vector<std::string> get(const T& internalId) const
        {
            if (internalId < 0 || internalId >= count)
            {
                throw std::runtime_error("Invalid internal ID");
            }

            tag_type* internalTags = tags[internalId];
            size_t size = *((size_t*) entries + internalId);
            std::vector<std::string> tags;
            tags.reserve(size);

            for (int i = 0; i < size; i++)
            {
                tag_type tagId = *(internalTags + i);
                std::string tag = lookup.at(tagId);
                tags.push_back(tag);
            }

            return tags;
        }

        [[nodiscard]] size_t size() const noexcept
        {
            return count;
        }

        [[nodiscard]] bool exists(const std::string& tag) const noexcept
        {
            return inverted.find(tag) != inverted.end();
        }

        const RelationshipGraph<tag_type>& getRelationships() const noexcept
        {
            return relationship_graph;
        }

        [[nodiscard]] unsigned frequency(const std::string& tag, unsigned level) const
        {
            if (level >= levels)
            {
                throw std::invalid_argument("Level value too great");
            }

            else if (!exists(tag))
            {
                return 0;
            }

            tag_type tagId = inverted.at(tag);
            return levelTagFrequency.at(level).at(tagId);
        }

        [[nodiscard]] unsigned maxLevelFrequency(unsigned level)
        {
            if (level >= levels)
            {
                throw std::invalid_argument("Level value too great");
            }

            return *(maxFrequenciesPerLevel + level);
        }

        [[nodiscard]] double tagsSimilarity(const std::vector<std::string>& tags1, const std::vector<std::string>& tags2) const noexcept
        {
            if (tags1.empty() || tags2.empty())
            {
                return 0.0;
            }

            double sum = 0.0;
            std::vector<tag_type> tagIds1, tagIds2;
            tagIds1.reserve(tags1.size());
            tagIds2.reserve(tags2.size());

            for (const std::string& tag : tags1)
            {
                tag_type tagId = inverted.at(tag);
                tagIds1.push_back(tagId);
            }

            for (const std::string& tag : tags2)
            {
                tag_type tagId = inverted.at(tag);
                tagIds2.push_back(tagId);
            }

            for (const tag_type& tag1 : tagIds1)
            {
                for (const tag_type& tag2 : tagIds2)
                {
                    int hops = relationship_graph.hops(tag1, tag2);

                    if (hops != -1)
                    {
                        sum += 1.0 / (hops + 1);
                    }
                }
            }

            return sum / unionSize(tagIds1, tagIds2);
        }

        void save_index(std::ofstream& output) const
        {

            relationship_graph.save_index(output);
        }

        void load_index(std::ifstream& input)
        {

            relationship_graph.load_index(input);
        }
    };
}