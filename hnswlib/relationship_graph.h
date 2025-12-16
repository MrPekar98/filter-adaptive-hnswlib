#pragma once

#include <fstream>
#include <unordered_map>
#include <unordered_set>
#include <queue>

namespace hnswlib
{
    template<typename tag_type>
    class RelationshipGraph
    {
    private:
        std::unordered_map<tag_type, std::string>& lookup;
        std::unordered_map<std::string, tag_type>& inverted;
        std::unordered_map<tag_type, std::unordered_set<tag_type>> adjMatrix;

    public:
        RelationshipGraph(std::unordered_map<tag_type, std::string>& lookup,
            std::unordered_map<std::string, tag_type>& inverted)
            : lookup(lookup), inverted(inverted)
        {}

        RelationshipGraph& operator=(const RelationshipGraph& other)
        {
            lookup = other.lookup;
            inverted = other.inverted;
            adjMatrix = other.adjMatrix;

            return *this;
        }

        void relate(const std::unordered_set<tag_type>& tagIds) noexcept
        {
            for (const tag_type& tagId : tagIds)
            {
                std::unordered_set<tag_type> related = tagIds;
                related.erase(tagId);

                if (adjMatrix.find(tagId) != adjMatrix.end())
                {
                    adjMatrix[tagId].insert(related.begin(), related.end());
                }

                else
                {
                    adjMatrix.insert({tagId, related});
                }
            }
        }

        [[nodiscard]] int hops(const tag_type& fromTag, const tag_type& toTag) const noexcept
        {
            if (fromTag == toTag)
            {
                return 0;
            }

            else if (adjMatrix.find(fromTag) == adjMatrix.end() || adjMatrix.find(toTag) == adjMatrix.end())
            {
                return -1;
            }

            std::queue<std::pair<tag_type, int>> distances;
            std::unordered_set<tag_type> visited;
            distances.emplace(fromTag, 0);
            visited.insert(fromTag);

            while (!distances.empty())
            {
                auto tag = distances.front();
                distances.pop();

                for (const tag_type& related : adjMatrix.at(tag.first))
                {
                    if (related == toTag)
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

        void clear() noexcept
        {
            adjMatrix.clear();
        }

        void saveIndex(std::ofstream& output) const
        {
            std::size_t size = adjMatrix.size();
            output.write((char*) &size, sizeof(std::size_t));

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
        }

        void loadIndex(std::ifstream& input, const std::unordered_map<tag_type, std::string>& lookup,
            const std::unordered_map<std::string, tag_type>& inverted)
        {
            std::size_t size;
            input.read((char*) & size, sizeof(std::size_t));

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
        }
    };
}