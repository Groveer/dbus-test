#include "server.h"

#include "common.h"

#include <bits/time.h>
#include <systemd/sd-bus-vtable.h>
#include <systemd/sd-bus.h>
#include <systemd/sd-event.h>

#include <stdlib.h>
#include <time.h>

typedef struct
{
    uint32_t count;
} counter_data;

int counter_get(sd_bus *bus,
                const char *path,
                const char *interface,
                const char *property,
                sd_bus_message *reply,
                void *user_data,
                sd_bus_error *error)
{
    counter_data *data = (counter_data *)user_data;
    return sd_bus_message_append(reply, "u", data->count);
}

int counter_set(sd_bus *bus,
                const char *path,
                const char *interface,
                const char *property,
                sd_bus_message *message,
                void *user_data,
                sd_bus_error *error)
{
    int ret = 0;
    uint32_t parameter = 0;
    ret = sd_bus_message_read(message, "u", &parameter);
    if (ret < 0)
        return ret;
    counter_data *data = (counter_data *)user_data;
    data->count = parameter;
    return ret;
}

int increment(sd_bus_message *message, void *user_data, sd_bus_error *error)
{
    int ret = 0;
    uint32_t parameter = 0;
    ret = sd_bus_message_read(message, "u", &parameter);
    if (ret < 0)
        return ret;
    counter_data *data = (counter_data *)user_data;
    data->count += parameter;

    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    printf("%lu\n", ts.tv_sec * 1000000 + ts.tv_nsec);

    sd_bus *sdbus = sd_bus_message_get_bus(message);
    const char *interface = sd_bus_message_get_interface(message);
    const char *path = sd_bus_message_get_path(message);
    ret = sd_bus_emit_signal(sdbus, path, interface, "CounterChanged", "u", data->count);
    if (ret < 0)
        return ret;

    return sd_bus_reply_method_return(message, "u", data->count);
}

int exit_progrma(sd_bus_message *message, void *user_data, sd_bus_error *error)
{
    exit(0);
}

static const sd_bus_vtable counter_vtable[] = {
    SD_BUS_VTABLE_START(0),
    SD_BUS_WRITABLE_PROPERTY(
        "Count", "u", counter_get, counter_set, 0, SD_BUS_VTABLE_PROPERTY_EMITS_CHANGE),
    SD_BUS_METHOD("IncrementBy", "u", "u", increment, SD_BUS_VTABLE_UNPRIVILEGED),
    SD_BUS_METHOD("Exit", "", "", exit_progrma, SD_BUS_VTABLE_UNPRIVILEGED),
    SD_BUS_SIGNAL("CounterChanged", "u", 0),
    SD_BUS_VTABLE_END
};

int server_start()
{
    _cleanup_(sd_event_unrefp) sd_event *event = NULL;
    _cleanup_(sd_bus_unrefp) sd_bus *sdbus = NULL;

    int ret = 0;
    ret = sd_event_default(&event);
    if (ret < 0)
        return ret;
    ret = sd_bus_default_user(&sdbus);
    if (ret < 0)
        return ret;
    counter_data data;
    data.count = 0;

    ret = sd_bus_add_object_vtable(sdbus, NULL, DBUS_PATH, DBUS_INTERFACE, counter_vtable, &data);
    if (ret < 0)
        return ret;
    ret = sd_bus_request_name(sdbus, DBUS_NAME, SD_BUS_NAME_REPLACE_EXISTING);
    if (ret < 0)
        return ret;

    ret = sd_bus_attach_event(sdbus, event, 0);
    if (ret < 0)
        return ret;

    return sd_event_loop(event);
}
