# Message Queue Telemetry Transport (MQTT) client for Google Cloud IoT Core
This sample app demonstrates connecting to Google Cloud IoT Core and publishing
a single message.

# Building
To build the sample, you must first download, build, and install the following
projects:

* [Jansson](https://github.com/akheron/jansson)
* [libjwt](https://github.com/benmcollins/libjwt)
* [OpenSSL](https://github.com/openssl/openssl)
* [paho.mqtt.c](https://github.com/eclipse/paho.mqtt.c)
* [Check](https://github.com/libcheck/check) (Optional, used for build tests)

After you have installed all the dependencies, you can build the sample as:

    gcc mqtt_ciotc.c -lssl -lcrypto -lpaho-mqtt3cs -ljwt -o mqtt_ciotc

or by invoking `make`.

Due to conflicting libraries existing on your build machine, you may need to
specify the library paths, for example, if you installed the dependencies in
$HOME/code, you could do the following to help the linker find the correct
libraries:

    gcc mqtt_ciotc.c -L$HOME/code/openssl -lssl -lcrypto -lpaho-mqtt3cs -L$HOME/code/libjwt -ljwt -o mqtt_ciotc

For simplifying the build process, a Docker image is provided that generates
the build configuration. After installing Docker, browse to the `docker` folder
and run:

    docker build -t your-docker-tag .

It will take a little while for the Docker image to build. After the Docker
image is built, you can access the builder as:

    docker run -t -i your-docker-tag /bin/bash

# Running
After you have successfully built the sample, you are almost ready to run the
sample.

1. Setup your libraries:

    export LD_LIBRARY_PATH=/usr/local/lib
    export LD_RUN_PATH=/usr/local/lib

2. Browse to the sample folder

    cd mqtt-ciotc

3. Run the sample
    ./mqtt_ciotc <message> \
        --deviceid <your device id>\
        --region <e.g. us-central1>\
        --registryid <your registry id>\
        --projectid <your project id>\
        --ecpath <e.g. ./ec_private.pem>\
        --rootpath <e.g. ./roots.pem>

You must provide the deviceid, registryid, and projectid parameters as they are
used in calculating the client ID used for connecting to the MQTT bridge. The
ecpath parameter should point to your private EC key created when you registered
your device. The rootpath parameter specifies the roots.pem file that can be
downloaded from https://pki.google.com/roots.pem. For example:

    wget https://pki.google.com/roots.pem

The following example demonstrates usage of the sample if my device ID is
my-device-id, registry ID is my-registry, and project ID is blue-ocean-123.

    mqtt_ciotc "Hello world!" --deviceid my-device-id --registryid my-registry\
        --projectid blue-ocean-123
