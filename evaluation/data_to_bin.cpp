#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <cstdint>
#include <unordered_map>

int main()
{
    uint32_t vectors = 32440681, dimension = 200;
    std::string vector_file = "data.txt", binary_file = "data/dataset_full/data.bin";
    std::ifstream reader(vector_file);
    std::ofstream writer(binary_file), label_writer("data/dataset_full/labels.txt");
    std::string line;
    unsigned count = 0, labelCount = 1;
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
        std::getline(reader, line);

        while (std::getline(tagStream, line, ' '))
        {
            if (labelIndex.find(line) == labelIndex.end())
            {
                labelIndex.insert({line, labelCount++});
            }

            int id = labelIndex.at(line);
            label_writer << id << ",";
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

    std::cout << "Done converting vectors" << std::endl;
    return 0;
}
