#include <faiss/IndexACORN.h>
#include <faiss/utils/utils.h>
#include <string>
#include <stdexcept>
#include <chrono>
#include <iostream>

int main()
{
    std::string indexDir = "indexes/acorn/", fvecsFile = "";
    std::size_t dimension = 200, embeddingsCount = 6966496, d, nb;
    int m = 32, gamma = 12, mBeta = 32;
    std::vector<int> metadata;
    faiss::IndexACORNFlat index(dimension, m, gamma, metadata, mBeta);
    float* xb = fvecs_read(fvecsFile.c_str(), &dimension2, &nb);

    if (d != dimension)
    {
        throw std::runtime_error("Fvecs dimensions are incorrect");
    }

    std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();
    index.add(nb, xb);

    std::chrono::hours duration = std::chrono::duration_cast<std::chrono::hours>(std::chrono::steady_clock::now() - startTime);
    std::cout << "Done indexing in " << duration.count() << " hours" << std::endl;

    return 0;
}
