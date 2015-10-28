for d in */; do 
	echo "Copy to $d"
	cp easyTests.sh big.jpg small.jpg link_sim "$d";
	mkdir "$d"Logs 2>/dev/null
done
for d in */; do 
	echo "In directory $d"
	chmod +x "$d"easyTests.sh
	cd "$d" 
	./easyTests.sh 2>/dev/null
	cd ..
done
