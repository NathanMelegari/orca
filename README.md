# Orca

`C` `HTTP` `Socket` `Linux`

Orca is a HTTP library written in C

# Objective



# Example

```c
#include "http/orca_http.h"
#include <stdio.h>

static void home(OrcaRequest *req, OrcaResponse *res) { printf("Hello from Orca!\n"); };
int main(void) {
    OrcaApp *app = orcaCreateApp();
    if(app == NULL) return 1;

    if(!orcaGet(app, "/", home)) {
        orcaDestroyApp(app);
        return 1;
    }

    orcaListenAndServe(app, 8080);

    orcaDestroyApp(app);
    return 0;
}
```

See 'examples/' for examples about how to use the Orca library.

# License

This software is licensed under the MIT License. See 'LICENSE' for more information.
