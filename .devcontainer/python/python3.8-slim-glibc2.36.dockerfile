FROM python:3.8-slim

RUN apt-get update && \
    apt-get install -y build-essential clang clang-format curl strace net-tools git libtool && \
    apt-get clean

WORKDIR /app

COPY ./src/requirements.txt requirements.txt
RUN pip install -r requirements.txt

COPY ./src/app.py app.py

EXPOSE 5000

ENTRYPOINT [ "python3" ]
CMD [ "app.py" ]
