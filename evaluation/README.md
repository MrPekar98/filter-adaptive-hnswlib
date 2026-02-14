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

### Setup of Baseline
#### Unified Navigating Graph
We setup <a href="https://github.com/YZ-Cai/Unified-Navigating-Graph">UNG</a> as our baseline for filtered vector search.
First, install the necessary tools.

```bash
apt install cmake build-essential git libboost-all-dev libomp-dev libmkl-avx2 -y
```

Clone and build the project.

```bash
git clone https://github.com/YZ-Cai/Unified-Navigating-Graph.git
cd Unified-Navigating-Graph/
mkdir build/
cd build/
cmake -DCMAKE_BUILD_TYPE=Release ../codes/
make -j
cd ..
```

#### ACORN

We setup <a href="https://github.com/guestrin-lab/ACORN/tree/main">ACORN</a> as our baseline for filtered vector search.
First, install the necessary tools.

```bash
apt install cmake build-essential git zlib1g-dev libopenblas-dev libomp-dev -y
```

Then, build ACORN.

```bash
git clone https://github.com/guestrin-lab/ACORN.git
cd ACORN/
cmake -DFAISS_ENABLE_GPU=OFF -DFAISS_ENABLE_PYTHON=OFF -DBUILD_TESTING=ON -DBUILD_SHARED_LIBS=ON -DCMAKE_BUILD_TYPE=Release -B build
make -C build -j faiss
cd ../
```

## Running Experiments

We use two HNSW setup variants as baselines, excluding Rii:

1. Full HNSW index that retrieves too large result sets and postfilters results according to the query filters. This is supposedly the worst performing index, since it does not consider filter information in the index construction.
2. One HNSW index per query filter. This builds the best performing HNSW indexes for each query filter.
3. Our adaptive HNSW index, whose ranking, memory, and runtime performance should be in between the two previous baselines.

In our experiments, we measure the performance of our adaptive HNSW in 3 dimensions: ranking quality, runtime, and memory.
Hypothetically, our adaptive HNSW index improves the ranking quality of a single HNSW index with postfiltering and improves at the same time the memory consumption from using multiple HNSW indexes to support all filter forms for filtering during vector search.
We allow our adaptive HNSW index to adapt to the query workload, which results in an acceptable increase in query runtime.

Now, construct the baseline indexes and store them on disk with the following commands.

```bash
g++ index_baselines.cpp -o index_baselines -I ../hnswlib/
./index_baselines
```

Now, build the evaluation scripts for the baselines and execute them.

```bash
g++ evaluate_baselines.cpp -o evaluate_baselines -I ../hnswlib/
./evaluate_baselines
```

The evaluation script outputs intermediate and final runtimes.
When all three baselines have been evaluated, we can evaluate the ranking using NDCG.

```bash
python ndcg.py
```

We can also evaluate recall.

```bash
python recall.py
```

### Evaluating External Baselines
#### Unified Navigating Graph
As previously mentioned, we also evaluate <a href="https://github.com/YZ-Cai/Unified-Navigating-Graph">UNG</a> as our external baseline for filtered vector search.
First, we setup the data.

```bash
mkdir -p data/dataset_full/
g++ -std=c++17 -o data_to_bin data_to_bin.cpp
./data_to_bin
```

Build the index.

```bash
mkdir indexes/ung/
git clone https://github.com/YZ-Cai/Unified-Navigating-Graph.git
mkdir Unified-Navigating-Graph/build/
cd Unified-Navigating-Graph/build/
make -j
cd ../../
./Unified-Navigating-Graph/build/apps/build_UNG_index \
    --data_type float --dist_fn L2 --num_threads 4 --max_degree 32 --Lbuild 100 --alpha 1.2 \
    --base_bin_file data/dataset_full/data.bin --base_label_file data/dataset_full/labels.txt \
    --index_path_prefix indexed/ung/ --scenario general --num_cross_edges 6
```

Now, perform filtered vector search.

```bash
./search_ung.sh
```

The results can then be found in `results/ung/`.

Similar as previously, we construct a ground truth set by constructing UNG indexes for each query filter.
We then compare the results from UNG to the UNG-version that has indexes for each query filter.
First, construct the indexes.

```bash
g++ -std=c++17 -o prepare_ung_grund_truth prepare_ung_grund_truth.cpp
./prepare_ung_grund_truth
```

Then, construct the indexes.

```bash
python construct_multi_index_ung.py
```

Finally, run the queries.

```bash
python search_gt_ung.py
```

You can now evaluate recall of UNG.

```bash
python recall_ung.py
```

#### ACORN

As previously mentioned, we also evaluate <a href="https://github.com/guestrin-lab/ACORN/tree/main">ACORN</a> as our external baseline for filtered vector search.
First, we setup the data.

```bash
mkdir data/
g++ -o gen_fvecs gen_fvecs.cpp
./gen_fvecs
```

Build the indexer and start indexing.

```bash
mkdir indexes/acorn/
cmake -B build .
cmake --build build -j
mv build/index_acorn .
./index_acorn
```
