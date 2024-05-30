from google.cloud import bigquery
import sys
import argparse
from google.api_core.exceptions import Conflict, NotFound

def main(args):
    # Construct a BigQuery client object.
    client = bigquery.Client()
    # If it does not already exist, create dataset to store table.
    dataset_id = f"{args.project_id}.{args.dataset_name}"
    dataset = bigquery.Dataset(dataset_id)
    dataset.location = args.dataset_location
    try:
        dataset = client.create_dataset(dataset, timeout=30)
        print("Created dataset {}.{}".format(client.project, dataset.dataset_id))
    except Conflict as e:
        if ("ALREADY_EXISTS" in e.details[0]['detail']):
            print(f"Dataset {dataset_id} already exists.")
        else:
            print(f"Unable to create dataset. Error with code {e.code} and message {e.message}")
            return
    except Exception as e:
        print(f"Unable to create dataset. Error with code {e.code} and message {e.message}")
        return

    # Verify table exists.
    table_id = f"{dataset_id}.{args.table_name}"
    try:
        table = client.get_table(table_id)
        print(f"Table {table_id} already exists. Run script with a new --table_name argument.")
        return
    except NotFound:
        pass
    except Exception as e:
        print(f"Unable to verify if table exists. Error with code {e.code} and message {e.message}")
        return

    # Create query job that writes the top 10 names to a table.
    job_config = bigquery.QueryJobConfig(destination=table_id)
    sql = """
    SELECT
        name,
        SUM(number) AS total
        FROM
        `bigquery-public-data.usa_names.usa_1910_2013`
        GROUP BY
        name
        ORDER BY
        total DESC
        LIMIT
        10;
    """

    # Start the query, passing in the extra configuration.
    query_job = client.query(sql, job_config=job_config)  # Make an API request.
    query_job.result()  # Wait for the job to complete.

    print(f"Query results loaded to the table {table_id}")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="A script to create BigQuery query job.")
    parser.add_argument("-p","--project_id", type=str,help="GCP project id")
    parser.add_argument("--dataset_name", type=str,help="Dataset name to store the table in")
    parser.add_argument("--dataset_location",type=str, default="US")
    parser.add_argument("--table_name", type=str,help="Table name to write the query results to")
    args = parser.parse_args()
    main(args)
