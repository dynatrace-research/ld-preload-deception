FROM gradle:7.4.1-jdk11

RUN apt-get update && \
    apt-get install -y build-essential clang clang-format curl strace net-tools git libtool && \
    apt-get clean

WORKDIR /app

# make sure to manually compile this beforehand
COPY ./src/springboot-hello-world/build/libs/spring-boot-0.0.1-SNAPSHOT.jar ./app.jar

EXPOSE 8080

ENTRYPOINT ["java", "-jar", "app.jar"]
