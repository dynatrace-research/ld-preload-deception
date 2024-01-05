FROM maven:3.9.0

RUN apt-get update && \
    apt-get install -y build-essential clang clang-format curl strace net-tools git libtool && \
    apt-get clean

WORKDIR /app

COPY ./src/hello-world/HelloWorld.java ./HelloWorld.java

EXPOSE 8000
EXPOSE 8080

ENTRYPOINT [ "java" ]
CMD [ "HelloWorld.java" ]
