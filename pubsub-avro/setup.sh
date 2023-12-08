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
