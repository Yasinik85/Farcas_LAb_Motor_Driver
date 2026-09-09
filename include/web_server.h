#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include "motor.h"

// Starts the HTTP server that serves the motor control GUI and its
// REST-ish API. `motor` must stay alive for as long as the server runs.
void web_server_start(Motor *motor);

#endif
