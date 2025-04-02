import os
from typing import Optional

import boto3
from botocore.config import Config


s3 = boto3.client(
    "s3",
    endpoint_url="https://f6d1d15e6f0b37b4b8fcad3c41a7922d.r2.cloudflarestorage.com",
    aws_access_key_id="3d43ea1903ba006bc666503b0913150b",
    aws_secret_access_key="8a9cdead6fa4762a10fae95122c630afe605c66dee317e5fc7e58287ba4a3399",
    config=Config(signature_version="s3v4"),
)


BUCKET_NAME = "pregnant-yellow-perch-6ri27"
CLOUDFLARE_PUBLIC_URL = "https://pregnant-yellow-perch-6ri27.sevalla.storage"

def upload_to_cloudflare(file_path: str, object_name: Optional[str] = None) -> str:
    # If S3 object_name was not specified, use file_paths basename
    if object_name is None:
        object_name = os.path.basename(file_path)

    try:
        # Upload the file
        s3.upload_file(file_path, BUCKET_NAME, object_name)
        
        # Generate a public URL for the uploaded file
        url = f"{CLOUDFLARE_PUBLIC_URL}{object_name}"
        
        # Delete the local file
        
        return url
    except Exception as e:
        print(f"An error occurred: {e}")
        return ""


def main():
    dir = "/var/lib/data/"
    for filename in os.listdir(dir):
        path = os.path.join(dir, filename)
        if os.path.isfile(path):
            upload_to_cloudflare(path)


if __name__ == "__main__":
  main()
