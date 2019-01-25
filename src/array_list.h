#ifndef SRC_ARRAY_LIST_H_
#define SRC_ARRAY_LIST_H_

#include <errno.h>

#define ARRAY_LIST(name, elem_type)					\
	struct name {							\
		/** Unidimensional array. */				\
		elem_type *array;					\
		/** Number of elements in @array. */			\
		unsigned int len;					\
		/** Actual allocated slots in @array. */		\
		unsigned int capacity;					\
	};								\
									\
	static int							\
	name##_init(struct name *list)					\
	{								\
		list->capacity = 8;					\
		list->len = 0;						\
		list->array = malloc(list->capacity			\
		    * sizeof(elem_type));				\
		return (list->array != NULL) ? 0 : -ENOMEM;		\
	}								\
									\
	static void							\
	name##_cleanup(struct name *list, void (*cb)(elem_type *))	\
	{								\
		unsigned int i;						\
		for (i = 0; i < list->len; i++)				\
			cb(&list->array[i]);				\
		free(list->array);					\
	}								\
									\
	/* Will store a shallow copy, not @elem */			\
	static int							\
	name##_add(struct name *list, elem_type *elem)			\
	{								\
		elem_type *tmp;						\
									\
		list->len++;						\
		while (list->len >= list->capacity) {			\
			list->capacity *= 2;				\
									\
			tmp = realloc(list->array, list->capacity	\
			    * sizeof(elem_type));			\
			if (tmp == NULL)				\
				return pr_enomem();			\
			list->array = tmp;				\
		}							\
									\
		list->array[list->len - 1] = *elem;			\
		return 0;						\
	}								\
									\
	static int							\
	name##_foreach(struct name *list,				\
	    int (*cb)(elem_type const *, void *),			\
	    void *arg)							\
	{								\
		unsigned int i;						\
		int error;						\
									\
		for (i = 0; i < list->len; i++) {			\
			error = cb(&list->array[i], arg);		\
			if (error)					\
				return error;				\
		}							\
									\
		return 0;						\
	}

#endif /* SRC_ARRAY_LIST_H_ */