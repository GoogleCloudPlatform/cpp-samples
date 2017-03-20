FROM debian:jessie

# Copy in service-account.json into /home/
COPY service-account.json /home/service-account.json
ENV GOOGLE_APPLICATION_CREDENTIALS /home/service-account.json
ENV GOOGLEAPIS_GENS_PATH /home/googleapis/gens

# Dependencies
RUN \
  apt-get update && \
  apt-get -y install \
          vim \
          curl \
          git-core \
          make \
          gcc \
          g++ && \
  apt-get -y update && \
  apt-get -y install \
          build-essential \
          autoconf \
          libtool \
          pkg-config

# Build source dependencies
RUN \
  cd /home/ && \
  git clone https://github.com/grpc/grpc.git && \
  cd grpc/ && \
  git submodule update --init && \
  make && \
  make install && \
  cd third_party/protobuf/ && \
  make install && \
  cd /home/ && \
  git clone https://github.com/googleapis/googleapis.git && \
  cd googleapis/ && \
  LANGUAGE=cpp make all

# Build Speech API
Run \
  cd /home/ && \
  git clone https://github.com/GoogleCloudPlatform/cpp-docs-samples.git && \
  cd cpp-docs-samples/speech/api && \
  make

