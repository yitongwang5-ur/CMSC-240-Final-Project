#pragma once

#include <QColor>

/**
 * @file Palette.h
 * @brief Centralised color palette for the UI and embedded charts.
 *
 * The first six colors mirror matplotlib's "tab:" palette so the QtCharts
 * views in ChartsTab look identical to the PNGs produced by vizualize.py.
 * The remaining colors define the chrome (UR brand red header, card
 * surfaces, borders, muted text).
 *
 * Anything that paints with color reads from here so a single edit to a
 * value updates both the chart and any corresponding chrome element.
 */
namespace ui::palette {

// matplotlib "tab:" palette — keep in sync with vizualize.py
inline const QColor blue   = QColor("#1f77b4"); ///< balance line
inline const QColor orange = QColor("#ff7f0e"); ///< even-pace / underspending warn
inline const QColor green  = QColor("#2ca02c"); ///< daily-spent bars / on-track
inline const QColor red    = QColor("#d62728"); ///< overspending alert
inline const QColor purple = QColor("#9467bd"); ///< per-shop visit count
inline const QColor cyan   = QColor("#17becf"); ///< per-shop dollar total

// Chrome
inline const QColor urRed  = QColor("#990000"); ///< University of Richmond brand red — title bar accent
inline const QColor cardBg = QColor("#ffffff"); ///< card surface
inline const QColor pageBg = QColor("#f7f7f7"); ///< window background
inline const QColor border = QColor("#dcdcdc"); ///< card / table borders
inline const QColor muted  = QColor("#6b6b6b"); ///< secondary text and axis labels

}
