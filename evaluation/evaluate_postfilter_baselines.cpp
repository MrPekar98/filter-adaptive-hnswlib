#include <postfilter_hnsw.h>
#include <string>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <vector>
#include <fstream>
#include <cstdlib>

using namespace hnswlib;

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

// Returns runtimes
std::vector<int> postFilterBaselineEvaluateQueries(const std::string& queryPath, const std::string& resultsPath, const hnswlib::PostfilterHNSW<float>& baseline, size_t k)
{
    std::vector<int> runtimes;

    for (const auto& entry : std::filesystem::directory_iterator(queryPath))
    {
        std::string queryFile = entry.path().string();
        std::string queryName = entry.path().filename().string();
        Query q = parse(queryFile);
        std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();
        std::priority_queue<std::pair<float, labeltype>> results = baseline.searchKnn((void*) q.embedding, k, q.tags);
        std::chrono::milliseconds duration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - startTime);
        runtimes.push_back(duration.count());

        std::ofstream outputStream(resultsPath + queryName);

        while (!results.empty())
        {
            std::pair<float, labeltype> result = results.top();
            results.pop();
            outputStream << result.second << "=" << result.first << "\n";
        }
    }

    return runtimes;
}

double average(const std::vector<int>& values)
{
    long sum = std::accumulate(values.begin(), values.end(), 0L);
    return static_cast<double>(sum) / values.size();
}

int main()
{
    std::string queryDir = "queries/single-tag/", heteroQueries = queryDir + "heterogeneous/", homoQueries = queryDir + "homogeneous/",
        resultDir = "results/", baselineResultDir = resultDir + "postfilter_baseline/";
    std::vector<int> allRuntimes;
    size_t max_elements = 32440681;
    size_t k = 100;
    hnswlib::InnerProductSpace space(200);
    hnswlib::PostfilterHNSW<float> postfilterBaseline(&space);
    postfilterBaseline.loadIndex("indexes/postfilter_index.idx", &space, max_elements);

    if (!std::filesystem::exists(resultDir))
    {
        std::filesystem::create_directory(resultDir);
    }

    if (!std::filesystem::exists(baselineResultDir))
    {
        std::filesystem::create_directory(baselineResultDir);
    }

    // Heterogeneous 90th percentile queries
    std::cout << "90th percentile heterogeneous queries" << std::endl;
    std::filesystem::create_directory(baselineResultDir + "hetero_90th-percentile/");

    std::vector<int> runtimes = postFilterBaselineEvaluateQueries(heteroQueries + "90th-percentile/", baselineResultDir + "hetero_90th-percentile/", postfilterBaseline, k);
    double averageRuntime = average(runtimes);
    std::cout << "Average runtime: " << averageRuntime << "ms\n" << std::endl;
    allRuntimes.insert(allRuntimes.end(), runtimes.begin(), runtimes.end());

    // Heterogeneous 50th percentile queries
    std::cout << "50th percentile heterogeneous queries" << std::endl;
    std::filesystem::create_directory(baselineResultDir + "hetero_50th-percentile/");

    runtimes = postFilterBaselineEvaluateQueries(heteroQueries + "50th-percentile/", baselineResultDir + "hetero_50th-percentile/", postfilterBaseline, k);
    averageRuntime = average(runtimes);
    std::cout << "Average runtime: " << averageRuntime << "ms\n" << std::endl;
    allRuntimes.insert(allRuntimes.end(), runtimes.begin(), runtimes.end());

    // Heterogeneous 10th percentile queries
    std::cout << "10th percentile heterogeneous queries" << std::endl;
    std::filesystem::create_directory(baselineResultDir + "hetero_10th-percentile/");

    runtimes = postFilterBaselineEvaluateQueries(heteroQueries + "10th-percentile/", baselineResultDir + "hetero_10th-percentile/", postfilterBaseline, k);
    averageRuntime = average(runtimes);
    std::cout << "Average runtime: " << averageRuntime << "ms\n" << std::endl;
    allRuntimes.insert(allRuntimes.end(), runtimes.begin(), runtimes.end());

    // Homogeneous 90th percentile queries
    std::cout << "90th percentile homogeneous queries" << std::endl;
    std::filesystem::create_directory(baselineResultDir + "homo_90th-percentile/");

    runtimes = postFilterBaselineEvaluateQueries(homoQueries + "90th-percentile/", baselineResultDir + "homo_90th-percentile/", postfilterBaseline, k);
    averageRuntime = average(runtimes);
    std::cout << "Average runtime: " << averageRuntime << "ms\n" << std::endl;
    allRuntimes.insert(allRuntimes.end(), runtimes.begin(), runtimes.end());

    // Homogeneous 50th percentile queries
    std::cout << "50th percentile homogeneous queries" << std::endl;
    std::filesystem::create_directory(baselineResultDir + "homo_50th-percentile/");

    runtimes = postFilterBaselineEvaluateQueries(homoQueries + "50th-percentile/", baselineResultDir + "homo_50th-percentile/", postfilterBaseline, k);
    averageRuntime = average(runtimes);
    std::cout << "Average runtime: " << averageRuntime << "ms\n" << std::endl;
    allRuntimes.insert(allRuntimes.end(), runtimes.begin(), runtimes.end());

    // Homogeneous 10th percentile queries
    std::cout << "10th percentile homogeneous queries" << std::endl;
    std::filesystem::create_directory(baselineResultDir + "homo_10th-percentile/");

    runtimes = postFilterBaselineEvaluateQueries(homoQueries + "10th-percentile/", baselineResultDir + "homo_10th-percentile/", postfilterBaseline, k);
    averageRuntime = average(runtimes);
    std::cout << "Average runtime: " << averageRuntime << "ms\n" << std::endl;
    allRuntimes.insert(allRuntimes.end(), runtimes.begin(), runtimes.end());

    double totalAverageRuntime = average(allRuntimes);
    std::cout << "Total average runtime: " << totalAverageRuntime << "ms" << std::endl;

    return 0;
}
