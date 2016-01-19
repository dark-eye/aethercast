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

#include <gio/gio.h>

#include <asm/types.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <sys/socket.h>
#include <sys/prctl.h>

#include <boost/filesystem.hpp>

#include <mcs/logger.h>
#include <mcs/networkutils.h>
#include <mcs/keep_alive.h>
#include <mcs/scoped_gobject.h>

#include <w11tng/config.h>

#include "dhcpclient.h"

namespace w11tng {

DhcpClient::Ptr DhcpClient::Create(const std::weak_ptr<Delegate> &delegate, const std::string &interface_name) {
    return std::shared_ptr<DhcpClient>(new DhcpClient(delegate, interface_name));
}

DhcpClient::DhcpClient(const std::weak_ptr<Delegate> &delegate, const std::string &interface_name) :
    delegate_(delegate),
    interface_name_(interface_name),
    pid_(-1),
    process_watch_(0) {
}

DhcpClient::~DhcpClient() {
    Stop();
}

mcs::IpV4Address DhcpClient::LocalAddress() const {
    return local_address_;
}

bool DhcpClient::Start() {
    MCS_DEBUG("");

    if (pid_ > 0)
        return true;

    if (!netlink_listener_) {
        netlink_listener_ = w11tng::NetlinkListener::Create(shared_from_this());
        // We're only interested in events for the network interface we're
        // operating on.
        netlink_listener_->SetInterfaceFilter(interface_name_);
    }

    lease_file_path_ = mcs::Utils::Sprintf("%s/aethercast-dhcp-client-leases-%s",
                                    boost::filesystem::temp_directory_path().string(),
                                    boost::filesystem::unique_path().string());
    if (!mcs::Utils::CreateFile(lease_file_path_)) {
        MCS_ERROR("Failed to create database for DHCP leases at %s",
                  lease_file_path_);
        return false;
    }

    auto argv = g_ptr_array_new();

    g_ptr_array_add(argv, (gpointer) kDhcpClientPath);

    // Disable background on lease (let dhcpd not fork)
    g_ptr_array_add(argv, (gpointer) "-d");
    // Don't be verbose
    g_ptr_array_add(argv, (gpointer) "-q");

    g_ptr_array_add(argv, (gpointer) "-v");

    // We only want dhclient to operate on the P2P interface an no other
    g_ptr_array_add(argv, (gpointer) interface_name_.c_str());

    g_ptr_array_add(argv, nullptr);

    auto cmdline = g_strjoinv(" ", reinterpret_cast<gchar**>(argv->pdata));
    MCS_DEBUG("Running dhclient with: %s", cmdline);
    g_free(cmdline);

    GError *error = nullptr;
    if (!g_spawn_async(nullptr, reinterpret_cast<gchar**>(argv->pdata), nullptr,
                       GSpawnFlags(G_SPAWN_DO_NOT_REAP_CHILD),
                       [](gpointer user_data) { prctl(PR_SET_PDEATHSIG, SIGKILL); },
                       nullptr, &pid_, &error)) {
        MCS_ERROR("Failed to spawn DHCP client: %s", error->message);
        g_error_free(error);
        g_ptr_array_free(argv, TRUE);
        return false;
    }

    process_watch_ = g_child_watch_add_full(0, pid_, [](GPid pid, gint status, gpointer user_data) {
        auto inst = static_cast<mcs::WeakKeepAlive<DhcpClient>*>(user_data)->GetInstance().lock();

        if (!WIFEXITED(status))
            MCS_WARNING("DHCP client (pid %d) exited with status %d", pid, status);
        else
            MCS_DEBUG("DHCP client (pid %d) successfully terminated", pid);

        if (not inst)
            return;

        inst->pid_ = -1;
    }, new mcs::WeakKeepAlive<DhcpClient>(shared_from_this()), [](gpointer data) { delete static_cast<mcs::WeakKeepAlive<DhcpClient>*>(data); });

    return true;
}

void DhcpClient::Stop() {
    if (pid_ <= 0)
        return;

    ::kill(pid_, SIGTERM);
    g_spawn_close_pid(pid_);

    pid_ = 0;

    if (process_watch_ > 0)
        g_source_remove(process_watch_);

    ::unlink(lease_file_path_.c_str());
}

void DhcpClient::OnInterfaceAddressChanged(const std::string &interface, const std::string &address) {
    if (interface != interface_name_)
        return;

    auto ipv4_addr = mcs::IpV4Address::from_string(address);
    if (ipv4_addr == local_address_)
        return;

    local_address_ = ipv4_addr;

    if (auto sp = delegate_.lock())
        sp->OnAddressAssigned(ipv4_addr);
}

}
