#include "../implot/implot.h"
#include "../implot/implot_internal.h"
#include "plots.hpp"

namespace ImPlot {

void PlotHistogram(const char* label_id, const double *bins, const double *counts, int count) {
    ImDrawList* draw_list = ImPlot::GetPlotDrawList();

    const ImU32 color = IM_COL32(66, 150, 249, 170);

    if (ImPlot::BeginItem(label_id)) {
        if (ImPlot::FitThisFrame()) {
            for (int i = 0; i < count; ++i) {
                ImPlot::FitPoint(ImPlotPoint(bins[i], 0));
                ImPlot::FitPoint(ImPlotPoint(bins[i], counts[i]));
            }
        }
        
        // render data
        for (int i = 0; i < count; ++i) {
            ImVec2 start_pos   = ImPlot::PlotToPixels(bins[i], 0);
            ImVec2 end_pos  = ImPlot::PlotToPixels(bins[i+1], counts[i]);
            draw_list->AddRectFilled(start_pos, end_pos, color);
        }

        ImPlot::EndItem();
    }
}

} // namespace ImPlot
