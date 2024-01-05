// Copyright 2024 Dynatrace LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Portions of this code, as identified in remarks, are provided under the
// Creative Commons BY-SA 4.0 or the MIT license, and are provided without
// any warranty. In each of the remarks, we have provided attribution to the
// original creators and other attribution parties, along with the title of
// the code (if known) a copyright notice and a link to the license, and a
// statement indicating whether or not we have modified the code.

import java.io.*;
import com.sun.net.httpserver.*;
import java.net.InetSocketAddress;

public class TestPage {

    public static void main(String[] args) throws Exception {
        HttpServer server = HttpServer.create(new InetSocketAddress(8080), 0);
        server.createContext("/", new MyHandler());
        server.setExecutor(null); // creates a default executor
        server.start();
    }

    static class MyHandler implements HttpHandler {
        @Override
        public void handle(HttpExchange httpExchange) throws IOException {
            String response = "Hello World!<br />This is a test page.";
            int statusCode = 200;

            if (httpExchange.getRequestURI().toString().compareTo("/") != 0) {
                response = "<h1>404 Not Found</h1>No context found for request";
                statusCode = 404;
            }

            httpExchange.sendResponseHeaders(statusCode, response.length());
            httpExchange.getResponseBody().write(response.getBytes());
            httpExchange.close();
        }
    }
}
