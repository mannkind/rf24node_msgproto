CC=g++
CFLAGS=-Ofast -mfpu=vfp -mfloat-abi=hard -march=armv6zk -mtune=arm1176jzf-s -Wall -std=c++0x
SOURCES=$(wildcard *.cpp)
OBJECTS=$(SOURCES:.cpp=.o)

# Generic rule
%.o: %.cpp
	$(CC) -c $(CFLAGS) $< -o $@

all: rf24node_msgproto
deps: csiphash libamqpcpp 
libdeps: librf24network libmosquitto librabbitmq

rf24node_msgproto: deps $(OBJECTS) 
	$(CC) $(CFLAGS) -lrf24-bcm -lrf24network -l:libmosquittopp.so -lrabbitmq RF24Node_MsgProto.cpp RF24NetworkWrapper.o MQTTWrapper.o AMQPWrapper.o RF24Node.o StringSplit.o libamqpcpp.a -o RF24Node_MsgProto

libs_dir:
	mkdir -p libs

csiphash:
	if [ ! -f csiphash.c ]; then \
		wget -q https://raw.githubusercontent.com/majek/csiphash/master/csiphash.c; \
	fi

libmosquitto: libs_dir
	cd libs; \
	if [ ! -d mosquitto-1.3.5 ]; then \
		wget -q http://mosquitto.org/files/source/mosquitto-1.3.5.tar.gz; \
		tar zxvf mosquitto-1.3.5.tar.gz; \
		rm -f mosquitto-1.3.5.tar.gz; \
	fi; \
	$(MAKE) -C mosquitto-1.3.5/lib; \
	sudo $(MAKE) -C mosquitto-1.3.5/lib install

libamqpcpp: libs_dir
	cd libs ; \
	if [ ! -d amqpcpp ]; then \
		git clone git@github.com:akalend/amqpcpp.git ; \
	fi; \
	$(MAKE) -C amqpcpp ; \
	cp amqpcpp/libamqpcpp.a ../

librabbitmq: libs_dir
	cd libs; \
	if [ ! -d rabbitmq-c ]; then \
		git clone git@github.com:alanxz/rabbitmq-c.git; \
	fi; \
		cd rabbitmq-c ; \
		git checkout e1746f92585d59364fc48b6305ce25d7fc86c2a4; \
	mkdir build;\
	cd build; \
	cmake ..; \
	cmake --build . --config Release --target librabbitmq; \
	sudo $(MAKE) -C librabbitmq install; \
	sudo ln -sf /usr/local/lib/arm-linux-gnueabihf/librabbitmq.so.1 /usr/local/lib/librabbitmq.so.1
	sudo ldconfig 

librf24: libs_dir
	cd libs; \
	if [ ! -d RF24 ]; then \
		git clone -q https://github.com/tmrh20/RF24.git RF24; \
	fi; \
	$(MAKE) -C RF24; \
	sudo $(MAKE) -C RF24 install
	
librf24network: libs_dir librf24
	cd libs; \
	if [ ! -d RF24Network ]; then \
		git clone -q https://github.com/tmrh20/RF24Network.git ntemp; \
		mv ntemp/RPi/RF24Network ./ ; \
		rm -rf ntemp; \
	fi; \
	$(MAKE) -C RF24Network librf24network; \
	sudo $(MAKE) -C RF24Network install

rf24network_wrapper:
	$(CC) $(CFLAGS) -c RF24NetworkWrapper.cpp

mqtt_wrapper: 
	$(CC) $(CFLAGS) -c MQTTWrapper.cpp

amqp_wrapper: 
	$(CC) $(CFLAGS) -c AMQPWrapper.cpp

stringsplit:
	$(CC) $(CFLAGS) -c StringSplit.cpp

rf24node: csiphash rf24network_wrapper mqtt_wrapper amqp_wrapper stringsplit
	$(CC) $(CFLAGS) -c RF24Node.cpp

clean_deps: 
	rm -rf csiphash libamqpcpp.a
clean_libdeps:
	rm -rf libs/
clean: clean_deps clean_libdeps
	rm -rf $(OBJECTS)
