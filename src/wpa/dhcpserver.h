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

#ifndef DHCPSERVER_H_
#define DHCPSERVER_H_

#include <string>

#include "gdhcp.h"

class DhcpServer {
public:
    class Delegate {
    public:
        virtual void OnLeaseAdded() = 0;
    };

    DhcpServer(Delegate *delegate, const std::string &interface_name_h);
    ~DhcpServer();

    bool Start();
    void Stop();

    std::string LocalAddress() const;

private:
    static void OnDebug(const char *str, gpointer user_data);

private:
    std::string interface_name_;
    int interface_index_;
    GDHCPServer *server_;
};

#endif