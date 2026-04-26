#pragma once
#include <vector>
#include <memory>

class UIElementBase;

class UIPanelImpl
{
public:
	std::vector<std::shared_ptr<UIElementBase>> m_children;
};