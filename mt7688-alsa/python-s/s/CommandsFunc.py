# -*- coding: utf-8 -*-
import os
from time import sleep

ALL_FUNCS_NAME = ["host_scene1", "host_scene2", "host_scene3", "host_scene4", "host_scene5",
                  "test_1", "test_2", "test_3"]
ALL_FUNCS = {}



#------------------interface
def command(scene_name, vistor_type="host"):
    def deco_(func):
        def wraps(*args, **kwargs):
            print(func.__name__)
            ALL_FUNCS[vistor_type + scene_name] = func
        return wraps
    return deco_

def play_media(visit, is_play=True, text=None, filename="test.wav"):
    if text:
        print({"Text": text})
        visit.tts(visit.ask_and_answer({"Text": text}), is_play)
        return
    spt = visit.stt(filename)
    visit.tts(visit.ask_and_answer({"Text": spt}), is_play)


def bell_ring(visit, is_play=True, libsc=None, cmd="BellRingFlag"):#libsc is not in this environment.so ignore it
    ring_bell = {cmd: is_play}
    print(ring_bell)
    # waiting for push the ring button
    print ("Waiting the button push!")
    if libsc:
        while (0 == libsc.get_button()):
            sleep(0.1)
    print ("The button push!")
    visit.tts(visit.ask_and_answer(ring_bell), is_play)
    sleep(0.5)

def open_expressbox(visit, is_play=True, topic="", msg="", cmd="LockRecogDeliveryFlag"):
    test = True
    from TrioAIHelper import SinglePub
    while True:
        SinglePub.single_publish(topic, message=msg)
        sleep(5)
        if test:
            break

    recog = {cmd: is_play}
    visit.tts(visit.ask_and_answer(recog), is_play)
    sleep(0.5)

def passwd_valid(visit, is_play=True, cmd="PasswdValidFlag"):
    unclocked = {cmd: is_play}
    print(unclocked)
    visit.tts(visit.ask_and_answer(unclocked), is_play)


@command("scene1", "host")
def host_scene1(visit, is_play=True):
    passwd_valid(visit, is_play=is_play)


@command("scene2", "host")
def host_scene2(visit, is_play=True):
    bell_ring(visit, is_play=is_play)
    play_media(visit)

@command("scene3", "host")
def host_scene3(visit, is_play=True):
    bell_ring(visit, is_play=is_play)
    tmp_text = "Yes, I forgot my password"
    play_media(visit, text=tmp_text)
    passwd_valid(visit, is_play=is_play)

@command("scene4", "host")
def host_scene4(visit, is_play=True):
    bell_ring(visit, is_play=is_play)
    tmp_text = "Yes, I forgot my password"
    play_media(visit, text=tmp_text)
    tmp_text = "I forgot my phone"
    play_media(visit, text=tmp_text)

@command("scene5", "host")
def host_scene5(visit, is_play=True):
    bell_ring(visit, is_play=is_play)
    tmp_text = "i just try the bell"
    play_media(visit, text=tmp_text)

@command("test", "host")
def test_1(visit, is_play=True):
    bell_ring(visit, is_play=is_play)
    tmp_text = "Yes, I forgot my password"
    play_media(visit, text=tmp_text)
    passwd_valid(visit, is_play=is_play)

@command("test", "family")
def test_2(visit, is_play=True):
    pass

@command("test", "stranger")
def test_3(visit, is_play=True, topic="mt7687expressbox", msg="open the expressbox"):
    open_expressbox(visit, is_play=is_play, topic=topic, msg=msg)
    tmp_text = "I am a post man"
    play_media(visit, text=tmp_text)
    tmp_text = "I am here to send a package"
    play_media(visit, text=tmp_text)



def init_all_funcs():
    for func_name in ALL_FUNCS_NAME:
        print(func_name)
        exec(func_name + "()")


init_all_funcs()
