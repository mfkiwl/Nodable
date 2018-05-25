#include "Nodable.h"
#include "View.h"   // base class
#include <imgui.h>

namespace Nodable{
	class WireView : public View
	{
	public:
		COMPONENT_CONSTRUCTOR(WireView);
		void draw()override;
	};
}