# Evaluation
This directory contains the evaluation data and scripts.
The queries are provided in `queries/`.
Queries are split into _heterogeneous_ queries, where each query entity is selected from a set of 100 unique RDF types, and the _homogeneous_ queries are selected such that every query entity share at least one RDF type.
The most popular RDF types, such as _owl:Thing_ and __CareerStation__ are not considered.
We further split the queries based on their frequency in the knowledge graph dataset.
One split contains entities with RDF types belonging to the 90th percentile of RDF type frequencies, another of the 50th percentile, and finally another of the 10th percentile.

The following setup steps recreate the experimental data, including the embeddings and knowledge graph.

## Setup
First, run the following command to fetch the RDF embeddings and entity types from the DBpedia 2021 knowledge graph.

```bash
./setup.sh
```

Run the following scripts to generate queries and ground truth.

```bash
python gen_queries.py
python process_input_data.py
```

To gain insights into the RDF type distribution, run the following script.

```bash
python histogram.py
```

Similarly, run the following script to gain insight into the type distribution in the query set.

```bash
python query_histogram.py
```

## Running Experiments

We use two HNSW setup variants as baselines:

1. Full HNSW index that retrieves too large result sets and postfilters results according to the query filters. This is supposedly the worst performing index, since it does not consider filter information in the index construction.
2. One HNSW index per query filter. This builds the best performing HNSW indexes for each query filter.

In our experiments, we measure the performance of our adaptive HNSW in 3 dimensions: ranking quality, runtime, and memory.
Hypothetically, our adaptive HNSW index improves the ranking quality of a single HNSW index with postfiltering and improves at the same time the memory consumption from using multiple HNSW indexes to support all filter forms for filtering during vector search.
We allow our adaptive HNSW index to adapt to the query workload, which results in an acceptable increase in query runtime.

Now, construct the baseline indexes and store them on disk with the following commands.

```bash
g++ index_baselines.cpp -o index_baselines -I ../hnswlib/
./index_baselines
```

Now, build the evaluation scripts for the baselines and execute them to obtain the upper and lower performance bounds.

```bash
g++ evaluate_baselines.cpp -o evaluate_baselines -I ../hnswlib/
./evaluate_baselines
```
