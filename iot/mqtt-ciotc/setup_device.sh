#!/bin/bash
# Copyright 2018 Google Inc.
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

set -eu

DEVICE_ID=my-device

ARGUMENT_LIST=(
    "registry-name"
    "registry-region"
    "device-id"
    "telemetry-topic"
)
opts=$(getopt \
    --longoptions "$(printf "%s:," "${ARGUMENT_LIST[@]}")" \
    --name "$(basename "$0")" \
    --options "" \
    -- "$@"
)

eval set -- "$opts"

while [[ $# -gt 0 ]]; do
    case "$1" in
        --registry-name)
            REGISTRY_NAME=$2
            shift 2
            ;;

        --registry-region)
            REGISTRY_REGION=$2
            shift 2
            ;;

        --device-id)
            DEVICE_ID=$2
            shift 2
            ;;

        --telemetry-topic)
            TELEMETRY_TOPIC=$2
            shift 2
            ;;

        *)
            shift 1
#            exit -1
            ;;
    esac
done

if [ -z "${REGISTRY_NAME}" ] || [ -z "${REGISTRY_REGION}" ] \
  || [ -z "${DEVICE_ID}" ] || [ -z "${TELEMETRY_TOPIC}" ]; then
   echo "Usage $0 --registry-name CLOUD_IOT_REGISTRY --registry-region CLOUD_IOT_REGION --device-id CLOUD_IOT_DEVICE_ID --telemetry-topic TELEMETRY_TOPIC"
   exit -1
fi

if [ ! -f rsa_private.pem ]; then
    openssl req -x509 -newkey rsa:2048 -keyout rsa_private.pem -nodes  -out rsa_cert.pem -subj "/CN=unused"
fi    

HAS_REGISTRY=$(gcloud iot registries list \
  --region=${REGISTRY_REGION} \
  --filter "id = ${REGISTRY_NAME}" \
  --format "csv[no-heading](id)" | grep -c ${REGISTRY_NAME} || true)
if [ $HAS_REGISTRY == "0" ]; then
    gcloud iot registries create ${REGISTRY_NAME} \
        --region=${REGISTRY_REGION} \
        --enable-mqtt-config \
        --no-enable-http-config \
        --event-notification-config=topic=${TELEMETRY_TOPIC}
fi
HAS_DEVICE=$(gcloud iot devices list \
  --registry=${REGISTRY_NAME} \
  --region=${REGISTRY_REGION} \
  --device-ids=${DEVICE_ID} \
  --format "csv[no-heading](id)" | grep -c ${DEVICE_ID} || true)
if [ $HAS_DEVICE == "0" ]; then
    gcloud iot devices create ${DEVICE_ID} \
        --region=${REGISTRY_REGION} \
        --registry=${REGISTRY_NAME} \
        --public-key=path=./rsa_cert.pem,type=rsa-x509-pem
        
fi
