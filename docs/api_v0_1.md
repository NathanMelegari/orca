# Orca API v0.1

This document describes the public API provided by Orca.

# Enums

## OrcaMethod

Defines the supported HTTP request methods:
- `ORCA_METHOD_GET`
- `ORCA_METHOD_POST`

# Structs

## OrcaRoute

Represents an HTTP route, containing its URL, request method, and handler.

## OrcaRouteArray

A dynamic array of `OrcaRoute` elements.

## OrcaRouter

Stores and manages the routes registered in an application.

## OrcaHeader

Represents an HTTP header, containing its name and value.

## OrcaHeaderArray

A dynamic array of `OrcaHeader` elements.

## OrcaRequest

Represents an HTTP request, containing its URL, method, headers, and body.

## OrcaResponse

Represents an HTTP response, containing its status code, content type, and body.

## OrcaApp

The main application context. Contains the application state and its router.

# Functions

## orcaCreateApp(void)

Creates and initializes the main application context.

Returns a pointer to `OrcaApp`, or `NULL` if allocation fails.

## orcaDestroyApp(OrcaApp *app)

Destroys the application context and releases its allocated resources.

## orcaGet(OrcaApp *app, const char *url, OrcaHandler handler)

Registers a route that handles HTTP GET requests for the specified URL.

## orcaPost(OrcaApp *app, const char *url, OrcaHandler handler)

Registers a route that handles HTTP POST requests for the specified URL.

## orcaRouterResolver(OrcaRouter *router, OrcaRequest *request)

Searches for a registered route matching the request method and URL.

Returns the corresponding handler, or `NULL` if no matching route is found.

## orcaRequestParse(const char *buffer, size_t length, OrcaRequest *request)

Parses an HTTP request from the provided buffer and stores the result in an `OrcaRequest` structure.

## orcaRequestFree(OrcaRequest *request)

Releases the resources allocated for an `OrcaRequest` structure.

## orcaResponseSerialize(const OrcaResponse *response, char **buffer, size_t *length)

Serializes an `OrcaResponse` into an HTTP response buffer, preparing it for transmission.

## orcaResponseFree(OrcaResponse *response)

Releases the resources allocated for an `OrcaResponse` structure.

## orcaListenAndServe(OrcaApp *app, uint32_t port)

Initializes the HTTP server and starts listening for incoming connections on the specified port.

# Types

## OrcaHandler

```c
typedef void (*OrcaHandler)(OrcaRequest *req, OrcaResponse *res);
```

A function pointer type used to define HTTP request handlers.

Handlers receive an `OrcaRequest` containing the incoming request and an `OrcaResponse` to be populated with the response.
