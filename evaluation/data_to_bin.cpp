#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <cstdint>
#include <unordered_map>
#include <vector>
#include <filesystem>

struct Query
{
    std::string uri;
    std::vector<std::string> tags;
    float* embedding;

    ~Query()
    {
        delete embedding;
    }
};

Query parse(const std::string& queryFile)
{
    std::ifstream stream(queryFile);
    std::string line;
    std::getline(stream, line);

    std::string uri = line;
    std::getline(stream, line);

    std::istringstream tagTokenStream(line);
    std::vector<std::string> tags;
    std::string tag;

    while (std::getline(tagTokenStream, tag, ' '))
    {
        tags.push_back(tag);
    }

    std::getline(stream, line);

    std::istringstream embeddingTokenStream(line);
    std::vector<double> embedding;
    std::string val;

    while (std::getline(embeddingTokenStream, val, ' '))
    {
        std::size_t pos;
        embedding.push_back(std::stod(val, &pos));

        if (val.length() != pos)
        {
            throw std::runtime_error("Failed parsing embeddings");
        }
    }

    float* embeddingArray = new float[embedding.size()];
    int i = 0;

    for (double val : embedding)
    {
        embeddingArray[i++] = val;
    }

    Query q = {.uri = uri, .tags = tags, .embedding = embeddingArray};
    return q;
}

int main()
{
    uint32_t vectors = 32440681, dimension = 200;
    std::string vector_file = "data.txt", binary_file = "data/dataset_full/data.bin";
    std::ifstream reader(vector_file);
    std::ofstream writer(binary_file), label_writer("data/dataset_full/labels.txt"), label_index_file("data/label_index.txt");
    std::string line;
    unsigned count = 0, labelCount = 2;
    std::unordered_map<std::string, int> labelIndex;
    writer.write(reinterpret_cast<const char*>(&vectors), sizeof(vectors));
    writer.write(reinterpret_cast<const char*>(&dimension), sizeof(dimension));
    std::cout << "Converting vectors to binary file" << std::endl;

    while (std::getline(reader, line))
    {
        std::string uri = line;
        std::getline(reader, line);

        std::string tagString = line;
        std::istringstream tagStream(tagString);
        bool isFirst = true;
        std::getline(reader, line);

        while (std::getline(tagStream, line, ' '))
        {
            if (labelIndex.find(line) == labelIndex.end())
            {
                labelIndex.insert({line, labelCount});
                label_index_file << labelCount++ << "=" << line << "\n";
            }

            if (!isFirst)
            {
                label_writer << ",";
            }

            int id = labelIndex.at(line);
            isFirst = false;
            label_writer << id;
        }

        if (isFirst) // A dummy label for thise entities that do not have a tag/label
        {
            label_writer << 1;
        }

        label_writer << "\n";

        std::string vectorString = line;
        std::istringstream vectorStream(vectorString);

        while (std::getline(vectorStream, line, ' '))
        {
            std::size_t pos;
            float val = std::stof(line);
            writer.write(reinterpret_cast<const char*>(&val), sizeof(val));
        }

        if (count % 1000 == 0)
        {
            std::cout << "Converted " << count << " vectors" << std::endl;
        }

        count++;
    }

    reader.close();
    writer.close();
    label_writer.close();
    label_index_file.close();
    std::cout << "Done converting vectors" << std::endl;

    std::string queryDirMultiTag = "queries/multi-tag/", queryDirSingleTag = "queries/single-tag/";
    vectors = 1;
    std::cout << "Generating query files" << std::endl;
    std::filesystem::create_directory("data/" + queryDirMultiTag);
    std::filesystem::create_directory("data/" + queryDirSingleTag);

    for (const auto& entry : std::filesystem::directory_iterator(queryDirMultiTag))
    {
        std::string queryPath = entry.path().string();
        std::filesystem::create_directory("data/" + queryPath);

        for (const auto& tagEntry : std::filesystem::directory_iterator(queryPath))
        {
            std::string percentilePath = tagEntry.path().string() + "/";
            std::filesystem::create_directory("data/" + percentilePath);

            for (const auto& percentileEntry : std::filesystem::directory_iterator(percentilePath))
            {
                std::string queryFile = percentileEntry.path().string();
                std::string queryName = percentileEntry.path().filename().string();
                Query q = parse(queryFile);
                std::ofstream query_writer("data/" + percentilePath + queryName + ".bin"), query_label_writer("data/" + percentilePath + queryName + "_labels.txt");
                bool isFirst = true;
                query_writer.write(reinterpret_cast<const char*>(&vectors), sizeof(vectors));
                query_writer.write(reinterpret_cast<const char*>(&dimension), sizeof(dimension));

                for (unsigned i = 0; i < vectors; i++)
                {
                    query_writer.write(reinterpret_cast<const char*>(&q.embedding[i]), sizeof(q.embedding[i]));
                }

                for (const std::string& tag : q.tags)
                {
                    if (!isFirst)
                    {
                        query_label_writer << ",";
                    }

                    int id = labelIndex.at(tag);
                    isFirst = false;
                    query_label_writer << id;
                }

                if (isFirst)
                {
                    query_label_writer << 1;
                }

                query_label_writer << "\n";
            }
        }
    }

    for (const auto& entry : std::filesystem::directory_iterator(queryDirSingleTag))
    {
        std::string queryPath = entry.path().string();
        std::filesystem::create_directory("data/" + queryPath);

        for (const auto& tagEntry : std::filesystem::directory_iterator(queryPath))
        {
            std::string percentilePath = tagEntry.path().string() + "/";
            std::filesystem::create_directory("data/" + percentilePath);

            for (const auto& percentileEntry : std::filesystem::directory_iterator(percentilePath))
            {
                std::string queryFile = percentileEntry.path().string();
                std::string queryName = percentileEntry.path().filename().string();
                Query q = parse(queryFile);
                std::ofstream query_writer("data/" + percentilePath + queryName + ".bin"), query_label_writer("data/" + percentilePath + queryName + "_labels.txt");
                bool isFirst = true;
                query_writer.write(reinterpret_cast<const char*>(&vectors), sizeof(vectors));
                query_writer.write(reinterpret_cast<const char*>(&dimension), sizeof(dimension));

                for (unsigned i = 0; i < vectors; i++)
                {
                    query_writer.write(reinterpret_cast<const char*>(&q.embedding[i]), sizeof(q.embedding[i]));
                }

                for (const std::string& tag : q.tags)
                {
                    if (!isFirst)
                    {
                        query_label_writer << ",";
                    }

                    int id = labelIndex.at(tag);
                    isFirst = false;
                    query_label_writer << id;
                }

                if (isFirst)
                {
                    query_label_writer << 1;
                }

                query_label_writer << "\n";
            }
        }
    }

    std::cout << "Done" << std::endl;
    return 0;
}
