FROM locustio/locust:2.17.0

WORKDIR /opt/locust

COPY requirements.txt ./requirements.txt
RUN pip install -r ./requirements.txt

COPY locustfile.py /mnt/locust/locustfile.py
COPY locustfile.py locustfile.py

USER root

RUN mkdir cvs_exports

EXPOSE 8089
