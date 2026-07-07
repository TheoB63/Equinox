#pragma once

#include "equinox/core/EquinoxTypes.h"

namespace Equinox
{
	class Event
	{
	public:
		virtual ~Event() = default;
		bool m_Handled = false;
	};
}