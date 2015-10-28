# Script that launches a series of short and easy tests to check the validity
# of our code.

####### FUNCTIONS
function contErr(){
   printf "You failed a test, are you sure you want to continue?[y/n] " && read inp
   if [ "$inp" == "n" ] || [ "$inp" == "no" ] || [ "$inp" == "N" ]; then
   	cat easyLog.txt
		exit 1;
   fi
}
function reset(){
	rm -rf *2.jpg
	killall link_sim* 2>/dev/null 1>/dev/null
	killall receiver* 2>/dev/null 1>/dev/null
	killall sender* 2>/dev/null 1>/dev/null
	sleep 0.5
}

spinner(){
	tput civis
	local pid=$1
	local max=$2
	local TIMING=$((30 * $max))
	local delay=0.03
	local zero=0
	local spinstr='|/~\'
	printf " [      ] \b\b\b\b\b\b\b\b"
	while [ "$(ps a | awk '{print $1}' | grep $pid)" ] &&
         [ $TIMING -ge $zero ]; do
		local temp=${spinstr#?}
		#printf "%3d %c" "$TIMING" "$spinstr"
		printf "    %c" "$spinstr"
		local spinstr=$temp${spinstr%"$temp"}
		sleep $delay
		printf "\b\b\b\b\b"
   	let TIMING-=1
	done
	if [ $TIMING < $zero ]; then
		echo "The function timed out"
	fi
	printf "        \b\b\b\b\b\b\b\b\b\b"
	tput cnorm
   #Taken from : http://fitnr.com/showing-a-bash-spinner.html
}

####### TESTS
#echo "-----These are our tests for the packet managing\n" > easyLog.txt
#./test >> log.txt
echo "-----These are our tests for our connection interface" > easyLog.txt

   # Test without hindrance on big & small
   echo "Test1: Normal transfer on small file"
   reset
   ./receiver :: 64512 -f small2.jpg 2>Logs/test1_rec.txt &
   sleep 1 && ./sender ::1 64512 -f small.jpg 2>Logs/test1_sen.txt &
   spinner $! 5
   chmod +r small2.jpg
   if ( diff small.jpg small2.jpg > /dev/null); then
      echo "Passed test without hindrance on small picture" >> easyLog.txt
   else
      echo "Failed test without hindrance on small picture" >> easyLog.txt
      contErr
   fi


   echo "Test2: Normal transfer on big file"
   reset
   ./receiver :: 64512 -f big2.jpg 2>Logs/test2_rec.txt &
   sleep 1 && ./sender ::1 64512 -f big.jpg 2>Logs/test2_sen.txt &
   spinner $! 10
   chmod +r big2.jpg
   if ( diff big.jpg big2.jpg > /dev/null ); then
      echo "Passed test without hindrance on big picture" >> easyLog.txt
   else
      echo "Failed test without hindrance on big picture" >> easyLog.txt
      contErr
   fi

   # Check with package cut
   echo "Test3: Transfer with package cut on small file"
   reset
   ./link_sim -p 64321 -P 64512 -c 50 2>/dev/null 1>/dev/null &
   ./receiver :: 64512 -f small2.jpg 2>Logs/test3_rec.txt &
   sleep 0.5 && ./sender ::1 64321 -f small.jpg 2>Logs/test3_sen.txt &
   spinner $! 10
   chmod +r small2.jpg
   if ( diff small.jpg small2.jpg > /dev/null ); then
      echo "Passed test with package cut on small picture" >> easyLog.txt
   else
      echo "Failed test with package cut on small picture" >> easyLog.txt
      contErr
   fi


   # Check with package loss
   echo "Test4: Transfer with package loss on small file"
   reset
   ./link_sim -p 64321 -P 64512  -l 50 2>/dev/null 1>/dev/null &
   ./receiver :: 64512 -f small2.jpg 2>Logs/test4_rec.txt &
   sleep 0.5 && ./sender ::1 64321 -f small.jpg 2>Logs/test4_sen.txt &
   spinner $! 20
   chmod +r small2.jpg
   if ( diff small.jpg small2.jpg > /dev/null ); then
      echo "Passed test with package loss on small picture" >> easyLog.txt
   else
      echo "Failed test with package loss on small picture" >> easyLog.txt
      contErr
   fi

   # Check with package corruption
   echo "Test5: Transfer with package corruption on small file"
   reset
   ./link_sim -p 64321 -P 64512  -e 50 2>/dev/null 1>/dev/null &
   ./receiver :: 64512 -f small2.jpg 2>Logs/test5_rec.txt &
   sleep 0.5 && ./sender ::1 64321 -f small.jpg 2>Logs/test5_sen.txt &
   spinner $! 20
   chmod +r small2.jpg
   if ( diff small.jpg small2.jpg > /dev/null ); then
      echo "Passed test with package corruption on small picture" >> easyLog.txt
   else
      echo "Failed test with package corruption on small picture" >> easyLog.txt
      contErr
   fi

   # Check with all around package tampering
   echo "Test6: Transfer with all around package tampering on small file"
   reset
   ./link_sim -p 64321 -P 64512  -e 15 -c 15 -l 15 2>/dev/null 1>/dev/null &
   ./receiver :: 64512 -f big2.jpg 2>Logs/test6_rec.txt &
   sleep 0.5 && ./sender ::1 64321 -f big.jpg 2>Logs/test6_sen.txt &
   spinner $! 20
   chmod +r big2.jpg
   if ( diff big.jpg big2.jpg > /dev/null ); then
      echo "Passed test with package tampering on big picture" >> easyLog.txt
   else
      echo "Failed test with package tampering on big picture" >> easyLog.txt
      contErr
   fi
   
   # Check with big delay and jitter
   echo "Test7: Transfer with big delay on small file"
   reset
   ./link_sim -p 64321 -P 64512  -d 1000 -j 200 2>/dev/null 1>/dev/null &
   ./receiver :: 64512 -f small2.jpg 2>Logs/test7_rec.txt &
   sleep 0.5 && ./sender ::1 64321 -f small.jpg 2>Logs/test7_sen.txt &
   spinner $! 20
   chmod +r small2.jpg
   if ( diff small.jpg small2.jpg > /dev/null ); then
      echo "Passed test with big delay and jitter on small picture" >> easyLog.txt
   else
      echo "Failed test with big delay and jitter on small picture" >> easyLog.txt
      contErr
   fi
   reset

   # Check with big delay and jitter
   echo "Test8: Transfer with big delay and packet tampering on small file"
   reset
   ./link_sim -p 64321 -P 64512  -d 350 -j 200 -e 15 -c 15 -l 15 2>/dev/null 1>/dev/null &
   ./receiver :: 64512 -f small2.jpg 2>Logs/test8_rec.txt &
   sleep 0.5 && ./sender ::1 64321 -f small.jpg 2>Logs/test8_sen.txt &
   spinner $! 30
   chmod +r small2.jpg
   if ( diff small.jpg small2.jpg > /dev/null ); then
      echo "Passed test with mild network simulation on small picture" >> easyLog.txt
   else
      echo "Failed test with mild network simulation on small picture" >> easyLog.txt
      contErr
   fi
   reset
   
   # Closest simulation to really shitty network
#   echo "Test9: Transfer with big delay an packet tampering on big file"
#   reset
#   ./link_sim -p 64321 -P 64512  -d 1000 -j 50 -e 15 -c 25 -l 15 2>/dev/null 1>/dev/null &
#   ./receiver :: 64512 -f big2.jpg 2>Logs/test9_rec.txt &
#   sleep 0.5 && ./sender ::1 64321 -f big.jpg 2>Logs/test_9sen.txt &
#   spinner $! 200
#   chmod +r big2.jpg
#   if ( diff big.jpg big2.jpg > /dev/null ); then
#      echo "Passed test with harsh network simulation on big picture" >> easyLog.txt
#   else
#      echo "Failed test with harsh network simulation on big picture" >> easyLog.txt
#      contErr
#   fi
#   reset

   # Printing test results
   echo "----------These are the test results"
   cat easyLog.txt



















