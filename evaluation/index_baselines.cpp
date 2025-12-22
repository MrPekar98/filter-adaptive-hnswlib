#include <postfilter_hnsw.h>
#include <multi_filter_hnsw.h>
#include <hnswalg.h>
#include <string>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <chrono>
#include <unordered_set>

using namespace hnswlib;

std::unordered_set<std::string> readQueryTags(const std::string& queryDir)
{
    std::unordered_set<std::string> tags;

    for (const auto& typeEntry : std::filesystem::directory_iterator(queryDir))
    {
        std::string typeSubDir = typeEntry.path().string();

        for (const auto& popularityEntry : std::filesystem::directory_iterator(typeSubDir))
        {
            std::string popularitySubDir = popularityEntry.path().string();

            for (const auto& queryFileEntry : std::filesystem::directory_iterator(popularitySubDir))
            {
                std::ifstream stream(queryFileEntry.path().string());
                std::string line;
                std::getline(stream, line);
                std::getline(stream, line);
                stream.close();

                std::istringstream tagStream(line);
                std::string tag;

                while (std::getline(tagStream, tag, ' '))
                {
                    tags.insert(tag);
                }
            }
        }
    }

    return tags;
}

int main()
{
    std::string dataFile = "data_small.txt", indexDir = "indexes/", postfilterIndexFile = indexDir + "postfilter_index.idx",
        multiIndexIndexFile = indexDir + "multi_index_index.idx", adaptiveIndexFile = indexDir + "adaptive_index.idx",
        mappingFile = indexDir + "baseline_mappings.txt", queryDir = "queries/single-tag/", line;
    std::unordered_set<std::string> queryTags = readQueryTags(queryDir);

    if (!std::filesystem::exists(indexDir))
    {
        std::filesystem::create_directory(indexDir);
    }

    unsigned dimension = 200, count = 1;
    size_t entries = 32440681;
    InnerProductSpace space(dimension);
    PostfilterHNSW<float> postfilterHnsw(&space, entries);
    MultiIndexHNSW<float> multiIndexHnsw(&space, queryTags, entries);
    HierarchicalNSW<float> adaptiveHnsw(&space, entries, 16, 200, 100, false, true);
    std::ifstream stream(dataFile);
    std::ofstream mappingStream(mappingFile);
    std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();
    std::cout << "Loading data now" << std::endl;

    while (std::getline(stream, line))
    {
        std::string uri = line;
        std::getline(stream, line);

        std::string tagsLine = line;
        std::vector<std::string> tags;
        std::istringstream tokenStreamTags(tagsLine);
        std::getline(stream, line);

        std::string embeddingsLine = line;
        float* embedding = new float[dimension];
        int e = 0;
        std::istringstream tokenStreamEmbeddings(embeddingsLine);

        while (std::getline(tokenStreamEmbeddings, line, ' '))
        {
            std::size_t pos;
            embedding[e++] = std::stod(line, &pos);

            if (line.length() != pos)
            {
                throw std::runtime_error("Failed parsing embeddings");
            }
        }

        while (std::getline(tokenStreamTags, line, ' '))
        {
            tags.push_back(line);
        }

        mappingStream << count << "=" << uri << "\n";
        postfilterHnsw.addPoint(embedding, count, tags);
        multiIndexHnsw.addPoint(embedding, count, tags);
        adaptiveHnsw.addPoint(embedding, count, tags);

        if (count++ % 1000 == 0)
        {
            std::chrono::seconds duration = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - startTime);
            std::cout << "Loaded 1000 (total " << (count - 1) << "/" << entries << ") entities in " << duration.count() << "s\n";

            startTime = std::chrono::steady_clock::now();
        }
    }

    std::cout << "Done loading index\nSaving on disk..." << std::endl;
    postfilterHnsw.saveIndex(postfilterIndexFile);
    multiIndexHnsw.saveIndex(multiIndexIndexFile);
    adaptiveHnsw.saveIndex(adaptiveIndexFile);
    adaptiveHnsw.dumpLevelDistribution("tag_distribution_dump.txt");
    std::cout << "Done" << std::endl;

    return 0;
}
