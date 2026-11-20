#ifndef ORCA_HTTP_LIBRARY_H
#define ORCA_HTTP_LIBRARY_H


#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef enum OrcaMethod {
	ORCA_METHOD_GET,
	ORCA_METHOD_POST
} OrcaMethod;

typedef struct OrcaRequest OrcaRequest;
typedef struct OrcaResponse OrcaResponse;

typedef void (*OrcaHandler)(OrcaRequest *, OrcaResponse *);

typedef struct OrcaRoute {
	char *url;
	OrcaMethod method;
	OrcaHandler handler;
} OrcaRoute;

typedef struct OrcaRouteArray {
	OrcaRoute *routes;
	size_t length;
	size_t capacity;
} OrcaRouteArray;

typedef struct OrcaRouter {
	OrcaRouteArray routesArray;
} OrcaRouter;

typedef struct OrcaHeader {
	char *name;
	char *value;
} OrcaHeader;

typedef struct OrcaHeaderArray {
	OrcaHeader *headers;
	size_t length;
	size_t capacity;
} OrcaHeaderArray;

typedef struct OrcaRequest {
	char *url;
	OrcaMethod method;
	OrcaHeaderArray headerArray;
	char *body;
	size_t bodyLength;
} OrcaRequest;

typedef struct OrcaResponse {
	uint16_t status;
	char *contentType;
	char *body;
	size_t bodyLength;
} OrcaResponse;

typedef struct OrcaApp {
	bool is_running;
	OrcaRouter router;
} OrcaApp;


OrcaApp *orcaCreateApp(void);
void orcaDestroyApp(OrcaApp *);

bool orcaGet(OrcaApp *, const char *url, OrcaHandler);
bool orcaPost(OrcaApp *, const char *url, OrcaHandler);
OrcaHandler orcaRouterResolver(OrcaRouter *, OrcaRequest *);

bool orcaRequestParse(const char *buffer, size_t length, OrcaRequest *request);
void orcaRequestFree(OrcaRequest *request);

bool orcaResponseSerialize(const OrcaResponse *response, char **buffer, size_t *length);
void orcaResponseFree(OrcaResponse *response);

bool orcaListenAndServe(OrcaApp *, uint32_t port);

char *orcaLoadHTML(const char *path);

#endif
