#include "engine.h"
#include "../events/evt_data_environment_loaded.h"
#include "../events/evt_data_new_actor.h"
#include "../events/evt_data_move_actor.h"
#include "../events/evt_data_destroy_actor.h"
#include "../events/evt_data_request_new_actor.h"
#include "d3d_renderer11.h"
#include "x_logic.h"
#include "main_menu_ui.h"
#include "main_menu_view.h"
#include "i_engine_view.h"

Engine* g_pApp = nullptr;
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

Engine::Engine() {
	g_pApp = this;
	m_game = nullptr;

	m_is_running = false;
	m_is_editor_running = false;
	m_is_quit_requested = false;
	m_is_quitting = false;
	m_has_modal_dialog = false;

	m_event_manager = nullptr;
}

Engine::~Engine() {}

bool Engine::Initialize(const RenderWindowConfig& cfg) {
	m_timer.Start();
	m_options = cfg.options;
	if (!m_render_window.Initialize(*this, cfg)) {
		return false;
	}

	m_renderer = std::make_unique<D3DRenderer11>();
	if (!m_renderer->Initialize(m_render_window)) {
		return false;
	}
	m_renderer->VSetBackgroundColor(20, 20, 200, 255);
	m_renderer->VOnRestore();

	m_event_manager = std::make_unique<EventManager>("GameCodeApp Event Mgr", true);
	if (!m_event_manager) {
		return false;
	}

	m_game = VCreateGameAndView();
	if (!m_game) {
		return false;
	}

	return true;
}

void Engine::Run() {
	while (ProcessMessages()) {
		if (!Update()) { break; };
		RenderFrame();
	}
}

void Engine::AbortGame() {
	m_is_quitting = true;
}

bool Engine::IsRunning() {
	return m_is_running;
}

LRESULT Engine::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	if (ImGui_ImplWin32_WndProcHandler(hwnd, uMsg, wParam, lParam)) { return true; }

	LRESULT result = 0;

	switch (uMsg) {
		case WM_DISPLAYCHANGE: {
			int colorDepth = (int)wParam;
			int width = (int)(short)LOWORD(lParam);
			int height = (int)(short)HIWORD(lParam);

			result = g_pApp->OnDisplayChange(colorDepth, width, height);
			break;
		}

		case WM_SYSCOMMAND: {
			result = g_pApp->OnSysCommand(wParam, lParam);
		}
		break;

		case WM_SYSKEYDOWN: {
			if (wParam == VK_RETURN) {
				return g_pApp->OnAltEnter();
			}
			return DefWindowProc(hwnd, uMsg, wParam, lParam);
		}
		break;

		case WM_CLOSE: {
			if (g_pApp->m_is_quitting) {
				result = g_pApp->OnClose();
			}
		}
		break;

		case WM_KEYDOWN:
		case WM_KEYUP:
		case WM_CHAR:
		case WM_MOUSEMOVE:
		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP:
		case MM_JOY1BUTTONDOWN:
		case MM_JOY1BUTTONUP:
		case MM_JOY1MOVE:
		case MM_JOY1ZMOVE:
		case MM_JOY2BUTTONDOWN:
		case MM_JOY2BUTTONUP:
		case MM_JOY2MOVE:
		case MM_JOY2ZMOVE: {
			if (g_pApp->m_game) {
				BaseEngineLogic* pGame = g_pApp->m_game.get();
				for (GameViewList::reverse_iterator i = pGame->m_game_views.rbegin(); i != pGame->m_game_views.rend(); ++i) {
					if ((*i)->VOnMsgProc(hwnd, uMsg, wParam, lParam)) {
						result = true;
						break;
					}
				}
			}
			break;
		}
		default:
			return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}
}

LRESULT Engine::OnDisplayChange(int colorDepth, int width, int height) {
	return 0;
}

LRESULT Engine::OnSysCommand(WPARAM wParam, LPARAM lParam) {
	switch (wParam) {
		case SC_MAXIMIZE: {
			if (!m_options.m_fullScreen && IsRunning()) {
				OnAltEnter();
			}
		}
		return 0;

		case SC_CLOSE: {
			if (lParam != MAKELPARAM(-1, -1)) {
				if (m_is_quit_requested) {
					return true;
				}
				m_is_quit_requested = true;
			}
			m_is_quitting = true;
			m_is_quit_requested = false;
		}
		return 0;

		default:
			return DefWindowProc(m_render_window.GetHWND(), WM_SYSCOMMAND, wParam, lParam);
	}

	return 0;
}

LRESULT Engine::OnClose() {
	DestroyWindow(m_render_window.GetHWND());
	return 0;
}

LRESULT Engine::OnAltEnter() {
	return 0;
}

LRESULT Engine::OnNcCreate(LPCREATESTRUCT cs) {
	return true;
}

const EngineOptions& Engine::GetConfig() {
	return m_options;
}

const RenderWindow& Engine::GetRenderWindow() {
	return m_render_window;
}

Renderer Engine::GetRendererImpl() {
	return Renderer::Renderer_D3D11;
}

std::unique_ptr<BaseEngineLogic> Engine::VCreateGameAndView() {
	std::unique_ptr pGame = std::make_unique<XLogic>();
	pGame->Init();

	std::shared_ptr<IEngineView> menuView(new MainMenuView(m_renderer.get()));
	pGame->VAddView(menuView);

	return pGame;
}

bool Engine::ProcessMessages() {
	return m_render_window.ProcessMessages();
}

bool Engine::Update() {
	m_timer.Tick();

	if (m_is_quitting) {
		PostMessage(m_render_window.GetHWND(), WM_CLOSE, 0, 0);
	}

	if (m_game) {
		IEventManager::Get()->VUpdate();
		m_game->VOnUpdate(m_timer.TotalTime(), m_timer.DeltaTime());
	}

	return true;
}

void Engine::RenderFrame() {
	BaseEngineLogic* pGame = g_pApp->m_game.get();
	for (GameViewList::iterator i = pGame->m_game_views.begin(), end = pGame->m_game_views.end(); i != end; ++i) {
		(*i)->VOnRender(m_timer.TotalTime(), m_timer.DeltaTime());
	}
	m_renderer->VPostRender();
}

void Engine::VRegisterEvents() {
	REGISTER_EVENT(EvtData_Environment_Loaded);
	REGISTER_EVENT(EvtData_New_Actor);
	REGISTER_EVENT(EvtData_Move_Actor);
	REGISTER_EVENT(EvtData_Destroy_Actor);
	REGISTER_EVENT(EvtData_Request_New_Actor);
}

int Engine::Modal(std::shared_ptr<IScreenElement> pModalScreen, int defaultAnswer) {
	auto pView = GetHumanView();
	if (!pView) { return defaultAnswer;	}
	pView->VPushElement(pModalScreen);
	m_has_modal_dialog += 1;
	return 0;
}

BaseEngineLogic* Engine::GetGameLogic() {
	return m_game.get();
}

IRenderer* Engine::GetRenderer() {
	return m_renderer.get();
}

bool Engine::VLoadGame() {
	return m_game->VLoadGame("MainMenu.xml");
}

std::shared_ptr<HumanView> Engine::GetHumanView() {
	std::shared_ptr<HumanView> pView;
	if(!m_game) { return pView; }
	for (GameViewList::iterator i = m_game->m_game_views.begin(); i != m_game->m_game_views.end(); ++i) {
		if ((*i)->VGetType() == EngineViewType::GameView_Human) {
			pView = std::dynamic_pointer_cast<HumanView>(*i);
			break;
		}
	}
	return pView;
}

std::shared_ptr<HumanView> Engine::GetHumanViewByName(std::string name) {
	std::shared_ptr<HumanView> pView;
	if (!m_game) { return pView; }
	for (GameViewList::iterator i = m_game->m_game_views.begin(); i != m_game->m_game_views.end(); ++i) {
		if ((*i)->VGetType() == EngineViewType::GameView_Human && (*i)->VGetName() == name) {
			pView = std::dynamic_pointer_cast<HumanView>(*i);
			break;
		}
	}
	return pView;
}

const GameTimer& Engine::GetTimer() {
	return m_timer;
}
