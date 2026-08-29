#pragma once

// 코드로 그리는 내장 아이콘.
//
// SVG 파일 대신 D2D 프리미티브로 직접 그린다. 성능 때문이 아니라 의존성
// 때문이다 — 아이콘 파일이 저장소 밖에 있으면 배포에서 빠지기 쉽고, 빠져도
// 조용히 안 그려질 뿐이라 코드 회귀로 오진하게 된다(실제로 두 번 그랬다).
//
// 모든 도형은 0..1 정규화 좌표로 그린다. 대상 사각형으로의 변환은
// UIIconRenderer 가 처리하므로 크기에 독립적이다.
enum class UIIconShape
{
	None = 0,

	// 툴바 / 컨텍스트 메뉴
	ZoomIn,
	ZoomOut,
	ZoomOneToOne,   // 모서리가 안쪽을 향한다 — 실제 크기
	ZoomFit,        // 모서리가 바깥을 향한다 — 맞춤
	MeasureDistance,
	Lut,

	// 컨텍스트 메뉴
	Check,
	ChevronRight,   // 하위 메뉴 표시

	// 상태바
	Cursor,
	Palette,
	ImageSize,

	Count
};
