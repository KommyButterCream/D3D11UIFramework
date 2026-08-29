// D3D11UIFramework 회귀 하네스
//
// 렌더 컨텍스트 없이 도는 것만 검증한다. 히트 테스트, hover 전이,
// 상태 머신, 커맨드 디스패치가 대상이고 그리기는 다루지 않는다.
//
// Initialize() 를 부르지 않으므로 m_context 는 null 로 남는다.
// AddChild 는 컨텍스트가 있을 때만 자식을 Initialize 하므로 그대로 통과하고,
// HitTest / OnMouseEvent / SetState 경로는 D2D 를 쓰지 않는다.
//
// 빌드: Test/build.bat  (D3D11UIFramework.lib 에 링크)

#include <windows.h>
#include <stdio.h>
#include <memory>
#include <vector>
#include <string>

#include "../D3D11UIFramework/Panel/UIPanel.h"
#include "../D3D11UIFramework/Button/UIButton.h"
#include "../D3D11UIFramework/Event/UIEventDispatcher.h"

namespace
{
	int g_fail = 0;
	int g_total = 0;

	void Expect(bool ok, const char* what)
	{
		++g_total;
		printf("%-6s %s\n", ok ? "PASS" : "FAIL", what);
		if (!ok) ++g_fail;
	}

	const char* StateName(UIElementState state)
	{
		switch (state)
		{
		case UIElementState::Normal:   return "Normal";
		case UIElementState::Hovered:  return "Hovered";
		case UIElementState::Pressed:  return "Pressed";
		case UIElementState::Disabled: return "Disabled";
		default:                       return "?";
		}
	}

	// 디스패치된 커맨드를 기록한다.
	struct CommandLog
	{
		std::vector<UICommand> commands;

		static void Callback(UICommand command, void* userData)
		{
			static_cast<CommandLog*>(userData)->commands.push_back(command);
		}

		void Clear() { commands.clear(); }
		size_t Count() const { return commands.size(); }
		UICommand Last() const { return commands.empty() ? UICommand::None : commands.back(); }
	};

	// 테스트용 패널 한 벌.
	//
	//   패널 (0,0)-(200,100)
	//     ├ 버튼 A (10,10)-(90,90)    ZoomIn
	//     └ 버튼 B (110,10)-(190,90)  ZoomOut
	struct Fixture
	{
		UIPanel panel;
		std::shared_ptr<UIButton> a = std::make_shared<UIButton>();
		std::shared_ptr<UIButton> b = std::make_shared<UIButton>();
		UIEventDispatcher dispatcher;
		CommandLog log;

		Fixture()
		{
			dispatcher.RegisterCallback(&CommandLog::Callback, &log);

			panel.SetLayout({ 0.0f, 0.0f, 200.0f, 100.0f });
			panel.SetLayoutType(UILayoutType::None);   // 자동 배치를 끄고 좌표를 고정한다

			a->SetLayout({ 10.0f, 10.0f, 90.0f, 90.0f });
			a->SetCommand(UICommand::ZoomIn);
			a->SetEventDispatcher(&dispatcher);

			b->SetLayout({ 110.0f, 10.0f, 190.0f, 90.0f });
			b->SetCommand(UICommand::ZoomOut);
			b->SetEventDispatcher(&dispatcher);

			panel.AddChild(a);
			panel.AddChild(b);
		}
	};
}

// ─────────────────────────────────────────────────────────────
// 히트 테스트
// ─────────────────────────────────────────────────────────────
static void TestHitTest()
{
	printf("\n[히트 테스트]\n");

	Fixture f;

	Expect(f.panel.HitTest(100.0f, 50.0f), "패널 안쪽");
	Expect(!f.panel.HitTest(300.0f, 50.0f), "패널 바깥");
	Expect(f.a->HitTest(50.0f, 50.0f), "버튼 A 안쪽");
	Expect(!f.a->HitTest(100.0f, 50.0f), "버튼 A 바깥(두 버튼 사이)");
	Expect(f.b->HitTest(150.0f, 50.0f), "버튼 B 안쪽");

	// 숨기면 히트가 안 잡혀야 한다.
	f.a->SetVisible(false);
	Expect(!f.a->HitTest(50.0f, 50.0f), "숨긴 요소는 히트 실패");
	f.a->SetVisible(true);
	Expect(f.a->HitTest(50.0f, 50.0f), "다시 보이면 히트 성공");
}

// ─────────────────────────────────────────────────────────────
// hover 전이
//
// 이 프레임워크에서 가장 깨지기 쉬운 부분이다. 패널이 이전 hover 자식에게
// Leave 를 보내지 않으면 하이라이트가 고착된다.
// ─────────────────────────────────────────────────────────────
static void TestHoverTransition()
{
	printf("\n[hover 전이]\n");

	Fixture f;

	f.panel.OnMouseEvent(UIMouseEventType::Move, 50.0f, 50.0f);
	Expect(f.a->GetState() == UIElementState::Hovered, "A 위로 이동 -> A Hovered");
	Expect(f.b->GetState() == UIElementState::Normal, "B 는 Normal 유지");

	// ★ A -> B 로 넘어갈 때 A 가 Normal 로 풀려야 한다.
	f.panel.OnMouseEvent(UIMouseEventType::Move, 150.0f, 50.0f);
	Expect(f.a->GetState() == UIElementState::Normal,
		"A -> B 이동 시 A 가 Normal 로 해제 (hover 고착 방지)");
	Expect(f.b->GetState() == UIElementState::Hovered, "B 가 Hovered");

	// 두 버튼 사이 빈 공간. 패널 안이지만 어느 자식도 아니다.
	f.panel.OnMouseEvent(UIMouseEventType::Move, 100.0f, 50.0f);
	Expect(f.b->GetState() == UIElementState::Normal, "빈 공간으로 이동 시 B 해제");

	// 패널 바깥.
	f.panel.OnMouseEvent(UIMouseEventType::Move, 50.0f, 50.0f);
	Expect(f.a->GetState() == UIElementState::Hovered, "A 재진입");

	f.panel.OnMouseEvent(UIMouseEventType::Move, 500.0f, 500.0f);
	Expect(f.a->GetState() == UIElementState::Normal, "패널 바깥으로 나가면 A 해제");
}

// ─────────────────────────────────────────────────────────────
// 소비(consume) 여부
//
// 반환값이 false 여야 호스트가 이미지 팬/ROI 로 이벤트를 넘긴다.
// 여기가 틀리면 툴바 위 클릭이 이미지까지 새거나, 반대로 이미지 클릭이 먹힌다.
// ─────────────────────────────────────────────────────────────
static void TestConsumption()
{
	printf("\n[이벤트 소비]\n");

	Fixture f;

	Expect(f.panel.OnMouseEvent(UIMouseEventType::Move, 50.0f, 50.0f),
		"자식 위 Move 는 소비");
	Expect(f.panel.OnMouseEvent(UIMouseEventType::LButtonDown, 50.0f, 50.0f),
		"자식 위 LButtonDown 은 소비");
	Expect(!f.panel.OnMouseEvent(UIMouseEventType::Move, 500.0f, 500.0f),
		"패널 바깥은 소비하지 않음");

	// 패널 안이지만 자식이 없는 자리 — 패널이 먹어야 한다(뒤로 새면 안 됨).
	Expect(f.panel.OnMouseEvent(UIMouseEventType::Move, 100.0f, 50.0f),
		"패널 안 빈 공간도 패널이 소비");

	// 툴바 위 더블클릭이 이미지 줌으로 새지 않아야 한다.
	Expect(f.panel.OnMouseEvent(UIMouseEventType::LButtonDoubleDown, 50.0f, 50.0f),
		"툴바 위 더블클릭 흡수");

	// 숨긴 패널은 아무것도 먹지 않는다.
	f.panel.SetVisible(false);
	Expect(!f.panel.OnMouseEvent(UIMouseEventType::Move, 50.0f, 50.0f),
		"숨긴 패널은 소비하지 않음");
	f.panel.SetVisible(true);
}

// ─────────────────────────────────────────────────────────────
// 버튼 상태 머신과 커맨드
// ─────────────────────────────────────────────────────────────
static void TestButtonStateAndCommand()
{
	printf("\n[버튼 상태 / 커맨드]\n");

	Fixture f;

	Expect(f.a->GetState() == UIElementState::Normal, "초기 Normal");

	f.panel.OnMouseEvent(UIMouseEventType::Move, 50.0f, 50.0f);
	Expect(f.a->GetState() == UIElementState::Hovered, "Move -> Hovered");

	f.panel.OnMouseEvent(UIMouseEventType::LButtonDown, 50.0f, 50.0f);
	Expect(f.a->GetState() == UIElementState::Pressed, "LButtonDown -> Pressed");
	Expect(f.log.Count() == 0, "누르는 동안에는 커맨드가 나가지 않음");

	f.panel.OnMouseEvent(UIMouseEventType::LButtonUp, 50.0f, 50.0f);
	Expect(f.log.Count() == 1 && f.log.Last() == UICommand::ZoomIn,
		"버튼 위에서 뗄 때 커맨드 1회 디스패치");
	Expect(f.a->GetState() == UIElementState::Hovered, "LButtonUp -> Hovered");

	// 눌렀다가 버튼 밖에서 떼면 커맨드가 나가면 안 된다.
	f.log.Clear();
	f.panel.OnMouseEvent(UIMouseEventType::Move, 50.0f, 50.0f);
	f.panel.OnMouseEvent(UIMouseEventType::LButtonDown, 50.0f, 50.0f);
	Expect(f.a->GetState() == UIElementState::Pressed, "다시 Pressed");

	// 버튼 밖(패널 안 빈 공간)에서 뗀다.
	f.a->OnMouseEvent(UIMouseEventType::LButtonUp, 100.0f, 50.0f);
	Expect(f.log.Count() == 0, "버튼 밖에서 떼면 커맨드 없음");
	Expect(f.a->GetState() == UIElementState::Normal, "버튼 밖에서 떼면 Normal");
}

// ─────────────────────────────────────────────────────────────
// 가시성
// ─────────────────────────────────────────────────────────────
static void TestVisibility()
{
	printf("\n[가시성]\n");

	Fixture f;

	f.panel.OnMouseEvent(UIMouseEventType::Move, 50.0f, 50.0f);
	Expect(f.a->GetState() == UIElementState::Hovered, "Hovered 상태 만들기");

	// 숨기면 상태가 풀려야 한다. 안 그러면 다시 보였을 때 하이라이트가 남는다.
	f.a->SetVisible(false);
	Expect(f.a->GetState() == UIElementState::Normal, "숨기면 상태가 Normal 로 리셋");

	f.a->SetVisible(true);
	Expect(f.a->GetState() == UIElementState::Normal, "다시 보여도 Normal 유지");

	// 숨긴 자식은 히트되지 않으므로 뒤의 형제가 잡혀야 한다.
	f.a->SetVisible(false);
	f.log.Clear();
	f.panel.OnMouseEvent(UIMouseEventType::LButtonDown, 50.0f, 50.0f);
	f.panel.OnMouseEvent(UIMouseEventType::LButtonUp, 50.0f, 50.0f);
	Expect(f.log.Count() == 0, "숨긴 버튼은 커맨드를 내지 않음");
}

// ─────────────────────────────────────────────────────────────
// 중첩 패널
// ─────────────────────────────────────────────────────────────
static void TestNestedPanel()
{
	printf("\n[중첩 패널]\n");

	UIEventDispatcher dispatcher;
	CommandLog log;
	dispatcher.RegisterCallback(&CommandLog::Callback, &log);

	UIPanel outer;
	outer.SetLayout({ 0.0f, 0.0f, 300.0f, 200.0f });
	outer.SetLayoutType(UILayoutType::None);

	auto inner = std::make_shared<UIPanel>();
	inner->SetLayout({ 50.0f, 50.0f, 250.0f, 150.0f });
	inner->SetLayoutType(UILayoutType::None);

	auto deep = std::make_shared<UIButton>();
	deep->SetLayout({ 100.0f, 80.0f, 200.0f, 120.0f });
	deep->SetCommand(UICommand::ZoomFit);
	deep->SetEventDispatcher(&dispatcher);

	inner->AddChild(deep);
	outer.AddChild(inner);

	outer.OnMouseEvent(UIMouseEventType::Move, 150.0f, 100.0f);
	Expect(deep->GetState() == UIElementState::Hovered, "중첩 자식까지 hover 전달");

	outer.OnMouseEvent(UIMouseEventType::LButtonDown, 150.0f, 100.0f);
	outer.OnMouseEvent(UIMouseEventType::LButtonUp, 150.0f, 100.0f);
	Expect(log.Count() == 1 && log.Last() == UICommand::ZoomFit,
		"중첩 자식의 커맨드 디스패치");

	outer.OnMouseEvent(UIMouseEventType::Move, 20.0f, 20.0f);
	Expect(deep->GetState() == UIElementState::Normal,
		"중첩 자식 밖으로 나가면 해제");
}

// ─────────────────────────────────────────────────────────────
// 레이아웃 배치
// ─────────────────────────────────────────────────────────────
static void TestLayout()
{
	printf("\n[세로 배치]\n");

	UIPanel panel;
	panel.SetLayout({ 0.0f, 0.0f, 50.0f, 300.0f });
	panel.SetLayoutType(UILayoutType::Vertical);
	panel.SetPadding(5.0f);
	panel.SetSpacing(8.0f);

	auto first = std::make_shared<UIButton>();
	auto second = std::make_shared<UIButton>();
	first->SetLayout({ 0.0f, 0.0f, 40.0f, 40.0f });
	second->SetLayout({ 0.0f, 0.0f, 40.0f, 40.0f });

	panel.AddChild(first);
	panel.AddChild(second);

	const Rect2f& r1 = first->GetLayout();
	const Rect2f& r2 = second->GetLayout();

	printf("       첫째 (%.0f,%.0f)-(%.0f,%.0f)  둘째 (%.0f,%.0f)-(%.0f,%.0f)\n",
		r1.left, r1.top, r1.right, r1.bottom,
		r2.left, r2.top, r2.right, r2.bottom);

	Expect(r1.top == 5.0f, "첫 자식이 padding 만큼 내려감");
	Expect(r2.top == r1.bottom + 8.0f, "둘째가 spacing 만큼 띄워짐");
	Expect(r1.left == 5.0f && r2.left == 5.0f, "좌측 padding 적용");
}

int main()
{
	printf("D3D11UIFramework 회귀 하네스 (렌더 컨텍스트 없음)\n");
	printf("================================================\n");

	TestHitTest();
	TestHoverTransition();
	TestConsumption();
	TestButtonStateAndCommand();
	TestVisibility();
	TestNestedPanel();
	TestLayout();

	printf("\n================================================\n");
	printf("%s  (%d/%d)\n",
		g_fail == 0 ? "=== ALL PASS ===" : "=== FAILURES ===",
		g_total - g_fail, g_total);

	return g_fail;
}
