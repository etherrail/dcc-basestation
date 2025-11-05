#ifndef MESSAGE_HEADER
#define MESSAGE_HEADER

#include <Arduino.h>

#define MAX_HEADERS 32

class Message {
	public:
		static bool valid(const String& source);

		Message(const String& route, const String& body = "");

		static Message from(const String &input);
		String toString() const;

		Message& addHeader(const String& name, const String& value = "");
		String getHeader(const String& name);

		String route;
		String body;

		String headers[MAX_HEADERS][2];
		int headerCount;

		void send();

		// error states
		static Message* noMessagePrefixError;
};

#endif