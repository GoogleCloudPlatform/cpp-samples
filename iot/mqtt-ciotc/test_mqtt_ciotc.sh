#!/usr/bin/env bash
#
# Copyright 2019 Google LLC
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

# A script to test the sample program including pre setup.

set -eu

readonly SRC_DIR="$(cd "$(dirname "$0")/"; pwd)"

echo "================================================================"
echo "Change working directory to ${SRC_DIR} $(date)."
cd "${SRC_DIR}"

# Install gcloud sdk
mkdir -p /opt/local

curl https://sdk.cloud.google.com -o gcloud-setup.sh
bash gcloud-setup.sh --disable-prompts --install-dir=/opt/local
export PATH="/opt/local/google-cloud-sdk/bin:${PATH}"

# Activate the service account, set the default project id
gcloud auth activate-service-account --key-file=/service_account.json
gcloud config set project "${PROJECT_ID}"
gcloud config set disable_prompts true

# Set variables
readonly TELEMETRY_TOPIC="my-telemetry-topic"
readonly SUBSCRIPTION="iot-subscription"
readonly REGISTRY_NAME="my-registry"
readonly DEVICE_ID="my-device-${RANDOM}"
readonly REGISTRY_REGION="us-central1"

# Create the topic if it doesn't exist.
HAS_TOPIC=$(gcloud pubsub topics list \
  --filter "name:topics/${TELEMETRY_TOPIC}" \
  --format "csv[no-heading](name)" \
  | grep -c "topics/${TELEMETRY_TOPIC}" || true)

if [ "${HAS_TOPIC}" == "0" ]; then
  gcloud pubsub topics create "${TELEMETRY_TOPIC}"
fi

# Create the subscription if it doesn't exist.
HAS_SUBSCRIPTION=$(gcloud pubsub subscriptions list \
  --filter "name:subscriptions/${SUBSCRIPTION}" \
  --format "csv[no-heading](name)" \
  | grep -c "subscriptions/${SUBSCRIPTION}" || true)

if [ "${HAS_SUBSCRIPTION}" == "0" ]; then
  gcloud pubsub subscriptions create "${SUBSCRIPTION}" \
    --topic "${TELEMETRY_TOPIC}"
fi

# Set up the registry and the device
./setup_device.sh \
  --registry-name "${REGISTRY_NAME}" \
  --registry-region "${REGISTRY_REGION}" \
  --device-id "${DEVICE_ID}" \
  --telemetry-topic "${TELEMETRY_TOPIC}"

# Download the roots.pem
if [ ! -f roots.pem ]; then
    wget https://pki.google.com/roots.pem
fi

# Run the sample with a random message
readonly RANDOM_MESSAGE="Hello my secret number is ${RANDOM}"

./mqtt_ciotc "${RANDOM_MESSAGE}" \
  --deviceid "${DEVICE_ID}" \
  --region "${REGISTRY_REGION}" \
  --registryid "${REGISTRY_NAME}" \
  --projectid "${PROJECT_ID}" \
  --keypath rsa_private.pem \
  --algorithm RS256 \
  --rootpath roots.pem

# Delete the device because we need to use different device id
# each time, and we don't want the leak.
gcloud iot devices delete ${DEVICE_ID} \
  --region "${REGISTRY_REGION}" \
  --registry "${REGISTRY_NAME}"

# Pull the message and report the result
for i in {0..5}
do
  FOUND=$(gcloud pubsub subscriptions pull --auto-ack ${SUBSCRIPTION} \
    | grep -c "${RANDOM_MESSAGE}" || true)
  if [ "${FOUND}" == "0" ]; then
    sleep 1
  else
    echo "Found the message"
    exit 0
  fi
done

echo "Coundn't find the message"
exit 1
