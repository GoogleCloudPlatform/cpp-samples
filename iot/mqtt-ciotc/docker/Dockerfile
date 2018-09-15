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

FROM ubuntu:16.04 as build-dependencies
RUN apt-get update
RUN apt-get install -y build-essential gcc clang git libssl-dev autoconf libtool cmake doxygen pkg-config unzip wget 
RUN mkdir /dependencies 
RUN cd /dependencies  && git clone https://github.com/akheron/jansson  
RUN cd /dependencies/jansson && cmake . && make && make install  
RUN cd /dependencies  && git clone https://github.com/openssl/openssl  
RUN cd /dependencies/openssl && ./config && make && make install  
RUN cd /dependencies  && git clone https://github.com/benmcollins/libjwt  
RUN cd /dependencies/libjwt && autoreconf -i && ./configure && make && make install  
RUN cd /dependencies  && git clone https://github.com/eclipse/paho.mqtt.c.git  
RUN cd /dependencies/paho.mqtt.c && cmake -DPAHO_WITH_SSL=TRUE -DPAHO_BUILD_DOCUMENTATION=TRUE -DPAHO_BUILD_SAMPLES=TRUE . && make all && make install  

FROM ubuntu:16.04

RUN export LD_LIBRARY_PATH=/usr/local/lib && \
    apt-get update && apt-get install -y gcc && \
    rm -rf /var/lib/apt/lists/*
COPY --from=build-dependencies /usr/local /usr/local    
VOLUME /src

