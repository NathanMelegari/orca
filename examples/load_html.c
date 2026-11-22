#include "../http/orca_http.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

void home(OrcaRequest *req, OrcaResponse *res) {
	if(req == NULL || res == NULL) return;

	res->status = 200;
	res->contentType = "text/html";
	res->body =
		"<!DOCTYPE html>"
		"<html>"
		"	<head>"
		"		<meta charset='utf-8'>"
		"		<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
		"		<title>Orca HTTP</title>"
		"	</head>"
		"	<body>"
		"		<header>"
		"			<h1>Orca HTTP</h1>"
		"			<span> A HTML page rendered with Orca HTTP</span>"
		"		</header>"
		"		<main>"
		"			<h2>Liked the repositiory</h2>"
		"			<a href='https://github.com/NathanMelegari/orca'>orca-repo</a>"
		"		</main>"
		"		<footer>"
		"			<span>Copyright &copy; 2026 Nathan Melegari</span>"
		"		</footer>"
		"	</body>"
		"</html>";
	res->bodyLength = strlen(res->body);

	time_t now = time(NULL);
	struct tm *localTime = localtime(&now);

	printf("%02d:%02d:%02d - Client connected on :8080\n", localTime->tm_hour, localTime->tm_min, localTime->tm_sec);
}

int main(void) {

	OrcaApp *app = orcaCreateApp();
	if(app == NULL) {
		fprintf(stderr, "Error: OrcaApp * is NULL\n");
		return EXIT_FAILURE;
	}

	const char *url = "/";
	if(!orcaGet(app, url, home)) {
		fprintf(stderr, "Error: failed in route '%s'\n", url);
		orcaDestroyApp(app);
		return EXIT_FAILURE;
	}

	orcaListenAndServe(app, 8080);

	orcaDestroyApp(app);
	return EXIT_SUCCESS;
}
