#! /usr/bin/env python

# Copyright (c) 2022 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
#
# SPDX-License-Identifier: GPL-2.0-only

# -*- coding: utf-8 -*-
## -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-

# A list of C++ examples to run in order to ensure that they remain
# buildable and runnable over time.  Each tuple in the list contains
#
#     (example_name, do_run, do_valgrind_run).
#
# See test.py for more information.
cpp_examples = [
    ("lena-lte-comparison-user --simTag=test1-user --trafficScenario=2 --simulator=5GLENA --technology=LTE --numRings=0 --ueNumPergNb=1 --calibration=false --freqScenario=0 --operationMode=FDD --direction=DL", "True", "True"),
    ("lena-lte-comparison-user --simTag=test2-user --trafficScenario=2 --simulator=5GLENA --technology=LTE --numRings=0 --ueNumPergNb=1 --calibration=false --freqScenario=0 --operationMode=FDD --direction=UL", "True", "True"),
    ("lena-lte-comparison-campaign --simTag=test1 --trafficScenario=2 --simulator=LENA --technology=LTE --numRings=0 --ueNumPergNb=1 --calibration=false --freqScenario=0 --operationMode=FDD --direction=DL", "True", "True"),
    ("lena-lte-comparison-campaign --simTag=test2 --trafficScenario=2 --simulator=LENA --technology=LTE --numRings=0 --ueNumPergNb=1 --calibration=false --freqScenario=0 --operationMode=FDD --direction=UL", "True", "True"),
    ("lena-lte-comparison-campaign --simTag=test3 --trafficScenario=2 --simulator=5GLENA --technology=LTE --numRings=0 --ueNumPergNb=1 --calibration=false --freqScenario=0 --operationMode=FDD --direction=UL", "True", "True"),
    ("lena-lte-comparison-campaign --simTag=test4 --trafficScenario=2 --simulator=5GLENA --technology=LTE --numRings=0 --ueNumPergNb=2 --calibration=false --freqScenario=0 --operationMode=FDD --direction=UL", "True", "True"),
    ("lena-lte-comparison-campaign --simTag=test5 --trafficScenario=2 --simulator=5GLENA --technology=LTE --numRings=0 --ueNumPergNb=2 --calibration=false --freqScenario=0 --operationMode=FDD --direction=DL", "True", "True"),
    ("lena-lte-comparison-campaign --simTag=test6 --trafficScenario=2 --simulator=5GLENA --technology=LTE --numRings=0 --ueNumPergNb=4 --calibration=false --freqScenario=0 --operationMode=FDD --direction=UL", "True", "True"),
    ("lena-lte-comparison-campaign --simTag=test7 --trafficScenario=2 --simulator=5GLENA --technology=LTE --numRings=0 --ueNumPergNb=4 --calibration=false --freqScenario=0 --operationMode=FDD --direction=DL", "True", "True"),
    ("lena-lte-comparison-campaign --simTag=test8 --trafficScenario=2 --simulator=5GLENA --technology=NR --numRings=0 --ueNumPergNb=4 --calibration=false --freqScenario=0 --operationMode=TDD --direction=UL", "True", "True"),
    ("lena-lte-comparison-campaign --simTag=test9 --trafficScenario=1 --simulator=5GLENA --technology=NR --numRings=0 --ueNumPergNb=4 --calibration=false --freqScenario=0 --operationMode=TDD --direction=DL", "True", "True"),
    ("lena-lte-comparison-campaign --simTag=test10 --trafficScenario=1 --simulator=5GLENA --technology=NR --numRings=0 --ueNumPergNb=4 --calibration=false --freqScenario=0 --operationMode=FDD --direction=UL", "True", "True"),
    ("cttc-nr-notching --gNbNum=1 --ueNumPergNb=2 --operationMode=FDD --enableOfdma=true --enableUl=1 --enableDl=1 --notchedRbStartDl=5 --numOfNotchedRbsDl=5 --notchedRbStartUl=15 --numOfNotchedRbsUl=3 --bandwidth=5e6", "True", "True"),
    ("cttc-nr-notching --gNbNum=1 --ueNumPergNb=2 --operationMode=FDD --enableOfdma=true --enableUl=1 --enableDl=1 --notchedRbStartDl=5 --numOfNotchedRbsDl=7 --notchedRbStartUl=15 --numOfNotchedRbsUl=5", "True", "True"),
    ("cttc-nr-notching --gNbNum=1 --ueNumPergNb=2 --operationMode=FDD --enableOfdma=true --enableUl=1 --enableDl=1 --validationValue1=10.18 --validationValue2=10.08", "True", "True"),
    ("cttc-nr-notching --gNbNum=1 --ueNumPergNb=2 --operationMode=FDD --enableOfdma=true --enableUl=0 --enableDl=1 --validationValue1=10.17 --validationValue2=10.17", "True", "True"),
    ("cttc-nr-notching --gNbNum=1 --ueNumPergNb=2 --operationMode=FDD --enableOfdma=false --enableUl=0 --enableDl=1 --validationValue1=10.18 --validationValue2=10.18", "True", "True"),
    ("cttc-nr-notching --gNbNum=1 --ueNumPergNb=1 --operationMode=FDD --enableOfdma=false --enableUl=0 --enableDl=1 --validationValue1=10.18", "True", "True"),
    ("cttc-nr-notching --gNbNum=1 --ueNumPergNb=1 --operationMode=TDD --enableOfdma=false --enableUl=0 --enableDl=1 --validationValue1=10.18", "True", "True"),
    ("cttc-nr-notching --gNbNum=1 --ueNumPergNb=1 --operationMode=TDD --enableOfdma=false --enableUl=1 --enableDl=0 --validationValue1=10.12", "True", "True"),
    ("cttc-nr-notching --gNbNum=1 --ueNumPergNb=2 --operationMode=FDD --enableOfdma=true --enableUl=1 --enableDl=1 --notchedRbStartDl=5 --numOfNotchedRbsDl=7 --notchedRbStartUl=15 --numOfNotchedRbsUl=5 --validationValue1=10.15 --validationValue2=10.18", "True", "True"),
    ("cttc-nr-notching --gNbNum=1 --ueNumPergNb=2 --operationMode=FDD --enableOfdma=true --enableUl=1 --enableDl=1 --notchedRbStartDl=5 --numOfNotchedRbsDl=5 --notchedRbStartUl=15 --numOfNotchedRbsUl=3 --bandwidth=5e6 --validationValue1=8.27 --validationValue2=9.028", "True", "True"),
    ("cttc-3gpp-channel-example", "True", "True"),
    ("cttc-3gpp-channel-nums", "True", "True"),
    ("cttc-3gpp-channel-nums-fdm", "True", "True"),
    ("cttc-3gpp-channel-simple-fdm", "True", "True"),
    ("cttc-3gpp-channel-simple-ran", "True", "True"),
    ("cttc-3gpp-indoor-calibration", "True", "True"),
    ("cttc-nr-demo", "True", "True"),
    ("cttc-lte-ca-demo --simTime=1", "True", "True"),
    ("cttc-nr-cc-bwp-demo --simTime=0.8 --tddPattern=\"DL|S|UL|UL|DL|DL|S|UL|UL|DL|\"", "True", "True"),
    ("cttc-nr-cc-bwp-demo --simTime=0.8 --tddPattern=\"DL|S|UL|DL|DL|DL|S|UL|DL|DL|\"", "True", "True"),
    ("cttc-nr-cc-bwp-demo --simTime=0.8 --tddPattern=\"DL|S|UL|UL|UL|DL|DL|DL|DL|DL|\"", "True", "True"),
    ("cttc-nr-cc-bwp-demo --simTime=0.8 --tddPattern=\"DL|S|UL|UL|DL|DL|DL|DL|DL|DL|\"", "True", "True"),
    ("cttc-nr-cc-bwp-demo --simTime=0.8 --tddPattern=\"DL|S|UL|DL|DL|DL|DL|DL|DL|DL|\"", "True", "True"),
    ("cttc-nr-cc-bwp-demo --simTime=0.8 --tddPattern=\"DL|S|UL|UL|UL|DL|S|UL|UL|DL|\"", "True", "True"),
    ("cttc-nr-cc-bwp-demo --simTime=0.8 --tddPattern=\"DL|S|UL|UL|UL|DL|S|UL|UL|UL|\"", "True", "True"),
    ("cttc-nr-cc-bwp-demo --simTime=0.8 --tddPattern=\"F|F|F|F|F|F|F|F|F|F|\"", "True", "True"),
    ("cttc-realistic-beamforming", "True", "True"),
    ("cttc-fh-compression --simTimeMs=800", "True", "True"),
    ("cttc-error-model --simTime=2", "True", "True"),
    ("cttc-error-model-amc --simTime=2", "True", "True"),
    ("cttc-error-model-comparison", "True", "True"),
    ("cttc-channel-randomness", "True", "True"),
    ("rem-beam-example", "True", "True"),
    ("rem-example", "True", "True"),
    ("cttc-nr-demo --ueNumPergNb=9", "True", "True"),
    ("cttc-nr-mimo-demo", "True", "True"),
    ("cttc-nr-mimo-demo --useFixedRi=0", "True", "True"),
    ("cttc-nr-mimo-demo --polSlantAngle1=0 --polSlantAngle2=90", "True", "True"),
    ("cttc-nr-mimo-demo --polSlantAngle1=0 --polSlantAngle2=90 --fixedRankIndicator=1", "True", "True"),
    ("cttc-nr-mimo-demo --polSlantAngle1=0 --polSlantAngle2=90 --useFixedRi=0", "True", "True"),
    ("cttc-nr-mimo-demo --crossPolarizedGnb=0 --crossPolarizedUe=0", "True", "True"),
    ("cttc-nr-traffic-ngmn-mixed", "True", "True"),
    ("cttc-nr-traffic-3gpp-xr", "True", "True"),
    ("traffic-generator-example", "True", "True"),
    ]

# A list of Python examples to run in order to ensure that they remain
# runnable over time.  Each tuple in the list contains
#
#     (example_name, do_run).
#
# See test.py for more information.
python_examples = []
