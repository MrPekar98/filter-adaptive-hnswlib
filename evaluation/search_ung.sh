#!/bin/bash

set -e

RESULT_DIR="results/ung/ung/"
DATA_TYPE="float"
DIST_FN="L2"
K=1000
DATA_DIR="data/dataset_full/"
BASE_BIN_FILE="${DATA_DIR}data.bin"
BASE_LABEL_FILE="${DATA_DIR}labels.txt"
QUERY_DIR="data/queries/single-tag/"
GT_FILE="ung_gt.bin"
INDEX_DIR="indexes/ung/"
SCENARIO="overlap"
ENTRYPOINTS=16
L_SEARCH="1000 2000 3000"

mkdir -p ${RESULT_DIR}
echo "Evaluating all queries"

for Q_TYPE in ${QUERY_DIR}* ;\
do
    for Q_PERCENTILE in ${Q_TYPE}/* ;\
    do
        for QUERY in ${Q_PERCENTILE}/*.bin ;\
        do
            Q_ID=${QUERY:0:-4}
            Q_LABEL="${Q_ID}_labels.txt"
            Q_RESULT_DIR="${RESULT_DIR}${Q_ID:13:100}"
            mkdir -p ${Q_ID}

            ./Unified-Navigating-Graph/build/apps/search_UNG_index \
                --data_type ${DATA_TYPE} \
                --dist_fn ${DIST_FN} \
                --num_threads 4 \
                --K ${K} \
                --base_bin_file ${BASE_BIN_FILE} \
                --base_label_file ${BASE_LABEL_FILE} \
                --query_bin_file data/queries/single-tag/heterogeneous/50th-percentile/q36.bin \
                --query_label_file data/queries/single-tag/heterogeneous/50th-percentile/q36_labels.txt \
                --gt_file ${GT_FILE} \
                --index_path_prefix ${INDEX_DIR} \
                --result_path_prefix ${Q_RESULT_DIR} \
                --scenario ${SCENARIO} \
                --num_entry_points ${ENTRYPOINTS} \
                --Lsearch ${L_SEARCH}

            echo
        done
    done
done

echo
echo "Done"
