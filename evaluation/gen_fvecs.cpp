#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdint>
#include <vector>
#include <unordered_map>

int main()
{
    std::string line;
    std::ifstream reader("data_small.txt");
    std::ofstream fvecWriter("data/vectors.fvecs");
    uint32_t dimension = 200;
    std::unordered_map<std::string, std::size_t> freqs;
    std::cout << "Constructing fvecs file" << std::endl;

    while (std::getline(reader, line))
    {
        std::string uri = line;
        std::getline(reader, line);

        std::istringstream tagTokenStream(line);
        std::string tag;

        while (std::getline(tagTokenStream, tag, ' '))
        {
            if (freqs.find(tag) == freqs.end())
            {
                freqs.insert({tag, 0});
            }

            freqs[tag]++;
        }

        std::getline(reader, line);

        std::istringstream embeddingTokenStream(line);
        std::string val;
        fvecWriter.write(reinterpret_cast<const char*>(&dimension), sizeof(dimension));

        while (std::getline(embeddingTokenStream, val, ' '))
        {
            float fVal = std::stof(val);
            fvecWriter.write(reinterpret_cast<const char*>(&fVal), sizeof(fVal));
        }
    }

    std::ofstream freqWriter("data/frequencies.txt");

    for (const auto& pair : freqs)
    {
        freqWriter << pair.first << "=" << pair.second << "\n";
    }

    std::cout << "Done" << std::endl;
    return 0;
}
