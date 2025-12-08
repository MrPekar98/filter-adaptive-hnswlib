#pragma once

#include "hnswlib.h"
#include "hnswalg.h"
#include <algorithm>

namespace hnswlib
{
    template<typename dist_t>
    class PostfilterHNSW: public AlgorithmInterface<dist_t>
    {
    private:
        HierarchicalNSW<dist_t> hnsw;

    public:
        PostfilterHNSW(SpaceInterface<dist_t> *s)
            : hnsw(s)
        {}

        PostfilterHNSW(
            SpaceInterface<dist_t> *s,
            const std::string &location,
            bool nmslib = false,
            size_t max_elements = 0,
            bool allow_replace_deleted = false)
            : hnsw(s, location, nmslib, max_elements, allow_replace_deleted)
        {}

        PostfilterHNSW(
            SpaceInterface<dist_t> *s,
            size_t max_elements,
            size_t M = 16,
            size_t ef_construction = 200,
            size_t random_seed = 100,
            bool allow_replace_deleted = false)
            : hnsw(s, max_elements, M, ef_construction, random_seed, allow_replace_deleted)
        {}

        void addPoint(const void *datapoint, labeltype label, const std::vector<std::string>& tags, bool replace_deleted = false)
        {
            hnsw.addPoint(datapoint, label, tags, replace_deleted);
        }

        std::priority_queue<std::pair<dist_t, labeltype>> searchKnn(const void *query_data, size_t k, const std::vector<std::string>& tags, BaseFilterFunctor* isIdAllowed = nullptr) const
        {
            size_t initialK = k * 100;
            std::priority_queue<std::pair<dist_t, labeltype>> initialResults = hnsw.searchKnn(query_data, initialK, {}, isIdAllowed);
            std::priority_queue<std::pair<dist_t, labeltype>> processedResults;

            while (!initialResults.empty() && processedResults.size() < k)
            {
                std::pair<dist_t, labeltype> top = initialResults.top();
                tableint internalId = hnsw.label_lookup_.at(top.second);
                std::vector<std::string> nodeTags = hnsw.tag_index.get(internalId);
                initialResults.pop();

                for (const std::string& nodeTag : nodeTags)
                {
                    if (std::find(tags.begin(), tags.end(), nodeTag) != tags.end())
                    {
                        processedResults.push(top);
                        break;
                    }
                }
            }

            return processedResults;
        }

        void saveIndex(const std::string &location)
        {
            hnsw.saveIndex(location);
        }

        void loadIndex(const std::string &location, SpaceInterface<dist_t> *s, size_t max_elements_i = 0)
        {
            hnsw.loadIndex(location, s, max_elements_i);
        }
    };
}