FROM selenium/standalone-chrome:latest AS base

RUN apt-get update &&  \
  apt-get install -y  python3.11 pipenv 

WORKDIR /app

COPY Pipfile Pipfile.lock* ./
RUN pipenv install --deploy --system

COPY src/ .

CMD ["python", "src/data_collection.py"]
