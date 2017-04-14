import requests
import json
import urllib

reg_str = "http://www.imio.ai/ai-bot/stt?lang=zh-CN" #port

#signup_req
req = open("test.wav", 'r').read()
#print(req)
#params = urllibparse.urlencode({'lang':'en-US'})
#params = urllibparse.urlencode({'lang':'zh-CN'})
headers ={'Content-type': 'audio/wav','samplerate': '16000'}
add_res = requests.post(reg_str, data = req, headers = headers)

print(add_res.url)
print (add_res.text)

res_str = json.loads(add_res.text)['data']['msg'][0]
print(res_str.encode('utf-8'))
