# Copyright 2017 Google Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

FROM ubuntu:16.04
RUN apt-get update && \
    apt-get install -y build-essential gcc clang git libssl-dev autoconf libtool cmake doxygen pkg-config unzip wget && \
    mkdir /dependencies && \
    cd /dependencies  && git clone https://github.com/akheron/jansson  && \
    cd jansson && cmake . && make && make install  && \
    cd /dependencies  && git clone https://github.com/openssl/openssl  && \
    cd openssl && ./config && make && make install  && \
    cd /dependencies  && git clone https://github.com/benmcollins/libjwt  && \
    cd libjwt && autoreconf -i && ./configure && make && make install  && \
    cd /dependencies  && git clone https://github.com/eclipse/paho.mqtt.c.git  && \
    cd paho.mqtt.c && cmake -DPAHO_WITH_SSL=TRUE -DPAHO_BUILD_DOCUMENTATION=TRUE -DPAHO_BUILD_SAMPLES=TRUE . && make all && make install  && \
    rm -rf /var/lib/apt/lists/* && \
    export LD_LIBRARY_PATH=/usr/local/lib
    
VOLUME /src

# gcc mqtt_ciotc.c -L/dependencies/openssl -lssl -lcrypto -lpaho-mqtt3cs -L/dependencies/libjwt -ljwt -o mqtt_ciotc
# openssl req -x509 -newkey rsa:2048 -keyout rsa_private.pem -nodes  -out rsa_cert.pem -subj "/CN=unused"