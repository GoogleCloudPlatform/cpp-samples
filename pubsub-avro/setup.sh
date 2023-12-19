#!/bin/bash
#
# Copyright 2023 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# Create the initial schema.
gcloud pubsub schemas create ${GOOGLE_CLOUD_SCHEMA_NAME} \
  --type=avro \
  --definition-file=${GOOGLE_CLOUD_SCHEMA_FILE1}

# Create the topic with the schema.
gcloud pubsub topics create ${GOOGLE_CLOUD_TOPIC} \
  --message-encoding=json \
  --schema=${GOOGLE_CLOUD_SCHEMA_NAME} \
  --schema-project=${GOOGLE_CLOUD_PROJECT} \
  --project=${GOOGLE_CLOUD_PROJECT}

# Create the subscription.
gcloud pubsub subscriptions create ${GOOGLE_CLOUD_SUBSCRIPTION} --topic=${GOOGLE_CLOUD_TOPIC}

# Send a message in the format of the initial schema.
gcloud pubsub topics publish ${GOOGLE_CLOUD_TOPIC} \
  --project ${GOOGLE_CLOUD_PROJECT} \
  --message '{"name": "New York", "post_abbr": "NY"}'

# Commit the revised schema.
gcloud pubsub schemas commit ${GOOGLE_CLOUD_SCHEMA_NAME} \
  --type=avro \
  --definition-file=${GOOGLE_CLOUD_SCHEMA_FILE2}

# Send a message in the format of the revised schema.
gcloud pubsub topics publish ${GOOGLE_CLOUD_TOPIC} \
  --project ${GOOGLE_CLOUD_PROJECT} \
  --message '{"name": "New York", "post_abbr": "NY", "population": 10000}'
