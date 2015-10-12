CC=g++
CFLAGS=-Ofast -mfpu=vfp -mfloat-abi=hard -march=armv6zk -mtune=arm1176jzf-s -Wall -std=c++0x
SOURCES=$(wildcard *.cpp)
OBJECTS=$(SOURCES:.cpp=.o)

# Generic rule
%.o: %.cpp
	$(CC) -c $(CFLAGS) $< -o $@

all: rf24node_msgproto
deps: librf24network libmosquitto librabbitmq libamqpcpp 

rf24node_msgproto: deps $(OBJECTS) 
	$(CC) $(CFLAGS) -lrf24-bcm -lrf24network -l:libmosquittopp.so -lrabbitmq $(OBJECTS) libs/amqpcpp/libamqpcpp.a -o RF24Node_MsgProto

libmosquitto:
	$(MAKE) -C libs/mosquitto/lib && sudo $(MAKE) -C libs/mosquitto/lib install

libamqpcpp: 
	$(MAKE) -C libs/amqpcpp 

librabbitmq: 
	mkdir -p libs/rabbitmq-c/build && cd libs/rabbitmq-c/build && cmake .. && cmake --build . --config Release --target librabbitmq && sudo $(MAKE) -C librabbitmq install && sudo ln -sf /usr/local/lib/arm-linux-gnueabihf/librabbitmq.so.1 /usr/local/lib/librabbitmq.so.1 && sudo ldconfig 

librf24: 
	$(MAKE) -C libs/RF24 && sudo $(MAKE) -C libs/RF24 install
	
librf24network: librf24
	mkdir -p libs/RF24Network/RPi/RF24 && cp libs/RF24/*.h libs/RF24Network/RPi/RF24 && $(MAKE) -C libs/RF24Network/RPi/RF24Network librf24network && sudo $(MAKE) -C libs/RF24Network/RPi/RF24Network install

cleandeps: 
	$(MAKE) -C libs/mosquitto/lib clean
	$(MAKE) -C libs/amqpcpp clean
	$(MAKE) -C libs/RF24 clean
	$(MAKE) -C libs/RF24Network/RPi/RF24Network clean
	rm -rf libs/rabbitmq-c/build
	rm -rf libs/RF24Network/RPi/RF24
	rm -f libs/RF24/arch/includes.h

clean:
	rm -f $(OBJECTS)
	rm -f RF24Node_MsgProto
