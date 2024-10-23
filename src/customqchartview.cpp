/*
 *  Copyright (C) 2024 Claude Dumas <claudedumas63@protonmail.com>. All rights reserved.
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

#include "customqchartview.h"


CustomQChartView::CustomQChartView(QChart *chart, bool rotatedAwayZoomIn, QWidget *parent) :
    QChartView(chart,parent)
{
    this->wheelRotatedAwayZoomIn = rotatedAwayZoomIn;

    // one followed by the other gives an effect of 1
    zoomInFactor = 0.8;
    zoomOutFactor = (1/zoomInFactor);
}


void CustomQChartView::zoom(qreal factor)
{
    this->chart()->zoom(factor);
}


void CustomQChartView::wheelEvent(QWheelEvent *event)  {
    QChart *theChart = this->chart();
    if(theChart==nullptr){
        return;
    }

    // Returns the relative amount that the wheel was rotated, in eighths of a degree. A positive
    // value indicates that the wheel was rotated forwards away from the user; a negative value
    // indicates that the wheel was rotated backwards toward the user. angleDelta(). y() provides the
    // angle through which the common vertical mouse wheel was rotated since the previous event.
    // angleDelta().x() provides the angle through which the horizontal mouse wheel was rotated, if
    // the mouse has a horizontal wheel;
    QPoint numDegrees = event->angleDelta();

    qreal factor = 1.0;
    if (numDegrees.y() > 0) {
        // wheel AWAY from user
        if(wheelRotatedAwayZoomIn==true){
            factor = zoomOutFactor;
        } else {
            factor = zoomInFactor;
        }
    } else {
        // wheel TOWARD the user
        if(wheelRotatedAwayZoomIn==true){
            factor = zoomInFactor;
        } else {
            factor = zoomOutFactor;
        }
    }

    theChart->zoom(factor);
    event->ignore();    // pass the event to the parent
}


// https://stackoverflow.com/questions/59961372/qcharts-crop-to-rectangle-and-use-horizontal-scroll
void CustomQChartView::mousePressEvent(QMouseEvent *event)
{
    QChart *theChart = this->chart();
    // start remembering mouse position since we are about to start scrolling
    if ( (event->buttons() == Qt::LeftButton) && (theChart!=nullptr) ){
        // event->pos is in viewport coordinate
        m_lastMousePos = mapToScene(event->pos());
    }
    QGraphicsView::mousePressEvent(event); // forward it
}


void CustomQChartView::mouseMoveEvent(QMouseEvent *event) {
    QChart *theChart = this->chart();
    if ( (event->buttons() == Qt::LeftButton) && (theChart!=nullptr) ){
        // event->pos is in viewport coordinate
        QPointF newValue = mapToScene(event->pos());
        QPointF delta = newValue - m_lastMousePos;
        theChart->scroll(-delta.x(), 0);
        theChart->scroll(0, delta.y());
        m_lastMousePos = newValue;
    }
    QGraphicsView::mouseMoveEvent(event); // forward it
}


bool CustomQChartView::getWheelRotatedAwayZoomIn() const
{
    return wheelRotatedAwayZoomIn;
}


void CustomQChartView::setWheelRotatedAwayZoomIn(bool newWheelRotatedAwayZoomIn)
{
    wheelRotatedAwayZoomIn = newWheelRotatedAwayZoomIn;
}

