#pragma once

#include <memory>

#include "application.hpp"

namespace logging {

class LogLayer {
public:
    auto build(std::shared_ptr<core::Application>) -> void;
};

} // namespace logging
