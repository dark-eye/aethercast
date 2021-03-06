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

#include <stdexcept>

#include "ac/forwardingcontroller.h"

namespace ac {
ForwardingController::ForwardingController(const Controller::Ptr& fwd) : fwd_{fwd} {
    if (not fwd_) {
        throw std::logic_error{"Cannot operate without a valid controller instance."};
    }
}

void ForwardingController::SetDelegate(const std::weak_ptr<Controller::Delegate> &delegate) {
    fwd_->SetDelegate(delegate);
}

void ForwardingController::ResetDelegate() {
    fwd_->ResetDelegate();
}

void ForwardingController::Connect(const NetworkDevice::Ptr &device, ResultCallback callback) {
    fwd_->Connect(device, callback);
}

void ForwardingController::Disconnect(const NetworkDevice::Ptr &device, ResultCallback callback) {
    fwd_->Disconnect(device, callback);
}

void ForwardingController::DisconnectAll(ResultCallback callback) {
    fwd_->DisconnectAll(callback);
}

ac::Error ForwardingController::Scan(const std::chrono::seconds &timeout) {
    return fwd_->Scan(timeout);
}

NetworkDeviceState ForwardingController::State() const {
    return fwd_->State();
}

std::vector<NetworkManager::Capability> ForwardingController::Capabilities() const {
    return fwd_->Capabilities();
}

bool ForwardingController::Scanning() const {
    return fwd_->Scanning();
}

bool ForwardingController::Enabled() const {
    return fwd_->Enabled();
}

Error ForwardingController::SetEnabled(bool enabled) {
    return fwd_->SetEnabled(enabled);
}
}
