#pragma once

#include <systemd/sd-bus.h>

#define _cleanup_(f) __attribute__((cleanup(f)))

static const char *const DBUS_NAME = "org.dbus.Test1";
static const char *const DBUS_PATH = "/org/dbus/Test1";
static const char *const DBUS_INTERFACE = "org.dbus.Test1";
