//
//    EstimatorControl.h: Parameter estimation control
//    Copyright (C) 2019 Gonzalo José Carracedo Carballal
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU Lesser General Public License as
//    published by the Free Software Foundation, either version 3 of the
//    License, or (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful, but
//    WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU Lesser General Public License for more details.
//
//    You should have received a copy of the GNU Lesser General Public
//    License along with this program.  If not, see
//    <http://www.gnu.org/licenses/>
//

#ifndef ESTIMATORCONTROL_H
#define ESTIMATORCONTROL_H

#include <QWidget>
#include <math.h>
#include <Suscan/Estimator.h>

namespace Ui {
  class EstimatorControl;
}

namespace SigDigger {
  class EstimatorControl : public QWidget
  {
      Q_OBJECT

      Suscan::Estimator estimator;
      float paramValue;
      bool available = false;

      void updateUI(void);

    public:
      explicit EstimatorControl(QWidget *parent, Suscan::Estimator const &);
      ~EstimatorControl();


      void setParameterValue(float param);
      void setParameterAvailable(bool result);

      float getParameterValue(void) const;
      bool isParamAvailable(void) const;

    public slots:
      void onEstimate(void);
      void onApply(void);

    signals:
      void estimatorChanged(Suscan::EstimatorId, bool);
      void apply(QString, float);

    private:
      Ui::EstimatorControl *ui;
  };
};

#endif // ESTIMATORCONTROL_H
