import time
import os
import histogram

def read_vector_entities(file):
    entities = set()

    with open(file, 'r') as handle:
        for line in handle:
            entity = line.split(' ')[0]
            entities.add(entity)

    return entities

def read_embeddings(file):
    embeddings = dict()

    with open(file, 'r') as handle:
        for line in handle:
            tokens = line.strip().split(' ')

            embeddings[tokens[0]] = list()

            for val in tokens[1:]:
                embeddings[tokens[0]].append(float(val))

    return embeddings

def types_ranked():
    freqs1 = histogram.read_frequencies('types_specific.ttl')
    freqs2 = histogram.read_frequencies('types_transitive.ttl')
    freqs = histogram.merge_frequencies(freqs1, freqs2)
    freq_pairs = list()

    for key, value in freqs.items():
        freq_pairs.append([key, value])

    return list(sorted(freq_pairs, key = lambda pair: pair[1], reverse = True))

def read_types(files):
    types = dict()

    for file in files:
        with open(file, 'r') as handle:
            for line in handle:
                split = line.split(' ')
                entity = split[0].replace('<', '').replace('>', '')
                type = split[2].replace('<', '').replace('>', '')

                if type not in types:
                    types[type] = set()

                types[type].add(entity)

    return types

def entity_types(files):
    entities = dict()

    for file in files:
        with open(file, 'r') as handle:
            for line in handle:
                split = line.split(' ')
                entity = split[0].replace('<', '').replace('>', '')
                type = split[2].replace('<', '').replace('>', '')

                if entity not in entities:
                    entities[entity] = set()

                entities[entity].add(type)

    return entities

def select_types(ranked_types, count):
    types = list()
    i = len(ranked_types) - 1

    while len(types) < count and i >= 0:
        if 'dbpedia' in ranked_types[i][0]:
            types.append(ranked_types[i])

        i -= 1

    return types

def write_queries(dir, entities, entity_types, embeddings):
    q = 1

    for entity in entities:
        with open(dir + '/q' + str(q), 'w') as handle:
            handle.write(entity + '\n')

            for type in entity_types[entity]:
                handle.write(type + ' ')

            handle.write('\n')

            for value in embeddings[entity]:
                handle.write(str(value) + ' ')

        q += 1

if __name__ == "__main__":
    start_time = time.time()
    queries = 100
    embeddings_file = 'vectors.txt'
    transitive_types_file = 'types_transitive.ttl'
    specific_types_file = 'types_specific.ttl'
    query_base_dir = 'queries/'
    sub_query_dir_single = query_base_dir + 'single-tag/'
    sub_query_dir_multi = query_base_dir + 'multi_tag/'
    homogeneous_dir = 'homogeneous/'
    heterogeneous_dir = 'heterogeneous/'

    if not os.path.exists(query_base_dir):
        os.mkdir(query_base_dir)
        os.mkdir(sub_query_dir_single)
        os.mkdir(sub_query_dir_multi)
        os.mkdir(sub_query_dir_single + homogeneous_dir)
        os.mkdir(sub_query_dir_single + heterogeneous_dir)
        os.mkdir(sub_query_dir_multi + homogeneous_dir)
        os.mkdir(sub_query_dir_multi + heterogeneous_dir)

    print('Reading entities and their types')

    entity_types = entity_types([transitive_types_file, specific_types_file])
    types = read_types([transitive_types_file, specific_types_file])
    print('Finding types based on popularity')

    type_ranking = types_ranked()
    popular_types = type_ranking[0:int(len(type_ranking) * 0.1)]	# 90th percentile
    semi_popular_types = type_ranking[0:int(len(type_ranking) * 0.5)]	# 50th percentile
    unpopular_types = type_ranking[0:int(len(type_ranking) * 0.9)]	# 10th percentile
    print('Reading embedding entities and selecting query entities')

    embeddings = read_embeddings(embeddings_file)
    embedding_entities = set(embeddings.keys())
    popular_query_types = [pair[0] for pair in select_types(popular_types, queries)]
    semi_popular_query_types = [pair[0] for pair in select_types(semi_popular_types, queries)]
    unpopular_query_types = [pair[0] for pair in select_types(unpopular_types, queries)]

    heterogeneous_90_percentile = list()
    heterogeneous_50_percentile = list()
    heterogeneous_10_percentile = list()

    os.mkdir(heterogeneous_dir + '90th-percentile/')

    for type in popular_query_types:
        entity = list(types[type])[0]
        heterogeneous_90_percentile.append(entity)

    os.mkdir(heterogeneous_dir + '50th-percentile/')

    for type in semi_popular_query_types:
        entity = list(types[type])[0]
        heterogeneous_50_percentile.append(entity)

    os.mkdir(heterogeneous_dir + '10th-percentile/')

    for type in unpopular_query_types:
        entity = list(types[type])[0]
        heterogeneous_10_percentile.append(entity)

    homogeneous_90_percentile = list()
    homogeneous_50_percentile = list()
    homogeneous_10_percentile = list()

    os.mkdir(homogeneous_dir + '90th-percentile/')

    type = popular_query_types[0]
    homogeneous_90_percentile = list(types[type])[0:queries]

    os.mkdir(homogeneous_dir + '50th-percentile/')

    type = semi_popular_query_types[0]
    homogeneous_50_percentile = list(types[type])[0:queries]

    os.mkdir(homogeneous_dir + '10th-percentile/')

    type = unpopular_query_types[-1]
    homogeneous_10_percentile = list(types[type])[0:queries]

    q = 1
    print('Writing queries')

    write_queries(sub_query_dir_multi + heterogeneous_dir + '90th-percentile/', heterogeneous_90_percentile, entity_types, embeddings)
    write_queries(sub_query_dir_multi + heterogeneous_dir + '50th-percentile/', heterogeneous_50_percentile, entity_types, embeddings)
    write_queries(sub_query_dir_multi + heterogeneous_dir + '10th-percentile/', heterogeneous_10_percentile, entity_types, embeddings)
    write_queries(sub_query_dir_multi + homogeneous_dir + '90th-percentile/', homogeneous_90_percentile, entity_types, embeddings)
    write_queries(sub_query_dir_multi + homogeneous_dir + '50th-percentile/', homogeneous_50_percentile, entity_types, embeddings)
    write_queries(sub_query_dir_multi + homogeneous_dir + '10th-percentile/', homogeneous_10_percentile, entity_types, embeddings)

    duration = ((time.time() - start_time) / 60) / 60
    print('Query generation took ' + str(duration) + ' hours')
