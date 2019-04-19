/*===================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center,
Division of Medical and Biological Informatics.
All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or http://www.mitk.org for details.

===================================================================*/

#ifndef QmitkC3xyData_h
#define QmitkC3xyData_h

#include <QVariant>

/** /brief This class holds the actual data for the chart generation with C3.
* data can be loaded in constructor directly or with SetData
* It is derived from QObject, because we need Q_PROPERTIES to send Data via QWebChannel to JavaScript.
*/
class QmitkChartxyData : public QObject
{
  Q_OBJECT

  Q_PROPERTY(QList<QVariant> m_YData READ GetYData WRITE SetYData NOTIFY SignalYDataChanged);
  Q_PROPERTY(QList<QVariant> m_XData READ GetXData WRITE SetXData NOTIFY SignalXDataChanged);
  Q_PROPERTY(QList<QVariant> m_XErrorDataPlus READ GetXErrorDataPlus WRITE SetXErrorDataPlus NOTIFY SignalErrorDataChanged);
  Q_PROPERTY(QList<QVariant> m_XErrorDataMinus READ GetXErrorDataMinus WRITE SetXErrorDataMinus NOTIFY SignalErrorDataChanged);
  Q_PROPERTY(QList<QVariant> m_YErrorDataPlus READ GetYErrorDataPlus WRITE SetYErrorDataPlus NOTIFY SignalErrorDataChanged);
  Q_PROPERTY(QList<QVariant> m_YErrorDataMinus READ GetYErrorDataMinus WRITE SetYErrorDataMinus NOTIFY SignalErrorDataChanged);
  Q_PROPERTY(QVariant m_ChartType READ GetChartType WRITE SetChartType NOTIFY SignalDiagramTypeChanged);
  Q_PROPERTY(QVariant m_Color READ GetColor WRITE SetColor NOTIFY SignalColorChanged);
  Q_PROPERTY(QVariant m_Label READ GetLabel WRITE SetLabel NOTIFY SignalLabelChanged);
  Q_PROPERTY(QVariant m_LineStyleName READ GetLineStyle WRITE SetLineStyle NOTIFY SignalLineStyleChanged);

public:
  explicit QmitkChartxyData(const QMap<QVariant, QVariant>& data, const QVariant& label, const QVariant& diagramType); //Constructor for Data2D (x:y=1:2, 2:6, 3:7)

  void SetData(const QMap<QVariant, QVariant>& data);

  Q_INVOKABLE QList<QVariant> GetYData() const { return m_YData; };
  Q_INVOKABLE void SetYData(const QList<QVariant>& yData) { m_YData =yData; };

  Q_INVOKABLE QList<QVariant> GetXData() const { return m_XData; };
  Q_INVOKABLE void SetXData(const QList<QVariant>& xData) { m_XData =xData; };

  Q_INVOKABLE QList<QVariant> GetXErrorDataPlus() const { return m_XErrorDataPlus; };
  Q_INVOKABLE void SetXErrorDataPlus(const QList<QVariant> &errorData) { m_XErrorDataPlus = errorData; };

  Q_INVOKABLE QList<QVariant> GetXErrorDataMinus() const { return m_XErrorDataMinus; };
  Q_INVOKABLE void SetXErrorDataMinus(const QList<QVariant> &errorData) { m_XErrorDataMinus = errorData; };

  Q_INVOKABLE QList<QVariant> GetYErrorDataPlus() const { return m_YErrorDataPlus; };
  Q_INVOKABLE void SetYErrorDataPlus(const QList<QVariant> &errorData) { m_YErrorDataPlus = errorData; };

  Q_INVOKABLE QList<QVariant> GetYErrorDataMinus() const { return m_YErrorDataMinus; };
  Q_INVOKABLE void SetYErrorDataMinus(const QList<QVariant> &errorData) { m_YErrorDataMinus = errorData; };

  Q_INVOKABLE QVariant GetChartType() const { return m_ChartType; };
  Q_INVOKABLE void SetChartType(const QVariant& chartType) { m_ChartType = chartType; };

  Q_INVOKABLE QVariant GetLabel() const { return m_Label; };
  Q_INVOKABLE void SetLabel(const QVariant& label) { m_Label = label; };

  Q_INVOKABLE QVariant GetColor() const { return m_Color; };
  Q_INVOKABLE void SetColor(const QVariant& color) { m_Color = color; };

  Q_INVOKABLE QVariant GetLineStyle() const { return m_LineStyleName; };
  Q_INVOKABLE void SetLineStyle(const QVariant& lineStyle) { m_LineStyleName = lineStyle; };

  
  /**
  * \brief Clears the Data.
  *
  * This function clears the data.
  */
  void ClearData();

signals:
  void SignalYDataChanged(const QList<QVariant> yData);
  void SignalXDataChanged(const QList<QVariant> xData);
  void SignalErrorDataChanged(const QList<QVariant> errorData);
  void SignalDiagramTypeChanged(const QVariant diagramType);
  void SignalColorChanged(const QVariant color);
  void SignalLabelChanged(const QVariant label);
  void SignalLineStyleChanged(const QVariant lineStyle);

private:
  QList<QVariant>  m_YData;
  QList<QVariant>  m_XData;
  QList<QVariant>  m_XErrorDataPlus;
  QList<QVariant>  m_XErrorDataMinus;
  QList<QVariant> m_YErrorDataPlus;
  QList<QVariant> m_YErrorDataMinus;
  QVariant         m_Label;
  QVariant         m_ChartType;
  QVariant         m_Color;
  QVariant         m_LineStyleName;
};

#endif  //QmitkC3xyData_h
