FROM python:3.8-slim
WORKDIR /app

# install dependencies
COPY ./requirements.txt requirements.txt
RUN pip install -r requirements.txt

COPY ./webhook.py webhook.py

# should be mounted upon container start
# COPY ./webhook.crt ./webhook.key /certs/

EXPOSE 443

# CMD sleep infinity
CMD gunicorn --certfile=/certs/webhook.crt --keyfile=/certs/webhook.key --bind 0.0.0.0:443 webhook:app
