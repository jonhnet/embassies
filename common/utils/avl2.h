/* Copyright (c) Microsoft Corporation                                       */
/*                                                                           */
/* All rights reserved.                                                      */
/*                                                                           */
/* Licensed under the Apache License, Version 2.0 (the "License"); you may   */
/* not use this file except in compliance with the License.  You may obtain  */
/* a copy of the License at http://www.apache.org/licenses/LICENSE-2.0       */
/*                                                                           */
/* THIS CODE IS PROVIDED *AS IS* BASIS, WITHOUT WARRANTIES OR CONDITIONS     */
/* OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION      */
/* ANY IMPLIED WARRANTIES OR CONDITIONS OF TITLE, FITNESS FOR A PARTICULAR   */
/* PURPOSE, MERCHANTABLITY OR NON-INFRINGEMENT.                              */
/*                                                                           */
/* See the Apache Version 2.0 License for specific language governing        */
/* permissions and limitations under the License.                            */
/*                                                                           */
/*++

Copyright (c) 1993-1999, 2002-2004, 2009	Microsoft Corporation

Module Name:

	avl2.h

Abstract:

	AVL tree template class implementation

Author:

	Bill Bolosky		[bolosky]		1993,1998-1999

Revision History:

--*/

#pragma once
#ifdef	__cplusplus

#include "SyncFactory.h"
#include "LiteLib.h"

typedef SyncFactoryMutex *PCRITICAL_SECTION;
typedef bool BOOLEAN;
#define TRUE	true
#define FALSE	false
inline void EnterCriticalSection(PCRITICAL_SECTION cs) { cs->lock(); }
inline void LeaveCriticalSection(PCRITICAL_SECTION cs) { cs->unlock(); }

#define avl_assert	lite_assert

//
// jonh factored and parameterized the avl2 AVLTree template into three components:
// - avl2.h is the main include file. It defines AVLTree, the user-facing template class.
// 	AVLTree takes three template arguments: elementClass, keyClass, and implClass.
// 	The first two parameters describe the data actually stored in the tree:
// 		* elementClass defines the type of objects actually stored in the tree.
// 			When an element is inserted, AVLTree owns the resulting element object;
// 			lookups return pointers (elementClass *).
// 			The elementClass must define a 'keyClass getKey()' method, so that the element
// 			can be sorted into the tree.
// 		* keyClass is the part of the element used as the sorting key. You can define
// 			operator< and relatives on the keyClass to produce your sorting semantics.
// 			Also, lookup methods can accept a key (instead of an element).
//
// 	The last parameter defines how the data is stored. The usual in-memory implementation
// 	is supplied in avl2mem.h.
// 		* implClass provides three typedefs (that is, names three other classes).
// 			* nodeClass defines how each tree node is implemented.
// 			* poolClass identifies the pool class used for allocating internal nodes
// 				from a pool. The pool is mostly internal to the allocatorClass,
// 				but it is exposed here to provide the type used in the formal constructor
// 				parameter of AVLTree, so that a specific pool instance may be passed
// 				into the allocator via the AVLTree constructor.
// 			* allocatorClass defines how instances of nodeClass are created and destroyed.
// 				It may use a poolClass instance internally to implement pool allocation.
//
//  Organization:
//  avl2.h defines AVLTree, the user-facing template. The methods of this class are
//  	wrappers for the actual iterative or recursive routines in avl2logic.h.
//  avl2mem.h defines the "MemImpl" class, the default value of implClass. When the
//  	implClass argument is omitted from the AVLTree invocation, MemImpl provides
//  	exactly the implementation that AVLTree had before jonh parameterized it.
//  avl2logic.h collects the actual insertion/deletion/rotation logic for AVL trees.
//  	These functions used to be methods on AVLElements (the in-memory nodeClass).
//  	To parameterize the nodeClass implementation, I factored the common logic
//  	out into static methods in class AVLNodeLogic. The result is that the syntax
//  	is slightly more cumbersome: methods have an explicit "self" argument (playing
//  	the role of the implicit OO "this" argument), and field accesses
//  	( ->left == foo, ->right = bar ) are replaced with accessors
//  	( ->getLeft() == foo, ->setRight(bar) ). (The accessors are inline, so there
//  	is no performance penalty in the compiled code.)
//  	The only alternative I could think of that would preserve the "this" syntax
//  	would have been to parameterize nodeClass using virtual function dispatch,
//		an approach which would have changed the storage overhead of the elements
//		and may have interfered with alternative implementations.
//

#define		AVL2_SAVE_SPACE		0	// This plays games with the pointers to make AVLElements smaller.  It assumes that AVLElements will always be aligned to at least 4 byte boundaries.  Doesn't work on 64 bit machines!

#if		AVL2_ASSERT_ON_ALLOCATION_FAILURE
//
// The caller may define AVL2_ASSERT_ON_ALLOCATION_FAILURE to cause the avl package to assert that all of its
// memory allocations have worked.
//
#define	assertOK(p)	avl_assert(NULL != (p) && "A memory allocation has failed within the AVL tree pacage")

#else	// AVL2_ASSERT_ON_ALLOCATION_FAILURE
#define	assertOK(p)	/* nada */
#endif	// AVL2_ASSERT_ON_ALLOCATION_FAILURE


#ifndef	CERTIFY_ALLOCATION
#define	CERTIFY_ALLOCATION(p) /* nothing */
#endif	// CERTIFY_ALLOCATION

enum AVLBalance {
    AVLNew = 0,				// Not yet inserted in a tree
    AVLLeft = 1,			// Left side is one deeper than the right
    AVLBalanced = 2,		// Left and right sides are evenly balanced
    AVLRight = 3,			// Right side is one deeper than left
};

//template<class elementClass, class keyClass> class AVLElement;

#include "avl2mem.h"

template<class elementClass, class keyClass, class nodeClass> class AVLNodeLogic;

template<
	class elementClass,
	class keyClass,
	class implClass = MemImpl<elementClass,keyClass>
> class AVLTree {
	typedef typename implClass::nodeClass nodeClass;
	typedef AVLNodeLogic<elementClass,keyClass,nodeClass> nodeLogicClass;

	// NB used to use this syntax:
	// friend class nodeLogicClass;
	// but it's not accepted by current complier! So, we settle for
	// expanding the typedef explicitly:
	friend class AVLNodeLogic<elementClass,keyClass,nodeClass>;

public:
					AVLTree(
						SyncFactory *sf);

					~AVLTree(void);

	elementClass	*lookup(
						elementClass				*element);

	elementClass	*lookup(
						keyClass					 key);

    elementClass	*findFirstLessThan(
					   elementClass					*element);

    elementClass	*findFirstLessThan(
					   keyClass						 key);

    elementClass	*findFirstLessThanOrEqualTo(
					   elementClass					*element);

    elementClass	*findFirstLessThanOrEqualTo(
					   keyClass						 key);

    elementClass	*findFirstGreaterThan(
					    elementClass				*element);

    elementClass	*findFirstGreaterThan(
					    keyClass					 key);

    elementClass	*findFirstGreaterThanOrEqualTo(
					    elementClass				*element);

    elementClass	*findFirstGreaterThanOrEqualTo(
					    keyClass					 key);

    elementClass	*findMin(void);

    elementClass	*findMax(void);

	typedef void ForeachCallback(elementClass *element, void *data);
	typedef bool FirstthatCallback(elementClass *element, void *data);

	void			foreach(ForeachCallback func, void *data);

	elementClass	*firstthat(FirstthatCallback func, void *data);

    int			 	empty(void);

    unsigned		size(void);

    void		 	check(void);

    BOOLEAN		 	insert(
					    elementClass				*element);

    void		 	remove(
					    elementClass				*element);

    elementClass    *remove(
                        keyClass                     key); // Finds and element with this key and (atomically) removes it if it exists.  Returns the element or NULL

    void			dumpPoolStats(void);

private:

	nodeClass		tree;

	typename implClass::allocatorClass allocator;
	bool			 poolExternal;
	bool			 criticalSectionExternal;

    unsigned		 insertions;
    unsigned		 deletions;
    unsigned		 singleRotations;
    unsigned		 doubleRotations;

	PCRITICAL_SECTION criticalSection;
};

#include "avl2logic.h"

    template<class elementClass, class keyClass, class implClass> elementClass *
AVLTree<elementClass,keyClass,implClass>::lookup(
    elementClass			*element)
{
	avl_assert(element);

	EnterCriticalSection(criticalSection);
	elementClass *retVal = lookup(element->getKey());
	LeaveCriticalSection(criticalSection);

	return retVal;

}

    template<class elementClass, class keyClass, class implClass> elementClass *
AVLTree<elementClass,keyClass,implClass>::lookup(
	keyClass				 key)
{
	EnterCriticalSection(criticalSection);

	elementClass *lookedUpElement = findFirstLessThanOrEqualTo(key);
	if (NULL != lookedUpElement && lookedUpElement->getKey() == key) {
		LeaveCriticalSection(criticalSection);
		return lookedUpElement;
	} else {
		LeaveCriticalSection(criticalSection);
		return NULL;
	}
}

    template<class elementClass, class keyClass, class implClass>  elementClass *
AVLTree<elementClass,keyClass,implClass>::findFirstLessThan(
    elementClass			*element)
{
    avl_assert(element);
	
	EnterCriticalSection(criticalSection);
	elementClass *retVal = findFirstLessThan(element->getKey());
	LeaveCriticalSection(criticalSection);

	return retVal;
}

    template<class elementClass, class keyClass, class implClass>  elementClass *
AVLTree<elementClass,keyClass,implClass>::findFirstLessThan(
	keyClass				 key)
{
	EnterCriticalSection(criticalSection);

    if (!tree) {
		LeaveCriticalSection(criticalSection);
		return(NULL);
	}

    nodeClass avlElement = nodeLogicClass::findFirstLessThan(tree, key);

	elementClass *retVal;

    if (avlElement) {
		retVal = avlElement->getElement();
    } else {
		retVal = NULL;
    }

	LeaveCriticalSection(criticalSection);

	return retVal;
}

    template<class elementClass, class keyClass, class implClass>  elementClass *
AVLTree<elementClass,keyClass,implClass>::findFirstLessThanOrEqualTo(
    elementClass			*element)
{
    avl_assert(element);
	
	EnterCriticalSection(criticalSection);
	elementClass *retVal = findFirstLessThanOrEqualTo(element->getKey());
	LeaveCriticalSection(criticalSection);

	return retVal;
}

    template<class elementClass, class keyClass, class implClass>  elementClass *
AVLTree<elementClass,keyClass,implClass>::findFirstLessThanOrEqualTo(
	keyClass				 key)
{
	EnterCriticalSection(criticalSection);

    if (!tree) {
		LeaveCriticalSection(criticalSection);
		return(NULL);
	}

    nodeClass avlElement = nodeLogicClass::findFirstLessThanOrEqualTo(tree, key);

	elementClass *retVal;

    if (avlElement) {
		retVal = avlElement->getElement();
    } else {
		retVal = NULL;
    }

	LeaveCriticalSection(criticalSection);

	return retVal;
}

    template<class elementClass, class keyClass, class implClass> void
AVLTree<elementClass,keyClass,implClass>::foreach(ForeachCallback func, void *data)
{
	elementClass *element;

	for (element = findMin(); NULL != element; element = findFirstGreaterThan(element))
		func(element, data);
}

    template<class elementClass, class keyClass, class implClass> elementClass *
AVLTree<elementClass,keyClass,implClass>::firstthat(FirstthatCallback func, void *data)
{
	elementClass *element;

	for (element = findMin(); NULL != element; element = findFirstGreaterThan(element))
		if (func(element, data))
			return element;
	
	return NULL;
}

    template<class elementClass, class keyClass, class implClass> 
AVLTree<elementClass,keyClass,implClass>::AVLTree(
	SyncFactory *sf)
	: allocator()
{
    tree = NULL;
    insertions = deletions = singleRotations = doubleRotations = 0;
	criticalSection = sf->new_mutex(/*recursive*/ true);
	criticalSectionExternal = TRUE;
}

template<class elementClass,class keyClass,class implClass>
AVLTree<elementClass,keyClass,implClass>::~AVLTree(void)
{
    avl_assert(tree == NULL);
	
	//avl_assert(criticalSectionExternal);
	delete criticalSection;
}

    template<class elementClass,class keyClass,class implClass> elementClass *
AVLTree<elementClass,keyClass,implClass>::findFirstGreaterThan(
	elementClass				*element)
{
	avl_assert(element);

	EnterCriticalSection(criticalSection);
	elementClass *retVal = findFirstGreaterThan(element->getKey());
	LeaveCriticalSection(criticalSection);

	return retVal;
}

    template<class elementClass,class keyClass,class implClass> elementClass *
AVLTree<elementClass, keyClass, implClass>::findFirstGreaterThan(
	keyClass					key)
{
	EnterCriticalSection(criticalSection);

    if (!tree) {
		LeaveCriticalSection(criticalSection);
		return(NULL);
	}

    nodeClass avlElement = nodeLogicClass::findFirstGreaterThan(tree, key);

	elementClass	*retVal;

    if (avlElement) {
		retVal = avlElement->getElement();
    } else {
		retVal = NULL;
    }

	LeaveCriticalSection(criticalSection);

	return retVal;
}


    template<class elementClass, class keyClass, class implClass> elementClass *
AVLTree<elementClass,keyClass,implClass>::findFirstGreaterThanOrEqualTo(
    elementClass			*element)
{
	avl_assert(element);
	
	EnterCriticalSection(criticalSection);
	elementClass *retVal =  findFirstGreaterThanOrEqualTo(element->getKey());
	LeaveCriticalSection(criticalSection);

	return retVal;
}

    template<class elementClass, class keyClass, class implClass> elementClass *
AVLTree<elementClass,keyClass,implClass>::findFirstGreaterThanOrEqualTo(
    keyClass				 key)
{
	EnterCriticalSection(criticalSection);

    if (!tree) {
		LeaveCriticalSection(criticalSection);

		return(NULL);
	}

    nodeClass avlElement = nodeLogicClass::findFirstGreaterThanOrEqualTo(tree, key);

	elementClass *retVal;
    if (avlElement) {
		retVal = avlElement->getElement();
    } else {
		retVal = NULL;
    }

	LeaveCriticalSection(criticalSection);
	return retVal;
}


    template<class elementClass, class keyClass, class implClass> int
AVLTree<elementClass,keyClass,implClass>::empty(void)
{
	EnterCriticalSection(criticalSection);

    avl_assert((tree == NULL) == (insertions == deletions));
    int retVal = (tree == NULL);

	LeaveCriticalSection(criticalSection);

	return retVal;
}

    template<class elementClass, class keyClass, class implClass> unsigned
AVLTree<elementClass,keyClass,implClass>::size(void)
{
	EnterCriticalSection(criticalSection);

    avl_assert(insertions >= deletions);
    avl_assert((tree == NULL) == (insertions == deletions));
    unsigned retVal = (insertions - deletions);

	LeaveCriticalSection(criticalSection);

	return retVal;
}

    template<class elementClass, class keyClass, class implClass> elementClass *
AVLTree<elementClass,keyClass,implClass>::findMin(void)
{
	EnterCriticalSection(criticalSection);

    if (!tree) {
		LeaveCriticalSection(criticalSection);
		return(NULL);
    }

    nodeClass candidate = tree;
    while (candidate->getLeft()) {
		avl_assert(candidate->getLeft()->getElement()->getKey() <= candidate->getElement()->getKey());
		candidate = candidate->getLeft();
    }

	elementClass *retVal = candidate->getElement();
	LeaveCriticalSection(criticalSection);

    return retVal;
}

    template<class elementClass, class keyClass, class implClass> elementClass *
AVLTree<elementClass,keyClass,implClass>::findMax(void)
{
	EnterCriticalSection(criticalSection);

    if (!tree) {
		LeaveCriticalSection(criticalSection);
		return(NULL);
    }

    nodeClass candidate = tree;
    while (candidate->getRight()) {
		avl_assert(candidate->getRight()->getElement()->getKey() >= candidate->getElement()->getKey());
		candidate = candidate->getRight();
    }
	elementClass *retVal = candidate->getElement();

	LeaveCriticalSection(criticalSection);
	return retVal;
}

    template<class elementClass, class keyClass, class implClass> void
AVLTree<elementClass,keyClass,implClass>::check(void)
{
	EnterCriticalSection(criticalSection);

    nodeClass currElement = NULL;
    nodeClass nextElement = NULL;
    nodeClass oldElement = NULL;

    unsigned countedElements = 0;
    if (tree) {
		avl_assert(tree->getParent() == NULL);
		unsigned overallDepth = nodeLogicClass::checkAndReturnDepth(tree, &countedElements);
		(void) overallDepth;
    }
    avl_assert(insertions-deletions == countedElements);

    // Check every element in the tree for consistance by verifying that it is in
    // the expected order.  If not, it is most likely that the element's operators
    // are not behaving as needed.
    for(currElement = tree; currElement != NULL; currElement = nextElement) {
		// Go left if we can (and have not already been here).

		if (currElement->getLeft() && oldElement == currElement->getParent()) {

		    nextElement = currElement->getLeft();
	    	avl_assert(nextElement->getKey() < currElement->getKey() && "The < operator appears to be broken");
		    avl_assert(currElement->getKey() > nextElement->getKey() && "The > operator appears to be broken");
		    avl_assert(!(nextElement->getKey() == currElement->getKey()) && "The == operator appears to be broken");

		} else if (currElement->getRight() && 
	       (oldElement == currElement->getLeft() || oldElement == currElement->getParent())) {

		    nextElement = currElement->getRight();
		    avl_assert(nextElement->getKey() > currElement->getKey() && "The > operator appears to be broken");
		    avl_assert(currElement->getKey() < nextElement->getKey() && "The < operator appears to be broken");
		    avl_assert(!(nextElement->getKey() == currElement->getKey()) && "The == operator appears to be broken");
		} else {
			// We are done below us, go up a node.
		    nextElement = currElement->getParent();
		}
		oldElement = currElement;
		avl_assert(oldElement->getKey() == currElement->getKey() && "The == operator appears to be broken");
    }

	LeaveCriticalSection(criticalSection);
}


    template <class elementClass, class keyClass, class implClass> BOOLEAN
AVLTree<elementClass,keyClass,implClass>::insert(
    elementClass	*element)
{
    avl_assert(element);

	EnterCriticalSection(criticalSection);

	nodeClass avlElement = allocator.Allocate();
	if (NULL == avlElement) {
		LeaveCriticalSection(criticalSection);
		return FALSE;
	}

	nodeLogicClass::initialize(avlElement);
	nodeLogicClass::insert(avlElement, this, element);

	LeaveCriticalSection(criticalSection);
	return TRUE;
}

    template <class elementClass, class keyClass, class implClass> void
AVLTree<elementClass,keyClass,implClass>::remove(
    elementClass	*element)
{
	avl_assert(element);
	avl_assert(tree);	// The element must be in the tree to be removed.  This asserts that the tree isn't empty.

	EnterCriticalSection(criticalSection);

	nodeClass candidate = nodeLogicClass::findFirstLessThanOrEqualTo(tree, element->getKey());
	avl_assert(candidate && candidate->getElement()->getKey() == element->getKey());
	nodeLogicClass::remove(candidate, this);
	allocator.Free(candidate);
	LeaveCriticalSection(criticalSection);
}

    template <class elementClass, class keyClass, class implClass> elementClass *
AVLTree<elementClass,keyClass,implClass>::remove(
    keyClass  key)
{
	elementClass *retVal = NULL;
	EnterCriticalSection(criticalSection);

    if (tree) {
    	nodeClass candidate = nodeLogicClass::findFirstLessThanOrEqualTo(tree, key);
		if (NULL != candidate && candidate->getElement()->getKey() == key) {
			retVal = candidate->getElement();
			nodeLogicClass::remove(candidate, this);
			allocator.Free(candidate);
		}
	}
	LeaveCriticalSection(criticalSection);

	return retVal;
}

    template <class elementClass, class keyClass, class implClass> void
AVLTree<elementClass,keyClass,implClass>::dumpPoolStats(void)
{
	allocator.DumpPoolStats();
#if USE_FILE_IO
	fprintf(stderr,"AVLTree: %d insertions, %d deletions, %d singleRotations, %d doubleRotations\n",
					insertions,deletions,singleRotations,doubleRotations);
#endif // USE_FILE_IO
}
#endif	// __cplusplus
