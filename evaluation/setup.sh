#!/bin/bash

set -e

echo "Fetching embeddings"
wget https://zenodo.org/records/6384728/files/embeddings.zip?download=1 -O embeddings.zip
unzip embeddings.zip
rm embeddings.zip

echo "Fetching DBpedia RDF types"
wget https://downloads.dbpedia.org/repo/dbpedia/mappings/instance-types/2021.09.01/instance-types_lang=en_specific.ttl.bz2 -O types_specific.ttl.bz2
wget https://downloads.dbpedia.org/repo/dbpedia/mappings/instance-types/2021.09.01/instance-types_lang=en_transitive.ttl.bz2 -O types_transitive.ttl.bz2
bzip2 -d types_specific.ttl.bz2
bzip2 -d types_transitive.ttl.bz2
