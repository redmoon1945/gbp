/*
 *  Copyright (C) 2024-2025 Claude Dumas <claudedumas63@protonmail.com>. All rights reserved.
 *  DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/#AGPL/>.
 */

#ifndef CUSTOMQCHARTVIEW_H
#define CUSTOMQCHARTVIEW_H

#include <QChartView>
#include <QChart>

class CustomQChartView : public QChartView
{
public:
    CustomQChartView(QChart* chart, bool rotatedAwayZoomIn, QWidget *parent = 0);
    void zoom(qreal factor);

    bool getWheelRotatedAwayZoomIn() const;
    void setWheelRotatedAwayZoomIn(bool newWheelRotatedAwayZoomIn);

protected:
    void wheelEvent(QWheelEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    double zoomInFactor;
    double zoomOutFactor;
    // min, max value for zoom in /out
    quint64 xAxisRangeMaxInSec;
    quint64 xAxisRangeMinInSec;
    double yAxisRangeMaxInSec;
    double yAxisRangeMinInSec;

    QPointF m_lastMousePos;  // last now position of the mouse

    // If true :
    //   vertical wheel rotating AWAY from the user will ZOOM IN
    // If False
    //   vertical wheel rotating TOWARD the user will ZOOM IN
    bool wheelRotatedAwayZoomIn;
};

#endif // CUSTOMQCHARTVIEW_H
