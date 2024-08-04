//
//    UIComponentFactory.cpp: Factory for UI components
//    Copyright (C) 2022 Gonzalo José Carracedo Carballal
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
#include <UIComponentFactory.h>
#include <UIMediator.h>
#include <cassert>

using namespace SigDigger;

UIMediator *
UIComponent::mediator() const
{
  return m_mediator;
}

void
UIComponent::setState(int, Suscan::Analyzer *)
{
  // NO-OP
}

void
UIComponent::setProfile(Suscan::Source::Config &)
{
  // NO-OP
}

void
UIComponent::setColorConfig(ColorConfig const &)
{
  // NO-OP
}

void
UIComponent::setQth(Suscan::Location const &)
{
  // NO-OP
}

void
UIComponent::setTimeStamp(struct timeval const &)
{
  // NO-OP
}

UIComponent::UIComponent(UIComponentFactory *factory, UIMediator *mediator)
  : FeatureObject(factory), PersistentObject(), m_mediator(mediator)
{
  assert(mediator != nullptr);

  m_mediator->registerUIComponent(this);
}

UIComponent::~UIComponent()
{
  if (m_mediator != nullptr)
    m_mediator->unregisterUIComponent(this);
}

UIComponentFactory::UIComponentFactory(
    Suscan::Plugin *plugin) : Suscan::FeatureFactory(plugin)
{

}
