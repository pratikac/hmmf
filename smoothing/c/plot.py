#!/usr/bin/python

import sys
from numpy import *
from pylab import *

truth = loadtxt('truth.dat')
obs = loadtxt('observations.dat')
fest = loadtxt('filter.dat')
sest = loadtxt('smoothing.dat')

d = float(sys.argv[1])
dt = d/float(len(truth[:,0]))
times = linspace(dt, d+dt, len(truth[:,0]))

#print "dt: ", dt
#print times

figure(1)
plot(times, truth[:,0], 'r-', label='sys')
#plot(times, obs[:,0], 'b-', label='obs')
plot(times, fest[:,0], 'g-', label='filter')
plot(times, sest[:,0], 'c-', label='smoothing')
axis('tight')
grid()
xlabel('t [s]')
legend(loc=4)
title('vanderpol_x')
savefig('smoothing_vanderpol_x.pdf', bbox_inches='tight')

figure(2)
plot(times, truth[:,1], 'r-', label='sys')
#plot(times, obs[:,1], 'b-', label='obs')
plot(times, fest[:,1], 'g-', label='filter')
plot(times, sest[:,1], 'c-', label='smoothing')
axis('tight')
grid()
xlabel('t [s]')
legend(loc=4)
title('vanderpol_x_dot')
savefig('smoothing_vanderpol_x_dot.pdf', bbox_inches='tight')


ferr = truth - fest
serr = truth - sest
n1 = [norm(ferr[i,:]) for i in range(len(ferr[:,0]))]
n2 = [norm(serr[i,:]) for i in range(len(serr[:,0]))]

print "ferr: ", mean(n1), " serr: ", mean(n2)

show()
