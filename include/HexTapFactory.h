#ifndef HEXTAPFACTORY_H
#define HEXTAPFACTORY_H

#include <Suscan/DecoderFactory.h>

namespace SigDigger {
  class HexTapFactory : public Suscan::DecoderFactory
  {
    public:
      std::string getName(void) const override;
      std::string getDescription(void) const override;
      Suscan::DecoderObjects *make(QObject *parent = nullptr) override;
  };
}

#endif // HEXTAPFACTORY_H
