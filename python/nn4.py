#!/usr/bin/env python

#--author: Peter Roelants
#--original source: http://peterroelants.github.io/posts/neural_network_implementation_part01/
from __future__ import print_function
import numpy as np
import sklearn.datasets
import datetime,time


#Generate the dataset
X,t = sklearn.datasets.make_circles(n_samples=100, shuffle=False, factor=0.3, noise=0.1)
T = np.zeros((100,2)) #define target matrix
T[t==1,1]=1
T[t==0,0]=1

#separate the red and blue points for plotting
x_red=X[t==0]
x_blue=X[t==1]

print ("shape of X: {}".format(X.shape))
print ("shape of Y: {}".format(T.shape))
#---print MUST use single quote,foramt must be same as actual type
for i in range(0,len(X)):
     print('X({:d}): {:.8f}  {:.8f}    \t T({:d}): {}'.format(i, X[i][0], X[i][1],i,T[i]))

#define the logistic function
def logistic(z):
	return 1/(1+np.exp(-z))

#define the softmax function
def softmax(z):
	return np.exp(z)/np.sum(np.exp(z),axis=1,keepdims=True)

#function to compute the hidden activations
def hidden_activations(X,Wh,bh):
	return logistic(X.dot(Wh)+bh)

#define output layer feedforward
def output_activations(H,Wo,bo):
	return softmax(H.dot(Wo)+bo)

#define the neural network function
def nn(X,Wh, bh, Wo,bo):
	return output_activations(hidden_activations(X,Wh,bh),Wo,bo)

#Define the network prediction function that only returns 1 or 0
def nn_predict(X,Wh,bh,Wo,bo):
	return np.around(nn(X,Wh,bh,Wo,bo))

#define the cost function
def cost(Y,T):
	return - np.multiply(T, np.log(Y)).sum()

#define the error function at the output
def error_output(Y,T):
	return Y-T

#define the gradient function for the weight paramters at the output layer
#Eo stands for 'delta 0'
def gradient_weight_out(H, Eo):
	return H.T.dot(Eo)

#define the gradient function for the bias parameters at the output layer
def gradient_bias_out(Eo):
	return np.sum(Eo, axis=0, keepdims=True)

#define the error function at the hidden layer
# 'delta' stands for 'error'
def error_hidden(H, Wo, Eo):
	# H*(1-H)*(E . Wo^T)
	return np.multiply(np.multiply(H,(1-H)),Eo.dot(Wo.T))

#define the gradient function for the weight parameters at the hiden layer
def gradient_weight_hidden(X,Eh):
	return X.T.dot(Eh)

#define the gradient function for the bias parameters at the output layer
def gradient_bias_hidden(Eh):
	return np.sum(Eh, axis=0, keepdims=True)

#------------- gradients check -----------------
#initialize weights and biases
init_var=0.1
#init hidden layer parameters
bh = np.random.randn(1,3)*init_var
Wh = np.random.randn(2,3)*init_var
#init. output layer parameters
bo = np.random.randn(1,2)*init_var
Wo = np.random.randn(3,2)*init_var

#compute the gradients by backpropagation
#compute the activations of the layers
H = hidden_activations(X,Wh,bh)
Y = output_activations(H,Wo,bo)

#compute the gradients of the output layer
Eo = error_output(Y,T)
JWo = gradient_weight_out(H, Eo)
Jbo = gradient_bias_out(Eo)

#compute the gradients of the hidden layer
Eh = error_hidden(H,Wo,Eo)
JWh = gradient_weight_hidden(X,Eh)
Jbh = gradient_bias_hidden(Eh)

#combine all parameter matrics in a list
params = [Wh,bh,Wo,bo]
#combine all parameter gradients in a list
grad_params = [JWh, Jbh, JWo, Jbo]

#set the small change to compute the numerical gradient
eps = 0.0001

#-----  check each parameter matrix ------------
for p_idx in range(len(params)):
	#check each parameter in each parameter matrix
	for row in range(params[p_idx].shape[0]):
	    for col in range(params[p_idx].shape[1]):
		#copy the parameter matrix and change the current parameter slightly
		p_matrix_min=params[p_idx].copy()
		p_matrix_min[row,col] -= eps
		p_matrix_plus=params[p_idx].copy()
		p_matrix_plus[row,col] += eps
		#copy the parameter list, and change the update parameter matrix
		params_min=params[:]
		params_min[p_idx]=p_matrix_min
		params_plus=params[:]
		params_plus[p_idx]=p_matrix_plus
		#compute the numerical gradient
		grad_num=(cost(nn(X,*params_plus),T)-cost(nn(X,*params_min),T))/(2*eps)
		#raise error if the numerical grade is not close to the backprop gradient
		if not np.isclose(grad_num, grad_params[p_idx][row,col]):
			raise ValueError('Numerical gradient of {:.6f} is not close to the backpropa \
gation gradient of {:.6f}!'.formate(float(grad_num),float(grad_params[p_idx][row,col])))

print ('No gradient error found!')
time.sleep(2) 


#define the update function to update the network parameters over 1 iteration
def backprop_gradients(X,T,Wh,bh,Wo,bo):
	#compute the output of the network
	#compute the activations of the layers
	H=hidden_activations(X,Wh,bh)
	Y=output_activations(H,Wo,bo)
	#compute the gradients of the output layer
	Eo=error_output(Y,T)
	JWo = gradient_weight_out(H,Eo)
	Jbo = gradient_bias_out(Eo)
	#compute the gradients of the hidden layer
	Eh = error_hidden(H,Wo,Eo)
	JWh = gradient_weight_hidden(X,Eh)
	Jbh = gradient_bias_hidden(Eh)

	return [JWh, Jbh, JWo,Jbo]

def update_velocity(X,T, ls_of_params, Vs, momentum_term, learning_rate):
	#ls_of_params = [Wh,bh,Wo,bo]
	#Js = [JWh, Jbh, JWo,Jbo]
	Js = backprop_gradients(X,T,*ls_of_params) #-- *ls_of_params : expand array elements for function input params
	return [ momentum_term*V - learning_rate* J for V,J in zip(Vs,Js)] 

def update_params(ls_of_params,Vs):
	# ls_of_params = [Wh, bh, Wo, bo]
	# Vs = [VWh, Vbh, VWo, Vbo]
	return [P+V for P,V in zip(ls_of_params,Vs)]

#---------  Run backpropagation ----------
#init. weights and biases
init_var=0.1
#init. hidden layer parameters
bh = np.random.randn(1,3)*init_var
Wh = np.random.randn(2,3)*init_var
#init. output layer parameters
bo = np.random.randn(1,2)*init_var
Wo = np.random.randn(3,2)*init_var
#parameters are already initilized randomly with the gradient checking
#set the learing rate
learning_rate = 0.02
momentum_term=0.9

#define the velocities Vs = [VWh, Vbh, VWo, Vbo]
Vs = [np.zeros_like(M) for M in [Wh, bh, Wo, bo]]
#start the gradient descent updates and print the interations
nb_of_iterations=5000
lr_update = learning_rate / nb_of_iterations #learning rate update rule
ls_costs = [cost(nn(X,Wh,bh,Wo,bo),T)]  # list of cost over the iteration

print(" ----- start iteration ----- ")
t_start = datetime.datetime.now()
for i in range(nb_of_iterations):
	# update the velocities and the parameters
	Vs = update_velocity(X,T, [Wh,bh,Wo,bo], Vs, momentum_term,learning_rate)
	Wh,bh,Wo,bo = update_params([Wh,bh,Wo,bo],Vs)
	print ("Iter.{}     Cost:{:.8f}" .format(i,cost(nn(X,Wh,bh,Wo,bo),T)) )
t_end = datetime.datetime.now()
print(" ----- end iteration ----- ")
print ("Iter. running time: %d s  %d ms" % ( (t_end-t_start).seconds,int(((t_end-t_start).microseconds)/1000) ))
