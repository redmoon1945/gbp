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

#include "customqchartview.h"
#include "currencyhelper.h"
#include <qdatetime.h>
#include <qdatetimeaxis.h>
#include <qvalueaxis.h>


CustomQChartView::CustomQChartView(QChart *chart, bool rotatedAwayZoomIn, QWidget *parent) :
    QChartView(chart,parent)
{
    this->wheelRotatedAwayZoomIn = rotatedAwayZoomIn;

    // one followed by the other gives an effect of 1
    zoomInFactor = 0.8;
    zoomOutFactor = (1/zoomInFactor);
    // calculate limits values for zooming
    xAxisRangeMinInSec = 24 * 60 * 60; // 1 day
    xAxisRangeMaxInSec = static_cast<quint64>(200 * (365.25*24*60*60)); // 200 average-length year
    yAxisRangeMinInSec = 1; // 1 unit of currency
    yAxisRangeMaxInSec = 2 * CurrencyHelper::maxValueAllowedForAmountInDouble(0);
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

    // *** we need a series to convert to "series domain" ***
    QList<QAbstractSeries*> seriesList = theChart->series();
    if (seriesList.size()==0) {
        return;
    }

    // *** Returns the relative amount that the wheel was rotated, in eighths of a degree. A
    // positive value indicates that the wheel was rotated forwards away from the user; a negative
    // value indicates that the wheel was rotated backwards toward the user. angleDelta(). y()
    // provides the angle through which the common vertical mouse wheel was rotated since the
    // previous event. angleDelta().x() provides the angle through which the horizontal mouse wheel
    // was rotated, if the mouse has a horizontal wheel ***
    QPoint numDegrees = event->angleDelta();

    // *** Build the final zoom factor ***
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

    // *** get the domain coordinated of the mouse point ***
    QPointF pt1 = event->position();
    QPointF pt2 = theChart->mapToValue(pt1.toPoint(), seriesList[0]);
    QDateTime mousePointDate = QDateTime::fromMSecsSinceEpoch(pt2.x());
    double mousePointValue = pt2.y();

    // *** X AXIS calculation ***
    QList<QAbstractAxis *> axList = theChart->axes(Qt::Horizontal);
    if(axList.count() != 1){
        return; // should not happen
    }
    QDateTimeAxis* xAxis = dynamic_cast<QDateTimeAxis*>(axList.at(0));
    if (xAxis==nullptr) {
        return;  // should not happen
    }
    QDateTime xMin = xAxis->min();
    QDateTime xMax = xAxis->max();
    if( (xMin.isValid()==false) || (xMax.isValid()==false) ){
        return; // should not happen
    }
    qint64 xLeftDeltaInSec = mousePointDate.toSecsSinceEpoch()-xMin.toSecsSinceEpoch();
    qint64 xRightDeltaInSec = xMax.toSecsSinceEpoch() - mousePointDate.toSecsSinceEpoch();
    qint64 xNewLeftDeltaInSec = static_cast<qint64>(xLeftDeltaInSec * factor);
    qint64 xNewRightDeltaInSec = static_cast<qint64>(xRightDeltaInSec * factor);
    QDateTime xNewMin = xMin;
    QDateTime xNewMax = xMax;
    bool applyXzoom = true;
    // check for max or min zoom level authorized
    if ( (xNewLeftDeltaInSec+xNewRightDeltaInSec)< xAxisRangeMinInSec)  {
        // excessive zoom in
        applyXzoom = false;
    } else if ((xNewLeftDeltaInSec+xNewRightDeltaInSec) > xAxisRangeMaxInSec){
        // excessive zoom out
        applyXzoom = false;
    } else {
        xNewMin = mousePointDate.addSecs(-xNewLeftDeltaInSec);
        xNewMax = mousePointDate.addSecs(xNewRightDeltaInSec);
    }

    // *** Y AXIS calculation ***
    QList<QAbstractAxis *> ayList = theChart->axes(Qt::Vertical);
    if(ayList.count() != 1){
        qInfo() << "ayList.count() != 1";   // Should not happen
        return;
    }
    QValueAxis* yAxis = dynamic_cast<QValueAxis*>(ayList.at(0));
    if (yAxis==nullptr) {
        qInfo() << "yAxis is not QValueAxis"; // Should not happen
        return;  // this was not a QValueAxis, this is unexpected
    }
    double yMin = yAxis->min(); // get the minimum value of the Y-axis
    double yMax = yAxis->max(); // get the maximum value of the Y-axis
    double yBottomDeltaInSec = mousePointValue - yMin;
    double yTopDeltaInSec = yMax - mousePointValue;
    double yNewBottomDeltaInSec = yBottomDeltaInSec * factor;
    double yNewTopDeltaInSec = yTopDeltaInSec * factor;
    double yNewMin = yMin;
    double yNewMax = yMax;
    bool applyYzoom = true;
    // check for max or min zoom level authorized
    if ( (yNewBottomDeltaInSec+yNewTopDeltaInSec)< yAxisRangeMinInSec )  {
        // excessive zoom in...
        applyYzoom = false;
    } else if( (yNewBottomDeltaInSec+yNewTopDeltaInSec) > yAxisRangeMaxInSec ) {
        // excessive zoom out...
        applyYzoom = false;
    } else {
        yNewMin = mousePointValue - yNewBottomDeltaInSec;
        yNewMax = mousePointValue + yNewTopDeltaInSec;
    }

    // apply zoom
    if (event->modifiers() & Qt::ShiftModifier){
        // HORIZONTAL ZOOM around the mouse point
        if (applyXzoom==true){
            xAxis->setRange(xNewMin, xNewMax);
        }
    } else if (event->modifiers() & Qt::ControlModifier){
        // VERTICAL ZOOM around the mouse point
        if(applyYzoom==true){
            yAxis->setRange(yNewMin, yNewMax);
        }
    } else{
        // HORIZONTAL AND VERTICAL ZOOM around the mouse point
        if (applyXzoom==true){
            xAxis->setRange(xNewMin, xNewMax);
        }
        if(applyYzoom==true){
            yAxis->setRange(yNewMin, yNewMax);
        }
    }
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
        QPoint p = event->pos();
        QPointF newValue = mapToScene(p);
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

