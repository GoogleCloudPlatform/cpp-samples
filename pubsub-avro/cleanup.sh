# Delete the topic, subscription, and schema.
gcloud pubsub topics delete ${GOOGLE_CLOUD_TOPIC} "--project=${GOOGLE_CLOUD_PROJECT}"
gcloud pubsub subscriptions delete ${GOOGLE_CLOUD_SUBSCRIPTION} "--project=${GOOGLE_CLOUD_PROJECT}"
gcloud pubsub schemas delete ${GOOGLE_CLOUD_SCHEMA_NAME} "--project=${GOOGLE_CLOUD_PROJECT}"
