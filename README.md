
## The Network Simulator, Version 3

## Installing Procedure

* Install Pre-Requesits for ns-3
* Install Pre-Requesits for AMuSt-libdash
* Install Pre-Requesits for Gurobi Optimizer
* Clone all relevant git repositories
* Build AMuSt-libdash
* Build ns-3 with dash-cloud-edge


## How to build:

First go into AMuSt-libdash-master/, read its README and build the libdash for ns-3.

Next, make sure you have all ns-3 dependencies and ZLib dev packages.

Then you can go to ns3/ and configure it:

```shell
CXXFLAGS="-Wall -std=c++11" ./waf configure --with-dash=../AMuSt-libdash/libdash --with-pybindgen=../pybindgen
```

...and build it as usual.

## References

[1] https://github.com/mkheirkhah/mptcp

[2] https://github.com/ChristianKreuzberger/AMuSt-ns3
