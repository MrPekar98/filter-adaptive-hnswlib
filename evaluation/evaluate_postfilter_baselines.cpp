#include <postfilter_hnsw.h>
#include <multi_filter_hnsw.h>
#include <hnswalg.h>
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
std::vector<int> evaluateQueries(const std::string& queryPath, const std::string& resultsPath, const hnswlib::AlgorithmInterface<float>* baseline, size_t k)
{
    std::vector<int> runtimes;

    for (const auto& entry : std::filesystem::directory_iterator(queryPath))
    {
        std::string queryFile = entry.path().string();
        std::string queryName = entry.path().filename().string();
        Query q = parse(queryFile);
        std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();
        std::priority_queue<std::pair<float, labeltype>> results = baseline->searchKnn((void*) q.embedding, k, q.tags);
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
        resultDir = "results/", postfilterBaselineResultDir = resultDir + "postfilter_baseline/", multiFilterBaselineResultDir = resultDir + "multifilter_baseline/", adaptiveIndexResultDir = resultDir + "adaptive/";
    std::vector<int> allPostfilterRuntimes, allMultiRuntimes, allAdaptiveRuntimes;
    size_t max_elements = 32440681;
    size_t k = 100;
    hnswlib::InnerProductSpace space(200);
    hnswlib::PostfilterHNSW<float> postfilterBaseline(&space);
    hnswlib::MultiIndexHNSW<float> multiIndexBaseline(&space, {});
    hnswlib::HierarchicalNSW<float> adaptiveHnsw(&space);
    std::cout << "Loading indexes\n" << std::endl;
    postfilterBaseline.loadIndex("indexes/postfilter_index.idx", &space, max_elements);
    multiIndexBaseline.loadIndex("indexes/multi_index_index.idx", &space, max_elements);

    if (!std::filesystem::exists(resultDir))
    {
        std::filesystem::create_directory(resultDir);
        std::filesystem::create_directory(postfilterBaselineResultDir);
        std::filesystem::create_directory(multiFilterBaselineResultDir);
        std::filesystem::create_directory(adaptiveIndexResultDir);
    }

    // Heterogeneous 90th percentile queries
    std::cout << "90th percentile heterogeneous queries" << std::endl;
    std::filesystem::create_directory(postfilterBaselineResultDir + "hetero_90th-percentile/");
    std::filesystem::create_directory(multiFilterBaselineResultDir + "hetero_90th-percentile/");
    std::filesystem::create_directory(adaptiveIndexResultDir + "hetero_90th-percentile/");

    std::vector<int> postfilterRuntimes = evaluateQueries(heteroQueries + "90th-percentile/", postfilterBaselineResultDir + "hetero_90th-percentile/", &postfilterBaseline, k);
    std::vector<int> multiRuntimes = evaluateQueries(heteroQueries + "90th-percentile/", multiFilterBaselineResultDir + "hetero_90th-percentile/", &multiIndexBaseline, k);
    std::vector<int> adaptiveRuntimes = evaluateQueries(heteroQueries + "90th-percentile/", adaptiveIndexResultDir + "hetero_90th-percentile/", &adaptiveHnsw, k);
    double averagePostfilterRuntime = average(postfilterRuntimes), averageMultiRuntime = average(multiRuntimes), averageAdaptiveRuntime = average(adaptiveRuntimes);
    std::cout << "Average postfilter runtime: " << averagePostfilterRuntime << "ms" << std::endl;
    std::cout << "Average multi-index runtime: " << averageMultiRuntime << "ms" << std::endl;
    std::cout << "Average adaptive index runtime: " << averageAdaptiveRuntime << "ms\n" << std::endl;
    allPostfilterRuntimes.insert(allPostfilterRuntimes.end(), postfilterRuntimes.begin(), postfilterRuntimes.end());
    allMultiRuntimes.insert(allMultiRuntimes.end(), multiRuntimes.begin(), multiRuntimes.end());
    allAdaptiveRuntimes.insert(allAdaptiveRuntimes.end(), adaptiveRuntimes.begin(), adaptiveRuntimes.end());

    // Heterogeneous 50th percentile queries
    std::cout << "50th percentile heterogeneous queries" << std::endl;
    std::filesystem::create_directory(postfilterBaselineResultDir + "hetero_50th-percentile/");
    std::filesystem::create_directory(multiFilterBaselineResultDir + "hetero_50th-percentile/");
    std::filesystem::create_directory(adaptiveIndexResultDir + "hetero_50th-percentile/");

    postfilterRuntimes = evaluateQueries(heteroQueries + "50th-percentile/", postfilterBaselineResultDir + "hetero_50th-percentile/", &postfilterBaseline, k);
    multiRuntimes = evaluateQueries(heteroQueries + "50th-percentile/", multiFilterBaselineResultDir + "hetero_50th-percentile/", &multiIndexBaseline, k);
    adaptiveRuntimes = evaluateQueries(heteroQueries + "50th-percentile/", adaptiveIndexResultDir + "hetero_50th-percentile/", &adaptiveHnsw, k);
    averagePostfilterRuntime = average(postfilterRuntimes), averageMultiRuntime = average(multiRuntimes), averageAdaptiveRuntime = average(adaptiveRuntimes);
    std::cout << "Average postfilter runtime: " << averagePostfilterRuntime << "ms" << std::endl;
    std::cout << "Average multi-index runtime: " << averageMultiRuntime << "ms" << std::endl;
    std::cout << "Average adaptive index runtime: " << averageAdaptiveRuntime << "ms\n" << std::endl;
    allPostfilterRuntimes.insert(allPostfilterRuntimes.end(), postfilterRuntimes.begin(), postfilterRuntimes.end());
    allMultiRuntimes.insert(allMultiRuntimes.end(), multiRuntimes.begin(), multiRuntimes.end());
    allAdaptiveRuntimes.insert(allAdaptiveRuntimes.end(), adaptiveRuntimes.begin(), adaptiveRuntimes.end());

    // Heterogeneous 10th percentile queries
    std::cout << "10th percentile heterogeneous queries" << std::endl;
    std::filesystem::create_directory(postfilterBaselineResultDir + "hetero_10th-percentile/");
    std::filesystem::create_directory(multiFilterBaselineResultDir + "hetero_10th-percentile/");
    std::filesystem::create_directory(adaptiveIndexResultDir + "hetero_10th-percentile/");

    postfilterRuntimes = evaluateQueries(heteroQueries + "10th-percentile/", postfilterBaselineResultDir + "hetero_10th-percentile/", &postfilterBaseline, k);
    multiRuntimes = evaluateQueries(heteroQueries + "10th-percentile/", multiFilterBaselineResultDir + "hetero_10th-percentile/", &multiIndexBaseline, k);
    adaptiveRuntimes = evaluateQueries(heteroQueries + "10th-percentile/", adaptiveIndexResultDir + "hetero_10th-percentile/", &adaptiveHnsw, k);
    averagePostfilterRuntime = average(postfilterRuntimes), averageMultiRuntime = average(multiRuntimes), averageAdaptiveRuntime = average(adaptiveRuntimes);
    std::cout << "Average postfilter runtime: " << averagePostfilterRuntime << "ms" << std::endl;
    std::cout << "Average multi-index runtime: " << averageMultiRuntime << "ms" << std::endl;
    std::cout << "Average adaptive index runtime: " << averageAdaptiveRuntime << "ms\n" << std::endl;
    allPostfilterRuntimes.insert(allPostfilterRuntimes.end(), postfilterRuntimes.begin(), postfilterRuntimes.end());
    allMultiRuntimes.insert(allMultiRuntimes.end(), multiRuntimes.begin(), multiRuntimes.end());
    allAdaptiveRuntimes.insert(allAdaptiveRuntimes.end(), adaptiveRuntimes.begin(), adaptiveRuntimes.end());

    // Homogeneous 90th percentile queries
    std::cout << "90th percentile homogeneous queries" << std::endl;
    std::filesystem::create_directory(postfilterBaselineResultDir + "homo_90th-percentile/");
    std::filesystem::create_directory(multiFilterBaselineResultDir + "homo_90th-percentile/");
    std::filesystem::create_directory(adaptiveIndexResultDir + "homo_90th-percentile/");

    postfilterRuntimes = evaluateQueries(heteroQueries + "90th-percentile/", postfilterBaselineResultDir + "homo_90th-percentile/", &postfilterBaseline, k);
    multiRuntimes = evaluateQueries(heteroQueries + "90th-percentile/", multiFilterBaselineResultDir + "homo_90th-percentile/", &multiIndexBaseline, k);
    adaptiveRuntimes = evaluateQueries(heteroQueries + "90th-percentile/", adaptiveIndexResultDir + "homo_90th-percentile/", &adaptiveHnsw, k);
    averagePostfilterRuntime = average(postfilterRuntimes), averageMultiRuntime = average(multiRuntimes), averageAdaptiveRuntime = average(adaptiveRuntimes);
    std::cout << "Average postfilter runtime: " << averagePostfilterRuntime << "ms" << std::endl;
    std::cout << "Average multi-index runtime: " << averageMultiRuntime << "ms" << std::endl;
    std::cout << "Average adaptive index runtime: " << averageAdaptiveRuntime << "ms\n" << std::endl;
    allPostfilterRuntimes.insert(allPostfilterRuntimes.end(), postfilterRuntimes.begin(), postfilterRuntimes.end());
    allMultiRuntimes.insert(allMultiRuntimes.end(), multiRuntimes.begin(), multiRuntimes.end());
    allAdaptiveRuntimes.insert(allAdaptiveRuntimes.end(), adaptiveRuntimes.begin(), adaptiveRuntimes.end());

    // Homogeneous 50th percentile queries
    std::cout << "50th percentile homogeneous queries" << std::endl;
    std::filesystem::create_directory(postfilterBaselineResultDir + "homo_50th-percentile/");
    std::filesystem::create_directory(multiFilterBaselineResultDir + "homo_50th-percentile/");
    std::filesystem::create_directory(adaptiveIndexResultDir + "homo_50th-percentile/");

    postfilterRuntimes = evaluateQueries(heteroQueries + "50th-percentile/", postfilterBaselineResultDir + "homo_50th-percentile/", &postfilterBaseline, k);
    multiRuntimes = evaluateQueries(heteroQueries + "50th-percentile/", multiFilterBaselineResultDir + "homo_50th-percentile/", &multiIndexBaseline, k);
    adaptiveRuntimes = evaluateQueries(heteroQueries + "50th-percentile/", adaptiveIndexResultDir + "homo_50th-percentile/", &adaptiveHnsw, k);
    averagePostfilterRuntime = average(postfilterRuntimes), averageMultiRuntime = average(multiRuntimes), averageAdaptiveRuntime = average(adaptiveRuntimes);
    std::cout << "Average postfilter runtime: " << averagePostfilterRuntime << "ms" << std::endl;
    std::cout << "Average multi-index runtime: " << averageMultiRuntime << "ms" << std::endl;
    std::cout << "Average adaptive index runtime: " << averageAdaptiveRuntime << "ms\n" << std::endl;
    allPostfilterRuntimes.insert(allPostfilterRuntimes.end(), postfilterRuntimes.begin(), postfilterRuntimes.end());
    allMultiRuntimes.insert(allMultiRuntimes.end(), multiRuntimes.begin(), multiRuntimes.end());
    allAdaptiveRuntimes.insert(allAdaptiveRuntimes.end(), adaptiveRuntimes.begin(), adaptiveRuntimes.end());

    // Homogeneous 10th percentile queries
    std::cout << "10th percentile homogeneous queries" << std::endl;
    std::filesystem::create_directory(postfilterBaselineResultDir + "homo_10th-percentile/");
    std::filesystem::create_directory(multiFilterBaselineResultDir + "homo_10th-percentile/");
    std::filesystem::create_directory(adaptiveIndexResultDir + "homo_10th-percentile/");

    postfilterRuntimes = evaluateQueries(heteroQueries + "10th-percentile/", postfilterBaselineResultDir + "homo_10th-percentile/", &postfilterBaseline, k);
    multiRuntimes = evaluateQueries(heteroQueries + "10th-percentile/", multiFilterBaselineResultDir + "homo_10th-percentile/", &multiIndexBaseline, k);
    adaptiveRuntimes = evaluateQueries(heteroQueries + "10th-percentile/", adaptiveIndexResultDir + "homo_10th-percentile/", &adaptiveHnsw, k);
    averagePostfilterRuntime = average(postfilterRuntimes), averageMultiRuntime = average(multiRuntimes), averageAdaptiveRuntime = average(adaptiveRuntimes);
    std::cout << "Average postfilter runtime: " << averagePostfilterRuntime << "ms" << std::endl;
    std::cout << "Average multi-index runtime: " << averageMultiRuntime << "ms" << std::endl;
    std::cout << "Average adaptive index runtime: " << averageAdaptiveRuntime << "ms\n" << std::endl;
    allPostfilterRuntimes.insert(allPostfilterRuntimes.end(), postfilterRuntimes.begin(), postfilterRuntimes.end());
    allMultiRuntimes.insert(allMultiRuntimes.end(), multiRuntimes.begin(), multiRuntimes.end());
    allAdaptiveRuntimes.insert(allAdaptiveRuntimes.end(), adaptiveRuntimes.begin(), adaptiveRuntimes.end());

    std::cout << std::endl << "Average postfilter baseline runtime: " << average(allPostfilterRuntimes) << "ms" << std::endl;
    std::cout << "Average multi-filter index runtime: " << average(allMultiRuntimes) << "ms" << std::endl;
    std::cout << "Average adaptive index runtime: " << average(allAdaptiveRuntimes) << "ms" << std::endl;

    return 0;
}
