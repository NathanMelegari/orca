#include "orca_http.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

OrcaApp *orcaCreateApp(void) {
	OrcaApp *app = calloc(1, sizeof(*app));
	if(app == NULL) {
		fprintf(stderr, "Error: failed to allocate context\n");
		return NULL;
	}

	app->is_running = true;

	app->router.routesArray.capacity = 12;
	app->router.routesArray.routes = malloc(
			app->router.routesArray.capacity *
			sizeof(*app->router.routesArray.routes)
	);
	if(app->router.routesArray.routes == NULL) {
		free(app);
		return NULL;
	}

	return app;
}

void orcaDestroyApp(OrcaApp *app) {
	if(app == NULL) return;

	if(app->router.routesArray.routes != NULL) {
		for(size_t i = 0; i <= app->router.routesArray.length; i++) {
			free(app->router.routesArray.routes[i].url);
		}
		free(app->router.routesArray.routes);
	}

	free(app);
}
