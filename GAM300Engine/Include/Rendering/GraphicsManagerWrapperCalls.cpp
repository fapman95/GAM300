
#include "GraphicsManagerWrapperCalls.h"
#include "GraphicsManager.h"
void TDS::GraphicsManagerWrapperCalls::ToggleViewFrom2D(bool condition)
{
    TDS::GraphicsManager::m_Instance->ToggleViewFrom2D(condition);
}

bool TDS::GraphicsManagerWrapperCalls::IsViewingFrom2D()
{
    return TDS::GraphicsManager::m_Instance->IsViewingFrom2D();
}

void TDS::GraphicsManagerWrapperCalls::SetLayerToRender(int ID)
{
    TDS::GraphicsManager::m_Instance->SetLayerToRender(ID);
}

int TDS::GraphicsManagerWrapperCalls::CurrentRenderedLayer()
{
    return TDS::GraphicsManager::getInstance().LayerToRender();
}

unsigned int TDS::GraphicsManagerWrapperCalls::GetPickedObject()
{
    return TDS::GraphicsManager::getInstance().PickedObject();
}

void TDS::GraphicsManagerWrapperCalls::SetClearColor(float x, float y, float z, float w)
{
    TDS::GraphicsManager::getInstance().SetClearColor(Vec4(x,y,z,w));
}

int TDS::GraphicsManagerWrapperCalls::GetWinPosX() {
    return TDS::GraphicsManager::getInstance().GetWindow()->GetWindowPosX();
}

int TDS::GraphicsManagerWrapperCalls::GetWinPosY() {
    return TDS::GraphicsManager::getInstance().GetWindow()->GetWindowPosY();
}
