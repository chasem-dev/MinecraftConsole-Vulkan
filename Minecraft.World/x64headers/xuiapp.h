// xuiapp.h - XUI framework stubs for non-Xbox platforms
#pragma once

#ifndef _XBOX

#include <cstddef>

// ---------------------------------------------------------------------------
// Handle types (already defined in extraX64.h: HXUIOBJ, HXUIDC, HXUIBRUSH)
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------
#ifndef XM_USER
#define XM_USER 0x0400
#endif

#ifndef XUI_ERR_SOURCEDATA_ITEM
#define XUI_ERR_SOURCEDATA_ITEM 0
#endif
#ifndef XUI_KEYDOWN
#define XUI_KEYDOWN             0
#endif
#ifndef XUI_TRANSITION_TO
#define XUI_TRANSITION_TO       0
#endif
#ifndef XUI_TRANSITION_BACKTO
#define XUI_TRANSITION_BACKTO   1
#endif
#ifndef XUI_TRANSITION_ACTION_DESTROY
#define XUI_TRANSITION_ACTION_DESTROY 0
#endif
#ifndef XUI_CLASS_SCENE
#define XUI_CLASS_SCENE         0
#endif
#ifndef XUI_CLASS_CONTROL
#define XUI_CLASS_CONTROL       1
#endif
#ifndef XUI_CLASS_EDIT
#define XUI_CLASS_EDIT          2
#endif
#ifndef XUI_CLASS_LIST
#define XUI_CLASS_LIST          3
#endif
#ifndef XUI_CLASS_LABEL
#define XUI_CLASS_LABEL         4
#endif
#ifndef XUI_ITEMCOUNT_MAX_LINES
#define XUI_ITEMCOUNT_MAX_LINES    0
#endif
#ifndef XUI_ITEMCOUNT_MAX_PER_LINE
#define XUI_ITEMCOUNT_MAX_PER_LINE 1
#endif
#ifndef PAGE_READWRITE
#define PAGE_READWRITE          0x04
#endif

#ifndef MAX_CREDIT_STRINGS
#define MAX_CREDIT_STRINGS 256
#endif

#ifndef SAFEZONE_HALF_WIDTH
#define SAFEZONE_HALF_WIDTH 32.0f
#endif
#ifndef SAFEZONE_HALF_HEIGHT
#define SAFEZONE_HALF_HEIGHT 18.0f
#endif

#ifndef BUTTONS_TOOLTIP_MAX
#define BUTTONS_TOOLTIP_MAX 9
#endif

#ifndef MAXULONG_PTR
#define MAXULONG_PTR ((unsigned long)~0)
#endif

// Base scene dimensions
#ifndef XUI_BASE_SCENE_WIDTH_HALF
#define XUI_BASE_SCENE_WIDTH_HALF   640.0f
#endif
#ifndef XUI_BASE_SCENE_HEIGHT_HALF
#define XUI_BASE_SCENE_HEIGHT_HALF  360.0f
#endif
#ifndef XUI_BASE_SCENE_WIDTH_QUARTER
#define XUI_BASE_SCENE_WIDTH_QUARTER 320.0f
#endif
#ifndef XUI_BASE_SCENE_HEIGHT_QUARTER
#define XUI_BASE_SCENE_HEIGHT_QUARTER 180.0f
#endif

// ---------------------------------------------------------------------------
// XUIMessageData - used as a type cast: (XUIMessageData*) pData
// ---------------------------------------------------------------------------
typedef void XUIMessageData;

// ---------------------------------------------------------------------------
// XUI message structure
// ---------------------------------------------------------------------------
#ifndef _XUI_MESSAGE_DEFINED
#define _XUI_MESSAGE_DEFINED
struct XUIMessage
{
  unsigned int dwMessage;
  void *pvData;
  int bHandled;
  XUIMessage() : dwMessage(0), pvData(nullptr), bHandled(0) {}
};
#endif

// Macros for XUI messages
#ifndef XuiMessage
#define XuiMessage(pMsg, id) do { (pMsg)->dwMessage = (id); (pMsg)->pvData = nullptr; (pMsg)->bHandled = 0; } while(0)
#endif
#define _XuiMessageExtra(pMsg, pData, sz) do { (pMsg)->pvData = (pData); } while(0)
#define XuiMessageExtraInfo(pMsg) ((pMsg)->pvData)
#define XuiMessageDataToExtra(type, pMsg) ((type*)((pMsg)->pvData))

// ---------------------------------------------------------------------------
// Message subtypes
// ---------------------------------------------------------------------------
struct XUIMessageInit : public XUIMessage
{
  int bHasScene;
  void *pvInitData;
  XUIMessageInit() : bHasScene(0), pvInitData(nullptr) {}
};

struct XUIMessageInput : public XUIMessage
{
  unsigned int dwKeyCode;
  int bKeyDown;
  unsigned int dwFlags;
  unsigned int dwUserIndex;
  unsigned int UserIndex;
  XUIMessageInput() : dwKeyCode(0), bKeyDown(0), dwFlags(0), dwUserIndex(0), UserIndex(0) {}
};

struct XUIMessageChar : public XUIMessage
{
  wchar_t wch;
  XUIMessageChar() : wch(0) {}
};

struct XUIMessageTimer : public XUIMessage
{
  unsigned int nTimerId;
  unsigned int &nId = nTimerId; // alias
  XUIMessageTimer() : nTimerId(0) {}
};

struct XUIMessageRender : public XUIMessage
{
  HXUIDC hdc;
  XUIMessageRender() : hdc(nullptr) {}
};

struct XUIMessageTransition : public XUIMessage
{
  int direction;
  int dwTransType;
  int dwTransAction;
  XUIMessageTransition() : direction(0), dwTransType(0), dwTransAction(0) {}
};

struct XUIMessageControlNavigate : public XUIMessage
{
  int navDir;
  int nControlNavigate;
  HXUIOBJ hObjDest;
  HXUIOBJ hObjSource;
  XUIMessageControlNavigate() : navDir(0), nControlNavigate(0), hObjDest(nullptr), hObjSource(nullptr) {}
};

struct XUIMessageGetItemCount : public XUIMessage
{
  int iItemCount;
  int &cItems = iItemCount; // alias
  int iType;
  XUIMessageGetItemCount() : iItemCount(0), iType(0) {}
};

struct XUIMessageGetSourceText : public XUIMessage
{
  int iItem;
  int iData;
  int bItemData;
  const wchar_t *szText;
  int bValid;
  XUIMessageGetSourceText() : iItem(0), iData(0), bItemData(0), szText(nullptr), bValid(0) {}
};

struct XUIMessageGetSourceImage : public XUIMessage
{
  int iItem;
  const wchar_t *szPath;
  HXUIBRUSH hBrush;
  XUIMessageGetSourceImage() : iItem(0), szPath(nullptr), hBrush(nullptr) {}
};

struct XUIMessageGetItemEnable : public XUIMessage
{
  int iItem;
  int bEnabled;
  XUIMessageGetItemEnable() : iItem(0), bEnabled(1) {}
};

// ---------------------------------------------------------------------------
// Notification subtypes
// ---------------------------------------------------------------------------
struct XUINotifyPress : public XUIMessage
{
  HXUIOBJ hObjSource;
  unsigned int dwKeyCode;
  unsigned int UserIndex;
  XUINotifyPress() : hObjSource(nullptr), dwKeyCode(0), UserIndex(0) {}
};

struct XUINotifyFocus : public XUIMessage
{
  HXUIOBJ hObjFocus;
  XUINotifyFocus() : hObjFocus(nullptr) {}
};

struct XUINotifySelChanged : public XUIMessage
{
  int iOldItem;
  int iItem;
  int bValid;
  XUINotifySelChanged() : iOldItem(0), iItem(0), bValid(0) {}
};

struct XUINotifyValueChanged : public XUIMessage
{
  int iValue;
  int &nValue = iValue; // alias
  XUINotifyValueChanged() : iValue(0) {}
};

typedef int XUI_CONTROL_NAVIGATE;

// ---------------------------------------------------------------------------
// Rect type
// ---------------------------------------------------------------------------
#ifndef _XUI_RECT_DEFINED
#define _XUI_RECT_DEFINED
struct XUIRect { float left, top, right, bottom; };
typedef XUIRect xuiRect;
#endif

// ---------------------------------------------------------------------------
// CXuiElement - base element wrapper
// ---------------------------------------------------------------------------
class CXuiElement
{
public:
  HXUIOBJ m_hObj;

  CXuiElement() : m_hObj(nullptr) {}
  CXuiElement(HXUIOBJ h) : m_hObj(h) {}

  int GetChildById(const wchar_t *, HXUIOBJ *out) { if (out) *out = nullptr; return 0; }
  int GetChildById(const wchar_t *, CXuiElement *out) { if (out) out->m_hObj = nullptr; return 0; }
  HXUIOBJ GetParent() { return nullptr; }
  void SetShow(int) {}
  int GetShow() { return 1; }
  void SetPosition(float, float, float) {}
  void GetPosition(float *, float *, float *) {}
  void SetBounds(float, float) {}
  void GetBounds(float *, float *) {}
  void SetOpacity(float) {}
  float GetOpacity() { return 1.0f; }

  operator HXUIOBJ() const { return m_hObj; }
  CXuiElement &operator=(HXUIOBJ h) { m_hObj = h; return *this; }
};

// ---------------------------------------------------------------------------
// CXuiControl
// ---------------------------------------------------------------------------
class CXuiControl : public CXuiElement
{
public:
  CXuiControl() {}
  CXuiControl(HXUIOBJ h) : CXuiElement(h) {}

  int GetText(wchar_t *, unsigned int) { return 0; }
  int SetText(const wchar_t *) { return 0; }
  int GetVisual(HXUIOBJ *out) { if (out) *out = nullptr; return 0; }
  void SetEnable(int) {}
  int GetEnable() { return 1; }
  void EnableInput(int) {}
  void SetFocus(int) {}
  int GetFocus() { return 0; }
  int SetCurSel(int) { return 0; }
  int GetCurSel() { return 0; }
  int SetTopItem(int) { return 0; }
  int GetItemCount() { return 0; }
  int InsertItems(int, int) { return 0; }
  int DeleteItems(int, int) { return 0; }
  int SetItemText(int, const wchar_t *) { return 0; }
  int GetItemText(int, wchar_t *, unsigned int) { return 0; }
  int SetItemEnable(int, int) { return 0; }
  int SetItemCheck(int, int) { return 0; }
  int GetItemCheck(int) { return 0; }
  int PlayVisualRange(int, int, float) { return 0; }
  int SetTimer(unsigned int, unsigned int) { return 0; }
  int KillTimer(unsigned int) { return 0; }
};

// ---------------------------------------------------------------------------
// CXuiCheckbox
// ---------------------------------------------------------------------------
class CXuiCheckbox : public CXuiControl
{
public:
  void SetCheck(int) {}
  int GetCheck() { return 0; }
  int IsChecked() { return 0; }
};

// ---------------------------------------------------------------------------
// CXuiSlider
// ---------------------------------------------------------------------------
class CXuiSlider : public CXuiControl
{
public:
  void SetRange(int, int) {}
  void SetValue(int) {}
  int GetValue() { return 0; }
  void SetStep(int) {}
};

// ---------------------------------------------------------------------------
// CXuiList / CXuiListImpl
// ---------------------------------------------------------------------------
struct LIST_ITEM_INFO
{
  int iIndex;
  int iData;
  int iSortIndex;
  int bEnabled;
  int bChecked;
  const wchar_t *szText;
  HXUIBRUSH hXuiBrush;
  LIST_ITEM_INFO() : iIndex(0), iData(0), iSortIndex(0), bEnabled(1), bChecked(0), szText(nullptr), hXuiBrush(nullptr) {}
};

class CXuiList : public CXuiControl
{
public:
};

class CXuiListImpl : public CXuiList
{
public:
  typedef LIST_ITEM_INFO LIST_ITEM_INFO;
};

class CXuiComboBoxImpl : public CXuiControl
{
public:
};

// ---------------------------------------------------------------------------
// CXuiImageElement / CXuiHtmlControl / CXuiSound
// ---------------------------------------------------------------------------
class CXuiImageElement : public CXuiElement
{
public:
  int SetImagePath(const wchar_t *) { return 0; }
  int SetBrush(HXUIBRUSH) { return 0; }
};

class CXuiHtmlControl : public CXuiControl
{
public:
  int SetHtml(const wchar_t *) { return 0; }
};

class CXuiSound
{
public:
  void Play() {}
};

// ---------------------------------------------------------------------------
// CXuiProgressBar
// ---------------------------------------------------------------------------
class CXuiProgressBar : public CXuiControl
{
public:
  void SetRange(int, int) {}
  void SetValue(int) {}
  int GetValue() { return 0; }
};

// ---------------------------------------------------------------------------
// CXuiScene / CXuiSceneImpl / CXuiSceneBase
// ---------------------------------------------------------------------------
class CXuiScene : public CXuiElement
{
public:
  CXuiScene() {}
};

class CXuiSceneImpl : public CXuiScene
{
public:
  CXuiSceneImpl() {}
};

class CXuiElementImplBase
{
public:
};

class CXuiControlImpl : public CXuiControl
{
public:
};

class CXuiModule
{
public:
};

// ESoundEffect is an enum defined in SoundTypes.h; do NOT forward declare as class

class CXuiSceneBase : public CXuiSceneImpl
{
public:
  static void ShowSafeArea(int) {}
  static void ShowLogo(int) {}
  static void ShowBackground(int) {}
  void PlayUISFX(int) {}
};

// CXuiStringTable is defined in extraX64.h

// ---------------------------------------------------------------------------
// NOTE: Custom control classes (CXuiCtrl4JEdit, CXuiCtrlSlotList,
// CXuiCtrl4JList, etc.) are defined in their own headers under
// Common/XUI/. They are NOT stubbed here to avoid redefinition.
// On macOS, Common/XUI/ source files are excluded from the build.
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// XUI global function stubs
// ---------------------------------------------------------------------------
inline int XuiElementGetChildById(HXUIOBJ, const wchar_t *, HXUIOBJ *out) { if (out) *out = nullptr; return 0; }
inline int XuiElementGetParent(HXUIOBJ, HXUIOBJ *out) { if (out) *out = nullptr; return 0; }
inline int XuiElementFindNamedFrame(HXUIOBJ, const wchar_t * = nullptr, int *out = nullptr) { if (out) *out = 0; return 0; }
inline int XuiElementGetPosition(HXUIOBJ, void *) { return 0; }
inline int XuiElementSetPosition(HXUIOBJ, void *) { return 0; }
inline int XuiElementGetBounds(HXUIOBJ, float *w, float *h) { if (w) *w = 0; if (h) *h = 0; return 0; }
inline int XuiElementSetBounds(HXUIOBJ, float, float) { return 0; }
inline int XuiElementSetShow(HXUIOBJ, int) { return 0; }
inline int XuiElementGetShow(HXUIOBJ) { return 1; }
inline int XuiElementPlayTimeline(HXUIOBJ, int, int, int, int = 0, int = 0) { return 0; }
inline int XuiElementSetUserFocus(HXUIOBJ, unsigned int = 0) { return 0; }
inline int XuiElementGetUserData(HXUIOBJ, void **out = nullptr) { if (out) *out = nullptr; return 0; }
inline int XuiElementSetUserData(HXUIOBJ, void *) { return 0; }
inline int XuiControlSetText(HXUIOBJ, const wchar_t *) { return 0; }
inline int XuiControlGetText(HXUIOBJ, wchar_t *, unsigned int) { return 0; }
inline int XuiControlGetVisual(HXUIOBJ, HXUIOBJ *out) { if (out) *out = nullptr; return 0; }
inline int XuiControlAttachVisual(HXUIOBJ) { return 0; }
inline int XuiControlSetEnable(HXUIOBJ, int) { return 0; }
inline int XuiControlSetFocus(HXUIOBJ, int = 0) { return 0; }
inline int XuiObjectFromHandle(HXUIOBJ, void **out) { if (out) *out = nullptr; return 0; }
inline int XuiObjectSetProperty(HXUIOBJ, unsigned int, unsigned int, void *) { return 0; }
inline int XuiSetTimer(HXUIOBJ, unsigned int, unsigned int) { return 0; }
inline int XuiKillTimer(HXUIOBJ, unsigned int) { return 0; }
inline int XuiBubbleMessage(HXUIOBJ, XUIMessage *) { return 0; }
inline int XuiCreateTextureBrushFromMemory(void *, unsigned int, unsigned int, unsigned int, unsigned int, HXUIBRUSH *out, ...) { if (out) *out = nullptr; return 0; }
inline int XuiDestroyBrush(HXUIBRUSH) { return 0; }
inline int XuiAttachBrush(HXUIOBJ, HXUIBRUSH) { return 0; }
inline int XuiControlInsertItems(HXUIOBJ, int, int) { return 0; }
inline int XuiControlDeleteItems(HXUIOBJ, int, int) { return 0; }
inline int XuiListSetCurSel(HXUIOBJ, int) { return 0; }
inline int XuiListGetCurSel(HXUIOBJ, int *out) { if (out) *out = 0; return 0; }
inline int XuiListSetTopItem(HXUIOBJ, int) { return 0; }
inline int XuiListGetCount(HXUIOBJ, int *out) { if (out) *out = 0; return 0; }

// ---------------------------------------------------------------------------
// XUI Macros (no-op on non-Xbox)
// ---------------------------------------------------------------------------
#define XUI_IMPLEMENT_CLASS(cls, name, type)
#define XUI_BEGIN_MSG_MAP()
#define XUI_END_MSG_MAP()
#define XUI_ON_XM_INIT(fn)
#define XUI_ON_XM_TIMER(fn)
#define XUI_ON_XM_KEYDOWN(fn)
#define XUI_ON_XM_CHAR(fn)
#define XUI_ON_XM_DESTROY(fn)
#define XUI_ON_XM_RENDER(fn)
#define XUI_ON_XM_GET_ITEMCOUNT_ALL(fn)
#define XUI_ON_XM_GET_ITEMCOUNT_MAX_LINES(fn)
#define XUI_ON_XM_GET_ITEMCOUNT_MAX_PER_LINE(fn)
#define XUI_ON_XM_SKIN_CHANGED(fn)
#define XUI_ON_XM_TRANSITION_START(fn)
#define XUI_ON_XM_SPLITSCREENPLAYER_MESSAGE(fn)
#define XUI_ON_XM_GET_SOURCE_TEXT(fn)
#define XUI_ON_XM_GET_SOURCE_IMAGE(fn)
#define XUI_ON_XM_GET_ITEMCHECK(fn)
#define XUI_ON_XM_SET_ITEMCHECK(fn)
#define XUI_ON_XM_GET_ITEMENABLE(fn)
#define XUI_ON_XM_SET_ITEMENABLE(fn)
#define XUI_ON_XM_NOTIFY_PRESS(fn)
#define XUI_ON_XM_NOTIFY_PRESS_EX(fn)
#define XUI_ON_XM_NOTIFY_FOCUS(fn)
#define XUI_ON_XM_NOTIFY_SELCHANGED(fn)
#define XUI_ON_XM_NOTIFY_VALUECHANGED(fn)
#define XUI_ON_XM_NOTIFY_VALUE_CHANGED(fn)
#define XUI_ON_XM_CONTROL_NAVIGATE(fn)
#define XUI_ON_XM_INPUT(fn)
#define XUI_ON_XM_NAV_RETURN(fn)
#define XUI_ON_XM_NOTIFY_SET_FOCUS(fn)

#endif // !_XBOX
