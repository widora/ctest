import requests
import json
import urllib

def getSentence(name, language):
    print ("Python to connect imio.ai ")

    if language:
        reg_str = "http://www.imio.ai/ai-bot/stt?lang=zh-CN" #port
    else:
        reg_str = "http://www.imio.ai/ai-bot/stt?lang=en-US" #port

    req = open(name, 'r').read()
    #print(req)
    #params = urllibparse.urlencode({'lang':'en-US'})  #zh-CN
    headers ={'Content-type': 'audio/wav','samplerate': '16000'}
    add_res = requests.post(reg_str, data = req, headers = headers)
    #print(add_res.url)
    print (add_res.text)

    res_str = json.loads(add_res.text)['data']['msg'][0]
    print(res_str.encode('utf-8'))
    return res_str.encode('utf-8')
