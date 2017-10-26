#!/usr/bin/env python
#--author: Peter Roelants
#--original source:http://peterroelants.github.io/posts/neural_network_implementation_part01/

import numpy as np
import datetime

#define and generate the samples
nb_of_samples_per_class =20
blue_mean=[0]
red_left_mean=[-2]
red_right_mean=[2]

std_dev=0.5
#generate test samples from both classes
x_blue=np.random.randn(nb_of_samples_per_class,1)*std_dev+blue_mean
x_red_left=np.random.randn(nb_of_samples_per_class/2,1)*std_dev+red_left_mean
x_red_right=np.random.randn(nb_of_samples_per_class/2,1)*std_dev+red_right_mean

#merge samples
x=np.vstack((x_blue,x_red_left,x_red_right))
t=np.vstack((np.ones((x_blue.shape[0],1)), #------------
	     np.zeros((x_red_left.shape[0],1)),
	     np.zeros((x_red_right.shape[0],1)) ))

for i in range(len(x)):
	print "x(%d):%f8  \t t(%d):%d" % (i,x[i],i,t[i])

#define the rbf function
def rbf(z):
	return np.exp(-z**2)

def logistic(z):
	return 1/(1+np.exp(-z))

def hidden_activations(x, wh):
	return rbf(x*wh)

def output_activations(h, wo):
	return logistic(h*wo-1)

def nn(x, wh, wo):
	return output_activations(hidden_activations(x,wh),wo)

def nn_predict(x, wh, wo):
	return np.around(nn(x, wh, wo))

def cost(y,t):
	return  - np.sum( np.multiply(t, np.log(y)) + np.multiply((1-t), np.log(1-y)) ) #--np.sum: sum all elements in an array

#define a function to calculate the cost for a given set of parameters
def cost_for_param(x, wh, xo, t):
	return cost(nn(x,wh,wo), t)

#define the error function
def gradient_output(y,t):
	return y-t

#define gradient function for the weight parameter at the output layer
def gradient_weight_out(h, grad_output):
	return h*grad_output

def gradient_hidden(wo,grad_output): 
	return wo*grad_output

def gradient_weight_hidden(x,zh,h,grad_hidden):
	return x*-2*zh*h*grad_hidden

#Define the update function to update the network parameters over 1 iteration
def backprop_update(x,t,wh,wo,learning_rate):
	zh=x*wh
	h=rbf(zh)
	y=output_activations(h, wo)
	#compute the gradient at the output
	grad_output=gradient_output(y,t)
	#get the delta for wo
	d_wo=learning_rate*gradient_weight_out(h,grad_output)
	#compute the gradient_hidden layer
	grad_hidden=gradient_hidden(wo,grad_output)
	#get the delta for wh
	d_wh=learning_rate*gradient_weight_hidden(x,zh,h,grad_hidden)
	#return the update parameters
	return (wh-d_wh.sum(), wo-d_wo.sum())

#--------------------------------
#set the initial weight parameter
wh=2
wo=-5
#set the learning rate
learning_rate=0.2

nb_of_iterations=10000
lr_update=learning_rate/nb_of_iterations #learning_rate update rule
w_cost_iter=[(wh,wo,cost_for_param(x,wh,wo,t))] #list to store the weight values over the iteration

print " --- start iteration ---"
t_start=datetime.datetime.now()
for i in range(nb_of_iterations):
	learning_rate -= lr_update #decrease the learning rate
	wh,wo=backprop_update(x,t,wh,wo,learning_rate)
	print"Iter. %d \t wh: %f8 \t wo: %f8 \t cost: %f8" % (i,wh,wo,cost_for_param(x,wh,wo,t))
	#w_cost_iter.append((wh,wo,cost_for_param(x,wh,wo,t))) #store the value

t_end=datetime.datetime.now()
print " --- end iteration ---"

print "time cost: %d s  %d ms" % ( (t_end-t_start).seconds,int(((t_end-t_start).microseconds)/1000) )
