#!/usr/bin/env python3
# Copyright 2021 Google LLC
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

"""Create k8s jobs to index GCS buckets."""

import argparse
import jinja2 as j2
import os

template = j2.Template("""apiVersion: apps/v1
kind: Deployment
metadata:
  name: worker
spec:
  replicas: 3
  selector:
    matchLabels:
      run: worker
  template:
    metadata:
      labels:
        run: worker
    spec:
      serviceAccountName: worker
      containers:
      - name: worker-image
        image: gcr.io/{{project}}/getting-started-cpp/gke
        imagePullPolicy: Always
        command: [ '/r/gke_index_gcs' ]
        env:
        - name: SPANNER_INSTANCE
          value: getting-started-cpp
        - name: SPANNER_DATABASE
          value: gcs-index
        - name: TOPIC_ID
          value: gke-gcs-indexing
        - name: SUBSCRIPTION_ID
          value: gke-gcs-indexing
        - name: GOOGLE_CLOUD_PROJECT
          value: {{project}}
        - name: PLACEHOLDER
          value: "19"
        resources:
          requests:
            cpu: '100m'
            memory: '128Mi'
""")

parser = argparse.ArgumentParser()
parser.add_argument('--project', type=str,
                    default=os.environ.get('GOOGLE_CLOUD_PROJECT'),
                    help='configure the Google Cloud Project')
args = parser.parse_args()

print(
    template.render(action='deploy', project=args.project))
