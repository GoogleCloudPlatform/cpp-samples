#!/usr/bin/env python3
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

"""Create k8s jobs to populate GCS buckets with randomly named objects."""

import argparse
import jinja2 as j2
import os

template = j2.Template("""apiVersion: apps/v1
kind: Deployment
metadata:
  name: worker
  namespace: {{namespace}}
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
        image: gcr.io/{{project}}/cpp-samples/populate-bucket:{{image_version}}
        imagePullPolicy: Always
        command: [
            '/r/populate_bucket', 'worker',
            '--project={{project}}',
            '--subscription=populate-bucket',
            '--concurrency=16'
        ]
        resources:
          requests:
            cpu: '100m'
            memory: '32Mi'
""")

parser = argparse.ArgumentParser()
parser.add_argument('--project', type=str,
                    default=os.environ.get('GOOGLE_CLOUD_PROJECT'),
                    help='configure the Google Cloud Project')
parser.add_argument('--image-version', type=str,
                    default='latest',
                    help='which image version to use')
parser.add_argument('--namespace', type=str,
                    default='populate-bucket',
                    help='the GKE namespace')
args = parser.parse_args()

print(
    template.render(action='deploy', project=args.project, image_version=args.image_version, namespace=args.namespace))
