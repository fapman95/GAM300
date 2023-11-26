#pragma once
#include "dotnet/ImportExport.h"
namespace TDS
{

	class DLL_API GraphicsManagerWrapperCalls
	{
	public:
		static void				ToggleViewFrom2D(bool condition);

		static bool				IsViewingFrom2D();

		static void				SetLayerToRender(int ID);

		static int				CurrentRenderedLayer();
		static unsigned int		GetPickedObject();

		static void				SetClearColor(float x, float y, float z, float w);
	};
}