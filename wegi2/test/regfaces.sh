#!/bin/sh
#screen -dmS C1 ./test_surfman -c -S 240,140 -p 10,10

screen -dmS C1 ./test_surfman -c -S 500,300 -p 10,10
sleep 1
screen -dmS C2TM ./surf_timer -c -S 500,300 -p 30,50
sleep 1
screen -dmS C3 ./test_surfman -c -S 500,300 -p 50,90
sleep 1
screen -dmS C4TRI ./surf_triangles -c -S 500,300 -p 70,130
sleep 1
screen -dmS C5 ./test_surfman -c -S 500,300 -p 90,170
sleep 1
screen -dmS C6 ./test_surfman -c -S 500,300 -p 110,210

