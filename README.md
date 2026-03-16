# Filtered Adaptive HNSW for Approximate Vector Search
This is a modification of the hnswlib repository.
Consult the official <a href="https://github.com/nmslib/hnswlib/blob/master/README.md">README</a> for the official instructions.

## Interface Changes
### Pre-Loading Node Tags for Query Filtering
Before adding points to the index, call the function `addNodeTags()` for all nodes in the dataset.
Once complete, call `flushTags()` before adding any points via `addPoint()`.