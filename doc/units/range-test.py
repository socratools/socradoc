#!/usr/bin/python

import math

data = [tuple([int(v,base=0) for v in line.split()]) for line in open("range.dat").readlines() if line[0] != "#"]

f = lambda x: 2.0 * (90.0 - 4.5 * math.log(x))

[(t[0], t[1], f(t[1])) for t in data]
