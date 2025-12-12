#pragma once

#include "hnswlib.h"
#include "hnswalg.h"
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <fstream>
#include <vector>

namespace hnswlib
{
	template<typename dist_t>
	class MultiIndexHNSW: public AlgorithmInterface<dist_t>
	{
	private:
		SpaceInterface<dist_t> *s;
		std::string location;
		bool nmslib, allow_replace_deleted;
		size_t max_elements, M, ef_construction, random_seed;
		std::unordered_map<std::string, unsigned> lookup;
		std::vector<HierarchicalNSW<dist_t>*> indexes;
		std::unordered_set<std::string> allowedTags;

	public:
		explicit MultiIndexHNSW(SpaceInterface<dist_t> *s, const std::unordered_set<std::string>& allowedTags)
			: s(s), nmslib(false), max_elements(max_elements), allow_replace_deleted(false),
				M(16), ef_construction(200), random_seed(100), allowedTags(allowedTags)
		{
			initIndexes();
		}

		MultiIndexHNSW(
			SpaceInterface<dist_t> *s,
            const std::string &location,
            const std::unordered_set<std::string>& allowedTags,
            bool nmslib = false,
            size_t max_elements = 0,
            bool allow_replace_deleted = false)
			: s(s), location(s), nmslib(nmslib), max_elements(max_elements), allow_replace_deleted(allow_replace_deleted),
				M(16), ef_construction(200), random_seed(100), allowedTags(allowedTags)
		{
			initIndexes();
		}

		MultiIndexHNSW(
			SpaceInterface<dist_t> *s,
			const std::unordered_set<std::string>& allowedTags,
            size_t max_elements,
            size_t M = 16,
            size_t ef_construction = 200,
            size_t random_seed = 100,
            bool allow_replace_deleted = false)
			: s(s), nmslib(false), max_elements(max_elements), allow_replace_deleted(allow_replace_deleted),
				M(M), ef_construction(ef_construction), random_seed(random_seed), allowedTags(allowedTags)
		{
			initIndexes();
		}

		void initIndexes()
		{
			for (const std::string& tag : allowedTags)
			{
				lookup.insert({tag, indexes.size()});
				indexes.push_back(new HierarchicalNSW<dist_t>(s, max_elements, M, ef_construction, random_seed, allow_replace_deleted));
			}
		}

		void addPoint(const void *datapoint, labeltype label, const std::vector<std::string>& tags, bool replace_deleted = false)
		{
			for (const std::string& tag : tags)
			{
				if (allowedTags.find(tag) != allowedTags.end())
				{
					unsigned index = lookup[tag];
					indexes[index]->addPoint(datapoint, label, {}, replace_deleted);
				}
			}
		}

		std::priority_queue<std::pair<dist_t, labeltype>> searchKnn(const void *query_data, size_t k, const std::vector<std::string>& tags, BaseFilterFunctor* isIdAllowed = nullptr) const
		{
			std::priority_queue<std::pair<dist_t, labeltype>> results;
			std::unordered_set<labeltype> exists;
			exists.reserve(k);

			for (const std::string& tag : tags)
			{
				if (lookup.find(tag) != lookup.end())
				{
					unsigned index = lookup.at(tag);
					std::priority_queue<std::pair<dist_t, labeltype>> tagResults = indexes[index]->searchKnn(query_data, k, {}, isIdAllowed);

					while (!tagResults.empty())
					{
						std::pair<dist_t, labeltype> result = tagResults.top();
						tagResults.pop();

						if (exists.find(result.second) == exists.end())
						{
							results.push(result);
							exists.insert(result.second);
						}
					}
				}
			}

			while (results.size() > k)
			{
				results.pop();
			}

			return results;
		}

		void saveIndex(const std::string& location)
		{
			std::ofstream metaStream(location + ".meta");
			std::unordered_map<unsigned, std::string> inverted;

			for (const auto& pair : lookup)
			{
				inverted.insert({pair.second, pair.first});
			}

			for (unsigned index = 0; index < indexes.size(); index++)
			{
				metaStream << inverted[index] << "\n";
				indexes[index]->saveIndex(location + "." + std::to_string(index));
			}
		}

		void loadIndex(const std::string& location, SpaceInterface<dist_t>* s, size_t max_elements_i = 0)
		{
			std::ifstream metaStream(location + ".meta");
			std::string tag;
			unsigned index = 0;

			while (std::getline(metaStream, tag))
			{
				HierarchicalNSW<dist_t> hnsw(s);
				hnsw.loadIndex(location + "." + std::to_string(index), s, max_elements_i);
				lookup.insert({tag, index});
				indexes.push_back(&hnsw);
			}
		}
	};
}