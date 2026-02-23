#pragma once
#include <cstdint>
#include <EASTL/vector.h>

namespace fb
{
	class ArrayTypeInfoData;
	class FieldInfoData;

	enum TypeEnum
	{
		kType_Inherited = 0x00,
		kType_DbObject = 0x01,
		kType_Struct = 0x02,
		kType_Class = 0x03,
		kType_Array = 0x04,
		kType_String = 0x06,
		kType_CString = 0x07,
		kType_Enum = 0x08,
		kType_FileRef = 0x09,
		kType_Boolean = 0x0A,
		kType_Int8 = 0x0B,
		kType_UInt8 = 0x0C,
		kType_Int16 = 0x0D,
		kType_UInt16 = 0x0E,
		kType_Int32 = 0x0F,
		kType_UInt32 = 0x10,
		kType_Int64 = 0x12,
		kType_UInt64 = 0x11,
		kType_Float32 = 0x13,
		kType_Float64 = 0x14,
		kType_Guid = 0x15,
		kType_Sha1 = 0x16,
		kTypeCode_BasicTypeCount = 0x17,
	};

	enum MemberEnum
	{
		kMemberType_Field,
		kMemberType_TypeInfo
	};

	enum CategoryEnum
	{
		kCategory_None = 0,
		kCategory_Class = 1,
		kCategory_Struct = 2,
		kCategory_Primitive = 3,
		kCategory_Array = 4,
		kCategory_Enum = 5
	};

	class MemberInfo
	{
	public:
		const void* m_infoData;
	};
	static_assert(sizeof(MemberInfo) == 0x8);

#pragma pack(push, 2)

	struct MemberInfoFlags
	{
		uint16_t m_flagBits;

		enum : uint16_t
		{
			kMemberTypeMask = 0x1,
			kTypeCategoryShift = 0x0,
			kTypeCategoryMask = 0xF,
			kTypeCodeShift = 0x4,
			kTypeCodeMask = 0x1F,
			kFlags_MetaData = 1 << 11,
			kFlags_Homogeneous = 1 << 12,
			kFlags_AlwaysPersist = 1 << 13,
			kFlags_Exposed = 1 << 13,
			kFlags_FlagsEnum = 1 << 13,
			kFlags_LayoutImmutable = 1 << 14,
			kFlags_Blittable = 1 << 15
		};

		MemberEnum getMemberType() const
		{
			return (MemberEnum)(this->m_flagBits & kMemberTypeMask);
		}

		CategoryEnum getCategory() const
		{
			return (CategoryEnum)((this->m_flagBits >> kTypeCategoryShift) & kTypeCategoryMask);
		}

		TypeEnum getType() const 
		{
			return (TypeEnum)((this->m_flagBits >> kTypeCodeShift) & kTypeCodeMask);
		}
	};
	static_assert(sizeof(MemberInfoFlags) == 0x2);

	struct MemberInfoData
	{
		const char* m_name;
		MemberInfoFlags m_flags;
	};
	static_assert(sizeof(MemberInfoData) == 0xA);

#pragma pack(pop)

	class TypeInfo : public MemberInfo
	{
	public:
		TypeInfo* m_nextTypeData; //0x0008
		uint16_t m_runtimeId; //0x0010
		uint16_t m_flags; //0x0012
		char pad_0014[4]; //0x0014

		bool isString() const
		{
			switch (getType())
			{
			case kType_String:
			case kType_CString:
				return true;
			default:
				return false;
			}
		}

		bool isNotDbObjOrInherited() const
		{
			return (getType()) - 6 > 1;
		}

		bool isClass() const
		{
			return (getTypeInfoData()->m_flags.m_flagBits & 0x1F0) == 0x30;
		}

		int getSize() const
		{
			return getTypeInfoData()->m_totalSize;
		}

		const char* getName() const
		{
			return getTypeInfoData()->m_name;
		}

		TypeEnum getType() const { return getTypeInfoData()->m_flags.getType(); }

	protected:
		struct TypeInfoData
		{
		public:
			const char* m_name; //0x0000
			MemberInfoFlags m_flags; //0x0008
			uint16_t m_totalSize; //0x000A
		private:
			char pad_000C[4]; //0x000C
		public:
			class ModuleInfo* m_module; //0x0010
			uint8_t m_alignment; //0x0018
			uint8_t m_pad; //0x0019
			uint16_t m_fieldCount; //0x001A
		private:
			char pad_001C[4]; //0x001C
		};
		static_assert(sizeof(TypeInfoData) == 0x20);

		const TypeInfo::TypeInfoData* getTypeInfoData() const { return reinterpret_cast<const TypeInfoData*>(m_infoData); }
	};
	static_assert(sizeof(TypeInfo) == 0x18);

	class ClassInfo : public TypeInfo
	{
	public:
		ClassInfo* m_baseClass; //0x0018
		void* m_defaultInstance; //0x0020
		uint16_t m_classId; //0x0028
		uint16_t m_lastSubclassId; //0x002A
		char pad_002C[4]; //0x002C
		ClassInfo* m_firstDerivedClass; //0x0030
		ClassInfo* m_nextSiblingClass; //0x0038
		char pad_0040[24]; //0x0040

	protected:
		struct ClassInfoData : public TypeInfoData
		{
		public:
			ClassInfo* m_baseClass; //0x0020
			void* m_ctor; //0x0028
			FieldInfoData* m_fields; //0x0030
		};
		static_assert(sizeof(ClassInfoData) == 0x38);

		const ClassInfoData* getClassTypeInfoData() const
		{
			return reinterpret_cast<const ClassInfoData*>(m_infoData);
		}
	};

/*#pragma pack(push, 2)

	class MemberInfoData
	{
	public:
		char* mName; //0x0000
		uint16_t mFlagBits; //0x0008
	}; //Size: 0x000A
	static_assert(sizeof(MemberInfoData) == 0xA);
#pragma pack(pop)*/

	class ModuleInfo
	{
	public:
		char* mName; //0x0000
		ModuleInfo* mNext; //0x0008
	};
	static_assert(sizeof(ModuleInfo) == 0x10);

	/*class FieldInfoData : public MemberInfoData
	{
	public:
		uint16_t mOffset; //0x000A
		char pad_000C[4]; //0x000C
		TypeInfo* mType; //0x0010
	};
	static_assert(sizeof(FieldInfoData) == 0x18);*/

	/*class ClassInfoData : public TypeInfoData
	{
	public:
		ClassInfo* mBaseClass; //0x0020
		void* mCtor; //0x0028
		FieldInfoData* mFields; //0x0030
	}; //Size: 0x0038
	static_assert(sizeof(ClassInfoData) == 0x38);*/

	class FieldInfo : public MemberInfo
	{
	public:
		struct FieldInfoData : public MemberInfoData
		{
		public:
			uint16_t m_offset; //0x000A
			char pad_000C[4]; //0x000C
			TypeInfo* m_type; //0x0010
		};
		static_assert(sizeof(FieldInfoData) == 0x18);
	}; //Size: 0x0008
	static_assert(sizeof(FieldInfo) == 0x8);

	/*class EnumTypeInfoData : public TypeInfoData
	{
	public:
		FieldInfoData* mFields; //0x0020
	}; //Size: 0x0028
	static_assert(sizeof(EnumTypeInfoData) == 0x28);

	class ValueTypeInfo : public TypeInfo
	{
	public:
	}; //Size: 0x0018
	static_assert(sizeof(ValueTypeInfo) == 0x18);

	class ValueTypeInfoData : public TypeInfoData
	{
	public:
		void* mDefaultValue; //0x0020
		FieldInfoData* mFields; //0x0028
	}; //Size: 0x0030
	static_assert(sizeof(ValueTypeInfoData) == 0x30);

	class ArrayTypeInfo : public TypeInfo
	{
	public:
	}; //Size: 0x0018
	static_assert(sizeof(ArrayTypeInfo) == 0x18);

	class ArrayTypeInfoData : public TypeInfoData
	{
	public:
		TypeInfo* mElementType; //0x0020
	}; //Size: 0x0028
	static_assert(sizeof(ArrayTypeInfoData) == 0x28);*/

	class EnumTypeInfo : public TypeInfo
	{
	public:
	}; //Size: 0x0018
	static_assert(sizeof(EnumTypeInfo) == 0x18);
}