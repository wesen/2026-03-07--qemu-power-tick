#ifndef WL_NET_H
#define WL_NET_H

#include <stdint.h>

#include "wl_app_core.h"

void attempt_connect(struct app *app);
void handle_socket_event(struct app *app, uint32_t events);
void inspect_socket_health_after_resume(struct app *app);

#endif
