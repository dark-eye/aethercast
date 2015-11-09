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

#ifndef NETWORKUTILS_H_
#define NETWORKUTILS_H_

#include <QString>

class NetworkUtils
{
public:
    static int retriveInterfaceIndex(const QString &name);
    static int modifyAddress(int cmd, int flags, int index, int family,
                             const char *address, const char *peer,
                             unsigned char prefixlen, const char *broadcast);
};

#endif