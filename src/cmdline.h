#pragma once

#include "efibootmgrw.h"

namespace efibootmgrw {

void parse_args(Context& ctx, lak::span<const char *> argv);

}