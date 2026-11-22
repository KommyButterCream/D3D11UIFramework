#include "pch.h"
#include "UIEventDispatcher.h"

void UIEventDispatcher::RegisterCallback(UIEventCallback callback, void* userData)
{
	m_callback = callback;
	m_userData = userData;
}

void UIEventDispatcher::Dispatch(UICommand command)
{
	if (m_callback)
		m_callback(command, m_userData);
}
