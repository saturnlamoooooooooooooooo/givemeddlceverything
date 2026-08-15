#pragma once
#include <Windows.h>
#include <cstdint>
#include <string>

struct MonoDomain;
struct MonoAssembly;
struct MonoImage;
struct MonoClass;
struct MonoClassField;
struct MonoMethod;
struct MonoObject;
struct MonoType;
struct MonoVTable;
struct MonoThread;

namespace mono
{
	constexpr uint32_t kObjectHeaderSize = 16;
	constexpr uint32_t kArrayLengthOffset = 24;
	constexpr uint32_t kArrayDataOffset = 32;
	extern MonoDomain* (*get_root_domain)();
	extern MonoThread* (*thread_attach)(MonoDomain* domain);
	extern void (*thread_detach)(MonoThread* thread);
	extern MonoAssembly* (*domain_assembly_open)(MonoDomain* domain, const char* name);
	extern MonoImage* (*assembly_get_image)(MonoAssembly* assembly);
	extern MonoClass* (*class_from_name)(MonoImage* image, const char* name_space, const char* name);
	extern MonoClassField* (*class_get_field_from_name)(MonoClass* klass, const char* name);
	extern MonoMethod* (*class_get_method_from_name)(MonoClass* klass, const char* name, int param_count);
	extern MonoVTable* (*class_vtable)(MonoDomain* domain, MonoClass* klass);
	extern int32_t (*class_array_element_size)(MonoClass* klass);
	extern MonoClass* (*class_get_element_class)(MonoClass* klass);
	extern MonoClass* (*class_from_mono_type)(MonoType* type);
	extern MonoClass* (*object_get_class)(MonoObject* object);
	extern MonoType* (*field_get_type)(MonoClassField* field);
	extern uint32_t (*field_get_offset)(MonoClassField* field);
	extern void (*field_static_get_value)(MonoVTable* vtable, MonoClassField* field, void* value);
	extern MonoObject* (*runtime_invoke)(MonoMethod* method, void* obj, void** params, MonoObject** exc);
	extern char* (*string_to_utf8)(MonoObject* str);
	extern void (*free)(void* ptr);
	extern MonoClass* (*get_boolean_class)();
	extern MonoObject* (*array_new)(MonoDomain* domain, MonoClass* element_class, uintptr_t count);

	bool Load(std::string& error);

	inline uint8_t* Raw(MonoObject* object)
	{
		return reinterpret_cast<uint8_t*>(object);
	}

	template <typename T>
	T ReadField(MonoObject* object, MonoClassField* field)
	{
		return *reinterpret_cast<T*>(Raw(object) + field_get_offset(field));
	}

	template <typename T>
	void WriteField(MonoObject* object, MonoClassField* field, T value)
	{
		*reinterpret_cast<T*>(Raw(object) + field_get_offset(field)) = value;
	}

	inline uint32_t StructFieldOffset(MonoClassField* field)
	{
		return field_get_offset(field) - kObjectHeaderSize;
	}

	inline uintptr_t ArrayLength(MonoObject* array)
	{
		return *reinterpret_cast<uintptr_t*>(Raw(array) + kArrayLengthOffset);
	}

	inline uint8_t* ArrayData(MonoObject* array)
	{
		return Raw(array) + kArrayDataOffset;
	}

	std::string ReadString(MonoObject* str);
}