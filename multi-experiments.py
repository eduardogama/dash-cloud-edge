import os
from dotenv import load_dotenv
from multiprocessing import Pool


load_dotenv()


sim_params = {
    'cores': int(os.getenv('cores')),
    'users': int(os.getenv('Users')),  # 15
    'rep': int(os.getenv('Repetition')),
    'stopTime': int(os.getenv('StopTime')),
    'protocol': os.getenv('AdaptationLogicToUse'),  # RateBasedAdaptationLogic RateAndBufferBasedAdaptationLogic
}


def start_simulation(sim_params, seed):
    # os.system('./waf --run \'dash-btree-botup --stopTime=%d --Client=%d --seed=%d --AdaptationLogicToUse=\'%s\'\'' %
    #           (sim_params['stopTime'], sim_params['users'], seed, sim_params['protocol']))
    print(seed)


def start_simulation_2(seed):
    os.system('./waf --run \'dash-btree-botup --stopTime=%d --Client=%d --seed=%d --AdaptationLogicToUse=\'%s\'\'' %
              (sim_params['stopTime'], sim_params['users'], seed, sim_params['protocol']))
    print(seed)


def main():
    print("Starting users:", sim_params['users'])

    q = [i for i in range(sim_params['rep'])]

    with Pool(5) as p:
        p.map(start_simulation_2, q)

    # jobs = []
    # while q:
    #     seed = q.pop()
    #
    #     job = Process(target=start_simulation, args=(sim_params, seed))
    #     jobs.append(job)
    #     job.start()
    #
    #     if len(jobs) >= sim_params['cores'] or not q:
    #         while jobs:
    #             job = jobs.pop()
    #             job.join()

    print("Done.")


if __name__ == '__main__':
    main()
