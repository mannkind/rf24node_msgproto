CC=g++
CFLAGS=-Ofast -mfpu=vfp -mfloat-abi=hard -march=armv6zk -mtune=arm1176jzf-s -Wall -std=c++0x

all: rf24node

rf24node: csiphash mosquitto rf24 rf24network
	$(CC) $(CFLAGS) -lrf24-bcm -lrf24network -l:libmosquittopp.so RF24Node.cpp -o RF24Node

csiphash:
	if [ ! -f csiphash.c ]; then \
		wget -q https://raw.githubusercontent.com/majek/csiphash/master/csiphash.c; \
	fi

mosquitto:
	if [ ! -d oojah-mosquitto-1a6b97c6bcc4 ]; then \
		wget -q https://bitbucket.org/oojah/mosquitto/get/v1.2.3.zip; \
		unzip v1.2.3.zip; \
		rm -f v1.2.3.zip; \
	fi 
	$(MAKE) -C oojah-mosquitto-1a6b97c6bcc4/lib
	sudo $(MAKE) -C oojah-mosquitto-1a6b97c6bcc4/lib install

rf24:
	if [ ! -d RF24 ]; then \
		git clone -q https://github.com/tmrh20/RF24.git rtemp; \
		mv rtemp/RPi/RF24 ./ ; \
		rm -rf rtemp; \
	fi
	cd RF24  
	$(MAKE) -C RF24 librf24-bcm
	sudo $(MAKE) -C RF24 install
	
rf24network: rf24
	if [ ! -d RF24Network ]; then \
		git clone -q https://github.com/tmrh20/RF24Network.git ntemp; \
		cd ntemp; \
		git checkout Development; \
		cd .. ; \
		mv ntemp/RPi/RF24Network ./ ; \
		rm -rf ntemp; \
	fi
	cd RF24Network
	$(MAKE) -C RF24Network librf24network
	sudo $(MAKE) -C RF24Network install

clean:
	rm -f csiphash.c v1.2.3.zip
	sudo rm -rf oojah-mosquitto-1a6b97c6bcc4 RF24 RF24Network rtemp ntemp
