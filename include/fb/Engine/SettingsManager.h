#pragma once

#define OFFSET_SETTINGSMANAGER_INSTANCE CYPRESS_GW_SELECT(0x141EAC0B0, 0x142B508E0)

#define OFFSET_FUNC_SETTINGSMANAGER_GETCONTAINER CYPRESS_GW_SELECT(0x1403BCFE0, 0x1401F2360)
#define OFFSET_FUNC_SETTINGSMANAGER_SET			 CYPRESS_GW_SELECT(0x1403BDFA0, 0x1401F01E0)

namespace fb
{
	class SettingsManager
	{
	public:
		template <class T>
		T* getContainer(const char* identifier)
		{
			auto func = reinterpret_cast<T * (*)(SettingsManager*, const char*)>(OFFSET_FUNC_SETTINGSMANAGER_GETCONTAINER);
			T* container = func(this, identifier);
			CYPRESS_ASSERT(container != nullptr, "Failed to get settings container for {}", identifier);
			return container;
		}
		
		bool set(const char* identifier, const char* value, void* typeInfo = nullptr)
		{
			auto func = reinterpret_cast<bool(*)(SettingsManager*, const char*, const char*, void*)>(OFFSET_FUNC_SETTINGSMANAGER_SET);
			return func(this, identifier, value, typeInfo);
		}

		static SettingsManager* GetInstance()
		{
			return *(SettingsManager**)OFFSET_SETTINGSMANAGER_INSTANCE;
		}
	};
}