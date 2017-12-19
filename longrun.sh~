#!/bin/bash

for i in 1 2 3 4 5 6 7 8 9 10
 do

	sleep 2 && rcsoccersim  & echo $!	
	PID1=$!  
	echo "Server PID : $PID1"
  
	sleep 10 && sh /home/navid/robo/learned_agent/src/startml.sh & echo $!
	PID2=$!  
	echo "LearnedAgent PID : $PID2"
	sleep 10 && sh /home/navid/robo/agent2d-3.1.1/src/start.sh & echo $!
	PID3=$!  
	echo "Agent2d PID : $PID3"

	sleep 20 && xdotool key Control_L+k
	sleep 350
	sleep 20 && xdotool key Control_L+k
	sleep 350

  
	killall rcssserver
	killall	rcssmonitor
	killall sample_player
	#kill -INT -$(ps -o pgid= $PID1 | grep -o '[0-9]*')
	#kill -INT -$(ps -o pgid= $PID2 | grep -o '[0-9]*')
	#kill -INT -$(ps -o pgid= $PID3 | grep -o '[0-9]*')
	#/usr/bin/rcssserver
 done

