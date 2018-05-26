#include "stdafx.h"
#include "SDL.h"
#include "ResourceManager.h"
#include "WindowManager.h"
#include "Window.h"
#include <algorithm>
#include <iostream>
#include "Basetsd.h"

namespace CoreUI
{
	WindowManager & WindowManager::Get()
	{
		static WindowManager manager;
		return manager;
	}

	void WindowManager::Init(RendererPtr & renderer)
	{
		m_renderer = renderer.get();
		GetEventType("timer");
	}

	void WindowManager::Draw()
	{
		for (auto & window : m_windows)
		{
			window->Draw();
		}

		// Active menu needs to be drawn on top of everything
		if (m_activeWindow)
		{
			m_activeWindow->DrawMenu();
		}
	}

	WindowPtr WindowManager::AddWindow(const char * id, Rect pos, CreationFlags flags)
	{
		return AddWindow(id, nullptr, pos, flags);
	}

	WindowPtr WindowManager::AddWindow(const char* id, WindowPtr parent, Rect pos, CreationFlags flags)
	{
		if (FindWindow(id) != nullptr)
		{
			throw std::invalid_argument("windows already exists:" + (std::string)id);
		}

		WindowPtr newWindow = Window::Create(id, m_renderer, parent.get(), RES().FindFont("default"), pos, flags);
		m_windows.push_back(newWindow);

		return newWindow;
	}

	WindowPtr WindowManager::FindWindow(const char* id)
	{
		if (id == nullptr)
		{
			throw std::invalid_argument("id can't be null");
		}

		for (auto & it : m_windows)
		{
			if (id == it->GetId())
			{
				return it;
			}
		}

		return nullptr;
	}

	WindowManager::WindowList WindowManager::GetWindowList(WindowRef parent)
	{
		WindowList childWindows;
		std::copy_if(m_windows.begin(), m_windows.end(), std::back_inserter(childWindows),
			[parent](WindowPtr window) { return window->GetParent() == parent; });
		return childWindows;
	}

	HitResult WindowManager::HitTest(PointRef pt)
	{
		for (auto it = m_windows.rbegin(); it != m_windows.rend(); ++it)
		{
			auto & window = *it;
			auto hitResult = window->HitTest(pt);

			if (hitResult)
			{
				return hitResult;
			}
		}

		return HIT_NOTHING;
	}

	CoreUI::WindowRef WindowManager::GetActive()
	{
		return m_activeWindow ? m_activeWindow : Window::GetNullWnd();
	}

	void WindowManager::SetActive(WindowRef win)
	{
		if (win == m_activeWindow)
		{
			return;
		}

		if (m_activeWindow)
		{
			m_activeWindow->PostEvent(Window::EVENT_WINDOW_DEACTIVATED);
		}
		
		m_activeWindow = win;

		if (m_activeWindow)
		{
			m_activeWindow->PostEvent(Window::EVENT_WINDOW_ACTIVATED);
		}

		MoveToFront(win);
	}
	const CaptureInfo & WindowManager::StartCapture(HitResult hit, PointRef pt)
	{
		if (hit && hit.target)
		{
			m_capture.Captured = true;
			m_capture.Target = hit;
			m_capture.Origin = hit.target->GetRect(true, false);
			m_capture.Delta = m_capture.Origin.Origin();
			m_capture.Delta.x -= pt->x;
			m_capture.Delta.y -= pt->y;
		}
		return m_capture;
	}

	Uint32 timerCallbackFunc(Uint32 interval, void *param)
	{
		const static Uint32 timerEventID = WINMGR().GetEventType("timer");
		if (timerEventID == -1)
			return -1;

		SDL_Event timerEvent;
		SDL_zero(timerEvent);

		timerEvent.type = timerEventID;
		timerEvent.user.code = PtrToUlong(param);

		SDL_PushEvent(&timerEvent);
		return(interval);
	}

	Uint32 WindowManager::AddTimer(Uint32 interval)
	{
		Uint32 extTimerID = (Uint32)m_timers.size();

		m_timers.push_back(SDL_AddTimer(interval, timerCallbackFunc, LongToPtr(extTimerID)));
		return extTimerID;
	}

	void WindowManager::DeleteTimer(Uint32 timerID)
	{
		if (timerID > m_timers.size() - 1)
		{
			throw std::invalid_argument("invalid timer id");
		}

		SDL_RemoveTimer(m_timers[timerID]);
		m_timers[timerID] = 0;
	}

	void WindowManager::RaiseSingleWindow(WindowRef win)
	{
		const std::string & id = win->GetId();

		auto found = std::find_if(m_windows.begin(), m_windows.end(), [id](WindowPtr window) { return window->GetId() == id; });
		if (found != m_windows.end())
		{
			m_windows.splice(m_windows.end(), m_windows, found);
		}
	}

	void WindowManager::RaiseChildren(WindowRef win)
	{
		RaiseSingleWindow(win);
		for (auto & child : win->GetChildWindows())
		{
			RaiseChildren(child.get());
		}
	}

	void WindowManager::MoveToFront(WindowRef win)
	{
		if (win == nullptr)
			return;

		if (win->HasParent())
		{
			RaiseChildren(win->GetParentWnd());
		}
		RaiseChildren(win);
	}

	TexturePtr WindowManager::SurfaceToTexture(SDL_Surface* surf)
	{
		TexturePtr texture = TexturePtr(SDL_CreateTextureFromSurface(m_renderer, surf), sdl_deleter());

		SDL_FreeSurface(surf);

		return std::move(texture);
	}

	std::string WindowManager::GetEventName(Uint32 eventId) const
	{
		auto found = m_registeredEventsReverse.find(eventId);
		if (found == m_registeredEventsReverse.end())
		{
			return "<null>";
		}
		return found->second;
	}

	Uint32 WindowManager::GetEventType(const char * type)
	{
		if (type == nullptr)
		{
			throw std::invalid_argument("event type is null");
		}

		Uint32 eventId = FindEventType(type);
		if (eventId == (Uint32)-1)
		{			
			eventId = SDL_RegisterEvents(1);
			if (eventId == (Uint32)-1)
			{
				throw std::out_of_range(" too many registered events");
			}
			m_registeredEvents[type] = eventId;
			m_registeredEventsReverse[eventId] = type;

			std::cout << "Register: " << type << ", eventID = " << eventId << std::endl;
		}		
		
		return eventId;
	}

	Uint32 WindowManager::FindEventType(const char * type) const
	{
		auto it = m_registeredEvents.find(type);
		if (it != m_registeredEvents.end())
		{
			return it->second;
		}
		return (Uint32)-1;
	}
}