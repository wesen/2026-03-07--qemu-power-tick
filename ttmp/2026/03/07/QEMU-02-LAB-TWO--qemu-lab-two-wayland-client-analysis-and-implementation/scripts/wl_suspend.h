#ifndef WL_SUSPEND_H
#define WL_SUSPEND_H

#include "wl_app_core.h"

void reset_idle_timer(struct app *app);
void maybe_exit_on_runtime_limit(struct app *app);
void enter_suspend_to_idle(struct app *app);

#endif
