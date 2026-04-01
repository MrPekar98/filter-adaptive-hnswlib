import os
import numpy as np
from sklearn.metrics import ndcg_score

def read_mappings(file):
    map = dict()
    inverted_map = dict()

    with open(file, 'r') as handle:
        for line in handle:
            split = line.strip().split('=')
            id = int(split[0])
            uri = split[1]
            map[id] = uri
            inverted_map[uri] = id

    return (map, inverted_map)

def read_ranking(file, k = None):
    ranking = list()

    with open(file, 'r') as handle:
        count = 0

        for line in handle:
            split = line.strip().split('=')
            id = int(split[0])
            distance = float(split[1])
            ranking.append((id, distance))
            count += 1

            if not k is None and count == k:
                break

    return ranking

def read_gt_ranking(file, inverted_mapping, k = None):
    ranking = list()

    with open(file, 'r') as handle:
        count = 0

        for line in handle:
            separator = line.rfind(',')
            uri = line[:separator]
            id = inverted_mapping[uri]
            distance = float(line.strip()[separator + 1:])
            ranking.append((id, distance))
            count += 1

            if not k is None and count == k:
                break

    return ranking

def ndcg(gt, prediction, mappings):
    node_ids = list(mappings.keys())
    gt_scores = {id:0 for id in node_ids}
    prediction_scores = {id:0 for id in node_ids}
    k = 0
    count = 0

    for item in gt:
        score = 1 / (1 + item[1])
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
    uri_index = mappings[0]
    inverted_index = mappings[1]
    gt_dir = 'ground_truth/single-tag/'
    top_k = 100

    for query_type in os.listdir(adaptive_result_dir):
        path = adaptive_result_dir + query_type + '/'
        print(query_type)
        print('Postfilter baseline')

        for query in os.listdir(path):
            gt_file = gt_dir + query_type.split('_')[0] + 'geneous/' + query_type.split('_')[1] + '/' + query
            postfilter_query_file = postfilter_result_dir + query_type + '/' + query
            gt_ranking = read_gt_ranking(gt_file, inverted_index, top_k)
            postfilter_ranking = read_ranking(postfilter_query_file, top_k)
            postfilter_ndcg = ndcg(gt_ranking, postfilter_ranking, uri_index)
            print(str(postfilter_ndcg).replace('.', ','))

        print('Multifilter baseline')

        for query in os.listdir(path):
            gt_file = gt_dir + query_type.split('_')[0] + 'geneous/' + query_type.split('_')[1] + '/' + query
            multifilter_query_file = multifilter_result_dir + query_type.split('_')[1] + '/' + query
            gt_ranking = read_gt_ranking(gt_file, inverted_index, top_k)
            multifilter_ranking = read_ranking(multifilter_query_file, top_k)
            multifilter_ndcg = ndcg(gt_ranking, multifilter_ranking, uri_index)
            print(str(multifilter_ranking).replace('.', ','))

        print('Adaptive baseline')

        for query in os.listdir(path):
            gt_file = gt_dir + query_type.split('_')[0] + 'geneous/' + query_type.split('_')[1] + '/' + query
            adaptive_query_file = adaptive_result_dir + query_type + '/' + query
            gt_ranking = read_gt_ranking(gt_file, inverted_index, top_k)
            adaptive_ranking = read_ranking(adaptive_query_file, top_k)
            adaptive_ndcg = ndcg(gt_ranking, adaptive_ranking, uri_index)
            print(str(adaptive_ndcg).replace('.', ','))

        print()
