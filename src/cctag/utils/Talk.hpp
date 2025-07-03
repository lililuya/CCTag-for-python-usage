/*
 * Copyright 2016, Simula Research Laboratory
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#pragma once

namespace cctag {
namespace talk {

extern bool on;

} // namespace talk
} // namespace cctag

#ifndef CCTAG_NO_COUT
#define DO_TALK(a) if(cctag::talk::on) { a }
#else // CCTAG_NO_COUT
#define DO_TALK(...)
#endif // CCTAG_NO_COUT
