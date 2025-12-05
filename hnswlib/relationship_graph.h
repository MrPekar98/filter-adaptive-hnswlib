#pragma once

#include <vector>
#include <fstream>

namespace hnswlib
{
    template<typename tag_type>
    class RelationshipGraph
    {
    private:

    public:
        void relate(const std::vector<tag_type>& tagIds) noexcept
        {

        }

        [[nodiscard]] int hops(const tag_type& fromTag, const tag_type& toTag) const noexcept
        {
            return -1;
        }

        void clear() noexcept
        {

        }

        void save_index(std::ofstream& output) const
        {

        }

        void load_index(std::ifstream& input)
        {

        }
    };
}