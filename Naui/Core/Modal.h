#pragma once
#pragma warning(push)
#pragma warning(disable : 4251)

#include "Base.h"
#include "Panel.h"
#include "Vector.h"

#include <functional>
#include <string>
#include <unordered_map>
#include <typeinfo>

namespace Naui {

enum class ModalFocusPolicy
{
	Free,
	SoftBlock,
	HardBlock,
};

enum class ModalPosition
{
	Default,
	Center,
	TopCenter,
	Custom,
};

class NAUI_API Modal : public PanelImGuiImpl
{
public:
	Modal() = default;
	Modal(const char* title) { m_title = title; }

	uint64_t GetUID() const { return (uint64_t)this; }
	const std::string& GetTypeName() const { return m_typeName; }
	void SetTypeName(const std::string& type) { m_typeName = type; }

	bool IsOpen() const { return m_open; }
	void SetOpen(bool value) { m_open = value; }

	void SetFocusPolicy(ModalFocusPolicy policy){ m_focusPolicy				= policy; }
	void SetCloseOnOverlayClick(bool value)		{ m_closeOnOverlayClick		= value; }
	void SetAllowMultipleInstances(bool value)	{ m_allowMultiple			= value; }
	void SetClosable(bool value)				{ m_closable				= value; }
	void SetPosition(ModalPosition pos)			{ m_position				= pos;   }
	void SetCustomPosition(const Vec2& pos)	{ m_customPos = pos; m_position = ModalPosition::Custom; }

	ModalFocusPolicy GetFocusPolicy()			const { return m_focusPolicy; }
	bool			AllowsMultipleInstances()	const { return m_allowMultiple; }
	bool			ClosesOnOverlayClick()		const { return m_closeOnOverlayClick; }
	bool			IsClosable()				const { return m_closable; }
	ModalPosition	GetPosition()				const { return m_position; }
	const Vec2&	GetCustomPosition()			const { return m_customPos; }
	virtual void OnRender() {}
	virtual void OnClose()  {}

protected:

	bool				m_open					= true;
	bool				m_closeOnOverlayClick	= true;
	bool				m_allowMultiple			= false;
	bool				m_closable				= true;
	ModalFocusPolicy	m_focusPolicy			= ModalFocusPolicy::SoftBlock;
	ModalPosition		m_position				= ModalPosition::Center;
	Vec2				m_customPos				= Vec2(0, 0);
	std::string			m_typeName;

	friend class ModalRenderer;
};

using ModalFactory = std::function<Modal*()>;

NAUI_API std::unordered_map<uint64_t, Modal*>& GetAllModals();
NAUI_API std::unordered_map<std::string, ModalFactory>& GetModalFactories();

NAUI_API void RegisterModalFactory(const std::string& typeName, ModalFactory factory);
NAUI_API void DestroyModal(uint64_t uid);
NAUI_API void DestroyAllModals();

NAUI_API void SetModalOverlayColor(const Vec4& color);
NAUI_API Vec4 GetModalOverlayColor();
NAUI_API void SetModalOverlayAlpha(float alpha);
NAUI_API void FocusModalWindow(const char* title);

template<typename T>
void RegisterModal()
{
	const std::string normalized = NormalizeTypeName(typeid(T).name());
	RegisterModalFactory(normalized, []() -> Modal*
	{
		Modal* m = new T;
		m->SetTypeName(NormalizeTypeName(typeid(T).name()));
		return m;
	});
}

// Opens the modal if not already open. Returns the instance so data can be
// set immediately without a separate GetModal call:
//   if (auto* p = Naui::TriggerModal<MyModal>()) p->myData = value;
template<typename T>
T* TriggerModal()
{
	auto& modals = GetAllModals();
	auto& factories = GetModalFactories();
	const std::string typeName = NormalizeTypeName(typeid(T).name());

	for (auto& [uid, modal] : modals)
	{
		if (modal->GetTypeName() != typeName)
			continue;

		if (!modal->AllowsMultipleInstances())
		{
			modal->SetOpen(true);
			FocusModalWindow(modal->GetTitle().c_str());
			return static_cast<T*>(modal);
		}
	}

	auto it = factories.find(typeName);
	if (it == factories.end())
		return nullptr;

	Modal* m = it->second();
	modals[m->GetUID()] = m;
	return static_cast<T*>(m);
}

template<typename T>
void CloseModal()
{
	const std::string typeName = NormalizeTypeName(typeid(T).name());
	auto& modals = GetAllModals();

	for (auto it = modals.begin(); it != modals.end(); ++it)
	{
		if (it->second->GetTypeName() == typeName)
		{
			it->second->OnClose();
			delete it->second;
			modals.erase(it);
			return;
		}
	}
}

template<typename T>
T* GetModal()
{
	for (auto& [uid, modal] : GetAllModals())
		if (auto casted = dynamic_cast<T*>(modal))
			return casted;

	return nullptr;
}

}

#pragma warning(pop)