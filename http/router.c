#include "orca_http.h"
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

static bool reallocRoutesArrayCapacity(OrcaRouter *router) {
	if(router == NULL) return false;

	size_t new_capacity = router->routesArray.capacity == 0 ? 4 :
		router->routesArray.capacity * 2;

	OrcaRoute *new_routes = realloc(
			router->routesArray.routes,
			new_capacity * sizeof(*router->routesArray.routes)
	);
	if(new_routes == NULL) {
		return false;
	}

	router->routesArray.routes = new_routes;
	router->routesArray.capacity = new_capacity;

	return true;
}

static bool pushRoute(OrcaRouter *router, OrcaRoute *route) {
	if(router == NULL || route == NULL) return false;

	if(router->routesArray.length >= router->routesArray.capacity) {
		if(!reallocRoutesArrayCapacity(router)) return false;
	}

	router->routesArray.routes[router->routesArray.length] = *route;
	router->routesArray.length++;

	return true;
}

bool orcaGet(OrcaApp *app, const char *url, OrcaHandler handler) {
	if(app == NULL || url == NULL || handler == NULL) return false;

	char *routeUrl = strdup(url);
	if(routeUrl == NULL) return false;

	OrcaRoute route = {
		.method = ORCA_METHOD_GET,
		.url = routeUrl,
		.handler = handler
	};

	if(!pushRoute(&app->router, &route)) {
		free(route.url);
		return false;
	}

	return true;
}

bool orcaPost(OrcaApp *app, const char *url, OrcaHandler handler) {
	if(app == NULL || url == NULL || handler == NULL) return false;

	char *routeUrl = strdup(url);
	if(routeUrl == NULL) return false;

	OrcaRoute route = {
		.method = ORCA_METHOD_POST,
		.url = routeUrl,
		.handler = handler
	};

	if(!pushRoute(&app->router, &route)) {
		free(route.url);
		return false;
	}

	return true;
}

OrcaHandler orcaRouterResolver(OrcaRouter *router, OrcaRequest *req) {
	if(router == NULL || req == NULL) return NULL;

	for(size_t i = 0; i < router->routesArray.length; i++) {
		OrcaRoute *route = &router->routesArray.routes[i];

		if(route->method == req->method && strcmp(route->url, req->url) == 0) {
			return route->handler;
		}
	}

	return NULL;
}
