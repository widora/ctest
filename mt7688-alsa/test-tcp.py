'''
import requests
import json
import urllib

reg_str = "http://www.imio.ai/ai-bot/stt?lang=zh-CN" #port

#signup_req
body = open("test.wav", 'r').read()
#print(body)
#params = urllibparse.urlencode({'lang':'en-US'})
#params = urllibparse.urlencode({'lang':'zh-CN'})
headers ={'Content-type': 'audio/wav','samplerate': '16000'}
add_res = requests.post(reg_str, data = body, headers = headers)

print(add_res.url)
print (add_res.text)

res_str = json.loads(add_res.text)['data']['msg'][0]
print(res_str.encode('utf-8'))
'''

#import requests
import json
import urllib
import pyaudio

headers = {'Content-type': 'audio/wav','samplerate': '16000'}
params = urllibparse.urlencode({"lang":"zh-CN", "uid":"10001", "visitorid":"Tom", "ownership":"host", "ring":True})
body = open("test.wav", 'r').read()
conn = httpclient.HTTPConnection('www.imio.ai')
conn.request('POST', '/ai-bot/audio_chat?%s' % params, body, headers)

data = json.load(conn.getresponse().read())['data']['msg']

p = pyaudio.PyAudio()
stream = p.open(format=pyaudio.paInt16, channels=1, rate=16000, output=True, frames_per_buffer=1024)
stream.write(data)
stream.close()

'''
print ("Python to connect imio.ai ")
#params = urllibparse.urlencode({"lang":"zh-CN", "uid":"10001", "visitorid":"Tom", "ownership":"host", "ring":False})
reg_str = "http://www.imio.ai/ai-bot/audio_chat?lang=zh-CN&uid=10001&visitorid=Tom&ownership=host&ring=True" #port True
#reg_str = "http://www.imio.ai/ai-bot/audio_chat?lang=en-US&uid=10001&visitorid=Tom&ownership=host&ring=False" #port

body = open("test.wav", 'r').read()
#print(body)
#params = urllibparse.urlencode({'lang':'en-US'})  #zh-CN
headers = {'Content-type': 'audio/wav','samplerate': '16000'}
add_res = requests.post(reg_str, data = body, headers = headers)
print (type(add_res))
print(add_res.url)
print (add_res.text)

# data = json.loads(add_res)['data']['msg']
# p = pyaudio.PyAudio()
# stream = p.open(format=pyaudio.paInt16,
                        # channels=1,
                        # rate=16000,
                        # output=True,
                        # frames_per_buffer=1024)
# stream.write(data)
# stream.close()
'''


