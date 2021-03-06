/*
 * Copyright (C) 2015 Canonical, Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <ac/logger.h>
#include <ac/keep_alive.h>
#include <ac/dbus/helpers.h>

extern "C" {
// Ignore all warnings coming from the external headers as we don't
// control them and also don't want to get any warnings from them
// which will only pollute our build output.
#pragma GCC diagnostic push
#pragma GCC diagnostic warning "-w"
#include "wpasupplicantinterface.h"
#pragma GCC diagnostic pop
}

#include "interfaceselector.h"

namespace w11tng {

InterfaceSelector::Ptr InterfaceSelector::Create() {
    return std::shared_ptr<InterfaceSelector>(new InterfaceSelector)->FinalizeConstruction();
}

InterfaceSelector::InterfaceSelector() {
}

InterfaceSelector::~InterfaceSelector() {
}

InterfaceSelector::Ptr InterfaceSelector::FinalizeConstruction() {
    auto sp = shared_from_this();

    GError *error = nullptr;
    connection_.reset(g_bus_get_sync(G_BUS_TYPE_SYSTEM, nullptr, &error));
    if (!connection_) {
        AC_ERROR("Failed to connect to system bus: %s", error->message);
        g_error_free(error);
        return sp;
    }

    return sp;
}

void InterfaceSelector::SetDelegate(const std::weak_ptr<Delegate> &delegate) {
    delegate_ = delegate;
}

void InterfaceSelector::ResetDelegate() {
    delegate_.reset();
}

void InterfaceSelector::Process(const InterfaceList &interfaces) {
    if (interfaces_.size() > 0)
        return;

    interfaces_ = interfaces;

    TryNextInterface();
}

void InterfaceSelector::TryNextInterface() {
    if (interfaces_.size() == 0) {
        if (auto sp = delegate_.lock())
            sp->OnInterfaceSelectionDone("");
        return;
    }

    auto object_path = interfaces_.back();
    interfaces_.pop_back();

    AC_DEBUG("Looking at %s", object_path);

    wpa_supplicant_interface_proxy_new(connection_.get(),
                                       G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START,
                                       kBusName,
                                       object_path.c_str(),
                                       nullptr, [](GObject *source, GAsyncResult *res, gpointer user_data) {

            auto inst = static_cast<ac::SharedKeepAlive<InterfaceSelector>*>(user_data)->ShouldDie();

            GError *error = nullptr;
            auto proxy = wpa_supplicant_interface_proxy_new_finish(res, &error);
            if (!proxy) {
                AC_ERROR("Failed to create proxy for interface: %s", error->message);
                g_error_free(error);
                return;
            }

            bool supports_p2p = false;

            auto capabilities = wpa_supplicant_interface_get_capabilities(proxy);
            ac::dbus::Helpers::ParseDictionary(capabilities, [&](const std::string &key, GVariant *value) {
                ac::dbus::Helpers::ParseArray(g_variant_get_variant(value), [&](GVariant *mode) {
                    if (std::string(g_variant_get_string(mode, nullptr)) == "p2p")
                        supports_p2p = true;
                });
            }, "Modes");

            if (supports_p2p) {
                AC_DEBUG("Found interface which supports P2P");
                // We take the first interface which supports p2p here and ignore
                // all others. That is really enough for now.
                if (auto sp = inst->delegate_.lock())
                    sp->OnInterfaceSelectionDone(g_dbus_proxy_get_object_path(G_DBUS_PROXY(proxy)));
                return;
            }
            else {
                AC_DEBUG("Interface %s does not support P2p, ignoring.",
                         g_dbus_proxy_get_object_path(G_DBUS_PROXY(proxy)));
            }

            g_object_unref(proxy);

            inst->TryNextInterface();

    }, new ac::SharedKeepAlive<InterfaceSelector>{shared_from_this()});
}

} // w11tng
