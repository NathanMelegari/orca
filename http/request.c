#include "orca_http.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum RequestState {
	STATE_REQUEST_LINE,
	STATE_HEADER,
	STATE_BODY
} RequestState;

typedef enum RequestToken {
	TOKEN_METHOD,
	TOKEN_URL,
	TOKEN_HTTP_VERSION,
	TOKEN_HEADER_NAME,
	TOKEN_HEADER_VALUE,
	TOKEN_CRLF,
	TOKEN_HEADER_END,
	TOKEN_BODY
} RequestToken;

typedef struct Token {
	RequestToken type;
	size_t start;
	size_t length;
} Token;

typedef struct RequestTokenArray {
	Token *tokens;
	size_t length;
	size_t capacity;
} RequestTokenArray;

typedef struct ReqLexer {
	const char *src;
	size_t length;
	size_t pos;
} ReqLexer;

typedef struct ReqParse {
	RequestTokenArray *tokenArray;
	size_t pos;
} ReqParse;

static bool reallocTokenArray(RequestTokenArray *array) {
	if(array == NULL) return false;

	size_t newCapacity;

	if(array->capacity == 0) newCapacity = 4;
	else {
		if(array->capacity > SIZE_MAX / 2) return false;
		newCapacity = array->capacity * 2;
	}

	if(newCapacity > SIZE_MAX / sizeof(*array->tokens)) return false;

	Token *newTokens = realloc(array->tokens, newCapacity * sizeof(*newTokens));
	if(newTokens == NULL) {
		return false;
	}

	array->tokens = newTokens;
	array->capacity = newCapacity;

	return true;
}

static bool pushToken(RequestTokenArray *array, Token token) {
	if(array == NULL || array->tokens == NULL) return false;

	if(array->length >= array->capacity) {
		if(!reallocTokenArray(array)) {
			return false;
		}
	}

	array->tokens[array->length++] = token;

	return true;
}

static bool lexerRequestLine(
		const char *buffer, size_t length, size_t start,
		Token *token, size_t *nextPosition
) {
	if(buffer == NULL || token == NULL || nextPosition == NULL) return false;

	size_t i = start;

	while(i < length && buffer[i] != ' ' && buffer[i] != '\r' && buffer[i] != '\n') {
		++i;
	}

	if(i == length || i == start) return false;

	token->start = start;
	token->length = i - start;

	if(token->length == 3 && memcmp(buffer + start, "GET", 3) == 0) token->type = TOKEN_METHOD;
	else if(token->length == 4 && memcmp(buffer + start, "POST", 4) == 0) token->type = TOKEN_METHOD;
	else if(token->length == 8 && memcmp(buffer + start, "HTTP/1.1", 8) == 0) token->type = TOKEN_HTTP_VERSION;
	else if(start > 0 && buffer[start - 1] == ' ') token->type = TOKEN_URL;
	else return false;

	*nextPosition = i;

	if(buffer[i] == ' ') {
		++(*nextPosition);
	}

	return true;
}

static bool lexerHeader(
		const char *buffer, size_t length, size_t start,
		Token *headerName, Token *headerValue, size_t *nextPosition
) {
	if(buffer == NULL || headerName == NULL || headerValue == NULL || nextPosition == NULL) {
		return false;
	}

	size_t i = start;

	while(i < length && buffer[i] != ':' && buffer[i] != '\r' && buffer[i] != '\n') {
		++i;
	}

	if(i >= length || buffer[i] != ':' || i == start) {
		return false;
	}

	*headerName = (Token){
		.type = TOKEN_HEADER_NAME,
		.start = start,
		.length = i - start
	};

	++i;

	while(i < length && (buffer[i] == ' ' || buffer[i] == '\t')) {
		++i;
	}

	size_t valueStart = i;

	while(i < length && buffer[i] != '\r' && buffer[i] != '\n') {
		++i;
	}

	if(i >= length || buffer[i] != '\r' || i + 1 >= length || buffer[i + 1] != '\n') {
		return false;
	}

	*headerValue = (Token){
		.type = TOKEN_HEADER_VALUE,
		.start = valueStart,
		.length = i - valueStart
	};

	*nextPosition = i;

	return true;
}

static bool lexerHeaderEnd(const char *buffer, size_t length, size_t *position, Token *token) {
	if(buffer == NULL || position == NULL || token == NULL) return false;

	size_t i = *position;

	if(i + 1 >= length || buffer[i] != '\r' || buffer[i + 1] != '\n') {
		return false;
	}

	*token = (Token){
		.type = TOKEN_HEADER_END,
		.start = i,
		.length = 2
	};

	*position += 2;

	return true;
}

static bool lexerBody(const char *buffer, size_t length, size_t start, size_t contentLength, Token *body, size_t *nextPosition) {
	if(buffer == NULL || body == NULL || nextPosition == NULL) {
		return false;
	}

	if(start > length || contentLength > length - start) {
		return false;
	}

	*body = (Token){
		.type = TOKEN_BODY,
		.start = start,
		.length = contentLength
	};

	*nextPosition = start + contentLength;

	return true;
}

static bool lexerCRLF(const char *buffer, size_t length, size_t *position, Token *token) {
	if(buffer == NULL || position == NULL || token == NULL) {
		return false;
	}

	size_t i = *position;

	if(i + 1 >= length || buffer[i] != '\r' || buffer[i + 1] != '\n') {
		return false;
	}

	*token = (Token){
		.type = TOKEN_CRLF,
		.start = i,
		.length = 2
	};

	*position += 2;
	return true;
}

static bool getContentLength(const OrcaHeaderArray *headers, size_t *contentLength) {
        if(headers == NULL || contentLength == NULL) return false;
	if(headers->length > 0 && headers->headers == NULL) return false;

        *contentLength = 0;

        bool found = false;

        for(size_t i = 0; i < headers->length; i++) {
                const OrcaHeader *header = &headers->headers[i];

		if(header->name == NULL || header->value == NULL) return false;

                if(strcasecmp(header->name, "Content-Length") == 0) {
                        if(found) return false;

                        found = true;

                        if(header->value[0] == '\0') return false;

                        size_t value = 0;

                        for(size_t j = 0; header->value[j] != '\0'; j++) {
                                unsigned char c = (unsigned char)header->value[j];

                                if(c < '0' || c > '9') return false;

                                size_t digit = c - '0';

                                if(value > (SIZE_MAX - digit) / 10) {
                                        return false;
                                }

                                value = value * 10 + digit;
                        }

                        *contentLength = value;
                }

                if(strcasecmp(header->name, "Transfer-Encoding") == 0) {
                        return false;
                }
        }

        return true;
}

static RequestTokenArray requestLexer(const char *buffer, size_t length) {
	if(buffer == NULL) return (RequestTokenArray){};

	ReqLexer lexer = {
		.src = buffer,
		.length = length,
		.pos = 0
	};

	RequestTokenArray tokenArray = {
		.tokens = 0,
		.capacity = 12,
		.length = 0
	};

	tokenArray.tokens = malloc(tokenArray.capacity * sizeof(*tokenArray.tokens));
	if(tokenArray.tokens == NULL) {
		return (RequestTokenArray){};
	}

	RequestState currentState = STATE_REQUEST_LINE;

	while(lexer.pos < length) {

		size_t start = lexer.pos;
		unsigned char c = lexer.src[lexer.pos];

		if(currentState == STATE_REQUEST_LINE && c != '\r') {
			Token requestLine = {0};
			size_t nextPosition = 0;

			if(!lexerRequestLine(buffer, length, start, &requestLine, &nextPosition)) {
				free(tokenArray.tokens);
				return (RequestTokenArray){};
			}

			if(!pushToken(&tokenArray, requestLine)) {
				free(tokenArray.tokens);
				return (RequestTokenArray){};
			}

			lexer.pos = nextPosition;

			if(requestLine.type == TOKEN_HTTP_VERSION) {
				currentState = STATE_HEADER;

				Token crlf = {0};

				if(!lexerCRLF(buffer, length, &lexer.pos, &crlf)) {
					free(tokenArray.tokens);
					return (RequestTokenArray){};
				}

				if(!pushToken(&tokenArray, crlf)) {
					free(tokenArray.tokens);
					return (RequestTokenArray){};
				}
			}

			continue;

		} else if(currentState == STATE_HEADER) {
			if(c == '\r') {
				Token headerEnd = {0};

				if(!lexerHeaderEnd(buffer, length, &lexer.pos, &headerEnd)) {
					free(tokenArray.tokens);
					return (RequestTokenArray){};
				}

				if(!pushToken(&tokenArray, headerEnd)) {
					free(tokenArray.tokens);
					return (RequestTokenArray){};
				}

			//	currentState = STATE_BODY;
				break;
			}

			Token name = {0};
			Token value = {0};
			size_t nextPosition = 0;

			if(!lexerHeader(buffer, length, lexer.pos, &name, &value, &nextPosition)) {
				free(tokenArray.tokens);
				return (RequestTokenArray){};
			}

			lexer.pos = nextPosition;

			Token crlf = {0};

			if(!lexerCRLF(buffer, length, &lexer.pos, &crlf)) {
				free(tokenArray.tokens);
				return (RequestTokenArray){};
			}

			if(!pushToken(&tokenArray, name) || !pushToken(&tokenArray, value) || !pushToken(&tokenArray, crlf)) {
				free(tokenArray.tokens);
				return (RequestTokenArray){};
			}

			continue;

/*		} else if(currentState == STATE_BODY && c != '\r') {
			size_t bodyStart = lexer.pos;
			size_t bodyLength = length;
			Token body = {0};
			size_t nextPosition = 0;

			if(!lexerBody(buffer, length, bodyStart, bodyLength, &body, &nextPosition)) {
				free(tokenArray.tokens);
				return (RequestTokenArray){};
			}

			if(!pushToken(&tokenArray, body)) {
				free(tokenArray.tokens);
				return (RequestTokenArray){};
			}

			lexer.pos = nextPosition;
*/
		} else {
			Token crlf = {0};

			if(!lexerCRLF(buffer, length, &lexer.pos, &crlf)) {
				free(tokenArray.tokens);
				return (RequestTokenArray){};
			}

			if(!pushToken(&tokenArray, crlf)) {
				free(tokenArray.tokens);
				return (RequestTokenArray){};
			}
		}
	}

	return tokenArray;
}

static bool reallocHeadersArray(OrcaHeaderArray *array) {
	if(array == NULL) return false;

	size_t newCapacity;

	if(array->capacity == 0) newCapacity = 4;
	else {
		if(array->capacity > SIZE_MAX / 2) return false;
		newCapacity = array->capacity * 2;
	}
	
	if(newCapacity > SIZE_MAX / sizeof(*array->headers)) return false;

	OrcaHeader *newHeaders = realloc(array->headers, newCapacity * sizeof(*newHeaders));
	if(newHeaders == NULL) {
		return false;
	}

	array->headers = newHeaders;
	array->capacity = newCapacity;
	return true;
}

static bool requestParse(const char *buffer, size_t length, RequestTokenArray *tokenArray, OrcaRequest *request) {
	if(buffer == NULL || tokenArray == NULL || tokenArray->tokens == NULL || tokenArray->length < 4 || request == NULL) return false;

	Token method = tokenArray->tokens[0];
	if(method.type != TOKEN_METHOD) {
		return false;
	}

	if(method.length == 3 && memcmp(buffer + method.start, "GET", 3) == 0) request->method = ORCA_METHOD_GET;
	else if(method.length == 4 && memcmp(buffer + method.start, "POST", 4) == 0) request->method = ORCA_METHOD_POST;
	else return false;

	Token url = tokenArray->tokens[1];
	if(url.type != TOKEN_URL) {
		return false;
	}

	request->url = malloc(url.length + 1);
	if(request->url == NULL) return false;

	memcpy(request->url, buffer + url.start, url.length);
	request->url[url.length] = '\0';

	Token httpVersion = tokenArray->tokens[2];
	if(httpVersion.type != TOKEN_HTTP_VERSION || httpVersion.length != 8 || memcmp(
				buffer + httpVersion.start, "HTTP/1.1", 8) != 0
	) {
		return false;
	}

	Token requestLineCRLF = tokenArray->tokens[3];
	if(requestLineCRLF.type != TOKEN_CRLF) {
		return false;
	}

	if(
		method.type != TOKEN_METHOD || url.type != TOKEN_URL ||
		httpVersion.type != TOKEN_HTTP_VERSION || requestLineCRLF.type != TOKEN_CRLF
	) {
		return false;
	}

	size_t i = 4;
	bool headerEndFound = false;

	while(i < tokenArray->length) {
		Token name = tokenArray->tokens[i++];

		if(name.type == TOKEN_HEADER_END) {
			headerEndFound = true;
			break;
		}

		if(name.type != TOKEN_HEADER_NAME || i >= tokenArray->length) {
			return false;
		}

		if(request->headerArray.length >= request->headerArray.capacity) {
			if(!reallocHeadersArray(&request->headerArray)) {
				return false;
			}
		}

		OrcaHeader *header = &request->headerArray.headers[request->headerArray.length];

		header->name = malloc(name.length + 1);
		header->value = NULL;

		if(header->name == NULL) return false;

		memcpy(header->name, buffer + name.start, name.length);
		header->name[name.length] = '\0';

		Token value = tokenArray->tokens[i++];

		if(value.type != TOKEN_HEADER_VALUE || i >= tokenArray->length) {
			free(header->name);
			header->name = NULL;
			return false;
		}

		header->value = malloc(value.length + 1);

		if(header->value == NULL) {
			free(header->name);
			header->name = NULL;
			return false;
		}

		memcpy(header->value, buffer + value.start, value.length);
		header->value[value.length] = '\0';

		Token crlf = tokenArray->tokens[i++];

		if(crlf.type != TOKEN_CRLF) {
			free(header->value);
			free(header->name);
			header->value = NULL;
			header->name = NULL;
			return false;
		}

		request->headerArray.length++;
	}

	size_t contentLength = 0;

	if(!getContentLength(&request->headerArray, &contentLength)) return false;
	if(!headerEndFound) return false;

	Token headerEnd = tokenArray->tokens[i - 1];
	if(headerEnd.type != TOKEN_HEADER_END) return false;

	size_t bodyStart = headerEnd.start + headerEnd.length;

	if(bodyStart > length) return false;

	if(contentLength > length - bodyStart) return false;

	request->bodyLength = contentLength;
	if(contentLength > 0) {
		request->body = malloc(contentLength);
		if(request->body == NULL) return false;

		memcpy(request->body, buffer + bodyStart, contentLength);
	}

	if(contentLength != length - bodyStart) return false;

	return true;
}

bool orcaRequestParse(const char *buffer, size_t length, OrcaRequest *request) {
	if(buffer == NULL || request == NULL) return false;

	*request = (OrcaRequest){0};

	RequestTokenArray tokens = requestLexer(buffer, length);
	if(tokens.tokens == NULL) return false;

	bool success = requestParse(buffer, length, &tokens, request);

	free(tokens.tokens);

	if(!success) {
		orcaRequestFree(request);
		return false;
	}

	return true;
}

void orcaRequestFree(OrcaRequest *request) {
	if(request == NULL) return;

	if(request->url != NULL) free(request->url);
	if(request->headerArray.headers != NULL) {
		for(size_t i = 0; i < request->headerArray.length; i++) {
			free(request->headerArray.headers[i].name);
			free(request->headerArray.headers[i].value);
		}
		free(request->headerArray.headers);
	}
	if(request->body != NULL) free(request->body);

	*request = (OrcaRequest){0};
}
