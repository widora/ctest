
import requests
import json
import urllib


def getSentence(name,language):
    print ("Python to connect imio.ai ")
    print (name + language) 

    if language:
        reg_str = "http://www.imio.ai/ai-bot/stt?lang=zh-CN" #port
    else:
        reg_str = "http://www.imio.ai/ai-bot/stt?lang=en-US" #port

    body = open(name, 'r').read()
    #print(body)
    #params = urllibparse.urlencode({'lang':'en-US'})  #zh-CN
    headers ={'Content-type': 'audio/wav','samplerate': '16000'}
    add_res = requests.post(reg_str, data = body, headers = headers)
    #print(add_res.url)
    print (add_res.text)

    res_str = json.loads(add_res.text)['data']['msg'][0]
    print (res_str.encode('utf-8'))
    #return str(res_str)

'''
import requests
import json
import urllib

print ("Python to connect imio.ai ")
reg_str = "http://www.imio.ai/ai-bot/stt?lang=zh-CN" #port
body = open("test.wav", 'r').read()
#print(body)
#params = urllibparse.urlencode({'lang':'en-US'})  #zh-CN
headers ={'Content-type': 'audio/wav','samplerate': '16000'}
add_res = requests.post(reg_str, data = body, headers = headers)
#print(add_res.url)
print (add_res.text)
res_str = json.loads(add_res.text)['data']['msg'][0]
print(res_str.encode('utf-8'))
'''
'''
import requests
import json
import urllib

def getSentence(name, language):
    print ("Python to connect imio.ai ")
    #params = urllibparse.urlencode({"lang":"zh-CN", "uid":"10001", "visitorid":"Tom", "ownership":"host", "ring":False})
    if language:
        reg_str = "http://www.imio.ai/ai-bot/audio_chat?lang=zh-CN&uid=10001&visitorid=Tom&ownership=host&ring=False" #port
    else:
        reg_str = "http://www.imio.ai/ai-bot/audio_chat?lang=en-US&uid=10001&visitorid=Tom&ownership=host&ring=False" #port

    body = open(name, 'r').read()
    #print(body)
    #params = urllibparse.urlencode({'lang':'en-US'})  #zh-CN
    headers = {'Content-type': 'audio/wav','samplerate': '16000'}
    add_res = requests.post(reg_str, data = body, headers = headers)
    #print(add_res.url)
    print (add_res.text)

    res_str = json.loads(add_res.text)['data']['msg'][0]
    print(res_str.encode('utf-8'))
    return res_str.encode('utf-8')
'''



