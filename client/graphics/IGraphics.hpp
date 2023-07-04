#pragma once

#include <memory>

class IGraphics {

};

using pIGraphics = std::unique_ptr<IGraphics>;
