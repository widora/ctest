#!/usr/bin/env python
# -*- coding: utf-8 -*-
#-------------------------------------------------
# Example of BAIDU Facial Reconginition
# 
#--------------------------------------------------
import sys, urllib, urllib2, json
import ssl
ssl._create_default_https_context = ssl._create_unverified_context

data = {}
#------------ your BAIDU access token  ------------
str_token='xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx' 

def request_baidu(url,data):
   print '//------ start urlencode data ...'
   decoded_data = urllib.urlencode(data)
   print '//------ start request() ...'
   req = urllib2.Request(url, decoded_data)
   req.add_header("Content-Type","application/x-www-form-urlencoded")
   print '//------ start urlopen() ...It may take a while....'
   resp = urllib2.urlopen(req)
   content = resp.read()
   return content

#--------- detect faces -------
# Example: ./face.py -det julia.jpg
if sys.argv[1]=='-det':
   url ='https://aip.baidubce.com/rest/2.0/face/v1/detect?access_token='+ str_token
   data['max_face_num'] = 5
   data['face_fields'] = "age,beauty,expression,faceshape,gender";

   print '//------ start read  image data ...'
   image_data = open(sys.argv[2],'rb').read()
   data['image'] = image_data.encode('base64').replace('\n','')

   content=request_baidu(url,data)
   json_content=json.loads(content)
   num = int(json_content['result_num'])
   for i in range(0,num):
   	   print 'Person %d --- age:%s, bueaty:%s, gender:%s' % \
	         (i,json_content['result'][i]['age'],json_content['result'][i]['beauty'],json_content['result'][i]['gender'])
	   print 'Location %s' % json_content['result'][i]['location']

#--------- add and register a faces -------
# default: old face data will be replaced.
# Example: ./face.py -add  Linus_Torvalds  linus.jpg
elif sys.argv[1]=='-add':
   url ='https://aip.baidubce.com/rest/2.0/face/v2/faceset/user/add?access_token=' + str_token

   #data['uid'] = "Linus_Torvalds"
   data['uid'] = sys.argv[2]
   data['user_info'] = "father of Linux"
   data['group_id'] = "linux_group"
   data['action_type'] = "replace"
   print '//------ start read  image data ...'
   image_data = open(sys.argv[3],'rb').read()
   data['image'] = image_data.encode('base64').replace('\n','')

   content=request_baidu(url,data)
   print content

#--------- get info of a registed user -------
# Example: ./face.py -get Linus_Torvalds
elif sys.argv[1]=='-get':
   url ='https://aip.baidubce.com/rest/2.0/face/v2/faceset/user/get?access_token=' + str_token
   #data['uid'] = "Linus_Torvalds"
   data['uid'] = sys.argv[2]

   content=request_baidu(url,data)
   print content

#------- list all registed users in a specified group -----
# Example: ./face.py -get_users linux_group 
elif sys.argv[1]=='-get_users':
   url ='https://aip.baidubce.com/rest/2.0/face/v2/faceset/group/getusers?access_token=' + str_token
   data['group_id'] = sys.argv[2]

   content=request_baidu(url,data)
   json_content=json.loads(content)
   num = int(json_content['result_num'])
   for i in range(0,num):
	print '%d: %s  ---  %s'% (i,json_content['result'][i]['uid'],json_content['result'][i]['user_info'])

#--------- identify a face  -------
# Example: ./face.py -id ruhau.jpg
elif sys.argv[1]=='-id':
   url ='https://aip.baidubce.com/rest/2.0/face/v2/identify?access_token=' + str_token
   data['group_id'] = "linux_group"
   data['user_top_num'] = 2
   print '//------ start read  image data ...'
   image_data = open(sys.argv[2],'rb').read()
   data['image'] = image_data.encode('base64').replace('\n','')

   content=request_baidu(url,data)
   json_content=json.loads(content)
   num = int(json_content['result_num'])
   for i in range(0,num):
	print '%d: %s    scores: %d    group: %s'% \
 	      (i,json_content['result'][i]['uid'],json_content['result'][i]['scores'][0],json_content['result'][i]['group_id'])

else:
   print 'command syntax error! check your option'

#if(content):
#     print(content)
