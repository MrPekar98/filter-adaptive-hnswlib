import os
import subprocess
from pathlib import Path

if __name__ == "__main__":
    threads = 4
    max_degree = 32
    alpha = 1.2
    l_build = 100
    dist_function = 'L2'
    cross_edges = 6
    scenario = 'general'

    query_dir = 'queries/multi-tag/'
    data_dir = 'data/dataset_full/'
    new_data_dir = data_dir + 'multi_index_baseline_dataset/'
    index_dir = "indexes/ung_multi_index/"
    data_files = Path(new_data_dir).glob('*.bin')

    if not os.path.exists(index_dir):
        os.mkdir(index_dir)

    for file in data_files:
        bin_file = str(file)
        label_file = bin_file.replace('.bin', '.txt')
        label_id = label_file.split('/')[-1].replace('.txt', '')
        os.mkdir(index_dir + label_id)
        command = './Unified-Navigating-Graph/build/apps/build_UNG_index --data_type float --dist_fn ' + dist_function + ' --num_threads ' + str(threads) + ' --max_degree ' + str(max_degree) + ' --Lbuild ' + str(l_build) + ' --alpha ' + str(alpha) + ' --base_bin_file ' + bin_file + ' --base_label_file ' + label_file + ' --index_path_prefix ' + index_dir + label_id + '/ --scenario ' + scenario + ' --num_cross_edges ' + str(cross_edges)
        os.system(command)
