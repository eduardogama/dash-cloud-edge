import os
from dotenv import load_dotenv
from multiprocessing import Pool
from multiprocessing import Process


load_dotenv()


sim_params = {
    'cores': int(os.getenv('cores')),              # number of CPU cores  
    'users': int(os.getenv('Users')),              # 15
    'rep': int(os.getenv('Repetition')),           # number of simulations repetitions
    'stopTime': int(os.getenv('StopTime')),        # Simulation duration
    'protocol': os.getenv('AdaptationLogicToUse'), # RateBasedAdaptationLogic RateAndBufferBasedAdaptationLogic
}


def start_simulation(seed):
    # os.system('./waf --run \'dash-btree-botup --stopTime=%d --Client=%d --seed=%d --AdaptationLogicToUse=\'%s\'\'' %
    #          (sim_params['stopTime'], sim_params['users'], seed, sim_params['protocol']))
    os.system('./waf --run \'binary-tree-scenario --stopTime=%d --Client=%d --seed=%d --HASLogic=\"dash::player::%s\"\'' % 
        (sim_params['stopTime'], sim_params['users'], seed, sim_params['protocol']))

def main():
    print("Starting users:", sim_params['users'])

    q = [i for i in range(sim_params['rep'])]

    with Pool(sim_params['cores']) as p:
        p.map(start_simulation, q)

    print("Done.")


if __name__ == '__main__':
    main()
