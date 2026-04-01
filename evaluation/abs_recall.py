import os

def recall(results, gt):
    correct = 0

    for result in results:
        if result in gt:
            correct += 1

    return float(correct) / len(gt)

def read_results(filename, k = None):
    results = list()
    count = 0

    with open(filename, 'r') as file:
        for line in file:
            id = int(line.split('=')[0])
            results.append(id)
            count += 1

            if not k is None and count == k:
                break

    return results

def read_gt(filename, inverted_mapping, k = None):
    gt = list()
    count = 0

    with open(filename, 'r') as file:
        for line in file:
            uri = line.strip()[:line.rfind(',')]
            id = inverted_mapping[uri]
            gt.append(id)
            count += 1

            if not k is None and count == k:
                break

    return gt

def read_mapping(filename):
    inverted_map = dict()

    with open(filename, 'r') as handle:
        for line in handle:
            split = line.strip().split('=')
            id = int(split[0])
            uri = split[1]
            inverted_map[uri] = id

    return inverted_map

if __name__ == "__main__":
    top_k = 100
    result_dir = 'results/adaptive/'
    gt_dir = 'ground_truth/single-tag/'
    inverted_index = read_mapping('indexes/baseline_mappings.txt')
    query_categories = os.listdir(result_dir)

    for category in query_categories:
        queries = os.listdir(result_dir + category)
        diversity = category.split('_')[0] + 'geneous'
        percentile = category.split('_')[1]
        print(category.replace('_', ' '))

        for query in queries:
            results = read_results(result_dir + category + '/' + query, k = top_k)
            gt = read_gt(gt_dir + diversity + '/' + percentile + '/' + query, inverted_index, k = top_k)
            score = recall(results, gt)
            print(str(score).replace('.', ','))

        print()
