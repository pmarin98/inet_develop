#!/usr/bin/python3

import os
import sys
import argparse

parser = argparse.ArgumentParser()
parser.add_argument("-s", "--scalarPacketlevel", default=True)
parser.add_argument("-sn", "--scalarPacketlevelNeuralNetwork", default=False)
parser.add_argument("-d", "--dimensionalPacketlevel", default=True)
parser.add_argument("-dn", "--dimensionalPacketlevelNeuralNetwork", default=False)
parser.add_argument("-b", "--dimensionalSymbollevel", default=True)
parser.add_argument("inifile")
args = parser.parse_args()

if args.scalarPacketlevel:
    print("Scalar Packetlevel")
    os.system(f"opp_runall -b 1 inet_dbg -f {args.inifile} -u Cmdenv -c ScalarPacketlevel")

if args.scalarPacketlevelNeuralNetwork:
    print("Scalar Packetlevel Neural Network")
    os.system(f"opp_runall -b 1 inet_dbg -f {args.inifile} -u Cmdenv -c ScalarPacketlevelNeuralNetwork")

if args.dimensionalPacketlevel:
    print("Dimensional Packetlevel")
    os.system(f"opp_runall -b 1 inet_dbg -f {args.inifile} -u Cmdenv -c DimensionalPacketlevel")

if args.dimensionalPacketlevelNeuralNetwork:
    print("Dimensional Packetlevel Neural Network")
    os.system(f"opp_runall -b 1 inet_dbg -f {args.inifile} -u Cmdenv -c DimensionalPacketlevelNeuralNetwork")

if args.dimensionalSymbollevel:
    print("Dimensional Symbollevel")
    os.system(f"opp_runall -b 1 inet_dbg -f {args.inifile} -u Cmdenv -c DimensionalSymbollevel")
