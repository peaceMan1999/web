bin=httpserver
cgi1=test_cgi
cgi2=mspaint_cgi
cc=g++
LD_FLAGS=-std=c++11 -lpthread
src=main.cc
curr=$(shell pwd)


ALL:$(bin) $(cgi1)
.PHONY:ALL

$(bin):$(src)
	$(cc) -o $@ $^ $(LD_FLAGS)

$(cgi1):$(curr)/cgi/test_cgi.cc
	$(cc) -o $@ $^ -std=c++11
	
# $(cgi2)
# $(cgi2):$(curr)/cgi/mspaint_cgi.cc
# 	g++ -o mspaint_cgi mspaint_cgi.cc -std=c++11 -I include -L lib -lmysqlclient -lpthread -ldl -static
# -I include -L lib -lmysqlclient -lpthread -ldl -static
# -I include -L lib -lmysqlclient 
.PHONY:clean
clean:
	rm -f $(bin)
	rm -rf output

.PHONY:o
o:
	mkdir output
	cp $(bin) output
	cp -rf wwwroot output
	cp $(cgi1) output/wwwroot
	cp $(cgi2) output/wwwroot
	cp shell_cgi.sh output/wwwroot
