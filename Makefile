all:
	g++ -std=c++11 -stdlib=libc++ -L/usr/local/lib/ -lexiv2 -o faceTagConvert faceTagConvert.cpp
