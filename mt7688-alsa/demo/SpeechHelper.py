import sys
import json
import wave
import uuid
from time import time
from os import environ as env
from xml.etree import ElementTree

if sys.version.startswith('3'):
    import http.client as httpclient
    import urllib.parse as urllibparse
else:
    import httplib as httpclient
    import urllib as urllibparse
'''
IMIO_HOME = env.get('IMIO_HOME')
if IMIO_HOME is not None:
    sys.path.append(IMIO_HOME)
'''
import constants as const


class SpeechHelper(object):
    def __init__(self, lang='zh-CN', gender='female'):
        self.__lang = lang
        self.__gender = gender
        self.__apiKey = const.COG_STT_TTS_KEY
        self.__req_body = ElementTree.Element('speak', version='1.0')
        self.__req_body.set('{http://www.w3.org/XML/1998/namespace}lang', lang)
        self.__voice = ElementTree.SubElement(self.__req_body, 'voice')
        self.__voice.set('{http://www.w3.org/XML/1998/namespace}lang', lang)
        self.__voice.set('{http://www.w3.org/XML/1998/namespace}gender', gender)
        self.__voice.set('name', 'Microsoft Server Speech Text to Speech Voice ' + self.__init_voice_font())
        #for e in self.__voice:
        #    print(type(e))
        #    print(e)
        self.__accesstoken = self.__init_access_token()

    def __init_access_token(self):
        params = ""
        get_token_headers = {"Ocp-Apim-Subscription-Key": self.__apiKey}
        # AccessTokenUri = "https://api.cognitive.microsoft.com/sts/v1.0/issueToken";
        AccessTokenHost = "api.cognitive.microsoft.com"
        path = "/sts/v1.0/issueToken"
        # Connect to server to get the Access Token
        conn = httpclient.HTTPConnection(AccessTokenHost)
        conn.request("POST", path, params, get_token_headers)
        response = conn.getresponse()
        # print(response.status, response.reason)
        data = response.read()
        conn.close()
        return data.decode("UTF-8")

    def __init_voice_font(self):
        if self.__lang == 'zh-CN':
            if self.__gender == 'female':
                voicefont = '(zh-CN, Yaoyao, Apollo)'
            else:
                voicefont = '(zh-CN, Kangkang, Apollo)'
        elif self.__lang == 'en-GB':
            if self.__gender == 'female':
                voicefont = '(en-GB, Susan, Apollo)'
            else:
                voicefont = '(en-GB, George, Apollo)'
        elif self.__lang == 'es-ES':
            if self.__gender == 'female':
                voicefont = '(es-ES, Laura, Apollo)'
            else:
                voicefont = '(es-ES, Pablo, Apollo)'
        elif self.__lang == 'en-IN':
            voicefont = '(en-IN, Ravi, Apollo)'
        else:
            # default as US english
            if self.__gender == 'female':
                voicefont = '(en-US, ZiraRUS)'
            else:
                voicefont = '(en-US, BenjaminRUS)'
                #voicefont = '(en-US, Eminem)'
        return voicefont

    def text_to_speech(self, text_in, play=False):
        """
        Convert input text to speech audio and play on microphone
        :param text_in:
        :return:
        """
        if text_in is None:
            return None
        if sys.version.startswith('2'):
            if not isinstance(text_in, unicode):
                self.__voice.text = unicode(text_in, 'utf-8')
            else:
                self.__voice.text = text_in
        else:
            self.__voice.text = text_in
        headers = {"Content-type": "application/ssml+xml",
                   "X-Microsoft-OutputFormat": "riff-16khz-16bit-mono-pcm",
                   "Authorization": "Bearer " + self.__accesstoken,
                   "X-Search-AppId": "07D3234E49CE426DAA29772419F436CA",
                   "X-Search-ClientID": "1ECFAE91408841A480F00935DC390960",
                   "User-Agent": "TTSForPython"}
        # Connect to server to synthesize the wave
        conn = httpclient.HTTPConnection("speech.platform.bing.com")
        conn.request("POST", "/synthesize", ElementTree.tostring(self.__req_body), headers)
        response = conn.getresponse()
        data = response.read()
        conn.close()
        #print('stream is:')
        #print(data)

        if play:
            import pyaudio
            #print('Playing speech')
            p = pyaudio.PyAudio()
            stream = p.open(format=pyaudio.paInt16,
                        channels=1,
                        rate=16000,
                        output=True,
                        frames_per_buffer=1024)
            stream.write(data)
            wfname = "test.wav"
            wf = wave.open(wfname, 'wb')
            wf.setnchannels(1)
            wf.setsampwidth(p.get_sample_size(pyaudio.paInt16))
            wf.setframerate(16000)
            wf.writeframes(stream)
            wf.close()
            stream.close()
            #os.system('aplay /root/test.wav')
            os.system('ls')

        return data

    def speech_to_text(self, wf):
        """
        Convert the speech audio data to text
        :param wf: wave file name, or the actual stream data from a wave file
        :return: False if MS Cognitive serice returns error, otherwise the resulted texts will be returned
        """
        texts = []
        my_uuid = uuid.uuid4()
        headers = {
            'Content-type': 'audio/wav',
            # 'Content-type': 'application/octet-stream',
            'samplerate': 16000,
            'Authorization': self.__accesstoken,
            # 'codec': "audio/pcm",
        }
        params = urllibparse.urlencode({
                'version': '3.0',
                'requestid': my_uuid,
                'appid': const.APP_ID,
                'format': 'json',
                'locale': self.__lang,
                'device.os': 'ubuntu',
                'scenarios': 'smd',
                'instanceid': my_uuid
        })
        print(type(wf))
        if isinstance(wf, str):
            body = open(wf, 'rb').read()
        elif isinstance(wf, bytes):
            body = wf
        else:
            return ''.join(texts)
        conn = httpclient.HTTPConnection('speech.platform.bing.com')
        conn.request('POST', '/recognize?%s' % params, body, headers)
        response = conn.getresponse()
        print(response)
        data = json.loads(response.read().decode('utf-8'))
        result = data['header']['status']
        if result != 'success':
            print('INSIDE: STT failed')
            return None
        else:
            outputs = data['results']
            for opt in outputs:
                if float(opt['confidence']) > 0.65:
                    texts.append(opt['name'])
        return ''.join(texts)


def main():
    #wf_name = 'temp.wav'
    spch = SpeechHelper(lang='en-US', gender='male')

    text = 'Look, if you had one shot, one opportunity'
    spch.text_to_speech(text, True)
    text = 'To seize everything you ever wanted…One moment'
    spch.text_to_speech(text, True)
    text = 'Would you capture it or just let it slip?'
    spch.text_to_speech(text, True)
    text = 'His palms are sweaty, knees weak, arms are heavy'
    spch.text_to_speech(text, True)
    text = 'There\'s vomit on his sweater already, mom\'s spaghetti'
    spch.text_to_speech(text, True)
    text = 'He\'s nervous, but on the surface he looks calm and ready'
    spch.text_to_speech(text, True)
    #res_text = spch.speech_to_text(wf_name)
    #print(res_text)
    #os.remove(wf_name)


    '''
    stream.start_stream()
    while stream.is_active():
        sleep(0.1)
    stream.stop_stream()
    stream.close()
    p.terminate()
    '''


if __name__ == '__main__':
    main()
