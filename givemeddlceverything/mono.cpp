#include "mono.h"

namespace mono
{
	MonoDomain* (*get_root_domain)() = nullptr;
	MonoThread* (*thread_attach)(MonoDomain*) = nullptr;
	void (*thread_detach)(MonoThread*) = nullptr;
	MonoAssembly* (*domain_assembly_open)(MonoDomain*, const char*) = nullptr;
	MonoImage* (*assembly_get_image)(MonoAssembly*) = nullptr;
	MonoClass* (*class_from_name)(MonoImage*, const char*, const char*) = nullptr;
	MonoClassField* (*class_get_field_from_name)(MonoClass*, const char*) = nullptr;
	MonoMethod* (*class_get_method_from_name)(MonoClass*, const char*, int) = nullptr;
	MonoVTable* (*class_vtable)(MonoDomain*, MonoClass*) = nullptr;
	int32_t (*class_array_element_size)(MonoClass*) = nullptr;
	MonoClass* (*class_get_element_class)(MonoClass*) = nullptr;
	MonoClass* (*class_from_mono_type)(MonoType*) = nullptr;
	MonoClass* (*object_get_class)(MonoObject*) = nullptr;
	MonoType* (*field_get_type)(MonoClassField*) = nullptr;
	uint32_t (*field_get_offset)(MonoClassField*) = nullptr;
	void (*field_static_get_value)(MonoVTable*, MonoClassField*, void*) = nullptr;
	MonoObject* (*runtime_invoke)(MonoMethod*, void*, void**, MonoObject**) = nullptr;
	char* (*string_to_utf8)(MonoObject*) = nullptr;
	void (*free)(void*) = nullptr;
	MonoClass* (*get_boolean_class)() = nullptr;
	MonoObject* (*array_new)(MonoDomain*, MonoClass*, uintptr_t) = nullptr;

	bool Load(std::string& error)
	{
		HMODULE runtime = GetModuleHandleA("mono-2.0-bdwgc.dll");
		if (runtime == nullptr)
		{
			error = "mono-2.0-bdwgc.dll isn't loaded - is this actually the game process?";
			return false;
		}

#define RESOLVE(pointer, exportName)                                                       \
	pointer = reinterpret_cast<decltype(pointer)>(GetProcAddress(runtime, exportName));     \
	if (pointer == nullptr)                                                                 \
	{                                                                                       \
		error = "missing Mono export: " exportName;                                         \
		return false;                                                                       \
	}

		RESOLVE(get_root_domain, "mono_get_root_domain");
		RESOLVE(thread_attach, "mono_thread_attach");
		RESOLVE(thread_detach, "mono_thread_detach");
		RESOLVE(domain_assembly_open, "mono_domain_assembly_open");
		RESOLVE(assembly_get_image, "mono_assembly_get_image");
		RESOLVE(class_from_name, "mono_class_from_name");
		RESOLVE(class_get_field_from_name, "mono_class_get_field_from_name");
		RESOLVE(class_get_method_from_name, "mono_class_get_method_from_name");
		RESOLVE(class_vtable, "mono_class_vtable");
		RESOLVE(class_array_element_size, "mono_class_array_element_size");
		RESOLVE(class_get_element_class, "mono_class_get_element_class");
		RESOLVE(class_from_mono_type, "mono_class_from_mono_type");
		RESOLVE(object_get_class, "mono_object_get_class");
		RESOLVE(field_get_type, "mono_field_get_type");
		RESOLVE(field_get_offset, "mono_field_get_offset");
		RESOLVE(field_static_get_value, "mono_field_static_get_value");
		RESOLVE(runtime_invoke, "mono_runtime_invoke");
		RESOLVE(string_to_utf8, "mono_string_to_utf8");
		RESOLVE(free, "mono_free");
		RESOLVE(get_boolean_class, "mono_get_boolean_class");
		RESOLVE(array_new, "mono_array_new");

#undef RESOLVE

		return true;
	}

	std::string ReadString(MonoObject* str)
	{
		if (str == nullptr)
		{
			return std::string();
		}

		char* utf8 = string_to_utf8(str);
		if (utf8 == nullptr)
		{
			return std::string();
		}

		std::string result(utf8);
		free(utf8);
		return result;
	}
}