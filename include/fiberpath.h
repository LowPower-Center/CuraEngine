
#ifndef FIBERPATH_H
#define FIBERPATH_H

#include "settings/Settings.h"
#include "utils/AABB3D.h"
#include "utils/Matrix4x3D.h"
#include "geometry/OpenPolyline.h"
#include "geometry/Shape.h"

namespace cura
{
/*!
A FiberPath is the container of a single layer fiberpath. It contains all the paths which contains the same z height.
*/
class FiberPath
{
public:
    OpenLinesSet paths;
    coord_t z_;
    FiberPath(coord_t z);
    FiberPath();
    Shape path2shape();
    void translate(Point3LL offset);
    void transform(const Matrix4x3D& transformation);

};

class FiberPaths
{
public:
    std::vector<FiberPath> paths;
    bool sorted;
    FiberPaths();
    void transform(const Matrix4x3D& transformation);
    void sort();
    FiberPath* getFiberPath(const coord_t z);
};



} // namespace cura

#endif // FIBERPATH_H
