bin=httpserver
cgi=test_cgi
cc=g++
LD_FLAGS=-std=c++11 -lpthread
src=main.cc
curr=$(shell pwd)


ALL:$(bin) $(cgi)
.PHONY:ALL

$(bin):$(src)
	$(cc) -o $@ $^ $(LD_FLAGS)

$(cgi):$(curr)/cgi/test_cgi.cc
	$(cc) -o $@ $^ -std=c++11

.PHONY:clean
clean:
	rm -f $(bin)
	rm -rf output

.PHONY:o
o:
	mkdir output
	cp $(bin) output
	cp -rf wwwroot output
	cp $(cgi) output/wwwroot
	cp shell_cgi.sh output/wwwroot
