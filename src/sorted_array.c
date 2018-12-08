#include "sorted_array.h"

#include <errno.h>
#include <stdlib.h>
#include "log.h"

struct sorted_array {
	void *array;
	/* Actual number of elements in @array */
	unsigned int count;
	/* Total allocated slots in @array */
	unsigned int len;
	/* Size of each array element */
	size_t size;
	/* Comparison function for element insertion */
	sarray_cmp cmp;

	unsigned int refcount;
};

struct sorted_array *
sarray_create(size_t elem_size, sarray_cmp cmp)
{
	struct sorted_array *result;

	result = malloc(sizeof(struct sorted_array));
	if (result == NULL)
		return NULL;

	result->array = calloc(8, elem_size);
	if (result->array == NULL) {
		free(result);
		return NULL;
	}
	result->count = 0;
	result->len = 8;
	result->size = elem_size;
	result->cmp = cmp;
	result->refcount = 1;

	return result;
}

void
sarray_get(struct sorted_array *sarray)
{
	sarray->refcount++;
}

void
sarray_put(struct sorted_array *sarray)
{
	sarray->refcount--;
	if (sarray->refcount == 0) {
		free(sarray->array);
		free(sarray);
	}
}

void
sarray_destroy(struct sorted_array *sarray)
{
	free(sarray->array);
	free(sarray);
}

/* Does not check boundaries. */
static void *
get_nth_element(struct sorted_array *sarray, unsigned int index)
{
	return ((char *)sarray->array) + index * sarray->size;
}

static int
compare(struct sorted_array *sarray, void *new)
{
	enum sarray_comparison cmp;

	if (sarray->count == 0)
		return 0;

	cmp = sarray->cmp(get_nth_element(sarray, sarray->count - 1), new);
	switch (cmp) {
	case SACMP_EQUAL:
		return -EEQUAL;
	case SACMP_CHILD:
		return -ECHILD2;
	case SACMP_PARENT:
		return -EPARENT;
	case SACMP_LEFT:
		return -ELEFT;
	case SACMP_RIGHT:
		return 0;
	case SACMP_ADJACENT_LEFT:
		return -EADJLEFT;
	case SACMP_ADJACENT_RIGHT:
		return -EADJRIGHT;
	case SACMP_INTERSECTION:
		return -EINTERSECTION;
	}

	pr_err("Programming error: Unknown comparison value: %d", cmp);
	return -EINVAL;
}

int
sarray_add(struct sorted_array *sarray, void *element)
{
	int error;
	void *tmp;

	error = compare(sarray, element);
	if (error)
		return error;

	if (sarray->count >= sarray->len) {
		tmp = realloc(sarray->array, 2 * sarray->len * sarray->size);
		if (tmp == NULL)
			return -ENOMEM;
		sarray->array = tmp;
		sarray->len *= 2;
	}

	memcpy(get_nth_element(sarray, sarray->count), element, sarray->size);
	sarray->count++;
	return 0;
}

/* https://stackoverflow.com/questions/364985 */
static int
pow2roundup(unsigned int x)
{
	--x;
	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
	x |= x >> 8;
	x |= x >> 16;
	return x + 1;
}

/*
 * Appends the elements from @addend to @sarray's tail.
 */
int
sarray_join(struct sorted_array *sarray, struct sorted_array *addend)
{
	int error;
	unsigned int new_count;
	size_t new_len;
	void *tmp;

	if (addend == NULL || addend->count == 0)
		return 0;

	error = compare(sarray, addend->array);
	if (error)
		return error;

	new_count = sarray->count + addend->count;
	if (new_count > sarray->len) {
		new_len = pow2roundup(new_count);
		tmp = realloc(sarray->array, new_len * sarray->size);
		if (tmp == NULL)
			return -ENOMEM;
		sarray->array = tmp;
		sarray->len = new_len;
	}

	memcpy(get_nth_element(sarray, sarray->count), addend->array,
	    new_count * sarray->size);
	sarray->count += addend->count;
	return 0;
}

bool
sarray_contains(struct sorted_array *sarray, void *elem)
{
	unsigned int left, mid, right;
	enum sarray_comparison cmp;

	if (sarray == NULL || sarray->count == 0)
		return false;

	left = 0;
	right = sarray->count - 1;

	while (left <= right) {
		mid = left + (right - left) / 2;
		cmp = sarray->cmp(get_nth_element(sarray, mid), elem);
		switch (cmp) {
		case SACMP_LEFT:
		case SACMP_ADJACENT_LEFT:
			right = mid - 1;
			continue;
		case SACMP_RIGHT:
		case SACMP_ADJACENT_RIGHT:
			left = mid + 1;
			continue;
		case SACMP_EQUAL:
		case SACMP_CHILD:
			return true;
		case SACMP_PARENT:
			return false;
		case SACMP_INTERSECTION:
			/* Fall through; it's not supposed to happen here. */
			break;
		}

		pr_err("Programming error: Unknown comparison value: %d", cmp);
		return false;
	}

	return false;
}

char const *sarray_err2str(int error)
{
	switch (abs(error)) {
	case EEQUAL:
		return "Resource equals an already existing resource";
	case ECHILD2:
		return "Resource is a subset of an already existing resource";
	case EPARENT:
		return "Resource is a superset of an already existing resource";
	case ELEFT:
		return "Resource sequence is not properly sorted";
	case EADJLEFT:
	case EADJRIGHT:
		return "Resource is adjacent to an existing resource (they are supposed to be aggregated)";
	case EINTERSECTION:
		return "Resource intersects with an already existing resource";
	}

	return strerror(error);
}
