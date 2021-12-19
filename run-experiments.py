import os
from dotenv import load_dotenv


load_dotenv()


def main():

    rep = os.getenv('Repetition')
    users = os.getenv('Users')  # 15
    stopTime = os.getenv('StopTime')
    protocol = os.getenv('AdaptationLogicToUse')  # RateBasedAdaptationLogic RateAndBufferBasedAdaptationLogic

    print("Initializing ...")

    for seed in range(0, rep):
        os.system('./waf --run \'dash-btree-botup --stopTime=%d --users=%d --seed=%d --AdaptationLogicToUse=\"dash::player::%s\"\'' %
                  (stopTime, users, seed, protocol))

    print("Done.")


if __name__ == '__main__':
    main()
