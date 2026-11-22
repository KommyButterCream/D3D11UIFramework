# D3D11UIFramework

## 프로젝트 설명

Direct3D 11, Direct2D, DirectWrite를 사용하는 UI 프레임워크 DLL이다. 버튼, 레이블, 패널, 슬라이더 등 렌더링 레이어에 배치할 수 있는 컨트롤과 입력 이벤트 처리를 제공한다.

## 빌드 환경

- Windows 10 또는 11 x64 및 Windows SDK
- Visual Studio 2026의 **C++를 사용한 데스크톱 개발** 워크로드와 MSVC v145
- C++20

## 저장소 배치와 빌드

다음 폴더를 같은 상위 폴더에 배치한다.

```text
Module/
├─ Core/
├─ D3D11Engine/
├─ D3D11EngineInterface/
└─ D3D11UIFramework/
```

Visual Studio 2026에서 `D3D11UIFramework.sln`을 열고 `Debug|x64` 또는 `Release|x64`로 빌드한다. 결과 DLL은 솔루션의 `x64/Debug/` 또는 `x64/Release/`에 생성된다.
