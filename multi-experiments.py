import os
from dotenv import load_dotenv
from multiprocessing import Pool
from multiprocessing import Process


load_dotenv()


sim_params = {
    'cores': int(os.getenv('cores')),
    'users': int(os.getenv('Users')),  # 15
    'rep': int(os.getenv('Repetition')),
    'stopTime': int(os.getenv('StopTime')),
    'protocol': os.getenv('AdaptationLogicToUse'),  # RateBasedAdaptationLogic RateAndBufferBasedAdaptationLogic
}


def start_simulation(sim_params, seed):
    os.system('./waf --run \'dash-btree-botup --stopTime=%d --Client=%d --seed=%d --AdaptationLogicToUse=\'%s\'\'' %
              (sim_params['stopTime'], sim_params['users'], seed, sim_params['protocol']))
    print(seed)


def main():
    print("Starting users:", sim_params['users'])
    jobs = []

    q = [i for i in range(sim_params['rep'])]

    while q:
        seed = q.pop()

        job = Process(target=start_simulation, args=(sim_params, seed))
        jobs.append(job)
        job.start()

        if len(jobs) >= sim_params['cores'] or not q:
            while jobs:
                job = jobs.pop()
                job.join()

    print("Done.")


def start_simulation_2(seed):
    os.system('./waf --run \'dash-btree-botup --stopTime=%d --Client=%d --seed=%d --AdaptationLogicToUse=\'%s\'\'' %
              (sim_params['stopTime'], sim_params['users'], seed, sim_params['protocol']))


def main_2():
    print("Starting users:", sim_params['users'])

    q = [i for i in range(sim_params['rep'])]

    with Pool(sim_params['cores']) as p:
        p.map(start_simulation_2, q)

    print("Done.")


if __name__ == '__main__':
    main_2()
