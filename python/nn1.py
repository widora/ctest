#!/usr/bin/env python
#--author: Peter Roelants
#--original source:http://peterroelants.github.io/posts/neural_network_implementation_part01/

from __future__ import print_function
import numpy as np
import datetime 

x=np.random.uniform(0,1,20)

def f(x):
    return x*2

noise_variance=0.2
noise_variance
noise=np.random.randn(x.shape[0])*noise_variance
t=f(x)+noise

def nn(x,w):
     return x*w
def cost(y,t):
     return ((t-y)**2).sum()
def gradient(w,x,t):
     return 2*x*(nn(x,w)-t)

def delta_w(w_k,x,t,learning_rate):
     return learning_rate*gradient(w_k,x,t).sum()

w=0.1
learning_rate=0.01
nb_of_iterations=10000

w_cost=[(w,cost(nn(x,w),t))]

print (' --- start --- ')
t_start=datetime.datetime.now()

for i in range(nb_of_iterations):
     dw=delta_w(w,x,t,learning_rate)
     w=w-dw
     w_cost.append((w,cost(nn(x,w),t)))

t_end=datetime.datetime.now()
print (' --- end --- ')

for i in range(0,len(w_cost)):
     print('Iter.{}: \t weight: {:.8f} \t cost: {:.8f} '.format(i, w_cost[i][0], w_cost[i][1]))

print ("Running time cost: %d s %d ms" % ( (t_end-t_start).seconds,int(((t_end-t_start).microseconds)/1000) ) )
