#ifndef IGQTPARALLELCOORDINATESOBJECTFILTER_H
#define IGQTPARALLELCOORDINATESOBJECTFILTER_H

#include <QWidget>
#include <string>

namespace Ui {
class igQtParallelCoordinatesObjectFilter;
}

class igQtParallelCoordinatesObjectFilter : public QWidget
{
    Q_OBJECT

public:
    explicit igQtParallelCoordinatesObjectFilter(int number, double maxValue, double minValue,
                                                 const std::string& variableName, QWidget* parent = nullptr);
    ~igQtParallelCoordinatesObjectFilter();
    int GetNumber() const;

signals:
    void ChangeMaxValue(int number, double value);
    void ChangeMinValue(int number, double value);

public slots:
    void MaxSpinBoxChanged(double value);
    void MinSpinBoxChanged(double value);

private:
    Ui::igQtParallelCoordinatesObjectFilter *ui;
    int m_Number{};
};

#endif // IGQTPARALLELCOORDINATESOBJECTFILTER_H
