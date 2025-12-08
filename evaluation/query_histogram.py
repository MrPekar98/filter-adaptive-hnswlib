import os

def merge_frequencies(frequency_dicts):
    merged = dict()

    for map in frequency_dicts:
        for key in map.keys():
            if not key in merged:
                merged[key] = 0

            merged[key] += map[key]

    return merged

def read_frequencies(dir):
    query_files = os.listdir(dir)
    freqs = dict()

    for query_file in query_files:
        with open(dir + query_file, 'r') as handle:
            lines = handle.readlines()
            types = lines[1].strip().split(' ')

            for type in types:
                if 'CareerStation' in type or ':Thing' in type:
                    continue

                elif not type in freqs:
                    freqs[type] = 0

                freqs[type] += 1

    return freqs

def print_freqs(freqs):
    keys = list(freqs.keys())

    for key in keys:
        print(key)

    print()

    for key in keys:
        print(freqs[key])

if __name__ == "__main__":
    query_dir = 'queries/single-tag/'
    homogeneous = query_dir + 'homogeneous/'
    heterogeneous = query_dir + 'heterogeneous/'

    freqs_hetero_90 = read_frequencies(heterogeneous + '90th-percentile/')
    freqs_hetero_50 = read_frequencies(heterogeneous + '50th-percentile/')
    freqs_hetero_10 = read_frequencies(heterogeneous + '10th-percentile/')
    freqs_homo_90 = read_frequencies(homogeneous + '90th-percentile/')
    freqs_homo_50 = read_frequencies(homogeneous + '50th-percentile/')
    freqs_homo_10 = read_frequencies(homogeneous + '10th-percentile/')

    print('Heterogeneous - 90th percentile')
    print_freqs(freqs_hetero_90)
    print()

    print('Heterogeneous - 50th percentile')
    print_freqs(freqs_hetero_50)
    print()

    print('Heterogeneous - 10th percentile')
    print_freqs(freqs_hetero_10)
    print()

    print('Homogeneous - 90th percentile')
    print_freqs(freqs_homo_90)
    print()

    print('Homogeneous - 50th percentile')
    print_freqs(freqs_homo_50)
    print()

    print('Homogeneous - 10th percentile')
    print_freqs(freqs_homo_10)
    print()

    freqs_hetero = merge_frequencies([freqs_hetero_90, freqs_hetero_50, freqs_hetero_10])
    freqs_homo = merge_frequencies([freqs_homo_90, freqs_homo_50, freqs_homo_10])

    print('Heterogeneous')
    print_freqs(freqs_hetero)
    print()

    print('Homogeneous')
    print_freqs(freqs_homo)
    print()

    freqs_all = merge_frequencies([freqs_hetero, freqs_homo])

    print('All queries')
    print_freqs(freqs_all)
