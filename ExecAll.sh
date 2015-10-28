for d in */; do 
	cp easyTests.sh "$d";
done
for d in */; do 
	chmod +x "$d"easyTests.sh
	./"$d"easyTests.sh;
done
