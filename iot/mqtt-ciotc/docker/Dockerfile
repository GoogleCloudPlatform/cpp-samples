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

#Download base image ubuntu 16.04
FROM ubuntu:16.04

# Update Ubuntu Software repository
RUN apt-get update

# Install nginx, php-fpm and supervisord from ubuntu repository
RUN apt-get install -y build-essential gcc clang git libssl-dev autoconf libtool cmake doxygen pkg-config unzip wget

# Alternative way of getting libjwt
# RUN add-apt-repository ppa:ben-collins/libjwt

# Another way
RUN git clone https://github.com/akheron/jansson
RUN cd jansson && cmake . && make && make install
RUN git clone https://github.com/openssl/openssl
RUN cd openssl && ./config && make && make install
RUN git clone https://github.com/benmcollins/libjwt
RUN cd libjwt && autoreconf -i && ./configure && make && make install
RUN git clone https://github.com/eclipse/paho.mqtt.c.git
RUN cd paho.mqtt.c && cmake -DPAHO_WITH_SSL=TRUE -DPAHO_BUILD_DOCUMENTATION=TRUE -DPAHO_BUILD_SAMPLES=TRUE . && make all && make install
RUN wget wheresgus.com/mqtt-ciotc.zip && unzip mqtt-ciotc.zip
RUN cd mqtt-ciotc && gcc mqtt_ciotc.c -L/openssl -lssl -lcrypto -lpaho-mqtt3cs -ljwt -o mqtt_ciotc
