import os
from dotenv import load_dotenv


load_dotenv()


def main():

    rep = 1 #int(os.getenv('Repetition'))                 # Number of simulations repetitions  
    users = 15 #int(os.getenv('Users'))                    # 15
    stopTime = 600 #int(os.getenv('StopTime'))              # Simulation duration
    protocol = RateAndBufferBasedAdaptationLogic #os.getenv('AdaptationLogicToUse')  # RateBasedAdaptationLogic RateAndBufferBasedAdaptationLogic

    print("Initializing ...")

    for seed in range(0, rep):
#        os.system('./waf --run \'dash-btree-botup --stopTime=%d --users=%d --seed=%d --AdaptationLogicToUse=\"dash::player::%s\"\'' % (stopTime, users, seed, protocol))
        os.system('./waf --run \'binary-tree-scenario --stopTime=%d --Client=%d --seed=%d --HASLogic=\"dash::player::%s\"\'' % (stopTime, users, seed, protocol))

    print("Done.")


if __name__ == '__main__':
    main()
