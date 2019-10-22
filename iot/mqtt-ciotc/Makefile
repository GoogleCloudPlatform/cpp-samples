OPENSSL_DIR ?= /usr/local/lib
CC=gcc # gcc || clang
CCOPTIONS=-std=c11 -Wall -Wextra -pedantic -Wstrict-overflow -fno-strict-aliasing
SSLPATH=-L$(OPENSSL_DIR)

all: mqtt_ciotc.c
	$(CC) $(CCOPTIONS) $(SSLPATH) mqtt_ciotc.c $(BUILDARGS)  -ljwt -lcrypto -lssl -lpaho-mqtt3cs -o mqtt_ciotc
