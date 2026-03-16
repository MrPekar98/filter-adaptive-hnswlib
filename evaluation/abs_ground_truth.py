
import os
import math
from threading import Thread, Lock
from time import sleep

lock = Lock()
tasks = list() # Contains pool of entities to process by the threads
loaded = 0
preparation_complete = False

class Entity:
    def __init__(self, uri, tags, vector):
        self.__uri = uri
        self.__tags = tags
        self.__vector = vector

    def get_uri(self):
        return self.__uri

    def get_tags(self):
        return self.__tags

    def get_vector(self):
        return self.__vector

def collect_queries(query_dir):
    diversities = os.listdir(query_dir)
    paths = list()

    for diversity in diversities:
        frequencies = os.listdir(query_dir + '/' + diversity)

        for frequency in frequencies:
            queries = os.listdir(query_dir + '/' + diversity + '/' + frequency)
            paths.extend([query_dir + '/' + diversity + '/' + frequency + '/' + query for query in queries])

    return paths

def mk_gt_dirs(gt_dir, query_dir):
    diversities = os.listdir(query_dir)

    for diversity in diversities:
        if not os.path.exists(gt_dir + diversity):
            os.mkdir(gt_dir + diversity)

        frequencies = os.listdir(query_dir + diversity)

        for frequency in frequencies:
            if not os.path.exists(gt_dir + diversity + '/' + frequency):
                os.mkdir(gt_dir + diversity + '/' + frequency)

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

    return dot / (dist1 + dist2)

def read_query(query_file):
    with open(query_file, 'r') as handle:
        lines = handle.readlines()
        uri = lines[0].strip()
        tags = list(lines[1].strip().split(' '))
        vector = [float(val) for val in lines[2].strip().split(' ')]
        return Entity(uri, tags, vector)

def worker(queries, gt_dir):
    global lock
    global tasks
    global loaded
    global preparation_complete
    chunk = 100

    while len(tasks) > 0 or not preparation_complete:
        if len(tasks) > 0:
            lock.acquire()
            local_tasks = tasks[0:chunk]
            del tasks[0:chunk]
            lock.release()

            for task in local_tasks:
                for query in queries:
                    has_overlap = False
                    query_entity = read_query(query)

                    for query_tag in query_entity.get_tags():
                        for tag in task.get_tags():
                            if query_tag == tag:
                                has_overlap = True
                                break

                        if has_overlap:
                            break

                    if has_overlap:
                        dist = distance(query_entity.get_vector(), task.get_vector())
                        gt_file = gt_dir
                        directories = query.split('/')

                        for directory in directories[2:]:
                            gt_file = gt_file + '/' + directory

                        with open(gt_file, 'a') as handle:
                            handle.write(task.get_uri() + ',' + str(dist) + '\n')

                lock.acquire()
                loaded += 1
                lock.release()

def watch(dataset_size):
    global loaded
    global tasks
    global preparation_complete

    while len(tasks) > 0 or not preparation_complete:
        print(' ' * 100, end = '\r')
        print('Constructed: ' + str((loaded / dataset_size) * 100)[0:5] + '%', end = '\r')
        sleep(1)

if __name__ == "__main__":
    #data_file = 'data.txt'
    data_file = 'data_small.txt'
    query_dir = 'queries/single-tag/'
    gt_dir = 'ground_truth/' + query_dir.split('/')[1] + '/'
    #dataset_size = 32440681
    dataset_size = 6966496
    loaded = 1
    step = 1
    threads = 4
    thread_pool = list()
    queries = collect_queries(query_dir)
    task_buffer = list()
    buffer_capacity = 100

    if not os.path.exists(gt_dir):
        os.makedirs(gt_dir)

    mk_gt_dirs(gt_dir, query_dir)

    for i in range(threads):
        t = Thread(target = worker, args = (queries, gt_dir))
        t.start()
        thread_pool.append(t)

    watcher = Thread(target = watch, args = (dataset_size,))
    watcher.start()

    with open(data_file, 'r') as handle:
        uri = None
        tags = list()

        for line in handle:
            if step == 1:
                uri = line.strip()

            elif step == 2:
                tags = list(line.strip().split(' '))

            elif step == 3:
                vector = [float(val) for val in line.strip().split(' ')]
                entity = Entity(uri, tags, vector)
                task_buffer.append(entity)

                if len(task_buffer) >= buffer_capacity:
                    lock.acquire()
                    tasks.extend(task_buffer)
                    lock.release()
                    task_buffer = list()

                uri = None
                tags = list()

            step = (step % 3) + 1

    watcher.join()
