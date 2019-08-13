//
//    AutoGain.cpp: AutoGain object
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

#include <AutoGain.h>

using namespace SigDigger;

GainTable::GainTable()
{

}

GainTable::GainTable(Suscan::Object const &obj)
{
  this->deserialize(obj);
}

void
GainTable::deserialize(Suscan::Object const &obj)
{
  std::string values;
  size_t pos = 0;
  std::string token;
  int value;

  this->name = obj.getField("gain").value();
  this->gains.clear();
  values = obj.getField("values").value();

  while ((pos = values.find(',')) != std::string::npos) {
    token = values.substr(0, pos);
    values.erase(0, pos + 1);

    if (sscanf(token.c_str(), "%d", &value) < 1)
      value = 0;

    this->gains.push_back(value);
  }

  if (values.length() != 0) {
    if (sscanf(token.c_str(), "%d", &value) < 1)
      value = 0;
    this->gains.push_back(value);
  }
}

AutoGain::AutoGain()
{

}

AutoGain::AutoGain(Suscan::Object const &obj)
{
  this->deserialize(obj);
}


void
AutoGain::deserialize(Suscan::Object const &obj)
{
  Suscan::Object gains;
  unsigned int i, count;
  unsigned int min = 0;

  this->name = obj.getField("name").value();
  this->driver = obj.getField("driver").value();

  gains = obj.getField("gains");

  SU_ATTEMPT(gains.getType() == SUSCAN_OBJECT_TYPE_SET);

  count = gains.length();

  for (i = 0; i < count; ++i) {
    this->gains.push_back(GainTable(gains[i]));
    if (i == 0 || this->gains[i].gains.size() < min)
      min = static_cast<unsigned>(this->gains[i].gains.size());
  }

  // I know
  this->max = static_cast<int>(min) - 1;
}

GainConfig::GainConfig(std::string name, int value)
{
  this->name = name;
  this->value = value;
}

std::vector<GainConfig>
AutoGain::translateGain(int val) const
{
  std::vector<GainConfig> result;
  unsigned int uval = static_cast<unsigned>(val);

  if (val >= this->getMin() && val <= this->getMax())
    for (unsigned i = 0; i < this->gains.size(); ++i)
      result.push_back(
            GainConfig(this->gains[i].name, this->gains[i].gains[uval]));

  return result;
}

