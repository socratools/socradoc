#!/usr/bin/python

import math

data = [tuple([int(v,base=0) for v in line.split()]) for line in open("thresh.dat").readlines() if line[0] != "#"]

f = lambda x: (10*math.log(x) - 160)*6/7

[(t[0], t[1], f(t[1])) for t in data]
