#!/usr/bin/env python
#---------------------------------------------
#  ----  Author: www.BIGIOT.net  ----
#    Chat with tuling AI
#---------------------------------------------

# -*- coding: utf-8 -*-

import requests
import json

#---- get your key after registering in www.tuling123.com -----
key = 'xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx'

def chitchat(chats):
    config =  {"key":key,"info":chats}
    postdata = json.dumps(config)

    r = requests.post("http://www.tuling123.com/openapi/api",data=postdata)
    data = r.text
    updata = json.loads(data)
    return updata['text']
