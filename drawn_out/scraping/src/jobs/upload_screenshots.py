import os
from typing import Optional

import boto3
from botocore.config import Config


SCREENSHOT_PATH = os.getenv("SCREENSHOT_PATH")
BUCKET_NAME = os.getenv("BUCKET_NAME") 
CLOUDFLARE_PUBLIC_URL = os.getenv("CLOUDFLARE_PUBLIC_URL") 
ENDPOINT_URL = os.getenv("ENDPOINT_URL")
AWS_ACCESS_KEY_ID = os.getenv("AWS_ACCESS_KEY_ID")
AWS_SECRET_ACCESS_KEY = os.getenv("AWS_SECRET_ACCESS_KEY")


s3 = boto3.client(
    "s3",
    endpoint_url=ENDPOINT_URL,
    aws_access_key_id=AWS_ACCESS_KEY_ID,
    aws_secret_access_key=AWS_SECRET_ACCESS_KEY,
    config=Config(signature_version="s3v4"),
)



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
    dir = SCREENSHOT_PATH
    for filename in os.listdir(dir):
        path = os.path.join(dir, filename)
        if os.path.isfile(path):
            upload_to_cloudflare(path)


if __name__ == "__main__":
  main()
