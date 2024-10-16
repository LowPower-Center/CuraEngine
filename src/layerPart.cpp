// Copyright (c) 2023 UltiMaker
// CuraEngine is released under the terms of the AGPLv3 or higher.

#include "layerPart.h"

#include "geometry/OpenPolyline.h"
#include "progress/Progress.h"
#include "settings/EnumSettings.h" //For ESurfaceMode.
#include "settings/Settings.h"
#include "sliceDataStorage.h"
#include "slicer.h"
#include "utils/OpenPolylineStitcher.h"
#include "utils/Simplify.h" //Simplifying the layers after creating them.
#include "utils/ThreadPool.h"
#include "fiberpath.h"
/*
The layer-part creation step is the first step in creating actual useful data for 3D printing.
It takes the result of the Slice step, which is an unordered list of polygons_, and makes groups of polygons_,
each of these groups is called a "part", which sometimes are also known as "islands". These parts represent
isolated areas in the 2D layer with possible holes.

Creating "parts" is an important step, as all elements in a single part should be printed before going to another part.
And all every bit inside a single part can be printed without the nozzle leaving the boundary of this part.

It's also the first step that stores the result in the "data storage" so all other steps can access it.
*/

namespace cura
{

void createLayerWithParts(const Settings& settings, SliceLayer& storageLayer, SlicerLayer* layer)
{
    OpenPolylineStitcher::stitch(layer->open_polylines_, storageLayer.open_polylines, layer->polygons_, settings.get<coord_t>("wall_line_width_0"));

    storageLayer.open_polylines = Simplify(settings).polyline(storageLayer.open_polylines);

    const bool union_all_remove_holes = settings.get<bool>("meshfix_union_all_remove_holes");
    if (union_all_remove_holes)
    {
        for (unsigned int i = 0; i < layer->polygons_.size(); i++)
        {
            if (layer->polygons_[i].orientation())
                layer->polygons_[i].reverse();
        }
    }

    std::vector<SingleShape> result;
    const bool union_layers = settings.get<bool>("meshfix_union_all");
    const ESurfaceMode surface_only = settings.get<ESurfaceMode>("magic_mesh_surface_mode");
    if (surface_only == ESurfaceMode::SURFACE && ! union_layers)
    { // Don't do anything with overlapping areas; no union nor xor
        result.reserve(layer->polygons_.size());
        for (const Polygon& poly : layer->polygons_)
        {
            if (poly.empty())
            {
                continue;
            }
            result.emplace_back();
            result.back().push_back(poly);
        }
    }
    else
    {
        result = layer->polygons_.splitIntoParts(union_layers || union_all_remove_holes);
    }

    for (auto& part : result)
    {
        storageLayer.parts.emplace_back();
        if (part.empty())
        {
            continue;
        }
        storageLayer.parts.back().outline = part;
        storageLayer.parts.back().boundaryBox.calculate(storageLayer.parts.back().outline);
        if (storageLayer.parts.back().outline.empty())
        {
            storageLayer.parts.pop_back();
        }
    }
}

void insertFiberPath(SliceMeshStorage& meshStorage, FiberPaths& fiberpath)
{
    for (LayerIndex layer_nr = 0; layer_nr < meshStorage.layers.size() - 1; layer_nr++)
    {
        SliceLayer& layer = meshStorage.layers[layer_nr];
        size_t z_height = layer.printZ;
        for (FiberPath& path : fiberpath.paths)
        {
            size_t match_z = path.z_;
            if (std::fabs(match_z - z_height) < (layer.thickness / 2 - 1))
            {
                for (SliceLayerPart& part : layer.parts)
                {
                    OpenLinesSet resLines = path.paths.lineCut(part.outline);
                    if (resLines.size() > 0)
                    {
                        part.fiberpath.push_back(resLines);
                    }
                }
            }

        }
    }
}
void createLayerParts(SliceMeshStorage& mesh, Slicer* slicer)
{
    const auto total_layers = slicer->layers.size();
    assert(mesh.layers.size() == total_layers);

    cura::parallel_for<size_t>(
        0,
        total_layers,
        [slicer, &mesh](size_t layer_nr)
        {
            SliceLayer& layer_storage = mesh.layers[layer_nr];
            SlicerLayer& slice_layer = slicer->layers[layer_nr];
            createLayerWithParts(mesh.settings, layer_storage, &slice_layer);
        });

    for (LayerIndex layer_nr = total_layers - 1; layer_nr >= 0; layer_nr--)
    {
        SliceLayer& layer_storage = mesh.layers[layer_nr];
        if (layer_storage.parts.size() > 0 || (mesh.settings.get<ESurfaceMode>("magic_mesh_surface_mode") != ESurfaceMode::NORMAL && layer_storage.open_polylines.size() > 0))
        {
            mesh.layer_nr_max_filled_layer = layer_nr; // last set by the highest non-empty layer
            break;
        }
    }
}

} // namespace cura
