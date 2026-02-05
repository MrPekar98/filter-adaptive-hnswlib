#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <set>
#include <unordered_set>
#include <unordered_map>
#include <filesystem>

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

std::unordered_map<std::string, unsigned> readLabelIndex(std::string file)
{
    std::ifstream reader(file);
    std::string line;
    std::unordered_map<std::string, unsigned> index;

    while (std::getline(reader, line))
    {
        std::string sec1 = line.substr(0, line.find("=")), sec2 = line.substr(line.find("=") + 1);
        index.insert({sec2, std::stoi(sec1)});
    }

    return index;
}

int main()
{
    unsigned threads = 4, maxDegree = 32, lBuild = 100, crossEdges = 6;
    double alpha = 1.2;
    std::string scenario = "general", distFunction = "L2", queryDir = "queries/multi-tag/",
        dataDir = "data/dataset_full/", newDataDir = dataDir + "multi_index_baseline_dataset/";
    std::filesystem::create_directory(newDataDir);

    uint64_t vectors = 32440681, dimension = 200;
    unsigned count = 0;
    std::string vectorFile = "data.txt", binaryFile = newDataDir + "data.bin", line;
    std::ifstream reader(vectorFile);
    std::unordered_set<std::string> queryTags = readQueryTags(queryDir);
    std::unordered_map<std::string, std::ofstream> fileIndex;
    std::unordered_map<std::string, std::ofstream> labelFileIndex;
    std::unordered_map<std::string, unsigned> labelIndex = readLabelIndex("data/label_index.txt");
    std::unordered_map<std::string, bool> isFirstIndex;
    std::cout << "Setting up index binary files" << std::endl;

    for (const std::string& tag : queryTags)
    {
        unsigned labelId = labelIndex[tag];
        fileIndex.emplace(tag, newDataDir + "label_" + std::to_string(labelId) + ".bin");
        fileIndex[tag].write(reinterpret_cast<const char*>(&vectors), sizeof(vectors));
        fileIndex[tag].write(reinterpret_cast<const char*>(&dimension), sizeof(dimension));
        labelFileIndex.emplace(tag, newDataDir + "label_" + std::to_string(labelId) + ".txt");
        isFirstIndex.insert({tag, true});
    }

    std::cout << "Converting vectors to binary files" << std::endl;

    while (std::getline(reader, line))
    {
        std::string uri = line;
        std::getline(reader, line);

        std::string tagString = line;
        std::istringstream tagStream(tagString);
        std::set<std::string> tags;
        std::getline(reader, line);

        std::string vectorString = line;

        while (std::getline(tagStream, line, ' '))
        {
            unsigned labelId = labelIndex[line];
            isFirstIndex[line] = false;
            labelFileIndex[line] << labelId;
            tags.insert(line);
        }

        for (const std::string& tag : tags)
        {
            if (isFirstIndex[tag]) // A dummy label when an entity does not have a tag
            {
                labelFileIndex[tag] << 1;
            }

            labelFileIndex[tag] << "\n";

            std::istringstream vectorStream(vectorString);

            while (std::getline(vectorStream, line, ' '))
            {
                std::size_t pos;
                float val = std::stof(line);
                fileIndex[tag].write(reinterpret_cast<const char*>(&val), sizeof(val));
            }
        }

        if (count % 1000 == 0)
        {
            std::cout << "Converted " << count << " vectors" << std::endl;
        }

        count++;
    }

    std::cout << "Done" << std::endl;

    return 0;
}
