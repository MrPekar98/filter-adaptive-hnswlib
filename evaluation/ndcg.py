import os
import numpy as np
from sklearn.metrics import ndcg_score

def read_mappings(file):
    map = dict()

    with open(file, 'r') as handle:
        for line in handle:
            split = line.strip().split('=')
            id = int(split[0])
            uri = split[1]
            map[id] = uri

    return map

def read_ranking(file):
    ranking = list()

    with open(file, 'r') as handle:
        for line in handle:
            split = line.strip().split('=')
            id = int(split[0])
            distance = float(split[1])
            ranking.append((id, distance))

    return ranking

def ndcg(gt, prediction, mappings):
    node_ids = list(mappings.keys())
    gt_scores = {id:0 for id in node_ids}
    prediction_scores = {id:0 for id in node_ids}
    k = 0
    count = 0

    for item in gt:
        score = 1 / (1 + (-1 * item[1]))
        gt_scores[item[0]] = score
        k += 1

    for item in prediction:
        if count >= k:
            break

        score = 1 / (1 + (-1 * item[1]))
        prediction_scores[item[0]] = score
        count += 1

    gt_ranking = np.array([[gt_scores[id] for id in node_ids]])
    prediction_ranking = np.array([[prediction_scores[id] for id in node_ids]])
    return ndcg_score(gt_ranking, prediction_ranking, k = k)

# The multi-filter baseline is treated as ground truth
if __name__ == "__main__":
    result_dir = 'results/'
    postfilter_result_dir = result_dir + 'postfilter_baseline/'
    adaptive_result_dir = result_dir + 'adaptive/'
    multifilter_result_dir = result_dir + 'multifilter_baseline/'
    mappings = read_mappings('indexes/baseline_mappings.txt')

    for query_type in os.listdir(multifilter_result_dir):
        path = multifilter_result_dir + query_type + '/'
        print(query_type)
        print('Postfilter baseline')

        for query in os.listdir(path):
            query_file = path + query
            postfilter_query_file = postfilter_result_dir + query_type + '/' + query
            multifilter_ranking = read_ranking(query_file)
            postfilter_ranking = read_ranking(postfilter_query_file)
            postfilter_ndcg = ndcg(multifilter_ranking, postfilter_ranking, mappings)
            print(str(postfilter_ndcg).replace('.', ','))

        print('Adaptive baseline')

        for query in os.listdir(path):
            query_file = path + query
            adaptive_query_file = adaptive_result_dir + query_type + '/' + query
            multifilter_ranking = read_ranking(query_file)
            adaptive_ranking = read_ranking(adaptive_query_file)
            adaptive_ndcg = ndcg(multifilter_ranking, adaptive_ranking, mappings)
            print(str(adaptive_ndcg).replace('.', ','))

        print()
