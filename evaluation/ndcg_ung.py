import os
import math
import numpy as np
from sklearn.metrics import ndcg_score

def distance(vec1, vec2):
    dot = 0
    dist1 = 0
    dist2 = 0

    for i in range(len(vec1)):
        dot += vec1[i] * vec2[i]
        dist1 += math.pow(vec1[i], 2)
        dist2 += math.pow(vec2[i], 2)

    dist1 = math.sqrt(dist1)
    dist2 = math.sqrt(dist2)

    return dot / (dist1 * dist2)

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

def read_vector_index(file):
    index = dict()

    with open(file, 'r') as handle:
        step = 1
        uri = None

        for line in handle:
            if step == 1:
                uri = line.strip()

            elif step == 3:
                vector = [float(val) for val in line.strip().split(' ')]
                index[uri] = vector

            step = (step % 3) + 1

    return index

if __name__ == "__main__":
    print('Reading data')

    top_k = 100
    query_dir = 'queries/single-tag/'
    vectors_file = 'data_small.txt'
    result_dir = 'results/ung/ung/single-tag/'
    gt_dir = 'results/ung/ung_gt/'
    mapping_file = 'data/dataset_small/uri_mapping.txt'
    data_dir = 'data/dataset_small/multi_index_baseline_dataset/'
    types = os.listdir(result_dir)
    postings_list = main_id_index('data/dataset_small/uri_mapping.txt')
    id_index = secondary_id_index('data/dataset_small/uri_mapping.txt')
    vector_index = read_vector_index(vectors_file)

    for type in types:
        percentiles = os.listdir(result_dir + type)
        print(type)

        for percentile in percentiles:
            queries = os.listdir(result_dir + type + '/' + percentile)
            print(percentile)

            for query in queries:
                if not 'result_L1000' in query:
                    continue

                result_path = result_dir + type + '/' + percentile + '/' + query
                results = set()
                gt = set()

                with open(result_path, 'r') as handle:
                    tmp_results = list(handle.readlines()[1].split(',')[1].split(' '))
                    count = 0

                    for result in tmp_results:
                        results.add(result)
                        count += 1

                        if count == top_k:
                            break

                gt_path = gt_dir + type + '/' + percentile + '/' + query.split('result')[0] + '/'
                indexes = os.listdir(gt_path)
                intermediate_k = int(math.ceil(float(top_k) / len(indexes)))

                for index in indexes:
                    label_uri_mapping_file = data_dir + 'label_' + index + '_uri_mapping.txt'
                    label_uri_index = secondary_id_index(label_uri_mapping_file)

                    with open(gt_path + index + '/result_L1000.csv', 'r') as handle:
                        sub_gt = list(handle.readlines()[1].split(',')[1].split(' '))
                        count = 0

                        for result in sub_gt:
                            if not result in label_uri_index:
                                continue

                            uri = label_uri_index[result]
                            global_id = postings_list[uri]
                            gt.add(global_id)
                            count += 1

                            if count == intermediate_k:
                                break

                predicted_scores = [0.0 for key in postings_list.keys()]
                gt_scores = [0.0 for key in postings_list.keys()]
                query_id = query.split('result')[0]
                query_file = query_dir + type + '/' + percentile + '/' + query_id
                query_vector = []

                with open(query_file, 'r') as handle:
                    query_vector = [float(val) for val in handle.readlines()[2].strip().split(' ')]

                for id in results:
                    if int(id) > 40000000:
                        continue

                    uri = id_index[id]
                    vector = vector_index[uri]
                    dist_score = 1 / (1 + distance(vector, query_vector))
                    predicted_scores[int(id)] = dist_score

                for id in gt:
                    uri = id_index[str(id)]
                    vector = vector_index[uri]
                    dist_score = 1 / (1 + distance(vector, query_vector))
                    gt_scores[int(id)] = dist_score

                ndcg = ndcg_score(np.array([gt_scores]), np.array([predicted_scores]), k = top_k)
                print(str(ndcg).replace('.', ','))

            print()

        print()
