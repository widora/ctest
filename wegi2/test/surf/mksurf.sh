#!/bin/sh
screen -dmS Mplayer ./test_surfuser -n "Mplayer"
screen -dmS Radio ./test_surfuser -n "NetRadio"
screen -dmS MAIL ./test_surfuser -n "邮件"
screen -dmS EDIT ./test_surfuser -n "WEditor" 
screen -dmS WIDORA ./test_surfuser -n "WidoraNEO"
screen -dmS GAME ./test_surfuser -n "游戏盒子"
screen -dmS TM ./surf_timer
#screen -dmS ALARM ./surf_alarm -p 30,30 -t "01:02:03" "时间刀落，快点哦! .."
