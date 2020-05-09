
#include <stdio.h>
#include <string>

#include <Application.h>
#include <Test.h>            // for RunAll()
#include <iostream>

using namespace Nodable;

int main(int argc, char* argv[])
{	
	// Run some tests before to do anything:

	if ( !RunAllTests() ) {
		std::cin.get();
		return 1;
	}

	// Here the tests are all OK, we can instantiate, init and run nodable loop.

	Application nodable("Nodable " NODABLE_VERSION);

	nodable.init();

	while (nodable.update())
	{
		if(auto view = nodable.getComponent<View>())
			view->draw();
	}

	nodable.shutdown();

	return 0;
}

