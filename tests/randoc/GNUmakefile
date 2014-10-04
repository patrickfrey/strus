all:
	cd src; make all; cd ..
test: all
	src/createRandomTestCollection data/teststorage 10000
clean:
	cd src; make clean; cd ..
	rm -Rf data/teststorage

