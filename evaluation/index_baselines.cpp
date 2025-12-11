#include <postfilter_hnsw.h>
#include <multi_filter_hnsw.h>
#include <string>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <chrono>

using namespace hnswlib;

int main()
{
    std::string dataFile = "data_small.txt", indexDir = "indexes/", postfilterIndexFile = indexDir + "postfilter_index.idx",
        multiIndexIndexFile = indexDir + "multi_index_index.idx", mappingFile = indexDir + "baseline_mappings.txt", line;

    if (!std::filesystem::exists(indexDir))
    {
        std::filesystem::create_directory(indexDir);
    }

    unsigned dimension = 200, count = 1;
    size_t entries = 32440681;
    InnerProductSpace space(dimension);
    PostfilterHNSW<float> postfilterHnsw(&space, entries);
    MultiIndexHNSW<float> multiIndexHnsw(&space, entries);
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
    std::cout << "Done" << std::endl;

    return 0;
}
