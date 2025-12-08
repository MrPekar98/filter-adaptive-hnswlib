def read_frequencies(file):
    freqs = dict()

    with open(file, 'r') as handle:
        for line in handle:
            object = line.split(' ')[2].replace('<', '').replace('>', '')

            if 'CareerStation' in object or ':Thing' in object:
                continue

            elif object not in freqs:
                freqs[object] = 0

            freqs[object] += 1

        return freqs

def merge_frequencies(freqs1, freqs2):
    merged = freqs1

    for key in freqs2.keys():
        if key in merged:
            merged[key] += freqs2[key]

        else:
            merged[key] = freqs2[key]

    return merged

if __name__ == "__main__":
    file1 = 'types_specific.ttl'
    file2 = 'types_transitive.ttl'
    print('Reading frequencies')

    freqs1 = read_frequencies(file1)
    freqs2 = read_frequencies(file2)
    print('Processing frecuencies')

    merged = merge_frequencies(freqs1, freqs2)
    print('Done\n')

    for key in freqs1.keys():
        print(key)

    print()

    for key in freqs1.keys():
        print(freqs1[key])
