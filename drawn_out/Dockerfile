FROM python:3.11-slim AS base

RUN apt-get update && apt-get install -y  pipenv

WORKDIR /app

COPY Pipfile Pipfile.lock* ./
RUN pipenv install --deploy --system

COPY src/ .
COPY migrations/ ./migrations

CMD ["python", "data_collection.py"]

