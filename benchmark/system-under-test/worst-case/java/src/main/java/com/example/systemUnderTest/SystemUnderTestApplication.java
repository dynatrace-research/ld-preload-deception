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

package com.example.systemUnderTest;

import org.springframework.boot.SpringApplication;
import org.springframework.boot.autoconfigure.SpringBootApplication;
import org.springframework.web.bind.annotation.*;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileWriter;
import java.io.IOException;
import java.nio.file.Paths;
import java.util.Scanner;
import java.util.logging.Level;
import java.util.logging.Logger;

@SpringBootApplication
@RestController
public class SystemUnderTestApplication {

	private static Logger logger = Logger.getLogger("system-under-test");
	private static int createdFiles;

	public static void main(String[] args) {
		SpringApplication.run(SystemUnderTestApplication.class, args);
	}

	@RequestMapping("/")
	public String home() {
		logger.log(Level.INFO, "Hello World was called.");
		return "Hello World!\n";
	}

	@PostMapping(path="benchmark")
	@ResponseBody
	public String fileManipulationEndpoint(@RequestBody MessageModel messageModel) {
		int fileId = createAndWriteFile(messageModel.getMessage());
		logFileContent(fileId);
		deleteContent(fileId);

		return "Successfully made file operation!\n";
	}

	private int createAndWriteFile(String message) {
		int fileId = createdFiles++;
		String fileName = Paths.get("",
				System.getProperty("java.io.tmpdir"),
				"performanceFile" + fileId + ".txt"
		).toString();

		File f = new File(fileName, "x");
		FileWriter fileWriter = null;

		try {
			fileWriter = new FileWriter(fileName);
			fileWriter.write(message + " @file:" + fileName);
			fileWriter.close();
		} catch (IOException e) {
			throw new RuntimeException(e);
		}
		return fileId;
	}

	private void logFileContent(int fileId) {
		File logFile = null;
		String fileName = Paths.get("",
				System.getProperty("java.io.tmpdir"),
				"performanceFile" + fileId + ".txt"
		).toString();

		try {
			logFile = new File(fileName);
			Scanner myReader = new Scanner(logFile);
			while (myReader.hasNextLine()) {
				String data = myReader.nextLine();
				logger.log(Level.INFO, data);
			}
			myReader.close();
		} catch (FileNotFoundException e) {
			logger.log(Level.INFO, "An error occurred while reading the file " + fileName + ".");
		}
	}

	private void deleteContent(int fileId) {
		String fileName = Paths.get("",
				System.getProperty("java.io.tmpdir"),
				"performanceFile" + fileId + ".txt"
		).toString();
		File file = new File(fileName);

		if (!file.delete()) {
			logger.log(Level.INFO, "Failed to delete the file \"" + fileName + "\"");
		}
	}

}
