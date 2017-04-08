#!/usr/bin/env python
#coding=utf-8
import urllib
import re

def getHtml(url):
    page = urllib.urlopen(url)
    html = page.read()
    return html

def getImg(html):
    reg = r"src='(.+?\.jpg)'"
    imgre = re.compile(reg)
    print "search for picture links..."
    imglist = re.findall(imgre,html)
    x = 0
    print "start retrieving picture..."
    try:
     for imgurl in imglist:
        print imgurl
        try:
          urllib.urlretrieve(imgurl,'%s.jpg' %x)
        except:pass
        print 'save to ',x,'.jpg'
        x+=1
    except: pass

html = getHtml("http://picture.youth.cn/")
getImg(html)



