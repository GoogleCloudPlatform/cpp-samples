# Copyright 2016 Google Inc.
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
GOOGLEAPIS_GENS_PATH ?= $(HOME)/gitrepos/googleapis/gens
GOOGLEAPIS_CCS = $(shell find $(GOOGLEAPIS_GENS_PATH) -name '*.pb.cc')
HOST_SYSTEM = $(shell uname | cut -f 1 -d_)
SYSTEM ?= $(HOST_SYSTEM)
CXX = g++
CPPFLAGS += -I/usr/local/include -pthread -I$(GOOGLEAPIS_GENS_PATH)
CXXFLAGS += -std=c++11
ifeq ($(SYSTEM),Darwin)
LDFLAGS += -L/usr/local/lib `pkg-config --libs grpc++ grpc`       \
           -lgrpc++_reflection                                    \
           -lprotobuf -lpthread -ldl
else
LDFLAGS += -L/usr/local/lib `pkg-config --libs grpc++ grpc`       \
           -Wl,--no-as-needed -lgrpc++_reflection -Wl,--as-needed \
           -lprotobuf -lpthread -ldl
endif

.PHONY: all
all: transcribe async_transcribe streaming_transcribe streaming_transcribe_singlethread

googleapis.ar: $(GOOGLEAPIS_CCS:.cc=.o)
	ar r $@ $?

transcribe: transcribe.o parse_arguments.o googleapis.ar
	$(CXX) $^ $(LDFLAGS) -o $@

async_transcribe: async_transcribe.o parse_arguments.o googleapis.ar
	$(CXX) $^ $(LDFLAGS) -o $@

streaming_transcribe: streaming_transcribe.o parse_arguments.o googleapis.ar
	$(CXX) $^ $(LDFLAGS) -o $@

streaming_transcribe_singlethread: streaming_transcribe_singlethread.o \
		parse_arguments.o googleapis.ar
	$(CXX) $^ $(LDFLAGS) -o $@

run_tests: all
	./transcribe -b 16000 resources/audio.raw
	./transcribe --bitrate 16000 resources/audio2.raw
	./transcribe resources/audio.flac
	./transcribe resources/quit.raw
	./streaming_transcribe -b 16000 resources/audio.raw
	./streaming_transcribe --bitrate 16000 resources/audio2.raw
	./streaming_transcribe resources/audio.flac
	./streaming_transcribe resources/quit.raw
	./streaming_transcribe_singlethread -b 16000 resources/audio.raw
	./streaming_transcribe_singlethread --bitrate 16000 resources/audio2.raw
	./streaming_transcribe_singlethread resources/audio.flac
	./streaming_transcribe_singlethread resources/quit.raw
	./async_transcribe gs://$(GOOGLE_STORAGE_BUCKET)/audio.raw
	./transcribe gs://$(GOOGLE_STORAGE_BUCKET)/audio.raw

clean:
	rm -f *.o transcribe async_transcribe streaming_transcribe streaming_transcribe_singlethread googleapis.ar $(GOOGLEAPIS_CCS:.cc=.o)

