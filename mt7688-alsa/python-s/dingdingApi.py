#-*- coding:utf-8 -*-
import requests
import random
import time
import json
import time 

#public string
access_token = '08dcdb662c3b0f8d540a31a0c32234ea311f60f164b307e138b3521e16cf70da6f6254c05d6857f28fe7ee04740f67b1'
sign = '12345'                                                       #string sign  and timestamp Not use 
uuid = 'ed1a71c1eb1aad4f77ef85cf3cd031cd'    

def generate_code():
    #Random password
    code_list = []
    for i in range(10): 
        code_list.append(str(i))
    myslice = random.sample(code_list, 6)  
    verification_code = ''.join(myslice) 
    #print (verification_code)
    return verification_code


def func(x):
    # return the x is interger or not
    try:
        x = int(x)
        return isinstance(x, int)
    except ValueError:
        return False


def add_password(status, number=None):
    '''
    add a password func
    json object {status begin end} status = 1 permanent 
    2 temp
    status=2 begin and end must exist side by side
    '''
    # 5 minute ago , But no effect 
    begin = int(time.time()) - 300
    '''
    time_local = time.localtime(begin)
    dt = time.strftime("%Y-%m-%d %H:%M:%S",time_local)
    time_locals = time.localtime(begin + 300)
    dts = time.strftime("%Y-%m-%d %H:%M:%S",time_locals)
    print (dt)
    print (dts)
    '''
    end = begin + 3600
    permission = {'status':str(status), 'begin':str(begin), 'end': str(end)}
    is_default = 0                    #is_default = 1 admin  other number or no value is temp
    notify = 2
    phone = 'x'
    country_code = '+86'
    name = str(int(time.time()))
    password = number
    
    if status == 1:
        if len(number) == 6:
            password = number        
            '''is_int = func(int(number))
            if is_int:
                print ("isInt")
            else:
                print ("isNot Int")'''
        else:
            retmsg = "Password is not available!"
            return retmsg
    if status == 2:
        password = str(generate_code())    
        
    add_param = {'access_token': access_token,
                          'sign': sign,
                          'uuid': uuid,
                          'password': password,
                          'permission': json.dumps(permission),
                          #'is_default': is_default,
                          #'notify': notify,
                          #'phone': phone,
                          #'country_code': country_code,
                          'name': name}

    add_url = "http://118.190.15.108:4085/api/lock/v1/pwd/operations/add";
    add_res = requests.post(add_url, data = add_param)
    #print(add_res.url)
    #print(add_res.text)
    
    res_str = json.loads(add_res.text)
    error_str = res_str["ErrNo"]
    if int(error_str) == 0:
        if status == 1:
            return True
        if status == 2:
            return password
    else:
        retmsg = "Error : " + str(error_str)
        print(retmsg)
        return retmsg

def list_password():
    #Get the password
    list_url = "http://118.190.15.108:4085/api/lock/v1/pwd/";
    param = {'access_token': access_token,
                    'sign': sign,
                    'uuid': uuid }
    list = requests.get(list_url, params = param)    
    #print(list.url)
    #print(list.text)
    list_str = json.loads(list.text)
    list_json_str = list_str["passwords"]
    #print (list_json_str) 
    passwords = sorted(list_json_str.keys()) #Note
    #print (passwords)
    #Parse json date
    list_num = []
    for i in range(len(passwords)):
        list_num.append(str(passwords[i]))
        #print str(i + 1) + " : " + str(passwords[i])
    #print (list_num)
    #Whether operating correctly
    list_string = json.loads(list.text)
    error_string = list_string["ErrNo"]
    if int(error_string) == 0:        
        return list_num
    else:
        print (str(error_string))
        return False
    
    #Get the passwordid    
    '''isTrue = True
    while isTrue:
        print 'Please input a number: '
        input_num = raw_input()
        input = func(input_num)
        if input:
            isTrue = False
        else:
            print "Input Error! Please input an interger: "
    #print list_num[int(input_num) - 1]
    passwordid = list_num[int(input_num) - 1]
    '''


def delete_password(num):
    #Delete the password
    list_password_str = list_password()
    passwordid = list_password_str[num - 1]
    #print passwordid
    param_del_data = {'access_token': access_token, 
                                    'sign': sign, 
                                    'uuid': uuid, 
                                    'passwordid': passwordid, 
                                    #'userid': userid 
                                    }
    delete_url = "http://118.190.15.108:4085/api/lock/v1/pwd/operations/delete"
    del_list = requests.post(delete_url, data = param_del_data)

    #print(del_list.url)
    #print(del_list.text)
    #Update the password
    del_str = json.loads(del_list.text)
    error_string = del_str["ErrNo"]
    if int(error_string) == 5021:
            return True
    else:
        print(str(error_string))
        return False


def update_password(num_id, new_password):
    #Update the password
    list_password_str = list_password()
    passwordid = list_password_str[num_id - 1]
    password = new_password
    param_update_data = {'access_token': access_token,
                         'sign': sign,
                         'uuid': uuid,
                         'passwordid': passwordid,
                         'password': password,
                         #'permission': permission
                         }
    update_url = "http://118.190.15.108:4085/api/lock/v1/pwd/operations/update"
    update_res = requests.post(update_url, data = param_update_data)

    #print(update_res.url)
    #print(update_res.text)
    #Whether operating correctly
    update_str = json.loads(update_res.text)
    error_string = update_str["ErrNo"]
    if int(error_string) == 0:
            return True
    else:
        print(str(error_string))
        return False

#Test Part
# print (list_password(1, "472235"))
print (add_password(1, "472235"))
#print (list_password())
#print (add_password(2, "201987"))
#print (list_password())
#print (delete_password(4))
#print (list_password())
#print (update_password(4, '704982'))











