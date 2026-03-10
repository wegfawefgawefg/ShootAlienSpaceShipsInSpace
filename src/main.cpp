#include "app.hpp"

int main() {
    App app;
    if (!app_init(app)) {
        return 1;
    }

    app_run(app);
    app_shutdown(app);
    return 0;
}
