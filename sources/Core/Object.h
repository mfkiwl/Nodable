#pragma once
#include "Nodable.h"
#include "Member.h"   // for Type enum
#include "mirror.h"
#include "Visibility.h"
#include "Way.h"

namespace Nodable
{
	/*
		Object is the base class for all classes that needs to have members (std::string key => Nodable::Value value)
	*/

	class Object
	{
	public:
		Object();
		virtual ~Object();

		/* Adds a new member identified by its _name. */
		Member*             add         (const char*, Visibility = Visibility::Default, Type = Type_Any, Way = Way_Default);

		bool                has         (const Member* _value);

		/* Set deleted flag on. Will be deleted by its controller next frame */
		void                flagForDeletion   (){ deleted = true;}

		/* Returns a pointer to the member value identified by its name.
		Or nullptr if this member doesn't exists. */
		Member*              get         (const std::string& _name)const;

		/* Returns a pointer to the member value identified by its name.
		Or nullptr if this member doesn't exists. */
		Member*              get         (const char* _name)const;
		
		/* Return all members of this object */
		const Members&      getMembers        ()const;

		/* Return the first member that has this connection type (cf. Way enum definition) or nullptr if no member is found.*/
		Member*             getFirstWithConn(Way)const;

		/* this method is automatically called when a member value changed */
		virtual void        onMemberValueChanged(const char* _name){};

		bool                needsToBeDeleted  (){return deleted;}

		template<typename T>
		void set(const char* _name, T _value)
		{
			members[std::string(_name)]->set(_value);
			this->onMemberValueChanged(_name);
		}

		template<typename T>
		T* as()
		{
            NODABLE_ASSERT(T::GetClass()->isChildOf(Object::getClass(), true));
            return reinterpret_cast<T*>(this);
		}

        template<typename T>
        const T* as()const
        {
            NODABLE_ASSERT(T::GetClass()->isChildOf(Object::getClass(), true));
            return reinterpret_cast<const T*>(this);
        }

	private:
		Members             members;
		bool                deleted = false;

	public:
		MIRROR_CLASS(Object)();
	};	
}