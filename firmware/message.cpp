#include "message.hpp"

#include <Arduino.h>

Message::Message(const String& route, const String& body) : route(route), body(body), headerCount(0) {}

bool Message::valid(const String &source) {
	if (source.indexOf("PT ") != 0) {
		return false;
	}

	return source.indexOf("\n\n") != -1;
}

Message Message::from(const String &input) {
	// read magic
	int start = input.indexOf("PT ");

	if (start != 0) {
		Message message("");

		return message;
	}
	
	// read route
	int afterRouteParameterIndex = input.indexOf(' ', 3);

	int headersStart = input.indexOf('\n');

	if (headersStart == -1) {
		headersStart = input.length();
	}

	Message message(input.substring(3, min(afterRouteParameterIndex, headersStart)));

	// TODO find cause of \n in route on empty-header messages
	while (message.route.indexOf('\n') != -1) {
		message.route.replace("\n", "");
	}

	// read header
	int bodyStart = input.indexOf("\n\n", headersStart);
	
	if (bodyStart == -1) {
		bodyStart = input.length();
	} else {
		message.body = input.substring(bodyStart + 2);  // Skip "\n\n"
	}

	String headersPart = input.substring(headersStart + 1, bodyStart);
	int position = 0;

	while (position < headersPart.length()) {
		int endLine = headersPart.indexOf('\n', position);

		if (endLine == -1) {
			endLine = headersPart.length();
		}

		String line = headersPart.substring(position, endLine);
		int colonPosition = line.indexOf(": ");

		if (colonPosition != -1) {
			String headerName = line.substring(0, colonPosition);
			String headerValue = line.substring(colonPosition + 2);

			message.addHeader(headerName, headerValue); 
		} else {
			message.addHeader(line, ""); 
		}

		position = endLine + 1;
	}

	return message;
};

Message& Message::addHeader(const String& name, const String& value) {
	if (headerCount < MAX_HEADERS) {
		headers[headerCount][0] = name;
		headers[headerCount][1] = value;

		headerCount++;
	}
	
	return *this;
};

String Message::getHeader(const String& name) {
	for (int index = 0; index < headerCount; index++) {
		if (headers[index][0].equals(name)) {
			return headers[index][1];
		}
	}
	
    return "";
}

String Message::toString() const {
    String serialized = "PT " + route + "\n";

    for (int headerIndex = 0; headerIndex < headerCount; headerIndex++) {
		if (headers[headerIndex][1].length() == 0) {
            serialized += headers[headerIndex][0] + "\n";
        } else {
            serialized += headers[headerIndex][0] + ": " + headers[headerIndex][1] + "\n";
        }
    }

	serialized += "\n" + body;

    return serialized;
};

void Message::send() {
	Serial.print(toString());
};