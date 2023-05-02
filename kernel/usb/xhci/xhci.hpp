#pragma once

#include "error.hpp"

namespace usb{
    namespace xhci{
        struct Controller{
            Controller(const uint64_t& addr){}
            Error Initialize(){
                return Error(Error::kSuccess, NULL, 0);
            }
            void Run(){}
        };
    }  // namespace xhci
}