#pragma once

namespace fb
{
	class ITypedObject
	{
	public:
		virtual class ClassInfo* getType();
	};
}