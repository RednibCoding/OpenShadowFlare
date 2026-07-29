#pragma once

#include <memory>

namespace osf::runtime {

class FrameApplication {
public:
    virtual ~FrameApplication() = default;

    FrameApplication(const FrameApplication&) = delete;
    FrameApplication& operator=(const FrameApplication&) = delete;

    virtual bool frame() = 0;

protected:
    FrameApplication() = default;
};

int runApplicationLoop(std::unique_ptr<FrameApplication> application);

}  // namespace osf::runtime
