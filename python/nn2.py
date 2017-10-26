#!/usr/bin/env python
#from __future__ import print_function
import numpy as np
import datetime

#Define and generate the sample
nb_of_samples_per_class=20 #the number of sample in each class
red_mean=[-1,0]
blue_mean=[1,0]
std_dev=1.2

#Generate sample from both classes
x_red=np.random.randn(nb_of_samples_per_class,2)*std_dev+red_mean
x_blue=np.random.randn(nb_of_samples_per_class,2)*std_dev+blue_mean

#Merge samples in set of input variables x, and corresponding set of output variables t
X=np.vstack((x_red,x_blue)) #X [a,b] len=40
t=np.vstack((np.zeros((nb_of_samples_per_class,1)),np.ones((nb_of_samples_per_class,1))))

print "%s----%d" % ("hello",len(X))
for i in range(0,len(X)):
	print "X[%d]=[%f,%f]" % (i,X[i,0],X[i,1])

def logistic(z):
	return 1/(1+np.exp(-z))

def nn(x,w):
	return logistic(x.dot(w.T)) #w.T-transpose, dot product

def nn_predict(x,w):
	return np.around(nn(x,w))

def cost(y,t):
	return - np.sum(np.multiply(t,np.log(y))+np.multiply((1-t),np.log(1-y)))


'''
#-----plot the cost in function of the weights
nb_of_ws=100
ws1=np.linspace(-5,5,num=nb_of_ws) #weight 1,
ws2=np.linspace(-5,5,num=nb_of_ws) #weight 2,
ws_x,ws_y=np.meshgrid(ws1,ws2)
cost_ws=np.zeros((nb_of_ws,nb_of_ws)) #--init. cost matrix
#fill the cost matrix for each combination of weights
for i in range(nb_of_ws):
	for j in range(nb_of_ws):
		cost_ws[i,j]=cost(nn(X,np.asmatrix([ws_x[i,j],ws_y[i,j]])),t) #asmatrix--not copy, but refer.
'''
#define the gradient function
def gradient(w,x,t):
	return (nn(x,w)-t).T*x

#define the update function delta w which returns the delta w for each weight in a vector
def delta_w(w_k,x,t,learning_rate):
	return learning_rate*gradient(w_k,x,t)

#set initial weight parameter
w=np.asmatrix([-4,-2])
#set the learning rate
learning_rate=0.05

#start the gradient descent
nb_of_iterations=10000

w_iter=[w] #list to stroe the weight values over the iterations

print " ---- start ---- "
t_start=datetime.datetime.now()
for i in range(nb_of_iterations):
	dw=delta_w(w,X,t,learning_rate)
	w=w-dw
	val_cost=cost(nn(X,w),t)
	print "Iter. %d \t w: [%f8, %f8] \t cost: %f8" %(i,w[0,0],w[0,1],val_cost)
	w_iter.append(w) #store the weight for plotting

print " ---- end ---- "
t_end=datetime.datetime.now()
print "time cost: %d s  %d ms" % ( (t_end-t_start).seconds,int(((t_end-t_start).microseconds)/1000) )



