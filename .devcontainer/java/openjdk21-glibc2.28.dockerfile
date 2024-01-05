FROM openjdk:21-buster

WORKDIR /app

COPY ./src/hello-world/HelloWorld.java ./HelloWorld.java

EXPOSE 8000
EXPOSE 8080

ENTRYPOINT [ "java" ]
CMD [ "HelloWorld.java" ]
