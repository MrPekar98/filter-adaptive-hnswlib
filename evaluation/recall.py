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

if __name__ == "__main__":
    top_k = 100
    result_dir = 'results/adaptive/'
    gt_dir = 'results/multifilter_baseline/'
    query_categories = os.listdir(result_dir)

    for category in query_categories:
        queries = os.listdir(result_dir + category)
        print(category.replace('_', ' '))

        for query in queries:
            results = read_results(result_dir + category + '/' + query, k = top_k)
            gt = read_results(gt_dir + category + '/' + query)
            score = recall(results, gt)
            print(str(score).replace('.', ','))

        print()
