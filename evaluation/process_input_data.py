import gen_queries as data

if __name__ == "__main__":
    kg_files = ['types_transitive.ttl', 'types_specific.ttl']
    print('Indexing entity types')

    types = data.entity_types(kg_files)
    print('Processing data')

    with open('data.txt', 'w') as out_handle:
        with open('vectors.txt', 'r') as in_handle:
            for line in in_handle:
                split = line.strip().split(' ')
                uri = split[0]
                entity_types = []

                if uri in types:
                    entity_types = types[uri]

                out_handle.write(uri + '\n')

                for type in entity_types:
                    out_handle.write(type + ' ')

                out_handle.write('\n')

                for val in split[1:]:
                    out_handle.write(val + ' ')

                out_handle.write('\n')
