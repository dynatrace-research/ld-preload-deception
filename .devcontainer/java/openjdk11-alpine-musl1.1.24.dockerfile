FROM adoptopenjdk/openjdk11:jre-11.0.6_10-alpine

RUN apk update && \
    apk add gcc clang curl strace net-tools git make libtool && \
    rm -rf /etc/apk/cache/*

WORKDIR /app

# make sure to manually compile this beforehand
COPY ./src/springboot-hello-world/build/libs/spring-boot-0.0.1-SNAPSHOT.jar ./app.jar

EXPOSE 8000
EXPOSE 8080

ENTRYPOINT ["java", "-jar", "app.jar"]
