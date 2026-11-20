#include "orca_http.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

static const char *getStatusMessage(uint16_t status) {
        switch(status) {
                case 200: return "OK";
                case 201: return "Created";
                case 204: return "No Content";
                case 400: return "Bad Request";
                case 404: return "Not Found";
                case 405: return "Method Not Allowed";
                case 413: return "Content Too Large";
                case 500: return "Internal Server Error";
                default: return "Unknown";
        }
}

bool orcaResponseSerialize(const OrcaResponse *response, char **buffer, size_t *length) {
        if(response == NULL || buffer == NULL || length == NULL) {
                return false;
        }

        *buffer = NULL;
        *length = 0;

        if(response->status == 0 || response->contentType == NULL) {
                return false;
        }

        if(response->body == NULL && response->bodyLength > 0) {
                return false;
        }

        const char *statusMessage = getStatusMessage(response->status);

        int headerLength = snprintf(
                NULL,
                0,
                "HTTP/1.1 %u %s\r\n"
                "Content-Type: %s\r\n"
                "Content-Length: %zu\r\n"
                "Connection: close\r\n"
                "\r\n",
                response->status,
                statusMessage,
                response->contentType,
                response->bodyLength
        );

        if(headerLength < 0) return false;

        size_t totalLength = (size_t)headerLength;

        if(response->bodyLength > SIZE_MAX - totalLength) {
                return false;
        }

        totalLength += response->bodyLength;

        char *result = malloc(totalLength == 0 ? 1 : totalLength);
        if(result == NULL) return false;

        snprintf(
                result,
                (size_t)headerLength + 1,
                "HTTP/1.1 %u %s\r\n"
                "Content-Type: %s\r\n"
                "Content-Length: %zu\r\n"
                "Connection: close\r\n"
                "\r\n",
                response->status,
                statusMessage,
                response->contentType,
                response->bodyLength
        );

        if(response->bodyLength > 0) {
                memcpy(
                        result + headerLength,
                        response->body,
                        response->bodyLength
                );
        }

        *buffer = result;
        *length = totalLength;

        return true;
}
