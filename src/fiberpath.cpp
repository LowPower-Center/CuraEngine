#include "fiberpath.h"

namespace cura
{
FiberPath::FiberPath()
{
}
FiberPath::FiberPath(coord_t z)
    : z_(z)
{
}
FiberPaths::FiberPaths()
{
    sorted = false;
}
} // namespace cura
