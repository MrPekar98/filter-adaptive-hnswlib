import os
import math

def recall(predicted, gt):
    correct = 0

    for result in predicted:
        if result in gt:
            correct += 1

    return float(correct) / len(gt)

def main_id_index(file):
    count = 0
    index = dict()

    with open(file, 'r') as handle:
        for line in handle:
            index[line.strip()] = count
            count += 1

    return index

def secondary_id_index(file):
    count = 0
    index = dict()

    with open(file, 'r') as handle:
        for line in handle:
            index[str(count)] = line.strip()
            count += 1

    return index

if __name__ == "__main__":
    result_dir = 'results/ung/ung/single-tag/'
    gt_dir = 'results/ung/ung_gt/'
    data_dir = 'data/dataset_small/multi_index_baseline_dataset/'
    k = 100
    types = os.listdir(result_dir)
    print('Reading data')

    postings_list = main_id_index('data/dataset/uri_mapping.txt')
    print()

    for type in types:
        type_dir = result_dir + type + '/'
        percentiles = os.listdir(type_dir)
        print(type)

        for percentile in percentiles:
            percentile_dir = type_dir + percentile + '/'
            queries = os.listdir(percentile_dir)
            print(percentile)

            for query in queries:
                if 'L1000' in query:
                    is_header = True
                    results = []
                    gt = []
                    count = 0

                    with open(percentile_dir + query, 'r') as handle:
                        for line in handle:
                            if is_header:
                                is_header = False
                                continue

                            for id in line.strip().split(',')[1].split(' '):
                                results.append(int(id))
                                count += 1

                                if count == k:
                                    break

                    gt_results = gt_dir + type + '/' + percentile + '/' + query.split('result')[0] + '/'
                    label_results = os.listdir(gt_results)
                    intermediate_k = int(math.ceil(float(k) / len(label_results)))

                    for label in label_results:
                        count = 0
                        label_uri_mapping_file = data_dir + 'label_' + label + '_uri_mapping.txt'
                        label_uri_index = secondary_id_index(label_uri_mapping_file)

                        with open(gt_results + label + '/result_L1000.csv', 'r') as handle:
                            is_header = True

                            for line in handle:
                                if is_header:
                                    is_header = False
                                    continue

                                for id in line.strip().split(',')[1].split(' '):
                                    if not id in label_uri_index:
                                        continue

                                    uri = label_uri_index[id]
                                    global_id = postings_list[uri]
                                    gt.append(global_id)
                                    count += 1

                                    if count == intermediate_k:
                                        break

                    score = recall(results, gt)
                    print(str(score).replace('.', ','))

            print()

        print()
