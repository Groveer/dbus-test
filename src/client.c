#include "client.h"

#include "common.h"

#include <bits/time.h>
#include <systemd/sd-bus.h>

#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

static int tmp_num = 0;
static const int test_times = 10;

int handle_counter_changed(sd_bus_message *msg, void *userdata, sd_bus_error *ret_error)
{
    uint32_t value = 0;
    int ret = sd_bus_message_read_basic(msg, 'u', &value);
    if (ret < 0) {
        printf("read name error: %s\n", strerror(-ret));
        return ret;
    }
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    printf("Signal:%u:Time:%lu\n", value, ts.tv_sec * 1000000 + ts.tv_nsec);
    tmp_num++;
    if (tmp_num == test_times) {
        sd_bus *sdbus = sd_bus_message_get_bus(msg);
        ret =
            sd_bus_call_method(sdbus, DBUS_NAME, DBUS_PATH, DBUS_INTERFACE, "Exit", NULL, NULL, "");
    }
    return ret;
}

void thread_run_method_ltp()
{
    _cleanup_(sd_bus_unrefp) sd_bus *sdbus = NULL;
    _cleanup_(sd_bus_message_unrefp) sd_bus_message *message = NULL;
    _cleanup_(sd_bus_error_free) sd_bus_error error = SD_BUS_ERROR_NULL;
    int ret = sd_bus_default_user(&sdbus);
    if (ret < 0) {
        printf("sd_bus_default_user error: %s\n", strerror(-ret));
        goto finish;
    }
    int n = 0;
    while (n < test_times) {
        int i = 0;
        const char *value;
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        printf("Method:Time1:%lu\n", ts.tv_sec * 1000000 + ts.tv_nsec);
        while (i < 100) {
            ret = sd_bus_call_method(sdbus,
                                     DBUS_NAME,
                                     DBUS_PATH,
                                     DBUS_INTERFACE,
                                     "IncrementBy",
                                     &error,
                                     &message,
                                     "u",
                                     1);
            if (ret < 0) {
                printf("sd_bus_call_method error: %s\n  bus error: %s\n",
                       strerror(-ret),
                       error.message);
                goto finish;
            }
            ret = sd_bus_message_read(message, "u", &value);
            if (ret < 0) {
                printf("sd_bus_message_read error: %s\n", strerror(-ret));
                goto finish;
            }
            i++;
        }
        clock_gettime(CLOCK_REALTIME, &ts);
        printf("Method:Time2:%lu\n", ts.tv_sec * 1000000 + ts.tv_nsec);
        n++;
    }
    ret = sd_bus_call_method(sdbus, DBUS_NAME, DBUS_PATH, DBUS_INTERFACE, "Exit", NULL, NULL, "");
finish:
    exit(0);
}

void thread_run_connection()
{
    _cleanup_(sd_bus_unrefp) sd_bus *sdbus = NULL;
    _cleanup_(sd_bus_message_unrefp) sd_bus_message *message = NULL;
    _cleanup_(sd_bus_error_free) sd_bus_error error = SD_BUS_ERROR_NULL;
    int ret = sd_bus_default_user(&sdbus);
    if (ret < 0) {
        printf("sd_bus_default_user error: %s\n", strerror(-ret));
        goto finish;
    }
    int i = 0;
    uint32_t value = 0;
    while (i < test_times) {
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        printf("Method:Time1:%lu\n", ts.tv_sec * 1000000 + ts.tv_nsec);
        ret = sd_bus_call_method(sdbus,
                                 DBUS_NAME,
                                 DBUS_PATH,
                                 DBUS_INTERFACE,
                                 "IncrementBy",
                                 &error,
                                 &message,
                                 "u",
                                 1);
        if (ret < 0) {
            printf("sd_bus_call_method error: %s\n  bus error: %s\n",
                   strerror(-ret),
                   error.message);
            goto finish;
        }
        value = 0;
        ret = sd_bus_message_read(message, "u", &value);
        if (ret < 0) {
            printf("sd_bus_message_read error: %s\n", strerror(-ret));
            goto finish;
        }
        if (value == 0) {
            printf("value is null\n");
            goto finish;
        }
        clock_gettime(CLOCK_REALTIME, &ts);
        printf("Method:Time2:%lu\n", ts.tv_sec * 1000000 + ts.tv_nsec);
        i++;
    }
finish:
    exit(0);
}

int client_start(int mode)
{
    _cleanup_(sd_bus_message_unrefp) sd_bus_message *message = NULL;
    _cleanup_(sd_event_unrefp) sd_event *event = NULL;
    _cleanup_(sd_bus_unrefp) sd_bus *sdbus = NULL;
    int ret = 0;

    ret = sd_event_default(&event);
    if (ret < 0)
        return ret;
    ret = sd_bus_default_user(&sdbus);
    if (ret < 0)
        return ret;

    pthread_t thread;
    if (mode == 0)
        pthread_create(&thread, NULL, (void *)&thread_run_method_ltp, NULL);
    if (mode == 1) {
        ret = sd_bus_match_signal(sdbus,
                                  NULL,
                                  DBUS_NAME,
                                  DBUS_PATH,
                                  DBUS_INTERFACE,
                                  "CounterChanged",
                                  handle_counter_changed,
                                  NULL);
        pthread_create(&thread, NULL, (void *)&thread_run_connection, NULL);
    }
    if (ret < 0)
        return ret;
    ret = sd_bus_attach_event(sdbus, event, 0);
    if (ret < 0)
        return ret;

    return sd_event_loop(event);
}
