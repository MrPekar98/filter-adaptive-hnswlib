#pragma once

#include "relationship_graph.h"
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <set>
#include <stdexcept>
#include <cstdlib>
#include <cstring>
#include <limits>

namespace hnswlib
{
    using tag_type = unsigned;

    template<typename T>
    class TagIndex
    {
    private:
        static constexpr size_t expand_size = 10000;
        static constexpr size_t internal_expand_size = 16;

        size_t capacity, levels = 1, count = 0;
        unsigned tagIdCounter = 1;
        size_t* capacities = nullptr;
        size_t* entries = nullptr;
        tag_type** tags = nullptr;

        std::vector<std::unordered_map<tag_type, size_t>> levelTagFrequency;
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

            size_t* capacity = capacities + internalId;
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
            : capacity(capacity), relationship_graph(RelationshipGraph<tag_type>(lookup, inverted))
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

                for (int i = levels; i < (level + 1); i++)
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
            std::unordered_set<tag_type> tagIds;
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
                tagIds.insert(tagId);

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
            if (internalId < 0)
            {
                throw std::runtime_error("Invalid internal ID");
            }

            else if (internalId > count)
            {
                return {};
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
            if (level >= levels || !exists(tag))
            {
                return 0;
            }

            try
            {
                tag_type tagId = inverted.at(tag);
                return levelTagFrequency.at(level).at(tagId);
            }

            catch (const std::out_of_range& exc)
            {
                return 0;
            }
        }

        [[nodiscard]] unsigned maxLevelFrequency(unsigned level)
        {
            if (level >= levels)
            {
                throw std::invalid_argument("Level value too great");
            }

            return *(maxFrequenciesPerLevel + level);
        }

        [[nodiscard]] double jaccardSimilarity(const std::vector<std::string>& tags1, const std::vector<std::string>& tags2) const
        {
            std::unordered_set<std::string> tagsUnion(tags1.begin(), tags1.end()), tagsIntersection, tags1Set, tags2Set;
            tagsUnion.insert(tags2.begin(), tags2.end());
            tags1Set.insert(tags1.begin(), tags1.end());
            tags2Set.insert(tags2.begin(), tags2.end());

            for (const std::string& tag : tags1Set)
            {
                if (tags2Set.find(tag) != tags2Set.end())
                {
                    tagsIntersection.insert(tag);
                }
            }

            return static_cast<double>(tagsIntersection.size()) / static_cast<double>(tagsUnion.size());
        }

        [[nodiscard]] double tagsSimilarity(const std::vector<std::string>& tags1, const std::vector<std::string>& tags2) const
        {
            if (tags1.empty() || tags2.empty())
            {
                return 0.0;
            }

            double minDist = std::numeric_limits<double>::max();

            for (const std::string& tag1 : tags1)
            {
                if (inverted.find(tag1) == inverted.end())
                {
                    throw std::runtime_error("Tag '" + tag1 + "' has not been indexes");
                }

                double avgDist = 0;
                int count = 0;
                tag_type tag1Id = inverted.at(tag1);

                for (const std::string& tag2 : tags2)
                {
                    if (inverted.find(tag2) == inverted.end())
                    {
                        throw std::runtime_error("Tag '" + tag2 + "' has not been indexes");
                    }

                    tag_type tag2Id = inverted.at(tag2);
                    avgDist += relationship_graph.distance(tag1Id, tag2Id);
                    count++;
                }

                avgDist /= count;

                if (avgDist < minDist)
                {
                    minDist = avgDist;
                }
            }

            return 1 / (1 + minDist);
        }

        RelationshipGraph<tag_type>& getRelationshipGraph() noexcept
        {
            return relationship_graph;
        }

        void saveIndex(std::ofstream& output) const
        {
            output.write((char*) &capacity, sizeof(size_t));
            output.write((char*) &levels, sizeof(size_t));
            output.write((char*) &count, sizeof(size_t));
            output.write((char*) &tagIdCounter, sizeof(unsigned));
            output.write((char*) capacities, sizeof(size_t) * capacity);
            output.write((char*) entries, sizeof(size_t) * capacity);
            output.write((char*) maxFrequenciesPerLevel, sizeof(size_t) * levels);

            for (size_t i = 0; i < capacity; i++)
            {
                size_t internalCapacity = *(capacities + i);
                output.write((char*) tags[i], sizeof(tag_type) * internalCapacity);
            }

            for (int level = 0; level < levels; level++)
            {
                std::unordered_map<tag_type, size_t> tagFrequencies = levelTagFrequency[level];
                std::size_t size = tagFrequencies.size();
                output.write((char*) &size, sizeof(std::size_t));

                for (const auto& pair : tagFrequencies)
                {
                    output.write((char*) &pair.first, sizeof(tag_type));
                    output.write((char*) &pair.second, sizeof(size_t));
                }
            }

            std::size_t size = lookup.size();
            output.write((char*) &size, sizeof(std::size_t));

            for (const auto& pair : lookup)
            {
                tag_type tagId = pair.first;
                std::string tag = pair.second;
                const char* cTag = tag.c_str();
                std::size_t tagLength = tag.length();
                output.write((char*) &tagId, sizeof(tag_type));
                output.write((char*) &tagLength, sizeof(std::size_t));
                output.write(cTag, sizeof(char) * tagLength);
            }

            relationship_graph.saveIndex(output);
        }

        void loadIndex(std::ifstream& input)
        {
            input.read((char*) &capacity, sizeof(size_t));
            input.read((char*) &levels, sizeof(size_t));
            input.read((char*) &count, sizeof(size_t));
            input.read((char*) &tagIdCounter, sizeof(unsigned));

            capacities = (size_t*) malloc(sizeof(size_t) * capacity);
            entries = (size_t*) malloc(sizeof(size_t) * capacity);
            maxFrequenciesPerLevel = (size_t*) malloc(sizeof(size_t) * levels);
            tags = (tag_type**) malloc(sizeof(tag_type*) * capacity);

            if (!capacities || ! entries || !tags)
            {
                throw std::runtime_error("Not enough memory");
            }

            input.read((char*) capacities, sizeof(size_t) * capacity);
            input.read((char* ) entries, sizeof(size_t) * capacity);
            input.read((char*) maxFrequenciesPerLevel, sizeof(size_t) * levels);

            for (size_t i = 0; i < capacity; i++)
            {
                size_t internalCapacity = *(capacities + i);
                tags[i] = (tag_type*) malloc(sizeof(tag_type) * internalCapacity);
                input.read((char*) tags[i], sizeof(tag_type) * internalCapacity);
            }

            for (int level = 0; level < levels; level++)
            {
                std::unordered_map<tag_type, size_t> tagFrequencies;
                std::size_t size;
                input.read((char*) &size, sizeof(std::size_t));
                levelTagFrequency.emplace_back();

                for (int i = 0; i < size; i++)
                {
                    tag_type tag;
                    size_t frequency;
                    input.read((char*) &tag, sizeof(tag_type));
                    input.read((char*) &frequency, sizeof(size_t));
                    tagFrequencies.insert({tag, frequency});
                }

                levelTagFrequency[level] = tagFrequencies;
            }

            std::size_t size;
            input.read((char*) &size, sizeof(std::size_t));

            for (int i = 0; i < size; i++)
            {
                tag_type tagId;
                char* cTag;
                std::size_t tagLength;
                input.read((char*) &tagId, sizeof(tag_type));
                input.read((char*) &tagLength, sizeof(std::size_t));

                cTag = (char*) malloc(tagLength);
                input.read(cTag, sizeof(char) * tagLength);

                std::string tag = cTag;
                tag = tag.substr(0, tagLength);
                lookup.insert({tagId, tag});
                inverted.insert({tag, tagId});
            }

            relationship_graph.loadIndex(input, lookup, inverted);
        }
    };
}