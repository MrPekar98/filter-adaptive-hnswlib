import os
import math

if __name__ == "__main__":
    result_dir = 'results/ung/ung_gt/'
    query_dir = 'data/queries/single-tag/'
    data_type = 'float'
    dist_fn = 'L2'
    k = 100
    data_dir = 'data/dataset_full/'
    base_bin_file = data_dir + 'data.bin'
    base_label_file = data_dir + 'labels.txt'
    gt_file = 'ung_gt.bin'
    index_dir = 'indexes/ung_multi_index/'
    scenario = 'overlap'
    entrypoints = 16
    l_search = '1000 2000 3000'

    if not os.path.exists(result_dir):
        os.mkdir(result_dir)

    q_diversities = os.listdir(query_dir)

    for q_diversity in q_diversities:
        q_diversity_dir = query_dir + q_diversity + '/'
        result_diversity_dir = result_dir + q_diversity + '/'
        q_percentiles = os.listdir(q_diversity_dir)

        if not os.path.exists(result_diversity_dir):
            os.mkdir(result_diversity_dir)

        for q_percentile in q_percentiles:
            q_percentile_dir = q_diversity_dir + q_percentile + '/'
            result_percentile_dir = result_diversity_dir + q_percentile + '/'
            queries = os.listdir(q_percentile_dir)

            if not os.path.exists(result_percentile_dir):
                os.mkdir(result_percentile_dir)

            for query in queries:
                if query.endswith('bin'):
                    binary_file = q_percentile_dir + '/' + query
                    label_file = binary_file.replace('.bin', '_labels.txt')
                    query_result_dir =  result_percentile_dir + query.replace('.bin', '/')
                    labels = list()

                    if not os.path.exists(query_result_dir):
                        os.mkdir(query_result_dir)

                    with open(label_file, 'r') as handle:
                        label_line = handle.readlines()[0].strip()
                        labels = label_line.split(',')

                    intermediate_k = int(math.ceil(float(k) / len(labels)))

                    for label in labels:
                        index_file = index_dir + 'label_' + label + '/'
                        label_result_dir = query_result_dir + label + '/'

                        if not os.path.exists(label_result_dir):
                            os.mkdir(label_result_dir)

                        command = './Unified-Navigating-Graph/build/apps/search_UNG_index --data_type ' + data_type + ' --dist_fn ' + dist_fn + ' --num_threads 4 --K ' + str(intermediate_k) + ' --base_bin_file ' + base_bin_file + ' --base_label_file ' + base_label_file + ' --query_bin_file ' + binary_file + ' --query_label_file ' + label_file + ' --gt_file ' + gt_file + ' --index_path_prefix ' + index_file + ' --result_path_prefix ' + label_result_dir + ' --scenario ' + scenario + ' --num_entry_points ' + str(entrypoints) + ' --Lsearch ' + l_search
                        os.system(command)

                    print()

            print()
