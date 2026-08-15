#include "complete.h"
#include "mono.h"
#include "steam.h"
#include <Windows.h>
#include <cstring>
#include <initializer_list>
#include <string>
#include <unordered_map>
#include <vector>

namespace ddlc
{
	namespace
	{
		MonoClassField* FindField(MonoClass* klass, std::initializer_list<const char*> names)
		{
			if (klass == nullptr)
			{
				return nullptr;
			}
			for (const char* name : names)
			{
				MonoClassField* field = mono::class_get_field_from_name(klass, name);
				if (field != nullptr)
				{
					return field;
				}
			}
			return nullptr;
		}

		struct Bindings
		{
			MonoDomain* domain = nullptr;
			MonoImage* image = nullptr;
			MonoObject* unlockSystem = nullptr;
			MonoClassField* unlocks = nullptr;
			MonoClassField* unlockProgress = nullptr;
			MonoClassField* unlocksCompleted = nullptr;
			MonoClassField* unlocksIgnored = nullptr;
			MonoClassField* boxesDrawn = nullptr;
			MonoClassField* jukeboxTime = nullptr;
			MonoClassField* boxAchievement = nullptr;
			MonoClassField* jukeboxAchievement = nullptr;
			MonoClassField* ignoreInCompletion = nullptr;
			MonoClassField* platformIds = nullptr;
			MonoClassField* unlockConditions = nullptr;
			MonoClassField* steamId = nullptr;
			MonoClassField* completed = nullptr;
			MonoClassField* completionTime = nullptr;
			MonoClassField* progressArray = nullptr;
			MonoClassField* lastProgress = nullptr;
		};

		bool Resolve(Bindings& b, std::string& error)
		{
#define REQUIRE(value, what)                       \
	if ((value) == nullptr)                        \
	{                                              \
		error = "couldn't resolve " what;          \
		return false;                              \
	}

			b.domain = mono::get_root_domain();
			REQUIRE(b.domain, "the root domain");

			MonoAssembly* assembly = mono::domain_assembly_open(b.domain, "DDLC");
			REQUIRE(assembly, "DDLC.dll");
			b.image = mono::assembly_get_image(assembly);
			REQUIRE(b.image, "the DDLC image");

			MonoClass* unlockSystemClass = mono::class_from_name(b.image, "", "UnlockSystem");
			REQUIRE(unlockSystemClass, "UnlockSystem");

			MonoClassField* activeInstance = mono::class_get_field_from_name(unlockSystemClass, "s_ActiveInstance");
			REQUIRE(activeInstance, "UnlockSystem.s_ActiveInstance");

			MonoVTable* vtable = mono::class_vtable(b.domain, unlockSystemClass);
			REQUIRE(vtable, "the UnlockSystem vtable");
			mono::field_static_get_value(vtable, activeInstance, &b.unlockSystem);
			if (b.unlockSystem == nullptr)
			{
				error = "UnlockSystem isn't up yet - reach the launcher desktop first.";
				return false;
			}

			b.unlocks = mono::class_get_field_from_name(unlockSystemClass, "m_Unlocks");
			REQUIRE(b.unlocks, "UnlockSystem.m_Unlocks");
			b.unlockProgress = mono::class_get_field_from_name(unlockSystemClass, "m_UnlockProgress");
			REQUIRE(b.unlockProgress, "UnlockSystem.m_UnlockProgress");
			b.unlocksCompleted = mono::class_get_field_from_name(unlockSystemClass, "m_UnlocksCompleted");
			REQUIRE(b.unlocksCompleted, "UnlockSystem.m_UnlocksCompleted");
			b.unlocksIgnored = mono::class_get_field_from_name(unlockSystemClass, "m_UnlocksIgnoredCompletion");
			REQUIRE(b.unlocksIgnored, "UnlockSystem.m_UnlocksIgnoredCompletion");
			b.boxesDrawn = mono::class_get_field_from_name(unlockSystemClass, "m_DesktopBoxesDrawn");
			REQUIRE(b.boxesDrawn, "UnlockSystem.m_DesktopBoxesDrawn");
			b.jukeboxTime = mono::class_get_field_from_name(unlockSystemClass, "m_JukeboxTotalPlayTime");
			REQUIRE(b.jukeboxTime, "UnlockSystem.m_JukeboxTotalPlayTime");
			b.boxAchievement = mono::class_get_field_from_name(unlockSystemClass, "m_BoxAchievementUnlocked");
			REQUIRE(b.boxAchievement, "UnlockSystem.m_BoxAchievementUnlocked");
			b.jukeboxAchievement = mono::class_get_field_from_name(unlockSystemClass, "m_JukeboxAchievementUnlocked");
			REQUIRE(b.jukeboxAchievement, "UnlockSystem.m_JukeboxAchievementUnlocked");

			MonoClass* unlockInfoClass = mono::class_from_name(b.image, "", "UnlockInfo");
			REQUIRE(unlockInfoClass, "UnlockInfo");
			b.ignoreInCompletion = mono::class_get_field_from_name(unlockInfoClass, "IgnoreInCompletion");
			REQUIRE(b.ignoreInCompletion, "UnlockInfo.IgnoreInCompletion");
			b.platformIds = mono::class_get_field_from_name(unlockInfoClass, "PlatformIds");
			REQUIRE(b.platformIds, "UnlockInfo.PlatformIds");
			b.unlockConditions = mono::class_get_field_from_name(unlockInfoClass, "UnlockConditions");
			REQUIRE(b.unlockConditions, "UnlockInfo.UnlockConditions");

			MonoClass* achievementIdsClass = mono::class_from_name(b.image, "", "UnlockInfo/AchievementIds");
			REQUIRE(achievementIdsClass, "UnlockInfo.AchievementIds");
			b.steamId = mono::class_get_field_from_name(achievementIdsClass, "Steam");
			REQUIRE(b.steamId, "UnlockInfo.AchievementIds.Steam");

			MonoClass* progressClass = mono::class_from_name(b.image, "", "UnlockSystem/UnlockProgress");
			REQUIRE(progressClass, "UnlockSystem.UnlockProgress");
			b.completed = mono::class_get_field_from_name(progressClass, "Completed");
			REQUIRE(b.completed, "UnlockProgress.Completed");
			b.completionTime = mono::class_get_field_from_name(progressClass, "CompletionTime");
			REQUIRE(b.completionTime, "UnlockProgress.CompletionTime");
			b.progressArray = mono::class_get_field_from_name(progressClass, "Progress");
			REQUIRE(b.progressArray, "UnlockProgress.Progress");
			b.lastProgress = mono::class_get_field_from_name(progressClass, "LastProgress");
			REQUIRE(b.lastProgress, "UnlockProgress.LastProgress");

#undef REQUIRE

			return true;
		}

		struct Dictionary
		{
			MonoObject* entries = nullptr;
			int32_t count = 0;
			int32_t elementSize = 0;
			uint32_t hashOffset = 0;
			uint32_t keyOffset = 0;
			uint32_t valueOffset = 0;
		};

		bool OpenDictionary(MonoObject* owner, MonoClassField* field, Dictionary& out, std::string& error)
		{
			MonoObject* dictionary = mono::ReadField<MonoObject*>(owner, field);
			if (dictionary == nullptr)
			{
				error = "a dictionary on UnlockSystem was null.";
				return false;
			}

			MonoClass* dictionaryClass = mono::object_get_class(dictionary);
			MonoClassField* entriesField = FindField(dictionaryClass, { "entries", "_entries" });
			MonoClassField* countField = FindField(dictionaryClass, { "count", "_count" });
			if (entriesField == nullptr || countField == nullptr)
			{
				error = "this Dictionary implementation doesn't look the way we expect.";
				return false;
			}

			MonoClass* entryArrayClass = mono::class_from_mono_type(mono::field_get_type(entriesField));
			MonoClass* entryClass = (entryArrayClass != nullptr) ? mono::class_get_element_class(entryArrayClass) : nullptr;
			MonoClassField* hashField = FindField(entryClass, { "hashCode", "_hashCode" });
			MonoClassField* keyField = FindField(entryClass, { "key", "_key" });
			MonoClassField* valueField = FindField(entryClass, { "value", "_value" });
			if (hashField == nullptr || keyField == nullptr || valueField == nullptr)
			{
				error = "couldn't read the Dictionary entry layout.";
				return false;
			}

			out.entries = mono::ReadField<MonoObject*>(dictionary, entriesField);
			out.count = mono::ReadField<int32_t>(dictionary, countField);
			out.elementSize = mono::class_array_element_size(entryClass);
			out.hashOffset = mono::StructFieldOffset(hashField);
			out.keyOffset = mono::StructFieldOffset(keyField);
			out.valueOffset = mono::StructFieldOffset(valueField);
			return true;
		}

		template <typename Fn>
		void ForEachEntry(const Dictionary& dictionary, Fn&& fn)
		{
			if (dictionary.entries == nullptr || dictionary.elementSize <= 0)
			{
				return;
			}

			uintptr_t capacity = mono::ArrayLength(dictionary.entries);
			uintptr_t used = static_cast<uintptr_t>(dictionary.count < 0 ? 0 : dictionary.count);
			if (used > capacity)
			{
				used = capacity;
			}

			uint8_t* base = mono::ArrayData(dictionary.entries);
			for (uintptr_t i = 0; i < used; i++)
			{
				uint8_t* entry = base + i * static_cast<size_t>(dictionary.elementSize);
				if (*reinterpret_cast<int32_t*>(entry + dictionary.hashOffset) < 0)
				{
					continue;
				}
				fn(*reinterpret_cast<uint32_t*>(entry + dictionary.keyOffset), *reinterpret_cast<MonoObject**>(entry + dictionary.valueOffset));
			}
		}

		int64_t FileTimeNow()
		{
			FILETIME now{};
			GetSystemTimeAsFileTime(&now);
			ULARGE_INTEGER packed{};
			packed.LowPart = now.dwLowDateTime;
			packed.HighPart = now.dwHighDateTime;
			return static_cast<int64_t>(packed.QuadPart);
		}

		int32_t ReadListCount(MonoObject* list)
		{
			if (list == nullptr)
			{
				return 0;
			}
			MonoClassField* sizeField = FindField(mono::object_get_class(list), { "_size", "_count" });
			return (sizeField != nullptr) ? mono::ReadField<int32_t>(list, sizeField) : 0;
		}
	}

	Result CompleteEverything()
	{
		Result result;

		Bindings b;
		std::string error;
		if (!Resolve(b, error))
		{
			result.text = error;
			return result;
		}

		Dictionary unlocks;
		Dictionary progress;
		if (!OpenDictionary(b.unlockSystem, b.unlocks, unlocks, error) || !OpenDictionary(b.unlockSystem, b.unlockProgress, progress, error))
		{
			result.text = error;
			return result;
		}

		std::unordered_map<uint32_t, MonoObject*> infoById;
		std::vector<std::string> achievementIds;
		int32_t counted = 0;
		int32_t ignored = 0;

		ForEachEntry(unlocks, [&](uint32_t id, MonoObject* info)
		{
			if (info == nullptr)
			{
				return;
			}
			infoById[id] = info;

			if (mono::ReadField<uint8_t>(info, b.ignoreInCompletion) != 0)
			{
				ignored++;
			}
			else
			{
				counted++;
			}

			MonoObject* platform = mono::ReadField<MonoObject*>(info, b.platformIds);
			if (platform != nullptr)
			{
				std::string steam = mono::ReadString(mono::ReadField<MonoObject*>(platform, b.steamId));
				if (!steam.empty())
				{
					achievementIds.push_back(steam);
				}
			}
		});

		const int64_t now = FileTimeNow();
		int32_t flipped = 0;

		ForEachEntry(progress, [&](uint32_t id, MonoObject* record)
		{
			if (record == nullptr)
			{
				return;
			}

			MonoObject* conditions = mono::ReadField<MonoObject*>(record, b.progressArray);
			if (conditions == nullptr)
			{
				auto info = infoById.find(id);
				int32_t conditionCount = (info != infoById.end()) ? ReadListCount(mono::ReadField<MonoObject*>(info->second, b.unlockConditions)) : 0;
				conditions = mono::array_new(b.domain, mono::get_boolean_class(), static_cast<uintptr_t>(conditionCount));
				mono::WriteField<MonoObject*>(record, b.progressArray, conditions);
			}

			if (conditions != nullptr)
			{
				std::memset(mono::ArrayData(conditions), 1, static_cast<size_t>(mono::ArrayLength(conditions)));
			}

			mono::WriteField<uint8_t>(record, b.completed, 1);
			mono::WriteField<float>(record, b.lastProgress, 1.0f);
			if (mono::ReadField<int64_t>(record, b.completionTime) == 0)
			{
				mono::WriteField<int64_t>(record, b.completionTime, now);
			}
			flipped++;
		});

		mono::WriteField<int32_t>(b.unlockSystem, b.unlocksCompleted, counted);
		mono::WriteField<int32_t>(b.unlockSystem, b.unlocksIgnored, ignored);
		mono::WriteField<int32_t>(b.unlockSystem, b.boxesDrawn, 50);
		mono::WriteField<float>(b.unlockSystem, b.jukeboxTime, 1201.0f);
		mono::WriteField<uint8_t>(b.unlockSystem, b.boxAchievement, 1);
		mono::WriteField<uint8_t>(b.unlockSystem, b.jukeboxAchievement, 1);

		std::string steamText;
		if (steam::Load(error))
		{
			std::string steamError;
			int granted = steam::GrantAll(achievementIds, steamError);
			steamText = steamError.empty() ? std::to_string(granted) + " achievements sent to Steam." : steamError;
		}
		else
		{
			steamText = error;
		}

		result.ok = true;
		result.text = std::to_string(flipped) + " unlocks completed. " + steamText;
		if (flipped < static_cast<int32_t>(infoById.size()))
		{
			result.text += " (" + std::to_string(static_cast<int32_t>(infoById.size()) - flipped) + " had no progress record and were skipped.)";
		}
		return result;
	}

	Result LockEverything()
	{
		Result result;

		Bindings b;
		std::string error;
		if (!Resolve(b, error))
		{
			result.text = error;
			return result;
		}

		Dictionary progress;
		if (!OpenDictionary(b.unlockSystem, b.unlockProgress, progress, error))
		{
			result.text = error;
			return result;
		}

		int32_t flipped = 0;
		ForEachEntry(progress, [&](uint32_t, MonoObject* record)
		{
			if (record == nullptr)
			{
				return;
			}

			MonoObject* conditions = mono::ReadField<MonoObject*>(record, b.progressArray);
			if (conditions != nullptr)
			{
				std::memset(mono::ArrayData(conditions), 0, static_cast<size_t>(mono::ArrayLength(conditions)));
			}

			mono::WriteField<uint8_t>(record, b.completed, 0);
			mono::WriteField<float>(record, b.lastProgress, 0.0f);
			mono::WriteField<int64_t>(record, b.completionTime, 0);
			flipped++;
		});

		mono::WriteField<int32_t>(b.unlockSystem, b.unlocksCompleted, 0);
		mono::WriteField<int32_t>(b.unlockSystem, b.boxesDrawn, 0);
		mono::WriteField<float>(b.unlockSystem, b.jukeboxTime, 0.0f);
		mono::WriteField<uint8_t>(b.unlockSystem, b.boxAchievement, 0);
		mono::WriteField<uint8_t>(b.unlockSystem, b.jukeboxAchievement, 0);

		result.ok = true;
		result.text = std::to_string(flipped) + " unlocks cleared. Settings should now read 0%. Steam untouched.";
		return result;
	}

	Result SaveNow()
	{
		Result result;

		MonoDomain* domain = mono::get_root_domain();
		MonoAssembly* assembly = (domain != nullptr) ? mono::domain_assembly_open(domain, "DDLC") : nullptr;
		MonoImage* image = (assembly != nullptr) ? mono::assembly_get_image(assembly) : nullptr;
		MonoClass* renpyClass = (image != nullptr) ? mono::class_from_name(image, "", "Renpy") : nullptr;
		MonoMethod* getLauncher = (renpyClass != nullptr) ? mono::class_get_method_from_name(renpyClass, "get_LauncherMain", 0) : nullptr;
		if (getLauncher == nullptr)
		{
			result.text = "Couldn't find Renpy.LauncherMain.";
			return result;
		}

		MonoObject* exception = nullptr;
		MonoObject* launcher = mono::runtime_invoke(getLauncher, nullptr, nullptr, &exception);
		if (exception != nullptr || launcher == nullptr)
		{
			result.text = "No launcher right now - open the desktop, then switch apps to save.";
			return result;
		}

		MonoMethod* save = mono::class_get_method_from_name(mono::object_get_class(launcher), "TriggerLauncherSave", 0);
		if (save == nullptr)
		{
			result.text = "Couldn't find TriggerLauncherSave.";
			return result;
		}

		exception = nullptr;
		mono::runtime_invoke(save, launcher, nullptr, &exception);
		if (exception != nullptr)
		{
			result.text = "Save threw (Unity is main-thread-only) - switch desktop apps instead.";
			return result;
		}

		result.ok = true;
		result.text = "Save started.";
		return result;
	}
}