#pragma once

#include <cstring>
#include <cstdio>

#define MAX_HEADERS 16
#define MAX_ROUTE_LENGTH 128
#define MAX_BODY_LENGTH 512
#define MAX_HEADER_NAME_LENGTH 64
#define MAX_HEADER_VALUE_LENGTH 128
#define MAX_SERIALIZED_LENGTH 1024

class Message {
	public:
		inline Message(
			const char *route = "",
			const char *body = ""
		) : headerCount(0) {
			setString(this->route, route, MAX_ROUTE_LENGTH);
			setString(this->body, body, MAX_BODY_LENGTH);

			clearHeaders();
		}

		inline static bool valid(const char *source) {
			if (source == nullptr) {
				return false;
			}

			// Must start with "PT "
			if (std::strncmp(source, "PT ", 3) != 0) {
				return false;
			}

			// Must contain "\n\n"
			const char *doubleNewline = std::strstr(source, "\n\n");
			return (doubleNewline != nullptr);
		}

		// parse buffer
		static Message from(const char *input) {
			Message message;

			if (input == nullptr) {
				return message;
			}

			const size_t length = std::strlen(input);
			if (length < 3 || std::strncmp(input, "PT ", 3) != 0) {
				// Not starting with "PT "
				return message;
			}

			// Find first newline (end of first line)
			const char *headersStart = std::strchr(input, '\n');
			if (!headersStart) {
				headersStart = input + length;  // No newline at all
			}

			// Find space after route parameter starting at position 3
			const char *afterRouteParam = std::strchr(input + 3, ' ');
			if (!afterRouteParam || afterRouteParam > headersStart) {
				afterRouteParam = headersStart;
			}

			// Copy route (between "PT " and first space/newline)
			size_t routeLen = static_cast<size_t>(afterRouteParam - (input + 3));
			if (routeLen >= MAX_ROUTE_LENGTH) {
				routeLen = MAX_ROUTE_LENGTH - 1;
			}

			std::memcpy(message.route, input + 3, routeLen);
			message.route[routeLen] = '\0';

			// strip any stray '\n' from route (parity with original comment)
			char *newlinePos = nullptr;
			while ((newlinePos = std::strchr(message.route, '\n')) != nullptr) {
				std::memmove(newlinePos, newlinePos + 1, std::strlen(newlinePos + 1) + 1);
			}

			// Find start of body ("\n\n" after headersStartPtr)
			const char *doubleNewline = nullptr;
			if (headersStart < input + length) {
				doubleNewline = std::strstr(headersStart, "\n\n");
			}

			const char *bodyPartStart = input + length;
			if (doubleNewline) {
				bodyPartStart = doubleNewline + 2; // skip "\n\n"

				// Copy body
				size_t bodyLen = static_cast<size_t>((input + length) - bodyPartStart);
				if (bodyLen >= MAX_BODY_LENGTH) {
					bodyLen = MAX_BODY_LENGTH - 1;
				}

				std::memcpy(message.body, bodyPartStart, bodyLen);
				message.body[bodyLen] = '\0';
			} else {
				// No body
				message.body[0] = '\0';
			}

			// Headers part is between first '\n' (exclusive) and body start (exclusive)
			const char *headersPartStart = headersStart;
			if (headersPartStart < input + length && *headersPartStart == '\n') {
				headersPartStart++;
			}

			const char *headersPartEnd = bodyPartStart;
			if (headersPartEnd > input + length) {
				headersPartEnd = input + length;
			}

			const char *pos = headersPartStart;
			while (pos < headersPartEnd && *pos != '\0') {
				const void *found = std::memchr(pos, '\n', static_cast<size_t>(headersPartEnd - pos));
				const char *endLine = found ? static_cast<const char *>(found) : headersPartEnd;

				size_t lineLen = static_cast<size_t>(endLine - pos);

				// Skip empty lines
				if (lineLen > 0) {
					char lineBuf[MAX_HEADER_NAME_LENGTH + MAX_HEADER_VALUE_LENGTH + 4];
					size_t copyLen = lineLen;

					if (copyLen >= sizeof(lineBuf)) {
						copyLen = sizeof(lineBuf) - 1;
					}

					std::memcpy(lineBuf, pos, copyLen);
					lineBuf[copyLen] = '\0';

					// Find ": " within this line
					char *colonSpace = std::strstr(lineBuf, ": ");
					if (colonSpace) {
						// Split into name and value
						size_t nameLen = static_cast<size_t>(colonSpace - lineBuf);
						size_t valueLen = std::strlen(colonSpace + 2);

						char nameBuf[MAX_HEADER_NAME_LENGTH];
						char valueBuf[MAX_HEADER_VALUE_LENGTH];

						if (nameLen >= MAX_HEADER_NAME_LENGTH) {
							nameLen = MAX_HEADER_NAME_LENGTH - 1;
						}

						std::memcpy(nameBuf, lineBuf, nameLen);
						nameBuf[nameLen] = '\0';

						if (valueLen >= MAX_HEADER_VALUE_LENGTH) {
							valueLen = MAX_HEADER_VALUE_LENGTH - 1;
						}

						std::memcpy(valueBuf, colonSpace + 2, valueLen);
						valueBuf[valueLen] = '\0';

						message.addHeader(nameBuf, valueBuf);
					} else {
						// Header without value
						char nameBuf[MAX_HEADER_NAME_LENGTH];
						size_t nameLen = std::strlen(lineBuf);

						if (nameLen >= MAX_HEADER_NAME_LENGTH) {
							nameLen = MAX_HEADER_NAME_LENGTH - 1;
						}

						std::memcpy(nameBuf, lineBuf, nameLen);
						nameBuf[nameLen] = '\0';

						message.addHeader(nameBuf, "");
					}
				}

				if (endLine == headersPartEnd) {
					break;
				}

				pos = endLine + 1;
			}

			return message;
		}

		// Add a header (name, value)
		inline Message &addHeader(const char *name, const char *value) {
			if (headerCount >= MAX_HEADERS) {
				return *this;
			}

			setString(headerNames[headerCount], name, MAX_HEADER_NAME_LENGTH);
			setString(headerValues[headerCount], value, MAX_HEADER_VALUE_LENGTH);

			headerCount++;
			return *this;
		}

		// Return pointer to header value, or "" if not found.
		// NOTE: pointer is to internal storage; don't free or modify it.
		inline const char *getHeader(const char *name) const {
			for (int i = 0; i < headerCount; ++i) {
				if (std::strcmp(headerNames[i], name) == 0) {
					return headerValues[i];
				}
			}

			return "";
		}

		// Serialize into a user-provided buffer.
		// bufferSize includes space for the terminating '\0'.
		inline void toString(char *buffer, size_t bufferSize) const {
			if (!buffer || bufferSize == 0) {
				return;
			}

			buffer[0] = '\0';
			size_t used = 0;

			auto append = [&](const char *s) {
				if (!s) {
					return;
				}

				size_t length = std::strlen(s);

				if (used >= bufferSize - 1) {
					return;
				}

				size_t copyLen = length;

				if (copyLen > bufferSize - 1 - used) {
					copyLen = bufferSize - 1 - used;
				}

				std::memcpy(buffer + used, s, copyLen);
				used += copyLen;
				buffer[used] = '\0';
			};

			append("PT ");
			append(route);
			append("\n");

			for (int i = 0; i < headerCount; ++i) {
				append(headerNames[i]);

				if (headerValues[i][0] != '\0') {
					append(": ");
					append(headerValues[i]);
				}

				append("\n");
			}

			append("\n");
			append(body);
		}

		// Send via standard output (ESP-IDF printf).
		// You can swap this for uart_write_bytes() if you want.
		inline void send() const {
			char buf[MAX_SERIALIZED_LENGTH];
			toString(buf, sizeof(buf));
			std::printf("%s", buf);
		}

		// Accessors for route/body if needed
		inline const char *getRoute() const { return route; }
		inline const char *getBody() const { return body; }

		inline void setRoute(const char *route) { setString(this->route, route, MAX_ROUTE_LENGTH); }
		inline void setBody(const char *body) { setString(this->body, body, MAX_BODY_LENGTH); }

	private:
		char route[MAX_ROUTE_LENGTH];
		char body[MAX_BODY_LENGTH];

		char headerNames[MAX_HEADERS][MAX_HEADER_NAME_LENGTH];
		char headerValues[MAX_HEADERS][MAX_HEADER_VALUE_LENGTH];
		int headerCount;

		// Utility: safe copy into fixed-size C-string
		inline static void setString(char *dest, const char *src, size_t maxLen) {
			if (!dest || maxLen == 0) {
				return;
			}

			if (!src) {
				dest[0] = '\0';
				return;
			}

			std::strncpy(dest, src, maxLen - 1);
			dest[maxLen - 1] = '\0';
		}

		inline void clearHeaders() {
			for (int i = 0; i < MAX_HEADERS; ++i) {
				headerNames[i][0]  = '\0';
				headerValues[i][0] = '\0';
			}

			headerCount = 0;
		}
};
