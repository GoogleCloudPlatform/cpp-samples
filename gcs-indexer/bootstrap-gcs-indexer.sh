#!/usr/bin/env bash
# Copyright 2020 Google LLC
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

if [[ -z "${GOOGLE_CLOUD_PROJECT:-}" ]]; then
  echo "You must set GOOGLE_CLOUD_PROJECT to the project id hosting the GCS" \
      "metadata indexer"
  exit 1
fi

if !type gcloud >/dev/null 2>&1; then
  echo "gcloud command not found."
  echo "This script uses gcloud extensively, please install it and add it to your PATH"
  echo "  https://cloud.google.com/sdk/install"
  exit 1
fi

if !type kubectl >/dev/null 2>&1; then
  echo "kubectl command not found. Trying to install via gcloud"
  if gcloud components install kubectl; then
    echo "Success"
  else
    echo "kubectl not found and could not be installed."
    echo "Please install it manually and add it to your PATH"
    exit 1
  fi
fi

GOOGLE_CLOUD_PROJECT="${GOOGLE_CLOUD_PROJECT:-}"
readonly GOOGLE_CLOUD_PROJECT
GOOGLE_CLOUD_REGION="${REGION:-us-central1}"
readonly GOOGLE_CLOUD_REGION
GOOGLE_CLOUD_SPANNER_INSTANCE="${GOOGLE_CLOUD_SPANNER_INSTANCE:-gcs-indexer}"
readonly GOOGLE_CLOUD_SPANNER_INSTANCE
GOOGLE_CLOUD_SPANNER_DATABASE="${GOOGLE_CLOUD_SPANNER_DATABASE:-gcs-indexer-db}"
readonly GOOGLE_CLOUD_SPANNER_DATABASE

# Enable (if they are not enabled already) the services will we will need
gcloud services enable spanner.googleapis.com \
    "--project=${GOOGLE_CLOUD_PROJECT}"
gcloud services enable cloudbuild.googleapis.com \
    "--project=${GOOGLE_CLOUD_PROJECT}"
gcloud services enable container.googleapis.com \
    "--project=${GOOGLE_CLOUD_PROJECT}"
gcloud services enable containerregistry.googleapis.com \
    "--project=${GOOGLE_CLOUD_PROJECT}"
gcloud services enable run.googleapis.com \
    "--project=${GOOGLE_CLOUD_PROJECT}"
gcloud services enable pubsub.googleapis.com \
    "--project=${GOOGLE_CLOUD_PROJECT}"

# Create a Cloud Spanner instance to host the GCS metadata index
gcloud spanner instances create "${GOOGLE_CLOUD_SPANNER_INSTANCE}" \
    "--project=${GOOGLE_CLOUD_PROJECT}" \
    "--config=regional-${GOOGLE_CLOUD_REGION}" \
    --description="Store the GCS metadata index" \
    --nodes=3

# Build the Docker Images
gcloud builds submit \
    "--project=${GOOGLE_CLOUD_PROJECT}" \
    "--substitutions=SHORT_SHA=$(git rev-parse --short HEAD)" \
    "--config=cloudbuild.yaml"

# Create a service account for the Admin Deployment
gcloud iam service-accounts create gcs-index-admin \
    "--project=${GOOGLE_CLOUD_PROJECT}" \
    --description="Perform administrative updates to the GCS metadata index"

# Create a service account that will update the index
readonly SA_ID="gcs-index-updater"
readonly SA_NAME="${SA_ID}@${GOOGLE_CLOUD_PROJECT}.iam.gserviceaccount.com"

if gcloud iam service-accounts describe "${SA_NAME}" \
               "--project=${GOOGLE_CLOUD_PROJECT}" >/dev/null 2>&1; then
    echo "The service account ${SA_ID} already exists"
else
    gcloud iam service-accounts create "${SA_ID}" \
        "--project=${GOOGLE_CLOUD_PROJECT}" \
        --description="Perform updates to the GCS metadata index"

    # Grant this service account permission to just update the table (but not
    # create other tables or databases)
    gcloud spanner instances add-iam-policy-binding \
        "${GOOGLE_CLOUD_SPANNER_INSTANCE}" \
        "--project=${GOOGLE_CLOUD_PROJECT}" \
        "--member=serviceAccount:${SA_NAME}" \
        "--role=roles/spanner.databaseUser"
fi

# Create the Cloud Run deployment to update the index
gcloud beta run deploy gcs-indexer-pubsub-handler \
    "--project=${GOOGLE_CLOUD_PROJECT}" \
    "--service-account=${SA_NAME}" \
    "--set-env-vars=SPANNER_PROJECT=${GOOGLE_CLOUD_PROJECT},SPANNER_INSTANCE=${GOOGLE_CLOUD_SPANNER_INSTANCE},SPANNER_DATABASE=${GOOGLE_CLOUD_SPANNER_DATABASE}" \
    "--image=gcr.io/${GOOGLE_CLOUD_PROJECT}/cpp-samples/gcs-indexer/pubsub-handler:latest" \
    "--region=${GOOGLE_CLOUD_REGION}" \
    "--platform=managed" \
    "--no-allow-unauthenticated"

# Capture the service URL.
SERVICE_URL=$(gcloud beta run services list \
    "--project=${GOOGLE_CLOUD_PROJECT}" \
    "--platform=managed" \
    '--format=csv[no-heading](URL)' \
    "--filter=SERVICE:gcs-indexer-pubsub-handler")
readonly SERVICE_URL

# Create the Cloud Pub/Sub topic and configuration
gcloud pubsub topics create gcs-indexer-updates \
    "--project=${GOOGLE_CLOUD_PROJECT}"

# Find out the project number as the service accounts used
# are named based on this number.
PROJECT_NUMBER=$(gcloud projects describe ${GOOGLE_CLOUD_PROJECT} \
    --format='csv[no-heading](projectNumber)')
readonly PROJECT_NUMBER

# Grant the Cloud Pub/Sub service account permissions to make calls
# on behalf of another account.
PUBSUB_SA_NAME="service-${PROJECT_NUMBER}@gcp-sa-pubsub.iam.gserviceaccount.com"
readonly PUBSUB_SA_NAME

# To call the Cloud Run endpoint we need a OpenID token. We cannot obtain
# such tokens for the for the Pub/Sub service account. We can create a special
# service account, grant it access to call Cloud Run, and grant the Pub/Sub
# service account permissions to create tokens for this account.
gcloud projects add-iam-policy-binding ${GOOGLE_CLOUD_PROJECT} \
    "--member=serviceAccount:${PUBSUB_SA_NAME}" \
    "--role=roles/iam.serviceAccountTokenCreator"

INVOKER_ID="gcs-indexer-run-invoker"
INVOKER_NAME="${INVOKER_ID}@${GOOGLE_CLOUD_PROJECT}.iam.gserviceaccount.com"
readonly INVOKER_ID
readonly INVOKER_NAME

if gcloud iam service-accounts describe "$INVOKER_NAME}" >/dev/null 2>&1; then
    echo "The service account ${INVOKER_ID} already exists"
else
    gcloud iam service-accounts create "${INVOKER_ID}" \
        "--project=${GOOGLE_CLOUD_PROJECT}" \
        --description="Calls the gcs-indexer in Cloud Run from Cloud Pub/Sub"
fi

gcloud beta run services add-iam-policy-binding gcs-indexer-pubsub-handler \
    "--project=${GOOGLE_CLOUD_PROJECT}" \
    "--region=${GOOGLE_CLOUD_REGION}" \
    "--platform=managed" \
    "--member=serviceAccount:${INVOKER_NAME}" \
    "--role=roles/run.invoker"

# Configure Cloud Pub/Sub to push updates to our Cloud Run Deployment
if gcloud pubsub subscriptions describe \
    "projects/${GOOGLE_CLOUD_PROJECT}/subscriptions/run-subscription" >/dev/null 2>&1; then
    echo "The subscription already exists"
else
    gcloud beta pubsub subscriptions create run-subscription \
        "--topic-project=${GOOGLE_CLOUD_PROJECT}" \
        "--topic=gcs-indexer-updates" \
        "--push-endpoint=${SERVICE_URL}/" \
        "--push-auth-service-account=${INVOKER_NAME}"
fi

# Create the GKE cluster
if gcloud container clusters describe gcs-indexer \
      "--project=${GOOGLE_CLOUD_PROJECT}" \
      "--region=${GOOGLE_CLOUD_REGION}" >/dev/null 2>&1; then
  echo "Cluster gcs-indexing already exists, reusing"
else
  gcloud container clusters create gcs-indexer \
      "--project=${GOOGLE_CLOUD_PROJECT}" \
      "--region=${GOOGLE_CLOUD_REGION}" \
      "--preemptible" \
      "--min-nodes=0" \
      "--max-nodes=20" \
      "--enable-autoscaling"
fi

gcloud container clusters get-credentials gcs-indexer \
      "--project=${GOOGLE_CLOUD_PROJECT}" \
      "--region=${GOOGLE_CLOUD_REGION}"

if kubectl get secret service-account-key >/dev/null 2>&1; then
    echo "The service account key for the ${SA_ID} service account already exists"
else
    KEY_FILE_DIR="/dev/shm"
    if [[ ! -d "${KEY_FILE_DIR}" ]]; then
       KEY_FILE_DIR="${HOME}"
    fi

    # Create new keys for this service account and download then to a temporary
    # place:
    gcloud iam service-accounts keys create "${KEY_FILE_DIR}/key.json" \
        "--iam-account=${SA_NAME}"

    # Copy the key to the GKE cluster:
    kubectl create secret generic service-account-key \
        "--from-file=key.json=${KEY_FILE_DIR}/key.json"

    rm "${KEY_FILE_DIR}/key.json"
fi

exit 0
