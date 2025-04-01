FROM python:3.12-slim AS base

RUN apt-get update

RUN pip install pipenv

WORKDIR /app

COPY Pipfile Pipfile.lock* ./

RUN pipenv install --deploy --system

COPY src/ ./src
COPY migrations/ ./migrations

CMD ["python3", "src/main.py"]

