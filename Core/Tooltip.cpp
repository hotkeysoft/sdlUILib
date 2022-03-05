#include "stdafx.h"
#include "Tooltip.h"
#include "Widgets/Label.h"

namespace CoreUI
{
	static const char* id = "_tooltip";

	Tooltip& Tooltip::Get()
	{
		static Tooltip tooltip;
		return tooltip;
	}

	void Tooltip::Show(WidgetRef owner, Point pos, const char* text)
	{
		if (m_wnd && (m_owner == owner) && (m_wnd->GetText() == text))
			return;

		Hide(m_owner);
		m_owner = owner;
		
		RendererRef renderer = WINMGR().GetRenderer();
		WidgetPtr label = Label::CreateAutoSize("_tooltipLabel", renderer, text);		
		label->SetPadding(Dimension(2,1));
		label->SetBorder(true);
		label->Init();
		Rect rect = label->GetRect();
		
		Rect size = WINMGR().GetWindowSize();
		if (pos.x + rect.w > size.w)
		{
			pos.x = size.w - rect.w;
		}

		pos.y -= (rect.h + 4);

		m_wnd = WINMGR().AddWindow(id, Rect(pos.x, pos.y, rect.w, rect.h), WIN_NOSCROLL | WIN_NOFOCUS | WIN_BORDERLESS);
		m_wnd->SetBackgroundColor(Color(255, 255, 128, 64));
		m_wnd->AddControl(label);
	}

	void Tooltip::Hide(WidgetRef owner)
	{
		if (m_wnd && (owner == m_owner))
		{
			WINMGR().RemoveWindow(id);
			m_wnd.reset();
		}
	}
}
