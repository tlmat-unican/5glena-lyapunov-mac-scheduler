# Extending QoS-aware scheduling in 5G-LENA: A Lyapunov based solution

This repository contains the implementation over ns-3 NR module ([5G-LENA](https://5g-lena.cttc.es/)) of a Guaranteed Flow Bit Rate (GFBR) aware MAC scheduler based on Lyapunov Theory. In a nutshell, the scheduler aims to stabilize traffic flows, ensuring the required throughput for each flow based on their QoS requirements, while minimizing the radio resources allocated by base stations.

## Dependencies

It has been tested in `Ubuntu 22.04.3 LTS (Jammy Jellyfish)`.

[Jupyter](https://jupyter.org/install) is used to generate plots.
## Project structure

The project contains a copy of ns-3 that includes `5G-LENA v2.5`.

The [contrib/nr/examples](https://github.com/tlmat-unican/5glena-lyapunov-mac-scheduler/blob/main/contrib/nr/examples) folder contains an example, [cttc-nr-traffic-3gpp-xr-qos-sched](https://github.com/tlmat-unican/5glena-lyapunov-mac-scheduler/blob/main/contrib/nr/examples/cttc-nr-traffic-3gpp-xr-qos-sched.cc), to simulate XR traffic. This example allows to configurate the MAC scheduler used.

[run.ipynb](https://github.com/tlmat-unican/5glena-lyapunov-mac-scheduler/run.ipynb) is a Jupyter notebook to automate experiment execution and to ease the generation of corresponding result figures.

## Getting started

### Building ns-3

Configure and compile ns-3.

```bash
$ ./ns3 configure --enable-examples --enable-tests
$ ./ns3 build
```

### Running the tests

Run [cttc-nr-traffic-3gpp-xr-qos-sched](https://github.com/tlmat-unican/5glena-lyapunov-mac-scheduler/blob/main/contrib/nr/examples/cttc-nr-traffic-3gpp-xr-qos-sched.cc) to test the scheduler.

```bash
$ ./ns3 run cttc-nr-traffic-3gpp-xr-qos-sched
```

The execution of this scenario generates the following logs:

* Scheduling decisions: alpha.txt [time (ms) | UE RNTI | RBGs allocated]
* Average throughput: res.txt [throughput (Mbps) | size (bytes) | time (ms)]

[run.ipynb](https://github.com/tlmat-unican/5glena-lyapunov-mac-scheduler/blob/main/run.ipynb) contains code snippets to reproduce the results shown in the paper.


**Disclaimer:** figures in manuscript are generated with Tikz using the same data.

    
## Contact

- [Neco Villegas (villegasn@unican.es)](mailto:villegasn@unican.es)
- [Ana Larrañaga (alarranaga@cttc.es)](mailto:alarranaga@cttc.es)
- [Luis Diez (diezlf@unican.es)](mailto:diezlf@unican.es)
- [Katerina Koutlia (kkoutlia@cttc.es)](mailto:kkoutlia@cttc.es)
- [Sandra Lagén (slagen@cttc.es)](mailto:slagen@cttc.es)
- [Ramón Agüero (agueroc@unican.es)](mailto:agueroc@unican.es)
