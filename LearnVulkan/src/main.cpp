#include <iostream>
#include "Application.h"
#include "deferred/DeferredApp.h"

int main()
{
	VulkanDeferredApp app("Vulkan App", 800, 800);
	app.Run();

	return 0;
}