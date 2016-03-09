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

#ifndef MEDIAMANAGERFACTORY_H_
#define MEDIAMANAGERFACTORY_H_

#include <memory>

#include "basesourcemediamanager.h"

namespace mcs {

// Only here to make unit testing easier for the factory class
class NullSourceMediaManager : public BaseSourceMediaManager {
public:
    void Play() override;
    void Pause() override;
    void Teardown() override;
    bool IsPaused() const override;

protected:
    bool Configure() override;
};

class MediaManagerFactory {
public:
    static std::shared_ptr<BaseSourceMediaManager> CreateSource(const std::string &remote_address);
};
} // namespace mcs
#endif
