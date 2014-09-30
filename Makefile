CC=g++
CFLAGS=-Ofast -mfpu=vfp -mfloat-abi=hard -march=armv6zk -mtune=arm1176jzf-s -Wall -std=c++0x -I./libs/
SOURCES=$(wildcard *.cpp)
OBJECTS=$(SOURCES:.cpp=.o)

# Generic rule
%.o: %.cpp
	$(CC) -c $(CFLAGS) $< -o $@

all: rf24node_msg

rf24node_msg: csiphash mqtt_wrapper rf24 rf24network $(OBJECTS)
	$(CC) $(CFLAGS) -lrf24-bcm -lrf24network -l:libmosquittopp.so RF24Node_MsgProto.cpp RF24NetworkWrapper.o MQTTWrapper.o RF24Node.o StringSplit.o -o RF24Node_MsgProto

libs_dir:
	mkdir -p libs

csiphash: libs_dir
	cd libs; \
	if [ ! -f csiphash.c ]; then \
		wget -q https://raw.githubusercontent.com/majek/csiphash/master/csiphash.c; \
	fi

mosquitto: libs_dir
	cd libs; \
	if [ ! -d oojah-mosquitto-1a6b97c6bcc4 ]; then \
		wget -q https://bitbucket.org/oojah/mosquitto/get/v1.2.3.zip; \
		unzip v1.2.3.zip; \
		rm -f v1.2.3.zip; \
	fi; \
	$(MAKE) -C oojah-mosquitto-1a6b97c6bcc4/lib; \
	sudo $(MAKE) -C oojah-mosquitto-1a6b97c6bcc4/lib install

rf24: libs_dir
	cd libs; \
	if [ ! -d RF24 ]; then \
		git clone -q https://github.com/tmrh20/RF24.git rtemp; \
		mv rtemp/RPi/RF24 ./ ; \
		rm -rf rtemp; \
	fi; \
	$(MAKE) -C RF24 librf24-bcm; \
	sudo $(MAKE) -C RF24 install
	
rf24network: libs_dir rf24
	cd libs; \
	if [ ! -d RF24Network ]; then \
		git clone -q https://github.com/tmrh20/RF24Network.git ntemp; \
		mv ntemp/RPi/RF24Network ./ ; \
		rm -rf ntemp; \
	fi; \
	$(MAKE) -C RF24Network librf24network; \
	sudo $(MAKE) -C RF24Network install

rf24network_wrapper: rf24network
	$(CC) $(CFLAGS) -c RF24NetworkWrapper.cpp

mqtt_wrapper: mosquitto
	$(CC) $(CFLAGS) -c MQTTWrapper.cpp

stringsplit:
	$(CC) $(CFLAGS) -c StringSplit.cpp

rf24node: rf24network_wrapper mqtt_wrapper stringsplit
	$(CC) $(CFLAGS) -c RF24Node.cpp

clean:
	rm -rf $(OBJECTS) libs/
