import os
from dotenv import load_dotenv
from multiprocessing import Process


load_dotenv()


def start_simulation(sim_params, seed):
    os.system('./waf --run \'dash-btree-botup --stopTime=%d --Client=%d --seed=%d --AdaptationLogicToUse=\'%s\'\'' %
              (sim_params['stopTime'], sim_params['users'], seed, sim_params['protocol']))


def main():
    sim_params = {
        'rep': int(os.getenv('Repetition')),
        'users': int(os.getenv('Users')),  # 15
        'stopTime': int(os.getenv('StopTime')),
        'protocol': os.getenv('AdaptationLogicToUse'),  # RateBasedAdaptationLogic RateAndBufferBasedAdaptationLogic
        'cores': int(os.getenv('cores'))
    }

    print("Starting users:", sim_params['users'])

    q = [i for i in range(sim_params['rep'])]

    jobs = []
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


if __name__ == '__main__':
    main()
