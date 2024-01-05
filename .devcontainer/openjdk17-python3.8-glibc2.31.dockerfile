FROM openjdk:17-bullseye

RUN apt-get update && \
    apt-get install -y build-essential clang clang-format curl strace net-tools git libtool python3.10 python3-pip && \
    apt-get clean && \
    update-alternatives --install /usr/bin/python python /usr/bin/python3 1

COPY ./python/src/requirements.txt /var/opt/requirements.txt
RUN pip install -r /var/opt/requirements.txt

WORKDIR /app

COPY ./java/src/hello-world/HelloWorld.java ./HelloWorld.java
RUN chmod a+w ./HelloWorld.java

EXPOSE 8000
EXPOSE 8080
EXPOSE 5000

ENTRYPOINT [ "java" ]
CMD [ "HelloWorld.java" ]
