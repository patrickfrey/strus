all:
	cd src; make all; cd ..
test: all
	@rm -Rf data/test
	src/createRandomTestCollection "path=data/test" 100 100 50
clean:
	cd src; make clean; cd ..
	rm -Rf data/teststorage

