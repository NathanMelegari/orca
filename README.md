# Orca

`C` `HTTP` `TCP/IP` `Sockets`

Orca is a lightweight HTTP server library written in C.

## Objective

Orca aims to provide a simple and lightweight interface for building HTTP servers in C, while keeping the underlying networking and request handling mechanisms explicit.

## Example

```c
#include "http/orca_http.h"
#include <stdio.h>

static void home(OrcaRequest *req, OrcaResponse *res) {
    (void)req;

    res->status = 200;
    res->contentType = "text/plain";
    res->body = "Hello from Orca!";
    res->bodyLength = 16;
}

int main(void) {
    OrcaApp *app = orcaCreateApp();
    if(app == NULL) return 1;

    if(!orcaGet(app, "/", home)) {
        orcaDestroyApp(app);
        return 1;
    }

    if(!orcaListenAndServe(app, 8080)) {
        orcaDestroyApp(app);
        return 1;
    }

    orcaDestroyApp(app);
    return 0;
}

See 'examples/' for more usage examples.

# License

This project is licensed under the MIT License. See 'LICENSE' for details.
