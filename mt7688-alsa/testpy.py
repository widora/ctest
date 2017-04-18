#!/usr/bin/python
# Filename: test.py
class Person:
	def sayHi(self):
		print 'hi'
class Second:
	def invoke(self,obj):
 		obj.sayHi()
def sayhi(name):
	print 'hi',name;

