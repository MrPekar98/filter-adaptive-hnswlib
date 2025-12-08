import os
import math

class Record:
    def __init__(self, uri, tags, embedding):
        self.uri = uri
        self.tags = tags
        self.embedding = embedding

def read_data(file):
    with open(file, 'r') as handle:
        step = 1
        uri = None
        tags = set()
        embedding = list()
        records = list()

        for line in handle:
            if step == 1:
                uri = line.strip()

            elif step == 2:
                for tag in line.strip().split(' '):
                    tags.add(tag)

            elif step == 3:
                for val in line.strip().split(' '):
                    embedding.append(float(val))

                records.append(Record(uri, tags, embedding))
                uri = None
                tags = set()
                embedding = list()

            step = (step % 3) + 1

        return records

def read_query(file):
    with open(file, 'r') as handle:
        lines = handle.readlines()
        uri = lines[0].strip()
        tags = set(lines[1].strip().split(' '))
        embedding = [float(val) for val in lines[2].strip().split(' ')]

        return Record(uri, tags, embedding)

def contains_overlap(query_tags, record_tags):
    for tag in query_tags:
        if tag in record_tags:
            return True

    return False

def similarity(vec1, vec2):
    euc = 0.0

    for i in range(len(vec1)):
        euc += pow(vec1[i] - vec2[i], 2)

    return math.sqrt(euc)

if __name__ == "__main__":
    dir = 'ground_truth/'
    query_dir = 'queries/'
    data_file = 'data.txt'
    data = read_data(data_file)
    query_files = os.listdir(query_dir)
    progress = 1
    print('Constructing ground truth')

    if not os.path.exists(dir):
        os.mkdir(dir)

    for query_file in query_files:
        query = read_query(query_dir + query_file)
        ranking = list()

        for record in data:
            if contains_overlap(query.tags, record.tags):
                sim = similarity(query.embedding, record.embedding)
                ranking.append([record.uri, sim])

        ranking.sort(key = lambda tuple: tuple[1])

        with open(dir + query_file, 'w') as handle:
            for result in ranking:
                handle.write(result[0] + ',' + str(result[1]) + '\n')

        print(' ' * 100, end = '\r')
        print(str(progress) + '/' + str(len(query_files)), end = '\r')
        progress += 1
