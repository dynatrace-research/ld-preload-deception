# SpringBoot demo application

Run the project

    gradlew bootRun --args='--server.port=8000'

Build the project

    gradlew bootJar

Take care of changing the server port in `server.port`

    ./src/main/resources/application.properties

In the container, start the built application like this

    java -jar ./build/libs/spring-boot-0.0.1-SNAPSHOT.jar
